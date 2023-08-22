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
#include "DFGInPlaceAbstractState.h"

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGBasicBlock.h"
#include "GetByIdStatus.h"
#include "JSCInlines.h"
#include "PutByIdStatus.h"
#include "StringObject.h"

namespace JSC { namespace DFG {

static const bool verbose = false;

InPlaceAbstractState::InPlaceAbstractState(Graph& graph)
    : m_graph(graph)
    , m_variables(m_graph.m_codeBlock->numParameters(), graph.m_localVars)
    , m_block(0)
{
}

InPlaceAbstractState::~InPlaceAbstractState() { }

void InPlaceAbstractState::beginBasicBlock(BasicBlock* basicBlock)
{
    ASSERT(!m_block);
    
    ASSERT(basicBlock->variablesAtHead.numberOfLocals() == basicBlock->valuesAtHead.numberOfLocals());
    ASSERT(basicBlock->variablesAtTail.numberOfLocals() == basicBlock->valuesAtTail.numberOfLocals());
    ASSERT(basicBlock->variablesAtHead.numberOfLocals() == basicBlock->variablesAtTail.numberOfLocals());
    
    for (size_t i = 0; i < basicBlock->size(); i++)
        forNode(basicBlock->at(i)).clear();

    m_variables = basicBlock->valuesAtHead;
    
    if (m_graph.m_form == SSA) {
        HashMap<Node*, AbstractValue>::iterator iter = basicBlock->ssa->valuesAtHead.begin();
        HashMap<Node*, AbstractValue>::iterator end = basicBlock->ssa->valuesAtHead.end();
        for (; iter != end; ++iter)
            forNode(iter->key) = iter->value;
    }
    basicBlock->cfaShouldRevisit = false;
    basicBlock->cfaHasVisited = true;
    m_block = basicBlock;
    m_isValid = true;
    m_foundConstants = false;
    m_branchDirection = InvalidBranchDirection;
    m_structureClobberState = basicBlock->cfaStructureClobberStateAtHead;
}

static void setLiveValues(HashMap<Node*, AbstractValue>& values, HashSet<Node*>& live)
{
    values.clear();
    
    HashSet<Node*>::iterator iter = live.begin();
    HashSet<Node*>::iterator end = live.end();
    for (; iter != end; ++iter)
        values.add(*iter, AbstractValue());
}

void InPlaceAbstractState::initialize()
{
    BasicBlock* root = m_graph.block(0);
    root->cfaShouldRevisit = true;
    root->cfaHasVisited = false;
    root->cfaFoundConstants = false;
    root->cfaStructureClobberStateAtHead = StructuresAreWatched;
    root->cfaStructureClobberStateAtTail = StructuresAreWatched;
    for (size_t i = 0; i < root->valuesAtHead.numberOfArguments(); ++i) {
        root->valuesAtTail.argument(i).clear();

        FlushFormat format;
        if (m_graph.m_form == SSA)
            format = m_graph.m_argumentFormats[i];
        else {
            Node* node = m_graph.m_arguments[i];
            if (!node)
                format = FlushedJSValue;
            else {
                ASSERT(node->op() == SetArgument);
                format = node->variableAccessData()->flushFormat();
            }
        }
        
        switch (format) {
        case FlushedInt32:
            root->valuesAtHead.argument(i).setType(SpecInt32);
            break;
        case FlushedBoolean:
            root->valuesAtHead.argument(i).setType(SpecBoolean);
            break;
        case FlushedCell:
            root->valuesAtHead.argument(i).setType(m_graph, SpecCell);
            break;
        case FlushedJSValue:
            root->valuesAtHead.argument(i).makeHeapTop();
            break;
        default:
            DFG_CRASH(m_graph, nullptr, "Bad flush format for argument");
            break;
        }
    }
    for (size_t i = 0; i < root->valuesAtHead.numberOfLocals(); ++i) {
        root->valuesAtHead.local(i).clear();
        root->valuesAtTail.local(i).clear();
    }
    for (BlockIndex blockIndex = 1 ; blockIndex < m_graph.numBlocks(); ++blockIndex) {
        BasicBlock* block = m_graph.block(blockIndex);
        if (!block)
            continue;
        ASSERT(block->isReachable);
        block->cfaShouldRevisit = false;
        block->cfaHasVisited = false;
        block->cfaFoundConstants = false;
        block->cfaStructureClobberStateAtHead = StructuresAreWatched;
        block->cfaStructureClobberStateAtTail = StructuresAreWatched;
        for (size_t i = 0; i < block->valuesAtHead.numberOfArguments(); ++i) {
            block->valuesAtHead.argument(i).clear();
            block->valuesAtTail.argument(i).clear();
        }
        for (size_t i = 0; i < block->valuesAtHead.numberOfLocals(); ++i) {
            block->valuesAtHead.local(i).clear();
            block->valuesAtTail.local(i).clear();
        }
    }
    if (m_graph.m_form == SSA) {
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            setLiveValues(block->ssa->valuesAtHead, block->ssa->liveAtHead);
            setLiveValues(block->ssa->valuesAtTail, block->ssa->liveAtTail);
        }
    }
}

bool InPlaceAbstractState::endBasicBlock(MergeMode mergeMode)
{
    ASSERT(m_block);
    
    BasicBlock* block = m_block; // Save the block for successor merging.
    
    block->cfaFoundConstants = m_foundConstants;
    block->cfaDidFinish = m_isValid;
    block->cfaBranchDirection = m_branchDirection;
    
    if (!m_isValid) {
        reset();
        return false;
    }
    
    bool changed = false;
    
    if ((mergeMode != DontMerge) || !ASSERT_DISABLED) {
        changed |= checkAndSet(block->cfaStructureClobberStateAtTail, m_structureClobberState);
    
        switch (m_graph.m_form) {
        case ThreadedCPS: {
            for (size_t argument = 0; argument < block->variablesAtTail.numberOfArguments(); ++argument) {
                AbstractValue& destination = block->valuesAtTail.argument(argument);
                changed |= mergeStateAtTail(destination, m_variables.argument(argument), block->variablesAtTail.argument(argument));
            }
            
            for (size_t local = 0; local < block->variablesAtTail.numberOfLocals(); ++local) {
                AbstractValue& destination = block->valuesAtTail.local(local);
                changed |= mergeStateAtTail(destination, m_variables.local(local), block->variablesAtTail.local(local));
            }
            break;
        }
            
        case SSA: {
            for (size_t i = 0; i < block->valuesAtTail.size(); ++i)
                changed |= block->valuesAtTail[i].merge(m_variables[i]);
            
            HashSet<Node*>::iterator iter = block->ssa->liveAtTail.begin();
            HashSet<Node*>::iterator end = block->ssa->liveAtTail.end();
            for (; iter != end; ++iter) {
                Node* node = *iter;
                changed |= block->ssa->valuesAtTail.find(node)->value.merge(forNode(node));
            }
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    ASSERT(mergeMode != DontMerge || !changed);
    
    reset();
    
    if (mergeMode != MergeToSuccessors)
        return changed;
    
    return mergeToSuccessors(block);
}

void InPlaceAbstractState::reset()
{
    m_block = 0;
    m_isValid = false;
    m_branchDirection = InvalidBranchDirection;
    m_structureClobberState = StructuresAreWatched;
}

bool InPlaceAbstractState::mergeStateAtTail(AbstractValue& destination, AbstractValue& inVariable, Node* node)
{
    if (!node)
        return false;
        
    AbstractValue source;
    
    switch (node->op()) {
    case Phi:
    case SetArgument:
    case PhantomLocal:
    case Flush:
        // The block transfers the value from head to tail.
        source = inVariable;
        break;
            
    case GetLocal:
        // The block refines the value with additional speculations.
        source = forNode(node);
        break;
            
    case SetLocal:
        // The block sets the variable, and potentially refines it, both
        // before and after setting it.
        source = forNode(node->child1());
        if (node->variableAccessData()->flushFormat() == FlushedDouble)
            RELEASE_ASSERT(!(source.m_type & ~SpecFullDouble));
        break;
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    
    if (destination == source) {
        // Abstract execution did not change the output value of the variable, for this
        // basic block, on this iteration.
        return false;
    }
    
    // Abstract execution reached a new conclusion about the speculations reached about
    // this variable after execution of this basic block. Update the state, and return
    // true to indicate that the fixpoint must go on!
    destination = source;
    return true;
}

bool InPlaceAbstractState::merge(BasicBlock* from, BasicBlock* to)
{
    if (verbose)
        dataLog("   Merging from ", pointerDump(from), " to ", pointerDump(to), "\n");
    ASSERT(from->variablesAtTail.numberOfArguments() == to->variablesAtHead.numberOfArguments());
    ASSERT(from->variablesAtTail.numberOfLocals() == to->variablesAtHead.numberOfLocals());
    
    bool changed = false;
    
    changed |= checkAndSet(
        to->cfaStructureClobberStateAtHead,
        DFG::merge(from->cfaStructureClobberStateAtTail, to->cfaStructureClobberStateAtHead));
    
    switch (m_graph.m_form) {
    case ThreadedCPS: {
        for (size_t argument = 0; argument < from->variablesAtTail.numberOfArguments(); ++argument) {
            AbstractValue& destination = to->valuesAtHead.argument(argument);
            changed |= mergeVariableBetweenBlocks(destination, from->valuesAtTail.argument(argument), to->variablesAtHead.argument(argument), from->variablesAtTail.argument(argument));
        }
        
        for (size_t local = 0; local < from->variablesAtTail.numberOfLocals(); ++local) {
            AbstractValue& destination = to->valuesAtHead.local(local);
            changed |= mergeVariableBetweenBlocks(destination, from->valuesAtTail.local(local), to->variablesAtHead.local(local), from->variablesAtTail.local(local));
        }
        break;
    }
        
    case SSA: {
        for (size_t i = from->valuesAtTail.size(); i--;)
            changed |= to->valuesAtHead[i].merge(from->valuesAtTail[i]);
        
        HashSet<Node*>::iterator iter = to->ssa->liveAtHead.begin();
        HashSet<Node*>::iterator end = to->ssa->liveAtHead.end();
        for (; iter != end; ++iter) {
            Node* node = *iter;
            if (verbose)
                dataLog("      Merging for ", node, ": from ", from->ssa->valuesAtTail.find(node)->value, " to ", to->ssa->valuesAtHead.find(node)->value, "\n");
            changed |= to->ssa->valuesAtHead.find(node)->value.merge(
                from->ssa->valuesAtTail.find(node)->value);
            if (verbose)
                dataLog("         Result: ", to->ssa->valuesAtHead.find(node)->value, "\n");
        }
        break;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    if (!to->cfaHasVisited)
        changed = true;
    
    if (verbose)
        dataLog("      Will revisit: ", changed, "\n");
    to->cfaShouldRevisit |= changed;
    
    return changed;
}

inline bool InPlaceAbstractState::mergeToSuccessors(BasicBlock* basicBlock)
{
    Node* terminal = basicBlock->terminal();
    
    ASSERT(terminal->isTerminal());
    
    switch (terminal->op()) {
    case Jump: {
        ASSERT(basicBlock->cfaBranchDirection == InvalidBranchDirection);
        return merge(basicBlock, terminal->targetBlock());
    }
        
    case Branch: {
        ASSERT(basicBlock->cfaBranchDirection != InvalidBranchDirection);
        bool changed = false;
        if (basicBlock->cfaBranchDirection != TakeFalse)
            changed |= merge(basicBlock, terminal->branchData()->taken.block);
        if (basicBlock->cfaBranchDirection != TakeTrue)
            changed |= merge(basicBlock, terminal->branchData()->notTaken.block);
        return changed;
    }
        
    case Switch: {
        // FIXME: It would be cool to be sparse conditional for Switch's. Currently
        // we're not. However I somehow doubt that this will ever be a big deal.
        ASSERT(basicBlock->cfaBranchDirection == InvalidBranchDirection);
        SwitchData* data = terminal->switchData();
        bool changed = merge(basicBlock, data->fallThrough.block);
        for (unsigned i = data->cases.size(); i--;)
            changed |= merge(basicBlock, data->cases[i].target.block);
        return changed;
    }
        
    case Return:
    case TailCall:
    case TailCallVarargs:
    case TailCallForwardVarargs:
    case Unreachable:
        ASSERT(basicBlock->cfaBranchDirection == InvalidBranchDirection);
        return false;

    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

inline bool InPlaceAbstractState::mergeVariableBetweenBlocks(AbstractValue& destination, AbstractValue& source, Node* destinationNode, Node* sourceNode)
{
    if (!destinationNode)
        return false;
    
    ASSERT_UNUSED(sourceNode, sourceNode);
    
    // FIXME: We could do some sparse conditional propagation here!
    
    return destination.merge(source);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

