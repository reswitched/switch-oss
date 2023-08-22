/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "DFGArgumentsEliminationPhase.h"

#if ENABLE(DFG_JIT)

#include "BytecodeLivenessAnalysisInlines.h"
#include "DFGArgumentsUtilities.h"
#include "DFGBasicBlockInlines.h"
#include "DFGBlockMapInlines.h"
#include "DFGClobberize.h"
#include "DFGCombinedLiveness.h"
#include "DFGForAllKills.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGLivenessAnalysisPhase.h"
#include "DFGOSRAvailabilityAnalysisPhase.h"
#include "DFGPhase.h"
#include "JSCInlines.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/ListDump.h>

namespace JSC { namespace DFG {

namespace {

#if !PLATFORM(WKC)
bool verbose = false;
#else
WKC_DEFINE_GLOBAL_BOOL(verbose, false);
#endif

class ArgumentsEliminationPhase : public Phase {
public:
    ArgumentsEliminationPhase(Graph& graph)
        : Phase(graph, "arguments elimination")
    {
    }
    
    bool run()
    {
        // For now this phase only works on SSA. This could be changed; we could have a block-local
        // version over LoadStore.
        DFG_ASSERT(m_graph, nullptr, m_graph.m_form == SSA);
        
        if (verbose) {
            dataLog("Graph before arguments elimination:\n");
            m_graph.dump();
        }
        
        identifyCandidates();
        if (m_candidates.isEmpty())
            return false;
        
        eliminateCandidatesThatEscape();
        if (m_candidates.isEmpty())
            return false;
        
        eliminateCandidatesThatInterfere();
        if (m_candidates.isEmpty())
            return false;
        
        transform();
        
        return true;
    }

private:
    // Just finds nodes that we know how to work with.
    void identifyCandidates()
    {
        for (BasicBlock* block : m_graph.blocksInNaturalOrder()) {
            for (Node* node : *block) {
                switch (node->op()) {
                case CreateDirectArguments:
                case CreateClonedArguments:
                    m_candidates.add(node);
                    break;
                    
                case CreateScopedArguments:
                    // FIXME: We could handle this if it wasn't for the fact that scoped arguments are
                    // always stored into the activation.
                    // https://bugs.webkit.org/show_bug.cgi?id=143072 and
                    // https://bugs.webkit.org/show_bug.cgi?id=143073
                    break;
                    
                default:
                    break;
                }
            }
        }
        
        if (verbose)
            dataLog("Candidates: ", listDump(m_candidates), "\n");
    }
    
    // Look for escaping sites, and remove from the candidates set if we see an escape.
    void eliminateCandidatesThatEscape()
    {
        auto escape = [&] (Edge edge) {
            if (!edge)
                return;
            m_candidates.remove(edge.node());
        };
        
        auto escapeBasedOnArrayMode = [&] (ArrayMode mode, Edge edge) {
            switch (mode.type()) {
            case Array::DirectArguments:
                if (edge->op() != CreateDirectArguments)
                    escape(edge);
                break;
            
            case Array::Int32:
            case Array::Double:
            case Array::Contiguous:
                if (edge->op() != CreateClonedArguments)
                    escape(edge);
                break;
            
            default:
                escape(edge);
                break;
            }
        };
        
        for (BasicBlock* block : m_graph.blocksInNaturalOrder()) {
            for (Node* node : *block) {
                switch (node->op()) {
                case GetFromArguments:
                    break;
                    
                case GetByVal:
                    escapeBasedOnArrayMode(node->arrayMode(), node->child1());
                    escape(node->child2());
                    escape(node->child3());
                    break;
                    
                case GetArrayLength:
                    escapeBasedOnArrayMode(node->arrayMode(), node->child1());
                    escape(node->child2());
                    break;
                    
                case LoadVarargs:
                    break;
                    
                case CallVarargs:
                case ConstructVarargs:
                case TailCallVarargs:
                case TailCallVarargsInlinedCaller:
                    escape(node->child1());
                    escape(node->child2());
                    break;

                case Check:
                    m_graph.doToChildren(
                        node,
                        [&] (Edge edge) {
                            if (edge.willNotHaveCheck())
                                return;
                            
                            if (alreadyChecked(edge.useKind(), SpecObject))
                                return;
                            
                            escape(edge);
                        });
                    break;
                    
                case MovHint:
                case PutHint:
                    break;
                    
                case GetButterfly:
                    // This barely works. The danger is that the GetButterfly is used by something that
                    // does something escaping to a candidate. Fortunately, the only butterfly-using ops
                    // that we exempt here also use the candidate directly. If there ever was a
                    // butterfly-using op that we wanted to exempt, then we'd have to look at the
                    // butterfly's child and check if it's a candidate.
                    break;
                    
                case CheckArray:
                    escapeBasedOnArrayMode(node->arrayMode(), node->child1());
                    break;
                    
                // FIXME: For cloned arguments, we'd like to allow GetByOffset on length to not be
                // an escape.
                // https://bugs.webkit.org/show_bug.cgi?id=143074
                    
                // FIXME: We should be able to handle GetById/GetByOffset on callee.
                // https://bugs.webkit.org/show_bug.cgi?id=143075
                    
                default:
                    m_graph.doToChildren(node, escape);
                    break;
                }
            }
        }

        if (verbose)
            dataLog("After escape analysis: ", listDump(m_candidates), "\n");
    }

