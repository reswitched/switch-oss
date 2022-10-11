/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGScriptElement_h
#define SVGScriptElement_h

#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedString.h"
#include "SVGElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGURIReference.h"
#include "ScriptElement.h"

namespace WebCore {

class SVGScriptElement final : public SVGElement
                             , public SVGURIReference
                             , public SVGExternalResourcesRequired
                             , public ScriptElement {
public:
    static Ref<SVGScriptElement> create(const QualifiedName&, Document&, bool wasInsertedByParser);

private:
    SVGScriptElement(const QualifiedName&, Document&, bool wasInsertedByParser, bool alreadyStarted);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;
    virtual void finishedInsertingSubtree() override;
    virtual void childrenChanged(const ChildChange&) override;

    virtual void svgAttributeChanged(const QualifiedName&) override;
    virtual bool isURLAttribute(const Attribute&) const override;
    virtual void finishParsingChildren() override;

    virtual void addSubresourceAttributeURLs(ListHashSet<URL>&) const override;

    virtual bool haveLoadedRequiredResources() override { return SVGExternalResourcesRequired::haveLoadedRequiredResources(); }

    virtual String sourceAttributeValue() const override;
    virtual String charsetAttributeValue() const override;
    virtual String typeAttributeValue() const override;
    virtual String languageAttributeValue() const override;
    virtual String forAttributeValue() const override;
    virtual String eventAttributeValue() const override;
    virtual bool asyncAttributeValue() const override;
    virtual bool deferAttributeValue() const override;
    virtual bool hasSourceAttribute() const override;

    virtual void dispatchLoadEvent() override { SVGExternalResourcesRequired::dispatchLoadEvent(this); }

    virtual Ref<Element> cloneElementWithoutAttributesAndChildren(Document&) override;
    virtual bool rendererIsNeeded(const RenderStyle&) override { return false; }

    // SVGExternalResourcesRequired
    virtual void setHaveFiredLoadEvent(bool haveFiredLoadEvent) override { ScriptElement::setHaveFiredLoadEvent(haveFiredLoadEvent); }
    virtual bool isParserInserted() const override { return ScriptElement::isParserInserted(); }
    virtual bool haveFiredLoadEvent() const override { return ScriptElement::haveFiredLoadEvent(); }
    virtual Timer* svgLoadEventTimer() override { return &m_svgLoadEventTimer; }

#ifndef NDEBUG
    virtual bool filterOutAnimatableAttribute(const QualifiedName&) const override;
#endif

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGScriptElement)
        DECLARE_ANIMATED_STRING_OVERRIDE(Href, href)
        DECLARE_ANIMATED_BOOLEAN_OVERRIDE(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    Timer m_svgLoadEventTimer;
};

} // namespace WebCore

#endif
