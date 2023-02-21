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
#include "DFGIntegerRangeOptimizationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBlockMapInlines.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "DFGPredictionPropagationPhase.h"
#include "DFGVariableAccessDataDump.h"
#include "JSCInlines.h"

namespace JSC { namespace DFG {

namespace {

const bool verbose = false;

int64_t clampedSumImpl() { return 0; }

template<typename... Args>
int64_t clampedSumImpl(int left, Args... args)
{
    return static_cast<int64_t>(left) + clampedSumImpl(args...);
}

template<typename... Args>
int clampedSum(Args... args)
{
    int64_t result = clampedSumImpl(args...);
    return static_cast<int>(std::min(
        static_cast<int64_t>(std::numeric_limits<int>::max()),
        std::max(
            static_cast<int64_t>(std::numeric_limits<int>::min()),
            result)));
}

class Relationship {
public:
    enum Kind {
        LessThan,
        Equal,
        NotEqual,
        GreaterThan
    };
    
    static Kind flipped(Kind kind)
    {
        switch (kind) {
        case LessThan:
            return GreaterThan;
        case Equal:
            return Equal;
        case NotEqual:
            return NotEqual;
        case GreaterThan:
            return LessThan;
        }
        RELEASE_ASSERT_NOT_REACHED();
        return kind;
    }
    
    Relationship()
        : m_left(nullptr)
        , m_right(nullptr)
        , m_kind(Equal)
        , m_offset(0)
    {
    }
    
    Relationship(Node* left, Node* right, Kind kind, int offset = 0)
        : m_left(left)
        , m_right(right)
        , m_kind(kind)
        , m_offset(offset)
    {
        RELEASE_ASSERT(m_left);
        RELEASE_ASSERT(m_right);
        RELEASE_ASSERT(m_left != m_right);
    }
    
    static Relationship safeCreate(Node* left, Node* right, Kind kind, int offset = 0)
    {
        if (!left || !right || left == right)
            return Relationship();
        return Relationship(left, right, kind, offset);
    }
    
    typedef void* (Relationship::*UnspecifiedBoolType);

    explicit operator bool() const { return m_left; }
    
    Node* left() const { return m_left; }
    Node* right() const { return m_right; }
    Kind kind() const { return m_kind; }
    int offset() const { return m_offset; }
    
    Relationship flipped() const
    {
        if (!*this)
            return Relationship();
        
        // This should return Relationship() if -m_offset overflows. For example:
        //
        //     @a > @b - 2**31
        //
        // If we flip it we get:
        //
        //     @b < @a + 2**31
        //
        // Except that the sign gets flipped since it's INT_MIN:
        //
        //     @b < @a - 2**31
        //
        // And that makes no sense. To see how little sense it makes, consider:
        //
        //     @a > @zero - 2**31
        //
        // We would flip it to mean:
        //
        //     @zero < @a - 2**31
        //
        // Which is absurd.
        
        if (m_offset == std::numeric_limits<int>::min())
            return Relationship();
        
        return Relationship(m_right, m_left, flipped(m_kind), -m_offset);
    }
    
