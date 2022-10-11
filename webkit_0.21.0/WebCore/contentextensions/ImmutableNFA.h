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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ImmutableNFA_h
#define ImmutableNFA_h

#include <wtf/Vector.h>

#if ENABLE(CONTENT_EXTENSIONS)

namespace WebCore {

namespace ContentExtensions {

template <typename CharacterType>
struct ImmutableRange {
    uint32_t targetStart;
    uint32_t targetEnd;
    CharacterType first;
    CharacterType last;
};

struct ImmutableNFANode {
    uint32_t rangesStart { 0 };
    uint32_t rangesEnd { 0 };
    uint32_t epsilonTransitionTargetsStart { 0 };
    uint32_t epsilonTransitionTargetsEnd { 0 };
    uint32_t actionStart { 0 };
    uint32_t actionEnd { 0 };
};

template <typename CharacterType, typename ActionType>
struct ImmutableNFA {
    Vector<ImmutableNFANode, 0, ContentExtensionsOverflowHandler> nodes;
    Vector<ImmutableRange<CharacterType>, 0, ContentExtensionsOverflowHandler> transitions;
    Vector<uint32_t, 0, ContentExtensionsOverflowHandler> targets;
    Vector<uint32_t, 0, ContentExtensionsOverflowHandler> epsilonTransitionsTargets;
    Vector<ActionType, 0, ContentExtensionsOverflowHandler> actions;

    struct ConstTargetIterator {
        const ImmutableNFA& immutableNFA;
        uint32_t position;

        const uint32_t& operator*() const { return immutableNFA.targets[position]; }
        const uint32_t* operator->() const { return &immutableNFA.targets[position]; }

        bool operator==(const ConstTargetIterator& other) const
        {
            ASSERT(&immutableNFA == &other.immutableNFA);
            return position == other.position;
        }
        bool operator!=(const ConstTargetIterator& other) const { return !(*this == other); }

        ConstTargetIterator& operator++()
        {
            ++position;
            return *this;
        }
    };

    struct IterableConstTargets {
        const ImmutableNFA& immutableNFA;
        uint32_t targetStart;
        uint32_t targetEnd;

        ConstTargetIterator begin() const { return { immutableNFA, targetStart }; }
        ConstTargetIterator end() const { return { immutableNFA, targetEnd }; }
    };

    struct ConstRangeIterator {
        const ImmutableNFA& immutableNFA;
        uint32_t position;

        bool operator==(const ConstRangeIterator& other) const
        {
            ASSERT(&immutableNFA == &other.immutableNFA);
            return position == other.position;
        }
        bool operator!=(const ConstRangeIterator& other) const { return !(*this == other); }

        ConstRangeIterator& operator++()
        {
            ++position;
            return *this;
        }

        CharacterType first() const
        {
            return range().first;
        }

        CharacterType last() const
        {
            return range().last;
        }

        IterableConstTargets data() const
        {
            const ImmutableRange<CharacterType>& range = this->range();
            return { immutableNFA, range.targetStart, range.targetEnd };
        };

    private:
        const ImmutableRange<CharacterType>& range() const
        {
            return immutableNFA.transitions[position];
        }
    };

    struct IterableConstRange {
        const ImmutableNFA& immutableNFA;
        uint32_t rangesStart;
        uint32_t rangesEnd;

        ConstRangeIterator begin() const { return { immutableNFA, rangesStart }; }
        ConstRangeIterator end() const { return { immutableNFA, rangesEnd }; }

#if CONTENT_EXTENSIONS_STATE_MACHINE_DEBUGGING
        void debugPrint() const
        {
            for (const auto& range : *this)
                WTFLogAlways("    %d-%d", range.first, range.last);
        }
#endif
    };

    IterableConstRange transitionsForNode(uint32_t nodeId) const
    {
        const ImmutableNFANode& node = nodes[nodeId];
        return { *this, node.rangesStart, node.rangesEnd };
    };

    uint32_t root() const
    {
        RELEASE_ASSERT(!nodes.isEmpty());
        return 0;
    }

    void finalize()
    {
        nodes.shrinkToFit();
        transitions.shrinkToFit();
        targets.shrinkToFit();
        epsilonTransitionsTargets.shrinkToFit();
        actions.shrinkToFit();
    }

    size_t memoryUsed() const
    {
        return nodes.capacity() * sizeof(ImmutableNFANode)
            + transitions.capacity() * sizeof(ImmutableRange<CharacterType>)
            + targets.capacity() * sizeof(uint32_t)
            + epsilonTransitionsTargets.capacity() * sizeof(uint32_t)
            + actions.capacity() * sizeof(ActionType);
    }
};

}

} // namespace WebCore

#endif // ENABLE(CONTENT_EXTENSIONS)

#endif // ImmutableNFA_h
