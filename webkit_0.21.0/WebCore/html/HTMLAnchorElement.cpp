/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#include "config.h"
#include "HTMLAnchorElement.h"

#include "DNS.h"
#include "ElementIterator.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderTypes.h"
#include "FrameSelection.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLParserIdioms.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "PingLoader.h"
#include "PlatformMouseEvent.h"
#include "RelList.h"
#include "RenderImage.h"
#include "ResourceRequest.h"
#include "SVGImage.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

HTMLAnchorElement::HTMLAnchorElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
    , m_hasRootEditableElementForSelectionOnMouseDown(false)
    , m_wasShiftKeyDownOnMouseDown(false)
    , m_linkRelations(0)
    , m_cachedVisitedLinkHash(0)
{
}

Ref<HTMLAnchorElement> HTMLAnchorElement::create(Document& document)
{
    return adoptRef(*new HTMLAnchorElement(aTag, document));
}

Ref<HTMLAnchorElement> HTMLAnchorElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new HTMLAnchorElement(tagName, document));
}

HTMLAnchorElement::~HTMLAnchorElement()
{
    clearRootEditableElementForSelectionOnMouseDown();
}

// This function does not allow leading spaces before the port number.
static unsigned parsePortFromStringPosition(const String& value, unsigned portStart, unsigned& portEnd)
{
    portEnd = portStart;
    while (isASCIIDigit(value[portEnd]))
        ++portEnd;
    return value.substring(portStart, portEnd - portStart).toUInt();
}

bool HTMLAnchorElement::supportsFocus() const
{
    if (hasEditableStyle())
        return HTMLElement::supportsFocus();
    // If not a link we should still be able to focus the element if it has tabIndex.
    return isLink() || HTMLElement::supportsFocus();
}

bool HTMLAnchorElement::isMouseFocusable() const
{
#if !(PLATFORM(EFL) || PLATFORM(GTK))
    // Only allow links with tabIndex or contentEditable to be mouse focusable.
    // This is our rule for the Mac platform; on many other platforms we focus any link you click on.
    if (isLink())
        return HTMLElement::supportsFocus();
#endif

    return HTMLElement::isMouseFocusable();
}

static bool hasNonEmptyBox(RenderBoxModelObject* renderer)
{
    if (!renderer)
        return false;

    // Before calling absoluteRects, check for the common case where borderBoundingBox
    // is non-empty, since this is a faster check and almost always returns true.
    // FIXME: Why do we need to call absoluteRects at all?
    if (!renderer->borderBoundingBox().isEmpty())
        return true;

    // FIXME: Since all we are checking is whether the rects are empty, could we just
    // pass in 0,0 for the layout point instead of calling localToAbsolute?
    Vector<IntRect> rects;
    renderer->absoluteRects(rects, flooredLayoutPoint(renderer->localToAbsolute()));
    size_t size = rects.size();
    for (size_t i = 0; i < size; ++i) {
        if (!rects[i].isEmpty())
            return true;
    }

#if PLATFORM(WKC)
    Node* node = static_cast<RenderObject*>(renderer)->node();
    if (!node)
        return false;
    if (!node->renderer())
        return false;
    ASSERT(is<HTMLAnchorElement>(*node));
    RenderStyle& style = node->renderer()->style();
    bool isOverflowXHidden = (style.overflowX() == OHIDDEN);
    bool isOverflowYHidden = (style.overflowY() == OHIDDEN);
    LayoutRect rect;
    node->getNodeCompositeRect(&rect, isOverflowXHidden, isOverflowYHidden);
    if (!rect.isEmpty())
        return true;
#endif

    return false;
}

bool HTMLAnchorElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    if (!isLink())
        return HTMLElement::isKeyboardFocusable(event);

    if (!isFocusable())
        return false;
    
    if (!document().frame())
        return false;

    if (!document().frame()->eventHandler().tabsToLinks(event))
        return false;

    if (!renderer() && ancestorsOfType<HTMLCanvasElement>(*this).first())
        return true;

    return hasNonEmptyBox(renderBoxModelObject());
}