    Relationship inverse() const
    {
        if (!*this)
            return *this;
        
        switch (m_kind) {
        case Equal:
            return Relationship(m_left, m_right, NotEqual, m_offset);
        case NotEqual:
            return Relationship(m_left, m_right, Equal, m_offset);
        case LessThan:
            if (sumOverflows<int>(m_offset, -1))
                return Relationship();
            return Relationship(m_left, m_right, GreaterThan, m_offset - 1);
        case GreaterThan:
            if (sumOverflows<int>(m_offset, 1))
                return Relationship();
            return Relationship(m_left, m_right, LessThan, m_offset + 1);
        }
        
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    bool isCanonical() const { return m_left < m_right; }
    
    Relationship canonical() const
    {
        if (isCanonical())
            return *this;
        return flipped();
    }
    
    bool sameNodesAs(const Relationship& other) const
    {
        return m_left == other.m_left
            && m_right == other.m_right;
    }
    
    bool operator==(const Relationship& other) const
    {
        return sameNodesAs(other)
            && m_kind == other.m_kind
            && m_offset == other.m_offset;
    }
    
    bool operator!=(const Relationship& other) const
    {
        return !(*this == other);
    }
    
    bool operator<(const Relationship& other) const
    {
        if (m_left != other.m_left)
            return m_left < other.m_left;
        if (m_right != other.m_right)
            return m_right < other.m_right;
        if (m_kind != other.m_kind)
            return m_kind < other.m_kind;
        return m_offset < other.m_offset;
    }
    
    // If possible, returns a form of this relationship where the given node is the left
    // side. Returns a null relationship if this relationship cannot say anything about this
    // node.
    Relationship forNode(Node* node) const
    {
        if (m_left == node)
            return *this;
        if (m_right == node)
            return flipped();
        return Relationship();
    }
    
    void setLeft(Node* left)
    {
        RELEASE_ASSERT(left != m_right);
        m_left = left;
    }
    bool addToOffset(int offset)
    {
        if (sumOverflows<int>(m_offset, offset))
            return false;
        m_offset += offset;
        return true;
    }

    // Attempts to create a relationship that summarizes the union of this relationship and
    // the other relationship. The null relationship is returned to indicate TOP. This is used
    // for merging the current relationship-at-head for some pair of nodes and a new
    // relationship-at-head being proposed by a predecessor. We wish to create a new
    // relationship that is true whenever either of them are true, which ensuring that we don't
    // do this forever. Anytime we create a relationship that is not equal to either of the
    // previous ones, we will cause the analysis fixpoint to reexecute.
    //
    // If *this and other are identical, we just return it.
    //
    // If they are different, we pick from a finite set of "general" relationships:
    //
    // Eq: this == other + C, where C is -1, 0, or 1.
    // Lt: this < other + C, where C is -1, 0, or 1.
    // Gt: this > other + C, where C is -1, 0, or 1.
    // Ne: this != other + C, where C is -1, 0, or 1.
    // TOP: the null relationship.
    //
    // Constraining C to -1,0,1 is necessary to ensure that the set of general relationships is
    // finite. This finite set of relationships forms a pretty simple lattice where a
    // relA->relB means "relB is more general than relA". For example, this<other+1 is more
    // general than this==other. I'll leave it as an exercise for the reader to see that a
    // graph between the 13 general relationships is indeed a lattice. The fact that the set of
    // general relationships is a finite lattice ensures monotonicity of the fixpoint, since
    // any merge over not-identical relationships returns a relationship that is closer to the
    // TOP relationship than either of the original relationships. Here's how convergence is
    // achieved for any pair of relationships over the same nodes:
    //
    // - If they are identical, then returning *this means that we won't be responsible for
    //   causing another fixpoint iteration. Once all merges reach this point, we're done.
    //
    // - If they are different, then we pick the most constraining of the 13 general
    //   relationships that is true if either *this or other are true. This means that if the
    //   relationships are not identical, the merged relationship will be closer to TOP than
    //   either of the originals. Returning a different relationship means that we will be
    //   responsible for the fixpoint to reloop, but we can only do this at most 13 times since
    //   that's how "deep" the general relationship lattice is.
    //
    // Note that C being constrained to -1,0,1 also ensures that we never have to return a
    // combination of Lt and Gt, as in for example this<other+C && this>other-D. That's why
    // this function can return zero or one relationships rather than a list of relationships.
    // The only possible values of C and D where this would work are -1 and 1, but in that case
    // we just say this==other. That said, the logic for merging two == relationships, like
    // this==other+C || this==other+D is to attempt to create these two relationships:
    // this>other+min(C,D)-1 && this<other+max(C,D)+1. But only one of these relationships will
    // belong to the set of general relationships.
    //
    // Here's an example of this in action:
    //
    // for (var i = a; ; ++i) { }
    //
    // Without C being constrained to -1,0,1, we could end up looping forever: first we'd say
    // that i==a, then we might say that i<a+2, then i<a+3, then i<a+4, etc. We won't do this
    // because i<a+2 is not a valid general relationship: so when we merge i==a from the first
    // iteration and i==a+1 from the second iteration, we create i>a-1 and i<a+2 but then
    // realize that only i>a-1 is a valid general relationship. This gives us exactly what we
    // want: a statement that i>=a.
    Relationship merge(const Relationship& other) const
    {
        if (!sameNodesAs(other))
            return Relationship();
        
        // Handle the super obvious case first.
        if (*this == other)
            return *this;
        
        // This does some interesting permutations to reduce the amount of duplicate code. For
        // example:
        //
        // initially: @a != @b, @a > @b
        //            @b != @a, @b < @a
        //            @b < @a, @b != @a
        //   finally: @b != a, @b < @a
        //
        // Another example:
        //
        // initially: @a < @b, @a != @b
        //   finally: @a != @b, @a < @b

        Relationship a = *this;
        Relationship b = other;
        bool needFlip = false;
        
        // Get rid of GreaterThan.
        if (a.m_kind == GreaterThan || b.m_kind == GreaterThan) {
            a = a.flipped();
            b = b.flipped();
            
            // In rare cases, we might not be able to flip. Just give up on life in those
            // cases.
            if (!a || !b)
                return Relationship();
            
            needFlip = true;
            
            // If we still have GreaterThan, then it means that we started with @a < @b and
            // @a > @b. That's pretty much always a tautology; we don't attempt to do smart
            // things for that case for now.
            if (a.m_kind == GreaterThan || b.m_kind == GreaterThan)
                return Relationship();
        }
        
        // Make sure that if we have a LessThan, then it's first.
        if (b.m_kind == LessThan)
            std::swap(a, b);
        
        // Make sure that if we have a NotEqual, then it's first.
        if (b.m_kind == NotEqual)
            std::swap(a, b);
        
        Relationship result = a.mergeImpl(b);
        
        if (needFlip)
            return result.flipped();
        
        return result;
    }
    
    // Attempts to construct one Relationship that adequately summarizes the intersection of
    // this and other. Returns a null relationship if the filtration should be expressed as two
    // different relationships. Returning null is always safe because relationship lists in
    // this phase always imply intersection. So, you could soundly skip calling this method and
    // just put both relationships into the list. But, that could lead the fixpoint to diverge.
    // Hence this will attempt to combine the two relationships into one as a convergence hack.
    // In some cases, it will do something conservative. It's always safe for this to return
    // *this, or to return other. It'll do that sometimes, mainly to accelerate convergence for
    // things that we don't think are important enough to slow down the analysis.
    Relationship filter(const Relationship& other) const
    {
        // We are only interested in merging relationships over the same nodes.
        ASSERT(sameNodesAs(other));
        
        if (*this == other)
            return *this;
        
        // From here we can assume that the two relationships are not identical. Usually we use
        // this to assume that we have different offsets anytime that everything but the offset
        // is identical.
        
        // We want equality to take precedent over everything else, and we don't want multiple
        // independent claims of equality. That would just be a contradiction. When it does
        // happen, we will be conservative in the sense that we will pick one.
        if (m_kind == Equal)
            return *this;
        if (other.m_kind == Equal)
            return other;
        
        // Useful helper for flipping.
        auto filterFlipped = [&] () -> Relationship {
            // If we cannot flip, then just conservatively return *this.
            Relationship a = flipped();
            Relationship b = other.flipped();
            if (!a || !b)
                return *this;
            Relationship result = a.filter(b);
            if (!result)
                return Relationship();
            result = result.flipped();
            if (!result)
                return *this;
            return result;
        };
        
        if (m_kind == NotEqual) {
            if (other.m_kind == NotEqual) {
                // We could do something smarter here. We could even keep both NotEqual's. We
                // would need to make sure that we correctly collapsed them when merging. But
                // for now, we just pick one of them and hope for the best.
                return *this;
            }
            
            if (other.m_kind == GreaterThan) {
                // Implement this in terms of NotEqual.filter(LessThan). 
                return filterFlipped();
            }
            
            ASSERT(other.m_kind == LessThan);
            // We have two claims:
            //     @a != @b + C
            //     @a  < @b + D
            //
            // If C >= D, then the NotEqual is redundant.
            // If C < D - 1, then we could keep both, but for now we just keep the LessThan.
            // If C == D - 1, then the LessThan can be turned into:
            //
            //     @a < @b + C
            //
            // Note that C == this.m_offset, D == other.m_offset.
            
            if (m_offset == other.m_offset - 1)
                return Relationship(m_left, m_right, LessThan, m_offset);
            
            return other;
        }
        
        if (other.m_kind == NotEqual)
            return other.filter(*this);
        
        if (m_kind == LessThan) {
            if (other.m_kind == LessThan) {
                return Relationship(
                    m_left, m_right, LessThan, std::min(m_offset, other.m_offset));
            }
            
            ASSERT(other.m_kind == GreaterThan);
            if (sumOverflows<int>(m_offset, -1))
                return Relationship();
            if (sumOverflows<int>(other.m_offset, 1))
                return Relationship();
            if (m_offset - 1 == other.m_offset + 1)
                return Relationship(m_left, m_right, Equal, m_offset - 1);
            
            return Relationship();
        }
        
        ASSERT(m_kind == GreaterThan);
        return filterFlipped();
    }
    
    int minValueOfLeft() const
    {
        if (m_left->isInt32Constant())
            return m_left->asInt32();
        
        if (m_kind == LessThan || m_kind == NotEqual)
            return std::numeric_limits<int>::min();
        
        int minRightValue = std::numeric_limits<int>::min();
        if (m_right->isInt32Constant())
            minRightValue = m_right->asInt32();
        
        if (m_kind == GreaterThan)
            return clampedSum(minRightValue, m_offset, 1);
        ASSERT(m_kind == Equal);
        return clampedSum(minRightValue, m_offset);
    }
    
    int maxValueOfLeft() const
    {
        if (m_left->isInt32Constant())
            return m_left->asInt32();
        
        if (m_kind == GreaterThan || m_kind == NotEqual)
            return std::numeric_limits<int>::max();
        
        int maxRightValue = std::numeric_limits<int>::max();
        if (m_right->isInt32Constant())
            maxRightValue = m_right->asInt32();
        
        if (m_kind == LessThan)
            return clampedSum(maxRightValue, m_offset, -1);
        ASSERT(m_kind == Equal);
        return clampedSum(maxRightValue, m_offset);
    }
    
    void dump(PrintStream& out) const
    {
        // This prints out the relationship without any whitespace, like @x<@y+42. This
        // optimizes for the clarity of a list of relationships. It's easier to read something
        // like [@1<@2+3, @4==@5-6] than it would be if there was whitespace inside the
        // relationships.
        
        if (!*this) {
            out.print("null");
            return;
        }
        
        out.print(m_left);
        switch (m_kind) {
        case LessThan:
            out.print("<");
            break;
        case Equal:
            out.print("==");
            break;
        case NotEqual:
            out.print("!=");
            break;
        case GreaterThan:
            out.print(">");
            break;
        }
        out.print(m_right);
        if (m_offset > 0)
            out.print("+", m_offset);
        else if (m_offset < 0)
            out.print("-", -static_cast<int64_t>(m_offset));
    }
    
private:
    Relationship mergeImpl(const Relationship& other) const
    {
        ASSERT(sameNodesAs(other));
        ASSERT(m_kind != GreaterThan);
        ASSERT(other.m_kind != GreaterThan);
        ASSERT(*this != other);
        
        // The purpose of this method is to guarantee that:
        //
        // - We avoid having more than one Relationship over the same two nodes. Therefore, if
        //   the merge could be expressed as two Relationships, we prefer to instead pick the
        //   less precise single Relationship form even if that means TOP.
        //
        // - If the difference between two Relationships is just the m_offset, then we create a
        //   Relationship that has an offset of -1, 0, or 1. This is an essential convergence
        //   hack. We need -1 and 1 to support <= and >=.
        
        // From here we can assume that the two relationships are not identical. Usually we use
        // this to assume that we have different offsets anytime that everything but the offset
        // is identical.
        
        if (m_kind == NotEqual) {
            if (other.m_kind == NotEqual)
                return Relationship(); // Different offsets, so tautology.
            
            if (other.m_kind == Equal) {
                if (m_offset != other.m_offset) {
                    // Saying that you might be B when you've already said that you're anything
                    // but A, where A and B are different, is a tautology. You could just say
                    // that you're anything but A. Adding "(a == b + 1)" to "(a != b + 5)" has
                    // no value.
                    return *this;
                }
                // Otherwise, same offsets: we're saying that you're either A or you're not
                // equal to A.
                
                return Relationship();
            }
            
            RELEASE_ASSERT(other.m_kind == LessThan);
            // We have these claims, and we're merging them:
            //     @a != @b + C
            //     @a < @b + D
            //
            // If we have C == D, then the merge is clearly just the NotEqual.
            // If we have C < D, then the merge is a tautology.
            // If we have C > D, then we could keep both claims, but we are cheap, so we
            // don't. We just use the NotEqual.
            
            if (m_offset < other.m_offset)
                return Relationship();
            
            return *this;
        }
        
        if (m_kind == LessThan) {
            if (other.m_kind == LessThan) {
                // Figure out what offset to select to merge them. The appropriate offsets are
                // -1, 0, or 1.
                
                // First figure out what offset we'd like to use.
                int bestOffset = std::max(m_offset, other.m_offset);
                
                // We have something like @a < @b + 2. We can't represent this under the
                // -1,0,1 rule.
                if (bestOffset <= 1)
                    return Relationship(m_left, m_right, LessThan, std::max(bestOffset, -1));
                
                return Relationship();
            }
            
            // The only thing left is Equal. We would have eliminated the GreaterThan's, and
            // if we merge LessThan and NotEqual, the NotEqual always comes first.
            RELEASE_ASSERT(other.m_kind == Equal);
            
            // This is the really interesting case. We have:
            //
            //     @a < @b + C
            //
            // and:
            //
            //     @a == @b + D
            //
            // Therefore we'd like to return:
            //
            //     @a < @b + max(C, D + 1)
            
            int bestOffset = std::max(m_offset, other.m_offset + 1);
            
            // We have something like @a < @b + 2. We can't do it.
            if (bestOffset <= 1)
                return Relationship(m_left, m_right, LessThan, std::max(bestOffset, -1));

            return Relationship();
        }
        
        // The only thing left is Equal, since we would have gotten rid of the GreaterThan's.
        RELEASE_ASSERT(m_kind == Equal);
        
        // We would never see NotEqual, because those always come first. We would never
        // see GreaterThan, because we would have eliminated those. We would never see
        // LessThan, because those always come first.
        
        RELEASE_ASSERT(other.m_kind == Equal);
        // We have @a == @b + C and @a == @b + D, where C != D. Turn this into some
        // inequality that involves a constant that is -1,0,1. Note that we will never have
        // lessThan and greaterThan because the constants are constrained to -1,0,1. The only
        // way for both of them to be valid is a<b+1 and a>b-1, but then we would have said
        // a==b.

        Relationship lessThan;
        Relationship greaterThan;
        
        int lessThanEqOffset = std::max(m_offset, other.m_offset);
        if (lessThanEqOffset >= -2 && lessThanEqOffset <= 0) {
            lessThan = Relationship(
                m_left, other.m_right, LessThan, lessThanEqOffset + 1);
            
            ASSERT(lessThan.offset() >= -1 && lessThan.offset() <= 1);
        }
        
        int greaterThanEqOffset = std::min(m_offset, other.m_offset);
        if (greaterThanEqOffset >= 0 && greaterThanEqOffset <= 2) {
            greaterThan = Relationship(
                m_left, other.m_right, GreaterThan, greaterThanEqOffset - 1);
            
            ASSERT(greaterThan.offset() >= -1 && greaterThan.offset() <= 1);
        }
        
        if (lessThan) {
            // Both relationships cannot be valid; see above.
            RELEASE_ASSERT(!greaterThan);
            
            return lessThan;
        }
        
        return greaterThan;
    }
    
    Node* m_left;
    Node* m_right;
    Kind m_kind;
    int m_offset; // This offset can be arbitrarily large.
};

typedef HashMap<Node*, Vector<Relationship>> RelationshipMap;

class IntegerRangeOptimizationPhase : public Phase {
public:
    IntegerRangeOptimizationPhase(Graph& graph)
        : Phase(graph, "integer range optimization")
        , m_zero(nullptr)
        , m_relationshipsAtHead(graph)
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        ASSERT(m_graph.m_form == SSA);
        
