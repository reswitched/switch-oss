/*
 * Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCFrameSelection.h"
#include "helpers/privates/WKCFrameSelectionPrivate.h"

#include "FrameSelection.h"

namespace WKC {

FrameSelectionPrivate::FrameSelectionPrivate(WebCore::FrameSelection* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

FrameSelectionPrivate::~FrameSelectionPrivate()
{
}

WKCRect
FrameSelectionPrivate::absoluteCaretBounds()
{
    m_webcore->setCaretRectNeedsUpdate();
    return (WKCRect)m_webcore->absoluteCaretBounds();
}

bool
FrameSelectionPrivate::isCaret() const
{
    return m_webcore->isCaret();
}

bool
FrameSelectionPrivate::isRange() const
{
    return m_webcore->isRange();
}

void
FrameSelectionPrivate::clear()
{
    m_webcore->clear();
}

void
FrameSelectionPrivate::setCaretVisible(bool caretIsVisible)
{
    m_webcore->setCaretVisible(caretIsVisible);
}

void
FrameSelectionPrivate::setCaretBlinkingSuspended(bool suspended)
{
    m_webcore->setCaretBlinkingSuspended(suspended);
}

bool
FrameSelectionPrivate::isContentEditable() const
{
    return m_webcore->selection().isContentEditable();
}

////////////////////////////////////////////////////////////////////////////////

FrameSelection::FrameSelection(FrameSelectionPrivate& parent)
    : m_private(parent)
{
}

FrameSelection::~FrameSelection()
{
}

WKCRect
FrameSelection::absoluteCaretBounds()
{
    return m_private.absoluteCaretBounds();
}

bool
FrameSelection::isCaret() const
{
    return m_private.isCaret();
}

bool
FrameSelection::isRange() const
{
    return m_private.isRange();
}

void
FrameSelection::clear()
{
    m_private.clear();
}

void
FrameSelection::setCaretVisible(bool caretIsVisible)
{
    m_private.setCaretVisible(caretIsVisible);
}

void
FrameSelection::setCaretBlinkingSuspended(bool suspended)
{
    m_private.setCaretBlinkingSuspended(suspended);
}

bool
FrameSelection::isContentEditable() const
{
    return m_private.isContentEditable();
}

} // namespace