static void appendServerMapMousePosition(StringBuilder& url, Event* event)
{
    ASSERT(event);
    if (!is<MouseEvent>(*event))
        return;

    ASSERT(event->target());
    Node* target = event->target()->toNode();
    ASSERT(target);
    if (!is<HTMLImageElement>(*target))
        return;

    HTMLImageElement& imageElement = downcast<HTMLImageElement>(*target);
    if (!imageElement.isServerMap())
        return;

    if (!is<RenderImage>(imageElement.renderer()))
        return;
    auto& renderer = downcast<RenderImage>(*imageElement.renderer());

    // FIXME: This should probably pass true for useTransforms.
    FloatPoint absolutePosition = renderer.absoluteToLocal(FloatPoint(downcast<MouseEvent>(*event).pageX(), downcast<MouseEvent>(*event).pageY()));
    int x = absolutePosition.x();
    int y = absolutePosition.y();
    url.append('?');
    url.appendNumber(x);
    url.append(',');
    url.appendNumber(y);
}

void HTMLAnchorElement::defaultEventHandler(Event* event)
{
    if (isLink()) {
        if (focused() && isEnterKeyKeydownEvent(event) && treatLinkAsLiveForEventType(NonMouseEvent)) {
            event->setDefaultHandled();
            dispatchSimulatedClick(event);
            return;
        }

        if (MouseEvent::canTriggerActivationBehavior(*event) && treatLinkAsLiveForEventType(eventType(event))) {
            handleClick(event);
            return;
        }

        if (hasEditableStyle()) {
            // This keeps track of the editable block that the selection was in (if it was in one) just before the link was clicked
            // for the LiveWhenNotFocused editable link behavior
            if (event->type() == eventNames().mousedownEvent && is<MouseEvent>(*event) && downcast<MouseEvent>(*event).button() != (unsigned short)RightButton && document().frame()) {
                setRootEditableElementForSelectionOnMouseDown(document().frame()->selection().selection().rootEditableElement());
                m_wasShiftKeyDownOnMouseDown = downcast<MouseEvent>(*event).shiftKey();
            } else if (event->type() == eventNames().mouseoverEvent) {
                // These are cleared on mouseover and not mouseout because their values are needed for drag events,
                // but drag events happen after mouse out events.
                clearRootEditableElementForSelectionOnMouseDown();
                m_wasShiftKeyDownOnMouseDown = false;
            }
        }
    }

    HTMLElement::defaultEventHandler(event);
}

void HTMLAnchorElement::setActive(bool down, bool pause)
{
    if (hasEditableStyle()) {
        EditableLinkBehavior editableLinkBehavior = EditableLinkDefaultBehavior;
        if (Settings* settings = document().settings())
            editableLinkBehavior = settings->editableLinkBehavior();
            
        switch (editableLinkBehavior) {
            default:
            case EditableLinkDefaultBehavior:
            case EditableLinkAlwaysLive:
                break;

            case EditableLinkNeverLive:
                return;

            // Don't set the link to be active if the current selection is in the same editable block as
            // this link
            case EditableLinkLiveWhenNotFocused:
                if (down && document().frame() && document().frame()->selection().selection().rootEditableElement() == rootEditableElement())
                    return;
                break;
            
            case EditableLinkOnlyLiveWithShiftKey:
                return;
        }

    }
    
    HTMLElement::setActive(down, pause);
}

void HTMLAnchorElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == hrefAttr) {
        bool wasLink = isLink();
        setIsLink(!value.isNull() && !shouldProhibitLinks(this));
        if (wasLink != isLink())
            setNeedsStyleRecalc();
        if (isLink()) {
            String parsedURL = stripLeadingAndTrailingHTMLSpaces(value);
            if (document().isDNSPrefetchEnabled()) {
                if (protocolIsInHTTPFamily(parsedURL) || parsedURL.startsWith("//"))
                    prefetchDNS(document().completeURL(parsedURL).host());
            }
        }
        invalidateCachedVisitedLinkHash();
    } else if (name == nameAttr || name == titleAttr) {
        // Do nothing.
    } else if (name == relAttr) {
        if (SpaceSplitString::spaceSplitStringContainsValue(value, "noreferrer", true))
            m_linkRelations |= RelationNoReferrer;
        if (m_relList)
            m_relList->updateRelAttribute(value);
    }
    else
        HTMLElement::parseAttribute(name, value);
}

void HTMLAnchorElement::accessKeyAction(bool sendMouseEvents)
{
    dispatchSimulatedClick(0, sendMouseEvents ? SendMouseUpDownEvents : SendNoEvents);
}

bool HTMLAnchorElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name().localName() == hrefAttr || HTMLElement::isURLAttribute(attribute);
}

bool HTMLAnchorElement::canStartSelection() const
{
    if (!isLink())
        return HTMLElement::canStartSelection();
    return hasEditableStyle();
}

bool HTMLAnchorElement::draggable() const
{
    // Should be draggable if we have an href attribute.
    const AtomicString& value = fastGetAttribute(draggableAttr);
    if (equalIgnoringCase(value, "true"))
        return true;
    if (equalIgnoringCase(value, "false"))
        return false;
    return hasAttribute(hrefAttr);
}

URL HTMLAnchorElement::href() const
{
    return document().completeURL(stripLeadingAndTrailingHTMLSpaces(getAttribute(hrefAttr)));
}

void HTMLAnchorElement::setHref(const AtomicString& value)
{
    setAttribute(hrefAttr, value);
}

bool HTMLAnchorElement::hasRel(uint32_t relation) const
{
    return m_linkRelations & relation;
}

DOMTokenList& HTMLAnchorElement::relList()
{
    if (!m_relList) 
        m_relList = std::make_unique<RelList>(*this);
    return *m_relList;
}

const AtomicString& HTMLAnchorElement::name() const
{
    return getNameAttribute();
}

short HTMLAnchorElement::tabIndex() const
{
    // Skip the supportsFocus check in HTMLElement.
    return Element::tabIndex();
}

String HTMLAnchorElement::target() const
{
    return getAttribute(targetAttr);
}

String HTMLAnchorElement::hash() const
{
    String fragmentIdentifier = href().fragmentIdentifier();
    if (fragmentIdentifier.isEmpty())
        return emptyString();
    return AtomicString(String("#" + fragmentIdentifier));
}

void HTMLAnchorElement::setHash(const String& value)
{
    URL url = href();
    if (value[0] == '#')
        url.setFragmentIdentifier(value.substring(1));
    else
        url.setFragmentIdentifier(value);
    setHref(url.string());
}

String HTMLAnchorElement::host() const
{
    const URL& url = href();
    if (url.hostEnd() == url.pathStart())
        return url.host();
    if (isDefaultPortForProtocol(url.port(), url.protocol()))
        return url.host();
    return url.host() + ":" + String::number(url.port());
}

void HTMLAnchorElement::setHost(const String& value)
{
    if (value.isEmpty())
        return;
    URL url = href();
    if (!url.canSetHostOrPort())
        return;

    size_t separator = value.find(':');
    if (!separator)
        return;

    if (separator == notFound)
        url.setHostAndPort(value);
    else {
        unsigned portEnd;
        unsigned port = parsePortFromStringPosition(value, separator + 1, portEnd);
        if (!port) {
            // http://dev.w3.org/html5/spec/infrastructure.html#url-decomposition-idl-attributes
            // specifically goes against RFC 3986 (p3.2) and
            // requires setting the port to "0" if it is set to empty string.
            url.setHostAndPort(value.substring(0, separator + 1) + "0");
        } else {
            if (isDefaultPortForProtocol(port, url.protocol()))
                url.setHostAndPort(value.substring(0, separator));
            else
                url.setHostAndPort(value.substring(0, portEnd));
        }
    }
    setHref(url.string());
}