        // Before we do anything, make sure that we have a zero constant at the top.
        for (Node* node : *m_graph.block(0)) {
            if (node->isInt32Constant() && !node->asInt32()) {
                m_zero = node;
                break;
            }
        }
        if (!m_zero) {
            m_zero = m_insertionSet.insertConstant(0, NodeOrigin(), jsNumber(0));
            m_insertionSet.execute(m_graph.block(0));
        }
        
        if (verbose) {
            dataLog("Graph before integer range optimization:\n");
            m_graph.dump();
        }
        
        // This performs a fixpoint over the blocks in reverse post-order. Logically, we
        // maintain a list of relationships at each point in the program. The list should be
        // read as an intersection. For example if we have {rel1, rel2, ..., relN}, you should
        // read this as:
        //
        //     TOP && rel1 && rel2 && ... && relN
        //
        // This allows us to express things like:
        //
        //     @a > @b - 42 && @a < @b + 25
        //
        // But not things like:
        //
        //     @a < @b - 42 || @a > @b + 25
        //
        // We merge two lists by merging each relationship in one list with each relationship
        // in the other list. Merging two relationships will yield a relationship list; as with
        // all such lists it is an intersction. Merging relationships over different variables
        // always yields the empty list (i.e. TOP). This merge style is sound because if we
        // have:
        //
        //     (A && B && C) || (D && E && F)
        //
        // Then a valid merge is just one that will return true if A, B, C are all true, or
        // that will return true if D, E, F are all true. Our merge style essentially does:
        //
        //     (A || D) && (A || E) && (A || F) && (B || D) && (B || E) && (B || F) &&
        //         (C || D) && (C || E) && (C || F)
        //
        // If A && B && C is true, then this returns true. If D && E && F is true, this also
        // returns true.
        //
        // While this appears at first like a kind of expression explosion, in practice it
        // isn't. The code that handles this knows that the merge of two relationships over
        // different variables is TOP (i.e. the empty list). For example if A above is @a < @b
        // and B above is @c > @d, where @a, @b, @c, and @d are different nodes, the merge will
        // yield nothing. In fact, the merge algorithm will skip such merges entirely because
        // the relationship lists are actually keyed by node.
        //
        // Note that it's always safe to drop any of relationship from the relationship list.
        // This merely increases the likelihood of the "expression" yielding true, i.e. being
        // closer to TOP. Optimizations are only performed if we can establish that the
        // expression implied by the relationship list is false for all of those cases where
        // some check would have failed.
        //
        // There is no notion of BOTTOM because we treat blocks that haven't had their
        // state-at-head set as a special case: we just transfer all live relationships to such
        // a block. After the head of a block is set, we perform the merging as above. In all
        // other places where we would ordinarily need BOTTOM, we approximate it by having some
        // non-BOTTOM relationship.
        
        BlockList postOrder = m_graph.blocksInPostOrder();

        // This loop analyzes the IR to give us m_relationshipsAtHead for each block. This
        // may reexecute blocks many times, but it is guaranteed to converge. The state of
        // the relationshipsAtHead over any pair of nodes converge monotonically towards the
        // TOP relationship (i.e. no relationships in the relationship list). The merge rule
        // when between the current relationshipsAtHead and the relationships being propagated
        // from a predecessor ensures monotonicity by converting disagreements into one of a
        // small set of "general" relationships. There are 12 such relationshis, plus TOP. See
        // the comment above Relationship::merge() for details.
        bool changed = true;
        while (changed) {
            changed = false;
            for (unsigned postOrderIndex = postOrder.size(); postOrderIndex--;) {
                BasicBlock* block = postOrder[postOrderIndex];
                DFG_ASSERT(
                    m_graph, nullptr,
                    block == m_graph.block(0) || m_seenBlocks.contains(block));
            
                m_relationships = m_relationshipsAtHead[block];
            
                for (unsigned nodeIndex = 0; nodeIndex < block->size(); ++nodeIndex) {
                    Node* node = block->at(nodeIndex);
                    if (verbose)
                        dataLog("Analysis: at ", node, ": ", listDump(sortedRelationships()), "\n");
                    executeNode(node);
                }
                
                // Now comes perhaps the most important piece of cleverness: if we Branch, and
                // the predicate involves some relation over integers, we propagate different
                // information to the taken and notTaken paths. This handles:
                // - Branch on int32.
                // - Branch on LogicalNot on int32.
                // - Branch on compare on int32's.
                // - Branch on LogicalNot of compare on int32's.
                Node* terminal = block->terminal();
                bool alreadyMerged = false;
                if (terminal->op() == Branch) {
                    Relationship relationshipForTrue;
                    BranchData* branchData = terminal->branchData();
                    
                    bool invert = false;
                    if (terminal->child1()->op() == LogicalNot) {
                        terminal = terminal->child1().node();
                        invert = true;
                    }
                    
                    if (terminal->child1().useKind() == Int32Use) {
                        relationshipForTrue = Relationship::safeCreate(
                            terminal->child1().node(), m_zero, Relationship::NotEqual, 0);
                    } else {
                        Node* compare = terminal->child1().node();
                        switch (compare->op()) {
                        case CompareEq:
                        case CompareStrictEq:
                        case CompareLess:
                        case CompareLessEq:
                        case CompareGreater:
                        case CompareGreaterEq: {
                            if (!compare->isBinaryUseKind(Int32Use))
                                break;
                    
                            switch (compare->op()) {
                            case CompareEq:
                            case CompareStrictEq:
                                relationshipForTrue = Relationship::safeCreate(
                                    compare->child1().node(), compare->child2().node(),
                                    Relationship::Equal, 0);
                                break;
                            case CompareLess:
                                relationshipForTrue = Relationship::safeCreate(
                                    compare->child1().node(), compare->child2().node(),
                                    Relationship::LessThan, 0);
                                break;
                            case CompareLessEq:
                                relationshipForTrue = Relationship::safeCreate(
                                    compare->child1().node(), compare->child2().node(),
                                    Relationship::LessThan, 1);
                                break;
                            case CompareGreater:
                                relationshipForTrue = Relationship::safeCreate(
                                    compare->child1().node(), compare->child2().node(),
                                    Relationship::GreaterThan, 0);
                                break;
                            case CompareGreaterEq:
                                relationshipForTrue = Relationship::safeCreate(
                                    compare->child1().node(), compare->child2().node(),
                                    Relationship::GreaterThan, -1);
                                break;
                            default:
                                DFG_CRASH(m_graph, compare, "Invalid comparison node type");
                                break;
                            }
                            break;
                        }
                    
                        default:
                            break;
                        }
                    }
                    
                    if (invert)
                        relationshipForTrue = relationshipForTrue.inverse();
                    
                    if (relationshipForTrue) {
                        RelationshipMap forTrue = m_relationships;
                        RelationshipMap forFalse = m_relationships;
                        
                        if (verbose)
                            dataLog("Dealing with true:\n");
                        setRelationship(forTrue, relationshipForTrue);
                        if (Relationship relationshipForFalse = relationshipForTrue.inverse()) {
                            if (verbose)
                                dataLog("Dealing with false:\n");
                            setRelationship(forFalse, relationshipForFalse);
                        }
                        
                        changed |= mergeTo(forTrue, branchData->taken.block);
                        changed |= mergeTo(forFalse, branchData->notTaken.block);
                        alreadyMerged = true;
                    }
                }

                if (!alreadyMerged) {
                    for (BasicBlock* successor : block->successors())
                        changed |= mergeTo(m_relationships, successor);
                }
            }
        }
        