    // Anywhere that a candidate is live (in bytecode or in DFG), check if there is a chance of
    // interference between the stack area that the arguments object copies from and the arguments
    // object's payload. Conservatively this means that the stack region doesn't get stored to.
    void eliminateCandidatesThatInterfere()
    {
        performLivenessAnalysis(m_graph);
        performOSRAvailabilityAnalysis(m_graph);
        m_graph.initializeNodeOwners();
        CombinedLiveness combinedLiveness(m_graph);
        
        BlockMap<Operands<bool>> clobberedByBlock(m_graph);
        for (BasicBlock* block : m_graph.blocksInNaturalOrder()) {
            Operands<bool>& clobberedByThisBlock = clobberedByBlock[block];
            clobberedByThisBlock = Operands<bool>(OperandsLike, m_graph.block(0)->variablesAtHead);
            for (Node* node : *block) {
                clobberize(
                    m_graph, node, NoOpClobberize(),
                    [&] (AbstractHeap heap) {
                        if (heap.kind() != Stack) {
                            ASSERT(!heap.overlaps(Stack));
                            return;
                        }
                        ASSERT(!heap.payload().isTop());
                        VirtualRegister reg(heap.payload().value32());
                        clobberedByThisBlock.operand(reg) = true;
                    },
                    NoOpClobberize());
            }
        }
        
        for (BasicBlock* block : m_graph.blocksInNaturalOrder()) {
            // Stop if we've already removed all candidates.
            if (m_candidates.isEmpty())
                return;
            
            // Ignore blocks that don't write to the stack.
            bool writesToStack = false;
            for (unsigned i = clobberedByBlock[block].size(); i--;) {
                if (clobberedByBlock[block][i]) {
                    writesToStack = true;
                    break;
                }
            }
            if (!writesToStack)
                continue;
            
            forAllKillsInBlock(
                m_graph, combinedLiveness, block,
                [&] (unsigned nodeIndex, Node* candidate) {
                    if (!m_candidates.contains(candidate))
                        return;
                    
                    // Check if this block has any clobbers that affect this candidate. This is a fairly
                    // fast check.
                    bool isClobberedByBlock = false;
                    Operands<bool>& clobberedByThisBlock = clobberedByBlock[block];
                    
                    if (InlineCallFrame* inlineCallFrame = candidate->origin.semantic.inlineCallFrame) {
                        if (inlineCallFrame->isVarargs()) {
                            isClobberedByBlock |= clobberedByThisBlock.operand(
                                inlineCallFrame->stackOffset + JSStack::ArgumentCount);
                        }
                        
                        if (!isClobberedByBlock || inlineCallFrame->isClosureCall) {
                            isClobberedByBlock |= clobberedByThisBlock.operand(
                                inlineCallFrame->stackOffset + JSStack::Callee);
                        }
                        
                        if (!isClobberedByBlock) {
                            for (unsigned i = 0; i < inlineCallFrame->arguments.size() - 1; ++i) {
                                VirtualRegister reg =
                                    VirtualRegister(inlineCallFrame->stackOffset) +
                                    CallFrame::argumentOffset(i);
                                if (clobberedByThisBlock.operand(reg)) {
                                    isClobberedByBlock = true;
                                    break;
                                }
                            }
                        }
                    } else {
                        // We don't include the ArgumentCount or Callee in this case because we can be
                        // damn sure that this won't be clobbered.
                        for (unsigned i = 1; i < static_cast<unsigned>(codeBlock()->numParameters()); ++i) {
                            if (clobberedByThisBlock.argument(i)) {
                                isClobberedByBlock = true;
                                break;
                            }
                        }
                    }
                    
                    if (!isClobberedByBlock)
                        return;
                    
                    // Check if we can immediately eliminate this candidate. If the block has a clobber
                    // for this arguments allocation, and we'd have to examine every node in the block,
                    // then we can just eliminate the candidate.
                    if (nodeIndex == block->size() && candidate->owner != block) {
                        m_candidates.remove(candidate);
                        return;
                    }
                    
                    // This loop considers all nodes up to the nodeIndex, excluding the nodeIndex.
                    while (nodeIndex--) {
                        Node* node = block->at(nodeIndex);
                        if (node == candidate)
                            break;
                        
                        bool found = false;
                        clobberize(
                            m_graph, node, NoOpClobberize(),
                            [&] (AbstractHeap heap) {
                                if (heap.kind() == Stack && !heap.payload().isTop()) {
                                    if (argumentsInvolveStackSlot(candidate, VirtualRegister(heap.payload().value32())))
                                        found = true;
                                    return;
                                }
                                if (heap.overlaps(Stack))
                                    found = true;
                            },
                            NoOpClobberize());
                        
                        if (found) {
                            m_candidates.remove(candidate);
                            return;
                        }
                    }
                });
        }
        
        // Q: How do we handle OSR exit with a live PhantomArguments at a point where the inline call
        // frame is dead?  A: Naively we could say that PhantomArguments must escape the stack slots. But
        // that would break PutStack sinking, which in turn would break object allocation sinking, in
        // cases where we have a varargs call to an otherwise pure method. So, we need something smarter.
        // For the outermost arguments, we just have a PhantomArguments that magically knows that it
        // should load the arguments from the call frame. For the inline arguments, we have the heap map
        // in the availabiltiy map track each possible inline argument as a promoted heap location. If the
        // PutStacks for those arguments aren't sunk, those heap locations will map to very trivial
        // availabilities (they will be flush availabilities). But if sinking happens then those
        // availabilities may become whatever. OSR exit should be able to handle this quite naturally,
        // since those availabilities speak of the stack before the optimizing compiler stack frame is
        // torn down.

        if (verbose)
            dataLog("After interference analysis: ", listDump(m_candidates), "\n");
    }
    
