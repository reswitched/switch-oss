/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2004, 2006, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef HTMLFrameElementBase_h
#define HTMLFrameElementBase_h

#include "FrameLoaderTypes.h"
#include "HTMLFrameOwnerElement.h"
#include "ScrollTypes.h"

namespace WebCore {

class HTMLFrameElementBase : public HTMLFrameOwnerElement {
public:
    URL location() const;
    void setLocation(const String&);

    virtual ScrollbarMode scrollingMode() const override final { return m_scrolling; }
    
    int marginWidth() const { return m_marginWidth; }
    int marginHeight() const { return m_marginHeight; }

    int width();
    int height();

    virtual bool canContainRangeEndPoint() const override final { return false; }

    bool isURLAllowed(const URL&) const override;

protected:
    HTMLFrameElementBase(const QualifiedName&, Document&);

    bool isURLAllowed() const;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override final;
    virtual void finishedInsertingSubtree() override final;
    virtual void didAttachRenderers() override;

private:
    virtual bool supportsFocus() const override final;
    virtual void setFocus(bool) override final;
    
    virtual bool isURLAttribute(const Attribute&) const override final;
    virtual bool isHTMLContentAttribute(const Attribute&) const override final;

    virtual bool isFrameElementBase() const override final { return true; }

    void setNameAndOpenURL();
    void openURL(LockHistory = LockHistory::Yes, LockBackForwardList = LockBackForwardList::Yes);

    AtomicString m_URL;
    AtomicString m_frameName;

    ScrollbarMode m_scrolling;

    int m_marginWidth;
    int m_marginHeight;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::HTMLFrameElementBase)
    static bool isType(const WebCore::HTMLElement& element) { return is<WebCore::HTMLFrameElement>(element) || is<WebCore::HTMLIFrameElement>(element); }
    static bool isType(const WebCore::Node& node) { return is<WebCore::HTMLElement>(node) && isType(downcast<WebCore::HTMLElement>(node)); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // HTMLFrameElementBase_h
