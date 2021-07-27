/*
 * Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_HITTESTRESULT_H_
#define _WKC_HELPERS_WKC_HITTESTRESULT_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHelpersEnums.h"

namespace WKC {
class HitTestResultPrivate;

class Element;
class Frame;
class Image;
class KURL;
class Node;
class Scrollbar;
class String;

class WKC_API HitTestResult {
public:
    HitTestResult(const WKCPoint&);
    ~HitTestResult();

    Node* innerNode() const;
    Node* innerNonSharedNode() const;
    WKCPoint point() const;
    WKCPoint localPoint() const;
    Element* URLElement() const;
    Scrollbar* scrollbar() const;
    bool isOverWidget() const;

    Frame* targetFrame() const;
    bool isSelected() const;
    String spellingToolTip(TextDirection&) const;
    String replacedString() const;
    String title(TextDirection&) const;
    String altDisplayString() const;
    String titleDisplayString() const;
    Image* image() const;
    WKCRect imageRect() const;
    KURL absoluteImageURL() const;
    KURL absoluteMediaURL() const;
    KURL absoluteLinkURL() const;
    String textContent() const;
    bool isLiveLink() const;
    bool isContentEditable() const;
    void toggleMediaControlsDisplay() const;
    void toggleMediaLoopPlayback() const;
    void enterFullscreenForVideo() const;
    bool mediaControlsEnabled() const;
    bool mediaLoopEnabled() const;
    bool mediaPlaying() const;
    bool mediaSupportsFullscreen() const;
    void toggleMediaPlayState() const;
    bool mediaHasAudio() const;
    bool mediaIsVideo() const;
    bool mediaMuted() const;
    void toggleMediaMuteState() const;

    HitTestResultPrivate* priv() const { return m_private; }

protected:
    HitTestResult(HitTestResultPrivate*); // Not for applications, only for WKC.

    // Heap allocation by operator new in applications is disallowed (allowed only in WKC).
    // This restriction is to avoid memory leaks or crashes.
    void* operator new(size_t);
    void* operator new[](size_t);

private:
    HitTestResult(const HitTestResult&);
    HitTestResult& operator=(const HitTestResult&);

    HitTestResultPrivate* m_private;
    bool m_owned;
};

} // namespace

#endif // _WKC_HELPERS_WKC_HITTESTRESULT_H_