        // Now we transform the code based on the results computed in the previous loop.
        changed = false;
        for (BasicBlock* block : m_graph.blocksInNaturalOrder()) {
            m_relationships = m_relationshipsAtHead[block];
            for (unsigned nodeIndex = 0; nodeIndex < block->size(); ++nodeIndex) {
                Node* node = block->at(nodeIndex);
                if (verbose)
                    dataLog("Transformation: at ", node, ": ", listDump(sortedRelationships()), "\n");
                
                // This ends up being pretty awkward to write because we need to decide if we
                // optimize by using the relationships before the operation, but we need to
                // call executeNode() before we optimize.
                switch (node->op()) {
                case ArithAdd: {
                    if (!node->isBinaryUseKind(Int32Use))
                        break;
                    if (node->arithMode() != Arith::CheckOverflow)
                        break;
                    if (!node->child2()->isInt32Constant())
                        break;
                    
                    auto iter = m_relationships.find(node->child1().node());
                    if (iter == m_relationships.end())
                        break;
                    
                    int minValue = std::numeric_limits<int>::min();
                    int maxValue = std::numeric_limits<int>::max();
                    for (Relationship relationship : iter->value) {
                        minValue = std::max(minValue, relationship.minValueOfLeft());
                        maxValue = std::min(maxValue, relationship.maxValueOfLeft());
                    }
                    
                    if (sumOverflows<int>(minValue, node->child2()->asInt32()) ||
                        sumOverflows<int>(maxValue, node->child2()->asInt32()))
                        break;
                    
                    executeNode(block->at(nodeIndex));
                    node->setArithMode(Arith::Unchecked);
                    changed = true;
                    break;
                }
                    
                case CheckInBounds: {
                    auto iter = m_relationships.find(node->child1().node());
                    if (iter == m_relationships.end())
                        break;
                    
                    bool nonNegative = false;
                    bool lessThanLength = false;
                    for (Relationship relationship : iter->value) {
                        if (relationship.minValueOfLeft() >= 0)
                            nonNegative = true;
                        
                        if (relationship.right() == node->child2()) {
                            if (relationship.kind() == Relationship::Equal
                                && relationship.offset() < 0)
                                lessThanLength = true;
                            
                            if (relationship.kind() == Relationship::LessThan
                                && relationship.offset() <= 0)
                                lessThanLength = true;
                        }
                    }
                    
                    if (nonNegative && lessThanLength) {
                        executeNode(block->at(nodeIndex));
                        node->remove();
                        changed = true;
                    }
                    break;
                }
                    
                default:
                    break;
                }
                
                executeNode(block->at(nodeIndex));
            }
        }
        
