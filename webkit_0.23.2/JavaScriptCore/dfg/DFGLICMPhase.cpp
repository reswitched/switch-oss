/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DFGLICMPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractInterpreterInlines.h"
#include "DFGAtTailAbstractState.h"
#include "DFGBasicBlockInlines.h"
#include "DFGClobberSet.h"
#include "DFGClobberize.h"
#include "DFGEdgeDominates.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "DFGSafeToExecute.h"
#include "JSCInlines.h"

namespace JSC { namespace DFG {

namespace {

struct LoopData {
    LoopData()
        : preHeader(0)
    {
    }
    
    ClobberSet writes;
    BasicBlock* preHeader;
};

} // anonymous namespace

class LICMPhase : public Phase {
    static const bool verbose = false;
    
public:
    LICMPhase(Graph& graph)
        : Phase(graph, "LICM")
        , m_state(graph)
        , m_interpreter(graph, m_state)
    {
    }
    
    bool run()
    {
        DFG_ASSERT(m_graph, nullptr, m_graph.m_form == SSA);
        
        m_graph.m_dominators.computeIfNecessary(m_graph);
        m_graph.m_naturalLoops.computeIfNecessary(m_graph);
        
        m_data.resize(m_graph.m_naturalLoops.numLoops());
        
        // Figure out the set of things each loop writes to, not including blocks that
        // belong to inner loops. We fix this later.
        for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            
            // Skip blocks that are proved to not execute.
            // FIXME: This shouldn't be needed.
            // https://bugs.webkit.org/show_bug.cgi?id=128584
            if (!block->cfaHasVisited)
                continue;
            
            const NaturalLoop* loop = m_graph.m_naturalLoops.innerMostLoopOf(block);
            if (!loop)
                continue;
            LoopData& data = m_data[loop->index()];
            for (unsigned nodeIndex = 0; nodeIndex < block->size(); ++nodeIndex) {
                Node* node = block->at(nodeIndex);
                
                // Don't look beyond parts of the code that definitely always exit.
                // FIXME: This shouldn't be needed.
                // https://bugs.webkit.org/show_bug.cgi?id=128584
                if (node->op() == ForceOSRExit)
                    break;

                addWrites(m_graph, node, data.writes);
            }
        }
        
        // For each loop:
        // - Identify its pre-header.
        // - Make sure its outer loops know what it clobbers.
        for (unsigned loopIndex = m_graph.m_naturalLoops.numLoops(); loopIndex--;) {
            const NaturalLoop& loop = m_graph.m_naturalLoops.loop(loopIndex);
            LoopData& data = m_data[loop.index()];
            for (
                const NaturalLoop* outerLoop = m_graph.m_naturalLoops.innerMostOuterLoop(loop);
                outerLoop;
                outerLoop = m_graph.m_naturalLoops.innerMostOuterLoop(*outerLoop))
                m_data[outerLoop->index()].writes.addAll(data.writes);
            
            BasicBlock* header = loop.header();
            BasicBlock* preHeader = 0;
            for (unsigned i = header->predecessors.size(); i--;) {
                BasicBlock* predecessor = header->predecessors[i];
                if (m_graph.m_dominators.dominates(header, predecessor))
                    continue;
                DFG_ASSERT(m_graph, nullptr, !preHeader || preHeader == predecessor);
                preHeader = predecessor;
            }
            
            DFG_ASSERT(m_graph, preHeader->terminal(), preHeader->terminal()->op() == Jump);
            
            // We should validate the pre-header. If we placed forExit origins on nodes only if
            // at the top of that node it is legal to exit, then we would simply check if Jump
            // had a forExit. We should disable hoisting to pre-headers that don't validate.
            // Or, we could only allow hoisting of things that definitely don't exit.
            // FIXME: https://bugs.webkit.org/show_bug.cgi?id=145204
            
            data.preHeader = preHeader;
        }
        
        m_graph.initializeNodeOwners();
        
        // Walk all basic blocks that belong to loops, looking for hoisting opportunities.
        // We try to hoist to the outer-most loop that permits it. Hoisting is valid if:
        // - The node doesn't write anything.
        // - The node doesn't read anything that the loop writes.
        // - The preHeader's state at tail makes the node safe to execute.
        // - The loop's children all belong to nodes that strictly dominate the loop header.
        // - The preHeader's state at tail is still valid. This is mostly to save compile
        //   time and preserve some kind of sanity, if we hoist something that must exit.
        //
        // Also, we need to remember to:
        // - Update the state-at-tail with the node we hoisted, so future hoist candidates
        //   know about any type checks we hoisted.
        //
        // For maximum profit, we walk blocks in DFS order to ensure that we generally
        // tend to hoist dominators before dominatees.
        Vector<const NaturalLoop*> loopStack;
        bool changed = false;
        for (BasicBlock* block : m_graph.blocksInPreOrder()) {
            const NaturalLoop* loop = m_graph.m_naturalLoops.innerMostLoopOf(block);
            if (!loop)
                continue;
            
            loopStack.resize(0);
            for (
                const NaturalLoop* current = loop;
                current;
                current = m_graph.m_naturalLoops.innerMostOuterLoop(*current))
                loopStack.append(current);
            
            // Remember: the loop stack has the inner-most loop at index 0, so if we want
            // to bias hoisting to outer loops then we need to use a reverse loop.
            
            if (verbose) {
                dataLog(
                    "Attempting to hoist out of block ", *block, " in loops:\n");
                for (unsigned stackIndex = loopStack.size(); stackIndex--;) {
                    dataLog(
                        "        ", *loopStack[stackIndex], ", which writes ",
                        m_data[loopStack[stackIndex]->index()].writes, "\n");
                }
            }
            
            for (unsigned nodeIndex = 0; nodeIndex < block->size(); ++nodeIndex) {
                Node*& nodeRef = block->at(nodeIndex);
                if (doesWrites(m_graph, nodeRef)) {
                    if (verbose)
                        dataLog("    Not hoisting ", nodeRef, " because it writes things.\n");
                    continue;
                }

                for (unsigned stackIndex = loopStack.size(); stackIndex--;)
                    changed |= attemptHoist(block, nodeRef, loopStack[stackIndex]);
            }
        }
        