    void transform()
    {
        InsertionSet insertionSet(m_graph);
        
        for (BasicBlock* block : m_graph.blocksInNaturalOrder()) {
            for (unsigned nodeIndex = 0; nodeIndex < block->size(); ++nodeIndex) {
                Node* node = block->at(nodeIndex);
                
                auto getArrayLength = [&] (Node* candidate) -> Node* {
                    return emitCodeToGetArgumentsArrayLength(
                        insertionSet, candidate, nodeIndex, node->origin);
                };
        
                switch (node->op()) {
                case CreateDirectArguments:
                    if (!m_candidates.contains(node))
                        break;
                    
                    node->setOpAndDefaultFlags(PhantomDirectArguments);
                    break;
                    
                case CreateClonedArguments:
                    if (!m_candidates.contains(node))
                        break;
                    
                    node->setOpAndDefaultFlags(PhantomClonedArguments);
                    break;
                    
                case GetFromArguments: {
                    Node* candidate = node->child1().node();
                    if (!m_candidates.contains(candidate))
                        break;
                    
                    DFG_ASSERT(
                        m_graph, node,
                        node->child1()->op() == CreateDirectArguments
                        || node->child1()->op() == PhantomDirectArguments);
                    VirtualRegister reg =
                        virtualRegisterForArgument(node->capturedArgumentsOffset().offset() + 1) +
                        node->origin.semantic.stackOffset();
                    StackAccessData* data = m_graph.m_stackAccessData.add(reg, FlushedJSValue);
                    node->convertToGetStack(data);
                    break;
                }
                    
                case GetArrayLength: {
                    Node* candidate = node->child1().node();
                    if (!m_candidates.contains(candidate))
                        break;
                    
                    // Meh, this is kind of hackish - we use an Identity so that we can reuse the
                    // getArrayLength() helper.
                    node->convertToIdentityOn(getArrayLength(candidate));
                    break;
                }
                    
                case GetByVal: {
                    // FIXME: For ClonedArguments, we would have already done a separate bounds check.
                    // This code will cause us to have two bounds checks - the original one that we
                    // already factored out in SSALoweringPhase, and the new one we insert here, which is
                    // often implicitly part of GetMyArgumentByVal. LLVM will probably eliminate the
                    // second bounds check, but still - that's just silly.
                    // https://bugs.webkit.org/show_bug.cgi?id=143076
                    
                    Node* candidate = node->child1().node();
                    if (!m_candidates.contains(candidate))
                        break;
                    
                    Node* result = nullptr;
                    if (node->child2()->isInt32Constant()) {
                        unsigned index = node->child2()->asUInt32();
                        InlineCallFrame* inlineCallFrame = candidate->origin.semantic.inlineCallFrame;
                        
                        bool safeToGetStack;
                        if (inlineCallFrame)
                            safeToGetStack = index < inlineCallFrame->arguments.size() - 1;
                        else {
                            safeToGetStack =
                                index < static_cast<unsigned>(codeBlock()->numParameters()) - 1;
                        }
                        if (safeToGetStack) {
                            StackAccessData* data;
                            VirtualRegister arg = virtualRegisterForArgument(index + 1);
                            if (inlineCallFrame)
                                arg += inlineCallFrame->stackOffset;
                            data = m_graph.m_stackAccessData.add(arg, FlushedJSValue);
                            
                            if (!inlineCallFrame || inlineCallFrame->isVarargs()
                                || index >= inlineCallFrame->arguments.size() - 1) {
                                insertionSet.insertNode(
                                    nodeIndex, SpecNone, CheckInBounds, node->origin,
                                    node->child2(), Edge(getArrayLength(candidate), Int32Use));
                            }
                            
                            result = insertionSet.insertNode(
                                nodeIndex, node->prediction(), GetStack, node->origin, OpInfo(data));
                        }
                    }
                    
                    if (!result) {
                        result = insertionSet.insertNode(
                            nodeIndex, node->prediction(), GetMyArgumentByVal, node->origin,
                            node->child1(), node->child2());
                    }
                    
                    // Need to do this because we may have a data format conversion here.
                    node->convertToIdentityOn(result);
                    break;
                }
                    
                case LoadVarargs: {
                    Node* candidate = node->child1().node();
                    if (!m_candidates.contains(candidate))
                        break;
                    
                    LoadVarargsData* varargsData = node->loadVarargsData();
                    InlineCallFrame* inlineCallFrame = candidate->origin.semantic.inlineCallFrame;
                    if (inlineCallFrame
                        && !inlineCallFrame->isVarargs()
                        && inlineCallFrame->arguments.size() - varargsData->offset <= varargsData->limit) {
                        Node* argumentCount = insertionSet.insertConstant(
                            nodeIndex, node->origin,
                            jsNumber(inlineCallFrame->arguments.size() - varargsData->offset));
                        insertionSet.insertNode(
                            nodeIndex, SpecNone, MovHint, node->origin,
                            OpInfo(varargsData->count.offset()), Edge(argumentCount));
                        insertionSet.insertNode(
                            nodeIndex, SpecNone, PutStack, node->origin,
                            OpInfo(m_graph.m_stackAccessData.add(varargsData->count, FlushedInt32)),
                            Edge(argumentCount, Int32Use));
                        
                        DFG_ASSERT(m_graph, node, varargsData->limit - 1 >= varargsData->mandatoryMinimum);
                        // Define our limit to not include "this", since that's a bit easier to reason about.
                        unsigned limit = varargsData->limit - 1;
                        Node* undefined = nullptr;
                        for (unsigned storeIndex = 0; storeIndex < limit; ++storeIndex) {
                            // First determine if we have an element we can load, and load it if
                            // possible.
                            
                            unsigned loadIndex = storeIndex + varargsData->offset;
                            
                            Node* value;
                            if (loadIndex + 1 < inlineCallFrame->arguments.size()) {
                                VirtualRegister reg =
                                    virtualRegisterForArgument(loadIndex + 1) +
                                    inlineCallFrame->stackOffset;
                                StackAccessData* data = m_graph.m_stackAccessData.add(
                                    reg, FlushedJSValue);
                                
                                value = insertionSet.insertNode(
                                    nodeIndex, SpecNone, GetStack, node->origin, OpInfo(data));
                            } else {
                                // FIXME: We shouldn't have to store anything if
                                // storeIndex >= varargsData->mandatoryMinimum, but we will still
                                // have GetStacks in that range. So if we don't do the stores, we'll
                                // have degenerate IR: we'll have GetStacks of something that didn't
                                // have PutStacks.
                                // https://bugs.webkit.org/show_bug.cgi?id=147434
                                
                                if (!undefined) {
                                    undefined = insertionSet.insertConstant(
                                        nodeIndex, node->origin, jsUndefined());
                                }
                                value = undefined;
                            }
                            
                            // Now that we have a value, store it.
                            
                            VirtualRegister reg = varargsData->start + storeIndex;
                            StackAccessData* data =
                                m_graph.m_stackAccessData.add(reg, FlushedJSValue);
                            
                            insertionSet.insertNode(
                                nodeIndex, SpecNone, MovHint, node->origin, OpInfo(reg.offset()),
                                Edge(value));
                            insertionSet.insertNode(
                                nodeIndex, SpecNone, PutStack, node->origin, OpInfo(data),
                                Edge(value));
                        }
                        
                        node->remove();
                        break;
                    }
                    
                    node->setOpAndDefaultFlags(ForwardVarargs);
                    break;
                }
                    
                case CallVarargs:
                case ConstructVarargs:
                case TailCallVarargs:
                case TailCallVarargsInlinedCaller: {
                    Node* candidate = node->child3().node();
                    if (!m_candidates.contains(candidate))
                        break;
                    
                    CallVarargsData* varargsData = node->callVarargsData();
                    InlineCallFrame* inlineCallFrame = candidate->origin.semantic.inlineCallFrame;
                    if (inlineCallFrame && !inlineCallFrame->isVarargs()) {
                        Vector<Node*> arguments;
                        for (unsigned i = 1 + varargsData->firstVarArgOffset; i < inlineCallFrame->arguments.size(); ++i) {
                            StackAccessData* data = m_graph.m_stackAccessData.add(
                                virtualRegisterForArgument(i) + inlineCallFrame->stackOffset,
                                FlushedJSValue);
                            
                            Node* value = insertionSet.insertNode(
                                nodeIndex, SpecNone, GetStack, node->origin, OpInfo(data));
                            
                            arguments.append(value);
                        }
                        
                        unsigned firstChild = m_graph.m_varArgChildren.size();
                        m_graph.m_varArgChildren.append(node->child1());
                        m_graph.m_varArgChildren.append(node->child2());
                        for (Node* argument : arguments)
                            m_graph.m_varArgChildren.append(Edge(argument));
                        switch (node->op()) {
                        case CallVarargs:
                            node->setOpAndDefaultFlags(Call);
                            break;
                        case ConstructVarargs:
                            node->setOpAndDefaultFlags(Construct);
                            break;
                        case TailCallVarargs:
                            node->setOpAndDefaultFlags(TailCall);
                            break;
                        case TailCallVarargsInlinedCaller:
                            node->setOpAndDefaultFlags(TailCallInlinedCaller);
                            break;
                        default:
                            RELEASE_ASSERT_NOT_REACHED();
                        }
                        node->children = AdjacencyList(
                            AdjacencyList::Variable,
                            firstChild, m_graph.m_varArgChildren.size() - firstChild);
                        break;
                    }
                    
                    switch (node->op()) {
                    case CallVarargs:
                        node->setOpAndDefaultFlags(CallForwardVarargs);
                        break;
                    case ConstructVarargs:
                        node->setOpAndDefaultFlags(ConstructForwardVarargs);
                        break;
                    case TailCallVarargs:
                        node->setOpAndDefaultFlags(TailCallForwardVarargs);
                        break;
                    case TailCallVarargsInlinedCaller:
                        node->setOpAndDefaultFlags(TailCallForwardVarargsInlinedCaller);
                        break;
                    default:
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                    break;
                }
                    
                case CheckArray:
                case GetButterfly: {
                    if (!m_candidates.contains(node->child1().node()))
                        break;
                    node->remove();
                    break;
                }
                    
                default:
                    break;
                }
            }
            
            insertionSet.execute(block);
        }
    }
    
    HashSet<Node*> m_candidates;
};

} // anonymous namespace

bool performArgumentsElimination(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Arguments Elimination Phase");
    return runPhase<ArgumentsEliminationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