        return changed;
    }

private:
    void executeNode(Node* node)
    {
        switch (node->op()) {
        case CheckInBounds: {
            setRelationship(Relationship::safeCreate(node->child1().node(), node->child2().node(), Relationship::LessThan));
            setRelationship(Relationship::safeCreate(node->child1().node(), m_zero, Relationship::GreaterThan, -1));
            break;
        }
            
        case ArithAdd: {
            // We're only interested in int32 additions and we currently only know how to
            // handle the non-wrapping ones.
            if (!node->isBinaryUseKind(Int32Use))
                break;
            
            // FIXME: We could handle the unchecked arithmetic case. We just do it don't right
            // now.
            if (node->arithMode() != Arith::CheckOverflow)
                break;
            
            // Handle add: @value + constant.
            if (!node->child2()->isInt32Constant())
                break;
            
            int offset = node->child2()->asInt32();
            
            // We add a relationship for @add == @value + constant, and then we copy the
            // relationships for @value. This gives us a one-deep view of @value's existing
            // relationships, which matches the one-deep search in setRelationship().
            
            setRelationship(
                Relationship(node, node->child1().node(), Relationship::Equal, offset));
            
            auto iter = m_relationships.find(node->child1().node());
            if (iter != m_relationships.end()) {
                Vector<Relationship> toAdd;
                for (Relationship relationship : iter->value) {
                    // We have:
                    //     add: ArithAdd(@x, C)
                    //     @x op @y + D
                    //
                    // The following certainly holds:
                    //     @x == @add - C
                    //
                    // Which allows us to substitute:
                    //     @add - C op @y + D
                    //
                    // And then carry the C over:
                    //     @add op @y + D + C
                    
                    Relationship newRelationship = relationship;
                    ASSERT(newRelationship.left() == node->child1().node());
                    
                    if (newRelationship.right() == node)
                        continue;
                    newRelationship.setLeft(node);
                    if (newRelationship.addToOffset(offset))
                        toAdd.append(newRelationship);
                }
                for (Relationship relationship : toAdd)
                    setRelationship(relationship, 0);
            }
            
            // Now we want to establish that both the input and the output of the addition are
            // within a particular range of integers.
            
            if (offset > 0) {
                // If we have "add: @value + 1" then we know that @value <= max - 1, i.e. that
                // @value < max.
                if (!sumOverflows<int>(std::numeric_limits<int>::max(), -offset, 1)) {
                    setRelationship(
                        Relationship::safeCreate(
                            node->child1().node(), m_zero, Relationship::LessThan,
                            std::numeric_limits<int>::max() - offset + 1),
                        0);
                }
                    
                // If we have "add: @value + 1" then we know that @add >= min + 1, i.e. that
                // @add > min.
                if (!sumOverflows<int>(std::numeric_limits<int>::min(), offset, -1)) {
                    setRelationship(
                        Relationship(
                            node, m_zero, Relationship::GreaterThan,
                            std::numeric_limits<int>::min() + offset - 1),
                        0);
                }
            }
            
            if (offset < 0 && offset != std::numeric_limits<int>::min()) {
                // If we have "add: @value - 1" then we know that @value >= min + 1, i.e. that
                // @value > min.
                if (!sumOverflows<int>(std::numeric_limits<int>::min(), offset, -1)) {
                    setRelationship(
                        Relationship::safeCreate(
                            node->child1().node(), m_zero, Relationship::GreaterThan,
                            std::numeric_limits<int>::min() + offset - 1),
                        0);
                }
                
                // If we have "add: @value + 1" then we know that @add <= max - 1, i.e. that
                // @add < max.
                if (!sumOverflows<int>(std::numeric_limits<int>::max(), -offset, 1)) {
                    setRelationship(
                        Relationship(
                            node, m_zero, Relationship::LessThan,
                            std::numeric_limits<int>::max() - offset + 1),
                        0);
                }
            }
            break;
        }
            
        case GetArrayLength: {
            setRelationship(Relationship(node, m_zero, Relationship::GreaterThan, -1));
            break;
        }
            
        case Upsilon: {
            setRelationship(
                Relationship::safeCreate(
                    node->child1().node(), node->phi(), Relationship::Equal, 0));
            
            auto iter = m_relationships.find(node->child1().node());
            if (iter != m_relationships.end()) {
                Vector<Relationship> toAdd;
                for (Relationship relationship : iter->value) {
                    Relationship newRelationship = relationship;
                    if (node->phi() == newRelationship.right())
                        continue;
                    newRelationship.setLeft(node->phi());
                    toAdd.append(newRelationship);
                }
                for (Relationship relationship : toAdd)
                    setRelationship(relationship);
            }
            break;
        }
            
        default:
            break;
        }
    }
    
    void setRelationship(Relationship relationship, unsigned timeToLive = 1)
    {
        setRelationship(m_relationships, relationship, timeToLive);
    }
    
    void setRelationship(
        RelationshipMap& relationshipMap, Relationship relationship, unsigned timeToLive = 1)
    {
        setOneSide(relationshipMap, relationship, timeToLive);
        setOneSide(relationshipMap, relationship.flipped(), timeToLive);
    }
    
    void setOneSide(
        RelationshipMap& relationshipMap, Relationship relationship, unsigned timeToLive = 1)
    {
        if (!relationship)
            return;
        
        if (verbose)
            dataLog("    Setting: ", relationship, " (ttl = ", timeToLive, ")\n");

        auto result = relationshipMap.add(
            relationship.left(), Vector<Relationship>());
        Vector<Relationship>& relationships = result.iterator->value;
        Vector<Relationship> toAdd;
        bool found = false;
        for (Relationship& otherRelationship : relationships) {
            if (otherRelationship.sameNodesAs(relationship)) {
                if (Relationship filtered = otherRelationship.filter(relationship)) {
                    ASSERT(filtered.left() == relationship.left());
                    otherRelationship = filtered;
                    found = true;
                }
            }
            
            if (timeToLive && otherRelationship.kind() == Relationship::Equal) {
                if (verbose)
                    dataLog("      Considering: ", otherRelationship, "\n");
                
                // We have:
                //     @a op @b + C
                //     @a == @c + D
                //
                // This implies:
                //     @c + D op @b + C
                //     @c op @b + C - D
                //
                // Where: @a == relationship.left(), @b == relationship.right(),
                // @a == otherRelationship.left(), @c == otherRelationship.right().
                
                if (otherRelationship.offset() != std::numeric_limits<int>::min()) {
                    Relationship newRelationship = relationship;
                    if (newRelationship.right() != otherRelationship.right()) {
                        newRelationship.setLeft(otherRelationship.right());
                        if (newRelationship.addToOffset(-otherRelationship.offset()))
                            toAdd.append(newRelationship);
                    }
                }
            }
        }
        
        if (!found)
            relationships.append(relationship);
        
        for (Relationship anotherRelationship : toAdd) {
            ASSERT(timeToLive);
            setOneSide(relationshipMap, anotherRelationship, timeToLive - 1);
        }
    }
    