        return changed;
    }

private:
    bool attemptHoist(BasicBlock* fromBlock, Node*& nodeRef, const NaturalLoop* loop)
    {
        Node* node = nodeRef;
        LoopData& data = m_data[loop->index()];
        
        if (!data.preHeader->cfaDidFinish) {
            if (verbose)
                dataLog("    Not hoisting ", node, " because CFA is invalid.\n");
            return false;
        }
        
        if (!edgesDominate(m_graph, node, data.preHeader)) {
            if (verbose) {
                dataLog(
                    "    Not hoisting ", node, " because it isn't loop invariant.\n");
            }
            return false;
        }
        
        // FIXME: At this point if the hoisting of the full node fails but the node has type checks,
        // we could still hoist just the checks.
        // https://bugs.webkit.org/show_bug.cgi?id=144525
        
        // FIXME: If a node has a type check - even something like a CheckStructure - then we should
        // only hoist the node if we know that it will execute on every loop iteration or if we know
        // that the type check will always succeed at the loop pre-header through some other means
        // (like looking at prediction propagation results). Otherwise, we might make a mistake like
        // this:
        //
        // var o = ...; // sometimes null and sometimes an object with structure S1.
        // for (...) {
        //     if (o)
        //         ... = o.f; // CheckStructure and GetByOffset, which we will currently hoist.
        // }
        //
        // When we encounter such code, we'll hoist the CheckStructure and GetByOffset and then we
        // will have a recompile. We'll then end up thinking that the get_by_id needs to be
        // polymorphic, which is false.
        //
        // We can counter this by either having a control flow equivalence check, or by consulting
        // prediction propagation to see if the check would always succeed. Prediction propagation
        // would not be enough for things like:
        //
        // var p = ...; // some boolean predicate
        // var o = {};
        // if (p)
        //     o.f = 42;
        // for (...) {
        //     if (p)
        //         ... = o.f;
        // }
        //
        // Prediction propagation can't tell us anything about the structure, and the CheckStructure
        // will appear to be hoistable because the loop doesn't clobber structures. The cell check
        // in the CheckStructure will be hoistable though, since prediction propagation can tell us
        // that o is always SpecFinalObject. In cases like this, control flow equivalence is the
        // only effective guard.
        //
        // https://bugs.webkit.org/show_bug.cgi?id=144527
        
        if (readsOverlap(m_graph, node, data.writes)) {
            if (verbose) {
                dataLog(
                    "    Not hoisting ", node,
                    " because it reads things that the loop writes.\n");
            }
            return false;
        }
        
        m_state.initializeTo(data.preHeader);
        if (!safeToExecute(m_state, m_graph, node)) {
            if (verbose) {
                dataLog(
                    "    Not hoisting ", node, " because it isn't safe to execute.\n");
            }
            return false;
        }
        
        if (verbose) {
            dataLog(
                "    Hoisting ", node, " from ", *fromBlock, " to ", *data.preHeader,
                "\n");
        }
        
        data.preHeader->insertBeforeTerminal(node);
        node->owner = data.preHeader;
        NodeOrigin originalOrigin = node->origin;
        node->origin.forExit = data.preHeader->terminal()->origin.forExit;
        if (!node->origin.semantic.isSet())
            node->origin.semantic = node->origin.forExit;
        
        // Modify the states at the end of the preHeader of the loop we hoisted to,
        // and all pre-headers inside the loop.
        // FIXME: This could become a scalability bottleneck. Fortunately, most loops
        // are small and anyway we rapidly skip over basic blocks here.
        for (unsigned bodyIndex = loop->size(); bodyIndex--;) {
            BasicBlock* subBlock = loop->at(bodyIndex);
            const NaturalLoop* subLoop = m_graph.m_naturalLoops.headerOf(subBlock);
            if (!subLoop)
                continue;
            BasicBlock* subPreHeader = m_data[subLoop->index()].preHeader;
            if (!subPreHeader->cfaDidFinish)
                continue;
            m_state.initializeTo(subPreHeader);
            m_interpreter.execute(node);
        }
        
        // It just so happens that all of the nodes we currently know how to hoist
        // don't have var-arg children. That may change and then we can fix this
        // code. But for now we just assert that's the case.
        DFG_ASSERT(m_graph, node, !(node->flags() & NodeHasVarArgs));
        
        nodeRef = m_graph.addNode(SpecNone, Check, originalOrigin, node->children);
        
        return true;
    }
    
    AtTailAbstractState m_state;
    AbstractInterpreter<AtTailAbstractState> m_interpreter;
    Vector<LoopData> m_data;
};

bool performLICM(Graph& graph)
{
    SamplingRegion samplingRegion("DFG LICM Phase");
    return runPhase<LICMPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

