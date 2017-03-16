/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_HITTESTRESULT_H_
#define _WKC_HELPERS_PRIVATE_HITTESTRESULT_H_

#include <wkc/wkcbase.h>
#include "helpers/WKCHitTestResult.h"

#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"

namespace WebCore {
class HitTestResult;
} // namespace

namespace WKC {

class ElementPrivate;
class ImagePrivate;
class FramePrivate;
class NodePrivate;
class ScrollbarPrivate;

class HitTestResultWrap : public HitTestResult {
WTF_MAKE_FAST_ALLOCATED;
public:
    HitTestResultWrap(HitTestResultPrivate* parent) : HitTestResult(parent) {}
    ~HitTestResultWrap() {}

    using HitTestResult::operator new;
    using HitTestResult::operator new[];
};

class HitTestResultPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HitTestResultPrivate(const WebCore::HitTestResult&); // instance from core to app
    HitTestResultPrivate(const HitTestResult&, const WKCPoint&); // instance from app to core
    ~HitTestResultPrivate();

    HitTestResultPrivate(const HitTestResultPrivate&);

    const WebCore::HitTestResult& webcore() const { return *m_webcore; }
    const HitTestResult& wkc() const { return *m_wkc; }

    Node* innerNode();
    Node* innerNonSharedNode();
    WKCPoint point() const;
    WKCPoint localPoint() const;
    Element* URLElement();
    Scrollbar* scrollbar();
    bool isOverWidget() const;

    Frame* targetFrame();
    bool isSelected() const;
    String spellingToolTip(TextDirection&) const;
    String replacedString() const;
    String title(TextDirection&) const;
    String altDisplayString() const;
    String titleDisplayString() const;
    Image* image();
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

private:
    HitTestResultPrivate& operator=(const HitTestResultPrivate&); // not needed

    const WebCore::HitTestResult* m_webcore;
    const HitTestResultWrap* m_wkc;
    const bool m_isWebcoreOwned;
    const bool m_isWkcOwned;

    NodePrivate* m_innerNode;
    NodePrivate* m_innerNonSharedNode;
    ElementPrivate* m_URLElement;
    FramePrivate* m_targetFrame;
    ImagePrivate* m_image;
    ScrollbarPrivate* m_scrollbar;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_HITTESTRESULT_H_
