/*
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGTextPathElement_h
#define SVGTextPathElement_h

#include "SVGNames.h"
#include "SVGTextContentElement.h"
#include "SVGURIReference.h"

namespace WebCore {

enum SVGTextPathMethodType {
    SVGTextPathMethodUnknown = 0,
    SVGTextPathMethodAlign,
    SVGTextPathMethodStretch
};

enum SVGTextPathSpacingType {
    SVGTextPathSpacingUnknown = 0,
    SVGTextPathSpacingAuto,
    SVGTextPathSpacingExact
};

template<>
struct SVGPropertyTraits<SVGTextPathMethodType> {
    static unsigned highestEnumValue() { return SVGTextPathMethodStretch; }

    static String toString(SVGTextPathMethodType type)
    {
        switch (type) {
        case SVGTextPathMethodUnknown:
            return emptyString();
        case SVGTextPathMethodAlign:
            return "align";
        case SVGTextPathMethodStretch:
            return "stretch";
        }
    
        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static SVGTextPathMethodType fromString(const String& value)
    {
        if (value == "align")
            return SVGTextPathMethodAlign;
        if (value == "stretch")
            return SVGTextPathMethodStretch;
        return SVGTextPathMethodUnknown;
    }
};

template<>
struct SVGPropertyTraits<SVGTextPathSpacingType> {
    static unsigned highestEnumValue() { return SVGTextPathSpacingExact; }

    static String toString(SVGTextPathSpacingType type)
    {
        switch (type) {
        case SVGTextPathSpacingUnknown:
            return emptyString();
        case SVGTextPathSpacingAuto:
            return "auto";
        case SVGTextPathSpacingExact:
            return "exact";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static SVGTextPathSpacingType fromString(const String& value)
    {
        if (value == "auto")
            return SVGTextPathSpacingAuto;
        if (value == "exact")
            return SVGTextPathSpacingExact;
        return SVGTextPathSpacingUnknown;
    }
};

class SVGTextPathElement final : public SVGTextContentElement,
                                 public SVGURIReference {
public:
    // Forward declare enumerations in the W3C naming scheme, for IDL generation.
    enum {
        TEXTPATH_METHODTYPE_UNKNOWN = SVGTextPathMethodUnknown,
        TEXTPATH_METHODTYPE_ALIGN = SVGTextPathMethodAlign,
        TEXTPATH_METHODTYPE_STRETCH = SVGTextPathMethodStretch,
        TEXTPATH_SPACINGTYPE_UNKNOWN = SVGTextPathSpacingUnknown,
        TEXTPATH_SPACINGTYPE_AUTO = SVGTextPathSpacingAuto,
        TEXTPATH_SPACINGTYPE_EXACT = SVGTextPathSpacingExact
    };

    static Ref<SVGTextPathElement> create(const QualifiedName&, Document&);

protected:
    virtual void finishedInsertingSubtree() override;

private:
    SVGTextPathElement(const QualifiedName&, Document&);

    virtual ~SVGTextPathElement();

    void clearResourceReferences();

    virtual void buildPendingResource() override;
    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;
    virtual void removedFrom(ContainerNode&) override;

    static bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual void svgAttributeChanged(const QualifiedName&) override;

    virtual RenderPtr<RenderElement> createElementRenderer(Ref<RenderStyle>&&, const RenderTreePosition&) override;
    virtual bool childShouldCreateRenderer(const Node&) const override;
    virtual bool rendererIsNeeded(const RenderStyle&) override;

    virtual bool selfHasRelativeLengths() const override;
 
    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGTextPathElement)
        DECLARE_ANIMATED_LENGTH(StartOffset, startOffset)
        DECLARE_ANIMATED_ENUMERATION(Method, method, SVGTextPathMethodType)
        DECLARE_ANIMATED_ENUMERATION(Spacing, spacing, SVGTextPathSpacingType)
        DECLARE_ANIMATED_STRING_OVERRIDE(Href, href)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif
