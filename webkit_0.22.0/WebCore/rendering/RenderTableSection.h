/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef RenderTableSection_h
#define RenderTableSection_h

#include "RenderTable.h"
#include <wtf/Vector.h>

namespace WebCore {

class RenderTableCell;
class RenderTableRow;

enum CollapsedBorderSide {
    CBSBefore,
    CBSAfter,
    CBSStart,
    CBSEnd
};

// Helper class for paintObject.
struct CellSpan {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
public:
    CellSpan(unsigned start, unsigned end)
        : start(start)
        , end(end)
    {
    }

    unsigned start;
    unsigned end;
};

class RenderTableSection final : public RenderBox {
public:
    RenderTableSection(Element&, Ref<RenderStyle>&&);
    RenderTableSection(Document&, Ref<RenderStyle>&&);
    virtual ~RenderTableSection();

    RenderTableRow* firstRow() const;
    RenderTableRow* lastRow() const;

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0) override;

    virtual Optional<int> firstLineBaseline() const override;

    void addCell(RenderTableCell*, RenderTableRow* row);

    int calcRowLogicalHeight();
    void layoutRows();
    void computeOverflowFromCells();

    RenderTable* table() const { return downcast<RenderTable>(parent()); }

    struct CellStruct {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
    public:
#endif
        Vector<RenderTableCell*, 1> cells; 
        bool inColSpan { false }; // true for columns after the first in a colspan

        RenderTableCell* primaryCell() { return hasCells() ? cells[cells.size() - 1] : 0; }
        const RenderTableCell* primaryCell() const { return hasCells() ? cells[cells.size() - 1] : 0; }
        bool hasCells() const { return cells.size() > 0; }
    };

    typedef Vector<CellStruct> Row;
    struct RowStruct {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
    public:
#endif
        Row row;
        RenderTableRow* rowRenderer { nullptr };
        LayoutUnit baseline;
        Length logicalHeight;
    };

    const BorderValue& borderAdjoiningTableStart() const;
    const BorderValue& borderAdjoiningTableEnd() const;
    const BorderValue& borderAdjoiningStartCell(const RenderTableCell*) const;
    const BorderValue& borderAdjoiningEndCell(const RenderTableCell*) const;

    const RenderTableCell* firstRowCellAdjoiningTableStart() const;
    const RenderTableCell* firstRowCellAdjoiningTableEnd() const;

    CellStruct& cellAt(unsigned row,  unsigned col);
    const CellStruct& cellAt(unsigned row, unsigned col) const;
    RenderTableCell* primaryCellAt(unsigned row, unsigned col);
    RenderTableRow* rowRendererAt(unsigned row) const;

    void appendColumn(unsigned pos);
    void splitColumn(unsigned pos, unsigned first);

    int calcOuterBorderBefore() const;
    int calcOuterBorderAfter() const;
    int calcOuterBorderStart() const;
    int calcOuterBorderEnd() const;
    void recalcOuterBorder();

    int outerBorderBefore() const { return m_outerBorderBefore; }
    int outerBorderAfter() const { return m_outerBorderAfter; }
    int outerBorderStart() const { return m_outerBorderStart; }
    int outerBorderEnd() const { return m_outerBorderEnd; }

    int outerBorderLeft(const RenderStyle* styleForCellFlow) const;
    int outerBorderRight(const RenderStyle* styleForCellFlow) const;
    int outerBorderTop(const RenderStyle* styleForCellFlow) const;
    int outerBorderBottom(const RenderStyle* styleForCellFlow) const;

    unsigned numRows() const;
    unsigned numColumns() const;
    void recalcCells();
    void recalcCellsIfNeeded();

    bool needsCellRecalc() const { return m_needsCellRecalc; }
    void setNeedsCellRecalc();

    LayoutUnit rowBaseline(unsigned row);
    void rowLogicalHeightChanged(unsigned rowIndex);

    void clearCachedCollapsedBorders();
    void removeCachedCollapsedBorders(const RenderTableCell&);
    void setCachedCollapsedBorder(const RenderTableCell&, CollapsedBorderSide, CollapsedBorderValue);
    CollapsedBorderValue cachedCollapsedBorder(const RenderTableCell&, CollapsedBorderSide);

    // distributeExtraLogicalHeightToRows methods return the *consumed* extra logical height.
    // FIXME: We may want to introduce a structure holding the in-flux layout information.
    int distributeExtraLogicalHeightToRows(int extraLogicalHeight);

    static RenderTableSection* createAnonymousWithParentRenderer(const RenderObject*);
    virtual RenderBox* createAnonymousBoxWithSameTypeAs(const RenderObject* parent) const override { return createAnonymousWithParentRenderer(parent); }
    
    virtual void paint(PaintInfo&, const LayoutPoint&) override;

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override;

private:
    enum ShouldIncludeAllIntersectingCells {
        IncludeAllIntersectingCells,
        DoNotIncludeAllIntersectingCells
    };

    virtual const char* renderName() const override { return (isAnonymous() || isPseudoElement()) ? "RenderTableSection (anonymous)" : "RenderTableSection"; }

    virtual bool canHaveChildren() const override { return true; }

    virtual bool isTableSection() const override { return true; }

    virtual void willBeRemovedFromTree() override;

    virtual void layout() override;

    void paintCell(RenderTableCell*, PaintInfo&, const LayoutPoint&);
    virtual void paintObject(PaintInfo&, const LayoutPoint&) override;
    void paintRowGroupBorder(const PaintInfo&, bool antialias, LayoutRect, BoxSide, CSSPropertyID borderColor, EBorderStyle, EBorderStyle tableBorderStyle);
    void paintRowGroupBorderIfRequired(const PaintInfo&, const LayoutPoint& paintOffset, unsigned row, unsigned col, BoxSide, RenderTableCell* = 0);
    int offsetLeftForRowGroupBorder(RenderTableCell*, const LayoutRect& rowGroupRect, unsigned row);

    int offsetTopForRowGroupBorder(RenderTableCell*, BoxSide borderSide, unsigned row);
    int verticalRowGroupBorderHeight(RenderTableCell*, const LayoutRect& rowGroupRect, unsigned row);
    int horizontalRowGroupBorderWidth(RenderTableCell*, const LayoutRect& rowGroupRect, unsigned row, unsigned column);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0) override;

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) override;

    void ensureRows(unsigned);

    void distributeExtraLogicalHeightToPercentRows(int& extraLogicalHeight, int totalPercent);
    void distributeExtraLogicalHeightToAutoRows(int& extraLogicalHeight, unsigned autoRowsCount);
    void distributeRemainingExtraLogicalHeight(int& extraLogicalHeight);

    bool hasOverflowingCell() const { return m_overflowingCells.size() || m_forceSlowPaintPathWithOverflowingCell; }
    void computeOverflowFromCells(unsigned totalRows, unsigned nEffCols);

    CellSpan fullTableRowSpan() const;
    CellSpan fullTableColumnSpan() const { return CellSpan(0, table()->columns().size()); }

    // Flip the rect so it aligns with the coordinates used by the rowPos and columnPos vectors.
    LayoutRect logicalRectForWritingModeAndDirection(const LayoutRect&) const;

    CellSpan dirtiedRows(const LayoutRect& repaintRect) const;
    CellSpan dirtiedColumns(const LayoutRect& repaintRect) const;

    // These two functions take a rectangle as input that has been flipped by logicalRectForWritingModeAndDirection.
    // The returned span of rows or columns is end-exclusive, and empty if start==end.
    // The IncludeAllIntersectingCells argument is used to determine which cells to include when
    // an edge of the flippedRect lies exactly on a cell boundary. Using IncludeAllIntersectingCells
    // will return both cells, and using DoNotIncludeAllIntersectingCells will return only the cell
    // that hittesting should return.
    CellSpan spannedRows(const LayoutRect& flippedRect, ShouldIncludeAllIntersectingCells) const;
    CellSpan spannedColumns(const LayoutRect& flippedRect, ShouldIncludeAllIntersectingCells) const;

    void setLogicalPositionForCell(RenderTableCell*, unsigned effectiveColumn) const;

    void firstChild() const  = delete;
    void lastChild() const  = delete;

    Vector<RowStruct> m_grid;
    Vector<int> m_rowPos;

    // the current insertion position
    unsigned m_cCol { 0 };
    unsigned m_cRow  { 0 };

    int m_outerBorderStart  { 0 };
    int m_outerBorderEnd  { 0 };
    int m_outerBorderBefore  { 0 };
    int m_outerBorderAfter  { 0 };

    bool m_needsCellRecalc  { false };

    // This HashSet holds the overflowing cells for faster painting.
    // If we have more than gMaxAllowedOverflowingCellRatio * total cells, it will be empty
    // and m_forceSlowPaintPathWithOverflowingCell will be set to save memory.
    HashSet<RenderTableCell*> m_overflowingCells;
    bool m_forceSlowPaintPathWithOverflowingCell { false };

    bool m_hasMultipleCellLevels { false };

    // This map holds the collapsed border values for cells with collapsed borders.
    // It is held at RenderTableSection level to spare memory consumption by table cells.
    HashMap<std::pair<const RenderTableCell*, int>, CollapsedBorderValue > m_cellsCollapsedBorders;
};

inline const BorderValue& RenderTableSection::borderAdjoiningTableStart() const
{
    if (hasSameDirectionAs(table()))
        return style().borderStart();
    return style().borderEnd();
}

inline const BorderValue& RenderTableSection::borderAdjoiningTableEnd() const
{
    if (hasSameDirectionAs(table()))
        return style().borderEnd();
    return style().borderStart();
}

inline RenderTableSection::CellStruct& RenderTableSection::cellAt(unsigned row,  unsigned col)
{
    recalcCellsIfNeeded();
    return m_grid[row].row[col];
}

inline const RenderTableSection::CellStruct& RenderTableSection::cellAt(unsigned row, unsigned col) const
{
    ASSERT(!m_needsCellRecalc);
    return m_grid[row].row[col];
}

inline RenderTableCell* RenderTableSection::primaryCellAt(unsigned row, unsigned col)
{
    recalcCellsIfNeeded();
    CellStruct& c = m_grid[row].row[col];
    return c.primaryCell();
}

inline RenderTableRow* RenderTableSection::rowRendererAt(unsigned row) const
{
    ASSERT(!m_needsCellRecalc);
    return m_grid[row].rowRenderer;
}

inline int RenderTableSection::outerBorderLeft(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isLeftToRightDirection() ? outerBorderStart() : outerBorderEnd();
    return styleForCellFlow->isFlippedBlocksWritingMode() ? outerBorderAfter() : outerBorderBefore();
}

inline int RenderTableSection::outerBorderRight(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isLeftToRightDirection() ? outerBorderEnd() : outerBorderStart();
    return styleForCellFlow->isFlippedBlocksWritingMode() ? outerBorderBefore() : outerBorderAfter();
}

inline int RenderTableSection::outerBorderTop(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isFlippedBlocksWritingMode() ? outerBorderAfter() : outerBorderBefore();
    return styleForCellFlow->isLeftToRightDirection() ? outerBorderStart() : outerBorderEnd();
}

inline int RenderTableSection::outerBorderBottom(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isFlippedBlocksWritingMode() ? outerBorderBefore() : outerBorderAfter();
    return styleForCellFlow->isLeftToRightDirection() ? outerBorderEnd() : outerBorderStart();
}

inline unsigned RenderTableSection::numRows() const
{
    ASSERT(!m_needsCellRecalc);
    return m_grid.size();
}

inline void RenderTableSection::recalcCellsIfNeeded()
{
    if (m_needsCellRecalc)
        recalcCells();
}

inline LayoutUnit RenderTableSection::rowBaseline(unsigned row)
{
    recalcCellsIfNeeded();
    return m_grid[row].baseline;
}

inline CellSpan RenderTableSection::fullTableRowSpan() const
{
    ASSERT(!m_needsCellRecalc);
    return CellSpan(0, m_grid.size());
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderTableSection, isTableSection())

#endif // RenderTableSection_h