String HTMLAnchorElement::hostname() const
{
    return href().host();
}

void HTMLAnchorElement::setHostname(const String& value)
{
    // Before setting new value:
    // Remove all leading U+002F SOLIDUS ("/") characters.
    unsigned i = 0;
    unsigned hostLength = value.length();
    while (value[i] == '/')
        i++;

    if (i == hostLength)
        return;

    URL url = href();
    if (!url.canSetHostOrPort())
        return;

    url.setHost(value.substring(i));
    setHref(url.string());
}

String HTMLAnchorElement::pathname() const
{
    return href().path();
}

void HTMLAnchorElement::setPathname(const String& value)
{
    URL url = href();
    if (!url.canSetPathname())
        return;

    if (value[0] == '/')
        url.setPath(value);
    else
        url.setPath("/" + value);

    setHref(url.string());
}

String HTMLAnchorElement::port() const
{
    if (href().hasPort())
        return String::number(href().port());

    return emptyString();
}

void HTMLAnchorElement::setPort(const String& value)
{
    URL url = href();
    if (!url.canSetHostOrPort())
        return;

    // http://dev.w3.org/html5/spec/infrastructure.html#url-decomposition-idl-attributes
    // specifically goes against RFC 3986 (p3.2) and
    // requires setting the port to "0" if it is set to empty string.
    unsigned port = value.toUInt();
    if (isDefaultPortForProtocol(port, url.protocol()))
        url.removePort();
    else
        url.setPort(port);

    setHref(url.string());
}

String HTMLAnchorElement::protocol() const
{
    return href().protocol() + ":";
}

void HTMLAnchorElement::setProtocol(const String& value)
{
    URL url = href();
    url.setProtocol(value);
    setHref(url.string());
}

String HTMLAnchorElement::search() const
{
    String query = href().query();
    return query.isEmpty() ? emptyString() : "?" + query;
}

String HTMLAnchorElement::origin() const
{
    return SecurityOrigin::create(href()).get().toString();
}

void HTMLAnchorElement::setSearch(const String& value)
{
    URL url = href();
    String newSearch = (value[0] == '?') ? value.substring(1) : value;
    // Make sure that '#' in the query does not leak to the hash.
    url.setQuery(newSearch.replaceWithLiteral('#', "%23"));

    setHref(url.string());
}

String HTMLAnchorElement::text()
{
    return textContent();
}

void HTMLAnchorElement::setText(const String& text, ExceptionCode& ec)
{
    setTextContent(text, ec);
}

String HTMLAnchorElement::toString() const
{
    return href().string();
}

bool HTMLAnchorElement::isLiveLink() const
{
    return isLink() && treatLinkAsLiveForEventType(m_wasShiftKeyDownOnMouseDown ? MouseEventWithShiftKey : MouseEventWithoutShiftKey);
}

void HTMLAnchorElement::sendPings(const URL& destinationURL)
{
    if (!fastHasAttribute(pingAttr) || !document().settings() || !document().settings()->hyperlinkAuditingEnabled())
        return;

    SpaceSplitString pingURLs(fastGetAttribute(pingAttr), false);
    for (unsigned i = 0; i < pingURLs.size(); i++)
        PingLoader::sendPing(*document().frame(), document().completeURL(pingURLs[i]), destinationURL);
}

