/*
 *  Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_FRAMESELECTION_H_
#define _WKC_HELPERS_PRIVATE_FRAMESELECTION_H_

#include "helpers/WKCFrameSelection.h"

namespace WebCore {
class FrameSelection;
} // namespace

namespace WKC {

class FrameSelectionWrap : public FrameSelection {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FrameSelectionWrap(FrameSelectionPrivate& parent) : FrameSelection(parent) {}
    ~FrameSelectionWrap() {}
};

class FrameSelectionPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FrameSelectionPrivate(WebCore::FrameSelection*);
    ~FrameSelectionPrivate();

    WebCore::FrameSelection* webcore() const { return m_webcore; }
    FrameSelection& wkc() { return m_wkc; }

    // Bounds of (possibly transformed) caret in absolute coords
    WKCRect absoluteCaretBounds();

    bool isCaret() const;
    bool isRange() const;
    void clear();
    void setCaretVisible(bool caretIsVisible);
    void setCaretBlinkingSuspended(bool suspended);
    bool isContentEditable() const;

private:
    WebCore::FrameSelection* m_webcore;
    FrameSelectionWrap m_wkc;

};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_FRAMESELECTION_H_
