/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "HTMLFrameOwnerElement.h"

#include "DOMWindow.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "RenderWidget.h"
#include "ShadowRoot.h"
#include "SVGDocument.h"
#include <wtf/Ref.h>

namespace WebCore {

HTMLFrameOwnerElement::HTMLFrameOwnerElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
    , m_contentFrame(nullptr)
    , m_sandboxFlags(SandboxNone)
{
}

RenderWidget* HTMLFrameOwnerElement::renderWidget() const
{
    // HTMLObjectElement and HTMLEmbedElement may return arbitrary renderers
    // when using fallback content.
    if (!is<RenderWidget>(renderer()))
        return nullptr;
    return downcast<RenderWidget>(renderer());
}

void HTMLFrameOwnerElement::setContentFrame(Frame* frame)
{
    // Make sure we will not end up with two frames referencing the same owner element.
    ASSERT(!m_contentFrame || m_contentFrame->ownerElement() != this);
    ASSERT(frame);
    // Disconnected frames should not be allowed to load.
    ASSERT(inDocument());
    m_contentFrame = frame;

    for (ContainerNode* node = this; node; node = node->parentOrShadowHostNode())
        node->incrementConnectedSubframeCount();
}

void HTMLFrameOwnerElement::clearContentFrame()
{
    if (!m_contentFrame)
        return;

    m_contentFrame = 0;

    for (ContainerNode* node = this; node; node = node->parentOrShadowHostNode())
        node->decrementConnectedSubframeCount();
}

void HTMLFrameOwnerElement::disconnectContentFrame()
{
    // FIXME: Currently we don't do this in removedFrom because this causes an
    // unload event in the subframe which could execute script that could then
    // reach up into this document and then attempt to look back down. We should
    // see if this behavior is really needed as Gecko does not allow this.
    if (Frame* frame = contentFrame()) {
        Ref<Frame> protect(*frame);
        frame->loader().frameDetached();
        frame->disconnectOwnerElement();
    }
}

HTMLFrameOwnerElement::~HTMLFrameOwnerElement()
{
    if (m_contentFrame)
        m_contentFrame->disconnectOwnerElement();
}

Document* HTMLFrameOwnerElement::contentDocument() const
{
    return m_contentFrame ? m_contentFrame->document() : 0;
}

DOMWindow* HTMLFrameOwnerElement::contentWindow() const
{
    return m_contentFrame ? m_contentFrame->document()->domWindow() : 0;
}

void HTMLFrameOwnerElement::setSandboxFlags(SandboxFlags flags)
{
    m_sandboxFlags = flags;
}

bool HTMLFrameOwnerElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    return m_contentFrame && HTMLElement::isKeyboardFocusable(event);
}

SVGDocument* HTMLFrameOwnerElement::getSVGDocument(ExceptionCode& ec) const
{
    Document* document = contentDocument();
    if (is<SVGDocument>(document))
        return downcast<SVGDocument>(document);
    // Spec: http://www.w3.org/TR/SVG/struct.html#InterfaceGetSVGDocument
    ec = NOT_SUPPORTED_ERR;
    return nullptr;
}

void HTMLFrameOwnerElement::scheduleSetNeedsStyleRecalc(StyleChangeType changeType)
{
    if (Style::postResolutionCallbacksAreSuspended()) {
        RefPtr<HTMLFrameOwnerElement> element = this;
#if !PLATFORM(WKC)
        Style::queuePostResolutionCallback([element, changeType]{
            element->setNeedsStyleRecalc(changeType);
        });
#else
        std::function<void()> p(std::allocator_arg, WTF::voidFuncAllocator(), [element, changeType]{
            element->setNeedsStyleRecalc(changeType);
        });
        Style::queuePostResolutionCallback(p);
#endif
    } else
        setNeedsStyleRecalc(changeType);
}

bool SubframeLoadingDisabler::canLoadFrame(HTMLFrameOwnerElement& owner)
{
    for (ContainerNode* node = &owner; node; node = node->parentOrShadowHostNode()) {
        if (disabledSubtreeRoots().contains(node))
            return false;
    }
    return true;
}

} // namespace WebCore