void HTMLAnchorElement::handleClick(Event* event)
{
    event->setDefaultHandled();

    Frame* frame = document().frame();
    if (!frame)
        return;

    if (document().pageCacheState() != Document::NotInPageCache)
        return;

    StringBuilder url;
    url.append(stripLeadingAndTrailingHTMLSpaces(fastGetAttribute(hrefAttr)));
    appendServerMapMousePosition(url, event);
    URL kurl = document().completeURL(url.toString());

    String downloadAttribute;
#if ENABLE(DOWNLOAD_ATTRIBUTE)
    if (hasAttribute(downloadAttr)) {
        ResourceRequest request(kurl);

        // FIXME: Why are we not calling addExtraFieldsToMainResourceRequest() if this check fails? It sets many important header fields.
        if (!hasRel(RelationNoReferrer)) {
            String referrer = SecurityPolicy::generateReferrerHeader(document().referrerPolicy(), kurl, frame->loader().outgoingReferrer());
            if (!referrer.isEmpty())
                request.setHTTPReferrer(referrer);
            frame->loader().addExtraFieldsToMainResourceRequest(request);
        }

        frame->loader().client().startDownload(request, fastGetAttribute(downloadAttr));
    } else
#endif

    frame->loader().urlSelected(kurl, target(), event, LockHistory::No, LockBackForwardList::No, hasRel(RelationNoReferrer) ? NeverSendReferrer : MaybeSendReferrer, document().shouldOpenExternalURLsPolicyToPropagate());

    sendPings(kurl);
}

HTMLAnchorElement::EventType HTMLAnchorElement::eventType(Event* event)
{
    ASSERT(event);
    if (!is<MouseEvent>(*event))
        return NonMouseEvent;
    return downcast<MouseEvent>(*event).shiftKey() ? MouseEventWithShiftKey : MouseEventWithoutShiftKey;
}

bool HTMLAnchorElement::treatLinkAsLiveForEventType(EventType eventType) const
{
    if (!hasEditableStyle())
        return true;

    Settings* settings = document().settings();
    if (!settings)
        return true;

    switch (settings->editableLinkBehavior()) {
    case EditableLinkDefaultBehavior:
    case EditableLinkAlwaysLive:
        return true;

    case EditableLinkNeverLive:
        return false;

    // If the selection prior to clicking on this link resided in the same editable block as this link,
    // and the shift key isn't pressed, we don't want to follow the link.
    case EditableLinkLiveWhenNotFocused:
        return eventType == MouseEventWithShiftKey || (eventType == MouseEventWithoutShiftKey && rootEditableElementForSelectionOnMouseDown() != rootEditableElement());

    case EditableLinkOnlyLiveWithShiftKey:
        return eventType == MouseEventWithShiftKey;
    }

    ASSERT_NOT_REACHED();
    return false;
}

bool isEnterKeyKeydownEvent(Event* event)
{
    return event->type() == eventNames().keydownEvent && is<KeyboardEvent>(*event) && downcast<KeyboardEvent>(*event).keyIdentifier() == "Enter";
}

bool shouldProhibitLinks(Element* element)
{
    return isInSVGImage(element);
}

bool HTMLAnchorElement::willRespondToMouseClickEvents()
{
    return isLink() || HTMLElement::willRespondToMouseClickEvents();
}

typedef HashMap<const HTMLAnchorElement*, RefPtr<Element>> RootEditableElementMap;

static RootEditableElementMap& rootEditableElementMap()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(RootEditableElementMap, map, ());
    return map;
}

Element* HTMLAnchorElement::rootEditableElementForSelectionOnMouseDown() const
{
    if (!m_hasRootEditableElementForSelectionOnMouseDown)
        return 0;
    return rootEditableElementMap().get(this);
}

void HTMLAnchorElement::clearRootEditableElementForSelectionOnMouseDown()
{
    if (!m_hasRootEditableElementForSelectionOnMouseDown)
        return;
    rootEditableElementMap().remove(this);
    m_hasRootEditableElementForSelectionOnMouseDown = false;
}

void HTMLAnchorElement::setRootEditableElementForSelectionOnMouseDown(Element* element)
{
    if (!element) {
        clearRootEditableElementForSelectionOnMouseDown();
        return;
    }

    rootEditableElementMap().set(this, element);
    m_hasRootEditableElementForSelectionOnMouseDown = true;
}

}
