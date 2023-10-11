/*
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (c) 2010-2020 ACCESS CO., LTD. All rights reserved.
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
#include "FontCache.h"

#include "Font.h"
#include "FontPlatformDataWKC.h"
#include <wtf/Assertions.h>

namespace WebCore {

void FontCache::platformInit()
{
}

bool FontCache::isSystemFontForbiddenForEditing(const String&)
{
    return false;
}

Vector<FontSelectionCapabilities> FontCache::getFontSelectionCapabilitiesInFamily(const AtomString&, AllowUserInstalledFonts)
{
    return { };
}

RefPtr<Font> FontCache::systemFallbackForCharacters(const FontDescription& desc, const Font* originalFontData, IsForPlatformFont isPlatformFont, PreferColoredFont preferColoredFont, const UChar* characters, unsigned int length)
{
    ASSERT(characters && (length==1||length==2));

    return lastResortFallbackFont(desc);
}

Ref<Font> FontCache::lastResortFallbackFont(const FontDescription& fontDescription)
{
    static NeverDestroyed<AtomString> name("systemfont", AtomString::ConstructFromLiteral);
    if (name.isNull())
        name.construct("systemfont", AtomString::ConstructFromLiteral);
    if (RefPtr<Font> font = fontForFamily(fontDescription, name))
        return *font;

    RELEASE_ASSERT_NOT_REACHED();
}

std::unique_ptr<FontPlatformData> FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomString& family, const FontFeatureSettings* fontFaceFeatures, FontSelectionSpecifiedCapabilities fontFaceCapabilities)
{
    FontPlatformData* ret = new FontPlatformData(fontDescription, family);

    if (!ret || !ret->font() || !ret->font()->font()) {
        delete ret;
        return nullptr;
    }

    return std::unique_ptr<FontPlatformData>(ret);
}

RefPtr<Font>
Font::platformCreateScaledFont(const FontDescription& orig, float scaleFactor) const
{
    FontDescription d(orig);
    d.setComputedSize(orig.computedSize() * scaleFactor);
    FontPlatformData* ret = new FontPlatformData(d, m_platformData.font()->familyName().data());

    if (!ret || !ret->font() || !ret->font()->font()) {
        delete ret;
        return adoptRef((Font *)0);
    }

    return Font::create(*ret);
}

Vector<String> FontCache::systemFontFamilies()
{
    // FIXME: <https://webkit.org/b/147033> Web Inspector: [WKC] Allow inspector to retrieve a list of system fonts
    Vector<String> fontFamilies;
    return fontFamilies;
}

const AtomString& FontCache::platformAlternateFamilyName(const AtomString&)
{
    return nullAtom();
}

}
