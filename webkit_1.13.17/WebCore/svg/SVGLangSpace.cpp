/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGLangSpace.h"

#include "XMLNames.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

void SVGLangSpace::setXmllang(const AtomicString& xmlLang)
{
    m_lang = xmlLang;
}

const AtomicString& SVGLangSpace::xmlspace() const
{
    if (!m_space) {
        static NeverDestroyed<const AtomicString> defaultString("default", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
        if (defaultString.isNull())
            defaultString.construct("default", AtomicString::ConstructFromLiteral);
#endif
        return defaultString;
    }

    return m_space;
}

void SVGLangSpace::setXmlspace(const AtomicString& xmlSpace)
{
    m_space = xmlSpace;
}

void SVGLangSpace::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name.matches(XMLNames::langAttr))
        setXmllang(value);
    if (name.matches(XMLNames::spaceAttr))
        setXmlspace(value);
}

bool SVGLangSpace::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName.matches(XMLNames::langAttr) || attrName.matches(XMLNames::spaceAttr);
}

static void addWithAndWithoutXMLPrefix(HashSet<QualifiedName>& set, const QualifiedName& attributeName)
{
    ASSERT(attributeName.prefix().isNull());
    set.add(attributeName);
    set.add({ xmlAtom(), attributeName.localName(), attributeName.namespaceURI() });
}

void SVGLangSpace::addSupportedAttributes(HashSet<QualifiedName>& set)
{
    addWithAndWithoutXMLPrefix(set, XMLNames::langAttr);
    addWithAndWithoutXMLPrefix(set, XMLNames::spaceAttr);
}

}