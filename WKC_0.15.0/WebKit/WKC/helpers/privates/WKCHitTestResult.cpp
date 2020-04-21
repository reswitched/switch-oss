/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCHitTestResult.h"
#include "helpers/privates/WKCHitTestResultPrivate.h"

#include "HitTestResult.h"
#include "HTMLAnchorElement.h"
#include "SVGElement.h"
#include "SVGNames.h"
#include "TypeCasts.h"
#include "URL.h"
#include "WTFString.h"

#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCImagePrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCScrollbarPrivate.h"

namespace WKC {

HitTestResultPrivate::HitTestResultPrivate(const WebCore::HitTestResult& parent)
    : m_webcore(&parent)
    , m_wkc(new HitTestResultWrap(this))
    , m_isWebcoreOwned(false)
    , m_isWkcOwned(true)
    , m_innerNode(0)
    , m_innerNonSharedNode(0)
    , m_URLElement(0)
    , m_targetFrame(0)
    , m_image(0)
    , m_scrollbar(0)
{
}

HitTestResultPrivate::HitTestResultPrivate(const HitTestResult& parent, const WKCPoint& pos)
    : m_webcore(new WebCore::HitTestResult(pos))
    , m_wkc(static_cast<const HitTestResultWrap*>(&parent))
    , m_isWebcoreOwned(true)
    , m_isWkcOwned(false)
    , m_innerNode(0)
    , m_innerNonSharedNode(0)
    , m_URLElement(0)
    , m_targetFrame(0)
    , m_image(0)
    , m_scrollbar(0)
{
}

HitTestResultPrivate::~HitTestResultPrivate()
{
    delete m_innerNode;
    delete m_innerNonSharedNode;
    delete m_URLElement;
    delete m_targetFrame;
    delete m_image;
    delete m_scrollbar;

    if (m_isWebcoreOwned)
        delete m_webcore;
    if (m_isWkcOwned)
        delete m_wkc;
}

HitTestResultPrivate::HitTestResultPrivate(const HitTestResultPrivate& other)
    : m_webcore(other.m_isWebcoreOwned ? new WebCore::HitTestResult(*(other.m_webcore)) : other.m_webcore)
    , m_wkc(other.m_isWkcOwned ? new HitTestResultWrap(this) : other.m_wkc)
    , m_isWebcoreOwned(other.m_isWebcoreOwned)
    , m_isWkcOwned(other.m_isWkcOwned)
    , m_innerNode(0)
    , m_innerNonSharedNode(0)
    , m_URLElement(0)
    , m_targetFrame(0)
    , m_image(0)
    , m_scrollbar(0)
{
#define duplicateMemberIfExists(coretype, methodname, membername, instantiator) \
    coretype methodname = other.webcore().methodname(); \
    if (other.membername && methodname) \
        membername = instantiator;

    duplicateMemberIfExists(WebCore::Node*, innerNode, m_innerNode, NodePrivate::create(innerNode))
    duplicateMemberIfExists(WebCore::Node*, innerNonSharedNode, m_innerNonSharedNode, NodePrivate::create(innerNonSharedNode))
    duplicateMemberIfExists(WebCore::Element*, URLElement, m_URLElement, ElementPrivate::create(URLElement))
    duplicateMemberIfExists(WebCore::Frame*, targetFrame, m_targetFrame, new FramePrivate(targetFrame))
    duplicateMemberIfExists(WebCore::Image*, image, m_image, new ImagePrivate(image))
    duplicateMemberIfExists(WebCore::Scrollbar*, scrollbar, m_scrollbar, new ScrollbarPrivate(scrollbar))

#undef duplicateMemberIfExists
}

HitTestResultPrivate&
HitTestResultPrivate::operator =(const HitTestResultPrivate& other)
{
    ASSERT_NOT_REACHED(); // Implement correctly when this method is needed.
    return *this;
}

Node*
HitTestResultPrivate::innerNode()
{
    WebCore::Node* inode = webcore().innerNode();
    if (!inode)
        return 0;

    delete m_innerNode;
    m_innerNode = NodePrivate::create(inode);
    return &m_innerNode->wkc();
}

Node*
HitTestResultPrivate::innerNonSharedNode()
{
    WebCore::Node* inode = webcore().innerNonSharedNode();
    if (!inode)
        return 0;

    delete m_innerNonSharedNode;
    m_innerNonSharedNode = NodePrivate::create(inode);
    return &m_innerNonSharedNode->wkc();
}

WKCPoint
HitTestResultPrivate::point() const
{
    return webcore().localPoint();
}

WKCPoint
HitTestResultPrivate::localPoint() const
{
    return webcore().localPoint();
}

Element*
HitTestResultPrivate::URLElement()
{
    WebCore::Element* element = webcore().URLElement();
    if (!element)
        return 0;

    delete m_URLElement;
    m_URLElement = ElementPrivate::create(element);
    return &m_URLElement->wkc();
}

Scrollbar*
HitTestResultPrivate::scrollbar()
{
    WebCore::Scrollbar* bar = webcore().scrollbar();
    if (!bar)
        return 0;
    if (!m_scrollbar || m_scrollbar->webcore()!=bar) {
        delete m_scrollbar;
        m_scrollbar = new ScrollbarPrivate(bar);
    }
    return &m_scrollbar->wkc();
}

bool
HitTestResultPrivate::isOverWidget() const
{
    return webcore().isOverWidget();
}


Frame*
HitTestResultPrivate::targetFrame()
{
    WebCore::Frame* frame = webcore().targetFrame();
    if (!frame)
        return 0;

    delete m_targetFrame;
    m_targetFrame = new FramePrivate(frame);
    return &m_targetFrame->wkc();
}

bool
HitTestResultPrivate::isSelected() const
{
    return webcore().isSelected();
}

String
HitTestResultPrivate::spellingToolTip(TextDirection& dir) const
{
    WebCore::TextDirection wd;
    WTF::String s = webcore().spellingToolTip(wd);
    dir = toWKCTextDirection(wd);
    return s;
}

String
HitTestResultPrivate::replacedString() const
{
    return webcore().replacedString();
}

String
HitTestResultPrivate::title(TextDirection& dir) const
{
    WebCore::TextDirection wd;
    String s = webcore().title(wd);
    dir = toWKCTextDirection(wd);
    return s;
}

String
HitTestResultPrivate::altDisplayString() const
{
    return webcore().altDisplayString();
}

String
HitTestResultPrivate::titleDisplayString() const
{
    return webcore().titleDisplayString();
}

Image*
HitTestResultPrivate::image()
{
    WebCore::Image* img = webcore().image();
    if (!img)
        return 0;

    delete m_image;
    m_image = new ImagePrivate(img);
    return &m_image->wkc();
}

WKCRect
HitTestResultPrivate::imageRect() const
{
    return webcore().imageRect();
}

KURL
HitTestResultPrivate::absoluteImageURL() const
{
    return webcore().absoluteImageURL();
}

KURL
HitTestResultPrivate::absoluteMediaURL() const
{
    return webcore().absoluteMediaURL();
}

KURL
HitTestResultPrivate::absoluteLinkURL() const
{
    return webcore().absoluteLinkURL();
}

String
HitTestResultPrivate::textContent() const
{
    return webcore().textContent();
}

bool
HitTestResultPrivate::isLiveLink() const
{
    if (!webcore().URLElement())
        return false;

    if (WTF::is<WebCore::HTMLAnchorElement>(webcore().URLElement()))
        return ((WebCore::HTMLAnchorElement *)(webcore().URLElement()))->isLiveLink();

    if (webcore().URLElement()->hasTagName(WebCore::SVGNames::aTag))
        return webcore().URLElement()->isLink();

    return false;
}

bool
HitTestResultPrivate::isContentEditable() const
{
    return webcore().isContentEditable();
}

void
HitTestResultPrivate::toggleMediaControlsDisplay() const
{
    webcore().toggleMediaControlsDisplay();
}

void
HitTestResultPrivate::toggleMediaLoopPlayback() const
{
    webcore().toggleMediaLoopPlayback();
}

void
HitTestResultPrivate::enterFullscreenForVideo() const
{
    webcore().enterFullscreenForVideo();
}

bool
HitTestResultPrivate::mediaControlsEnabled() const
{
    return webcore().mediaControlsEnabled();
}

bool
HitTestResultPrivate::mediaLoopEnabled() const
{
    return webcore().mediaLoopEnabled();
}

bool
HitTestResultPrivate::mediaPlaying() const
{
    return webcore().mediaPlaying();
}

bool
HitTestResultPrivate::mediaSupportsFullscreen() const
{
    return webcore().mediaSupportsFullscreen();
}

void
HitTestResultPrivate::toggleMediaPlayState() const
{
    webcore().toggleMediaPlayState();
}

bool
HitTestResultPrivate::mediaHasAudio() const
{
    return webcore().mediaHasAudio();
}

bool
HitTestResultPrivate::mediaIsVideo() const
{
    return webcore().mediaIsVideo();
}

bool
HitTestResultPrivate::mediaMuted() const
{
    return webcore().mediaMuted();
}

void
HitTestResultPrivate::toggleMediaMuteState() const
{
    webcore().toggleMediaMuteState();
}

////////////////////////////////////////////////////////////////////////////////

HitTestResult::HitTestResult(HitTestResultPrivate* parent)
    : m_private(parent)
    , m_owned(false)
{
}

HitTestResult::HitTestResult(const WKCPoint& pos)
    : m_private(new HitTestResultPrivate(*this, pos))
    , m_owned(true)
{
}

HitTestResult::~HitTestResult()
{
    if (m_owned)
        delete m_private;
}

HitTestResult::HitTestResult(const HitTestResult& other)
    : m_private(other.m_private)
    , m_owned(other.m_owned)
{
    if (m_owned)
        m_private = new HitTestResultPrivate(*(other.m_private)); // create my own copy
}

HitTestResult&
HitTestResult::operator=(const HitTestResult& other)
{
    if (this != &other) {
        m_owned = other.m_owned;
        if (m_owned) {
            delete m_private;
            m_private = new HitTestResultPrivate(*(other.m_private)); // create my own copy
        } else {
            m_private = other.m_private;
        }
    }
    return *this;
}

void*
HitTestResult::operator new(size_t size)
{
    return WTF::fastMalloc(size);
}

void*
HitTestResult::operator new[](size_t size)
{
    return WTF::fastMalloc(size);
}

Node*
HitTestResult::innerNode() const
{
    return m_private->innerNode();
}

Node*
HitTestResult::innerNonSharedNode() const
{
    return m_private->innerNonSharedNode();
}

WKCPoint
HitTestResult::point() const
{
    return m_private->point();
}

WKCPoint
HitTestResult::localPoint() const
{
    return m_private->localPoint();
}

Element*
HitTestResult::URLElement() const
{
    return m_private->URLElement();
}

Scrollbar*
HitTestResult::scrollbar() const
{
    return m_private->scrollbar();
}

bool
HitTestResult::isOverWidget() const
{
    return m_private->isOverWidget();
}

Frame*
HitTestResult::targetFrame() const
{
    return m_private->targetFrame();
}

bool
HitTestResult::isSelected() const
{
    return m_private->isSelected();
}

String
HitTestResult::spellingToolTip(TextDirection& dir) const
{
    return m_private->spellingToolTip(dir);
}

String
HitTestResult::replacedString() const
{
    return m_private->replacedString();
}

String
HitTestResult::title(TextDirection& dir) const
{
    return m_private->title(dir);
}

String
HitTestResult::altDisplayString() const
{
    return m_private->altDisplayString();
}

String
HitTestResult::titleDisplayString() const
{
    return m_private->titleDisplayString();
}

Image*
HitTestResult::image() const
{
    return m_private->image();
}

WKCRect
HitTestResult::imageRect() const
{
    return m_private->imageRect();
}

KURL
HitTestResult::absoluteImageURL() const
{
    return m_private->absoluteImageURL();
}

KURL
HitTestResult::absoluteMediaURL() const
{
    return m_private->absoluteMediaURL();
}

KURL
HitTestResult::absoluteLinkURL() const
{
    return m_private->absoluteLinkURL();
}

String
HitTestResult::textContent() const
{
    return m_private->textContent();
}

bool
HitTestResult::isLiveLink() const
{
    return m_private->isLiveLink();
}

bool
HitTestResult::isContentEditable() const
{
    return m_private->isContentEditable();
}

void
HitTestResult::toggleMediaControlsDisplay() const
{
    m_private->toggleMediaControlsDisplay();
}

void
HitTestResult::toggleMediaLoopPlayback() const
{
    m_private->toggleMediaLoopPlayback();
}

void
HitTestResult::enterFullscreenForVideo() const
{
    m_private->enterFullscreenForVideo();
}

bool
HitTestResult::mediaControlsEnabled() const
{
    return m_private->mediaControlsEnabled();
}

bool
HitTestResult::mediaLoopEnabled() const
{
    return m_private->mediaLoopEnabled();
}

bool
HitTestResult::mediaPlaying() const
{
    return m_private->mediaPlaying();
}

bool
HitTestResult::mediaSupportsFullscreen() const
{
    return m_private->mediaSupportsFullscreen();
}

void
HitTestResult::toggleMediaPlayState() const
{
    m_private->toggleMediaPlayState();
}

bool
HitTestResult::mediaHasAudio() const
{
    return m_private->mediaHasAudio();
}

bool
HitTestResult::mediaIsVideo() const
{
    return m_private->mediaIsVideo();
}

bool
HitTestResult::mediaMuted() const
{
    return m_private->mediaMuted();
}

void 
HitTestResult::toggleMediaMuteState() const
{
    m_private->toggleMediaMuteState();
}

} // namespace