    bool mergeTo(RelationshipMap& relationshipMap, BasicBlock* target)
    {
        if (verbose) {
            dataLog("Merging to ", pointerDump(target), ":\n");
            dataLog("    Incoming: ", listDump(sortedRelationships(relationshipMap)), "\n");
            dataLog("    At head: ", listDump(sortedRelationships(m_relationshipsAtHead[target])), "\n");
        }
        
        if (m_seenBlocks.add(target)) {
            // This is a new block. We copy subject to liveness pruning.
            auto isLive = [&] (Node* node) {
                if (node == m_zero)
                    return true;
                return target->ssa->liveAtHead.contains(node);
            };
            
            for (auto& entry : relationshipMap) {
                if (!isLive(entry.key))
                    continue;
                
                Vector<Relationship> values;
                for (Relationship relationship : entry.value) {
                    ASSERT(relationship.left() == entry.key);
                    if (isLive(relationship.right())) {
                        if (verbose)
                            dataLog("  Propagating ", relationship, "\n");
                        values.append(relationship);
                    }
                }
                
                std::sort(values.begin(), values.end());
                m_relationshipsAtHead[target].add(entry.key, values);
            }
            return true;
        }
        
        // Merge by intersecting. We have no notion of BOTTOM, so we use the omission of
        // relationships for a pair of nodes to mean TOP. The reason why we don't need BOTTOM
        // is (1) we just overapproximate contradictions and (2) a value never having been
        // assigned would only happen if we have not processed the node's predecessor. We
        // shouldn't process blocks until we have processed the block's predecessor because we
        // are using reverse postorder.
        Vector<Node*> toRemove;
        bool changed = false;
        for (auto& entry : m_relationshipsAtHead[target]) {
            auto iter = relationshipMap.find(entry.key);
            if (iter == relationshipMap.end()) {
                toRemove.append(entry.key);
                changed = true;
                continue;
            }
            
            Vector<Relationship> mergedRelationships;
            for (Relationship targetRelationship : entry.value) {
                for (Relationship sourceRelationship : iter->value) {
                    if (verbose)
                        dataLog("  Merging ", targetRelationship, " and ", sourceRelationship, ":\n");
                    Relationship newRelationship =
                        targetRelationship.merge(sourceRelationship);
                    
                    if (verbose)
                        dataLog("    Got ", newRelationship, "\n");
                    
                    if (!newRelationship)
                        continue;
                    
                    // We need to filter() to avoid exponential explosion of identical
                    // relationships. We do this here to avoid making setOneSide() do
                    // more work, since we expect setOneSide() will be called more
                    // frequently. Here's an example. At some point someone might start
                    // with two relationships like @a > @b - C and @a < @b + D. Then
                    // someone does a setRelationship() passing something that turns
                    // both of these into @a == @b. Now we have @a == @b duplicated.
                    // Let's say that this duplicate @a == @b ends up at the head of a
                    // loop. If we didn't have this rule, then the loop would propagate
                    // duplicate @a == @b's onto the existing duplicate @a == @b's.
                    // There would be four pairs of @a == @b, each of which would
                    // create a new @a == @b. Now we'd have four of these duplicates
                    // and the next time around we'd have 8, then 16, etc. We avoid
                    // this here by doing this filtration. That might be a bit of
                    // overkill, since it's probably just the identical duplicate
                    // relationship case we want' to avoid. But, I'll keep this until
                    // we have evidence that this is a performance problem. Remember -
                    // we are already dealing with a list that is pruned down to
                    // relationships with identical left operand. It shouldn't be a
                    // large list.
                    bool found = false;
                    for (Relationship& existingRelationship : mergedRelationships) {
                        if (existingRelationship.sameNodesAs(newRelationship)) {
                            Relationship filtered =
                                existingRelationship.filter(newRelationship);
                            if (filtered) {
                                existingRelationship = filtered;
                                found = true;
                                break;
                            }
                        }
                    }
                    
                    if (!found)
                        mergedRelationships.append(newRelationship);
                }
            }
            std::sort(mergedRelationships.begin(), mergedRelationships.end());
            if (entry.value == mergedRelationships)
                continue;
            
            entry.value = mergedRelationships;
            changed = true;
        }
        for (Node* node : toRemove)
            m_relationshipsAtHead[target].remove(node);
        
        return changed;
    }
        
    Vector<Relationship> sortedRelationships(const RelationshipMap& relationships)
    {
        Vector<Relationship> result;
        for (auto& entry : relationships)
            result.appendVector(entry.value);
        std::sort(result.begin(), result.end());
        return result;
    }
    
    Vector<Relationship> sortedRelationships()
    {
        return sortedRelationships(m_relationships);
    }
    
    Node* m_zero;
    RelationshipMap m_relationships;
    BlockSet m_seenBlocks;
    BlockMap<RelationshipMap> m_relationshipsAtHead;
    InsertionSet m_insertionSet;
};
    
} // anonymous namespace

bool performIntegerRangeOptimization(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Integer Range Optimization Phase");
    return runPhase<IntegerRangeOptimizationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

