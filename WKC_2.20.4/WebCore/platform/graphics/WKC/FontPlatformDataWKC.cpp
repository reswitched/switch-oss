/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "FontPlatformData.h"
#include "FontPlatformDataWKC.h"

#include "CString.h"
#include "FontDescription.h"
#include "WTFString.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>

static bool gEnableScalingMonosizeFont = false;

namespace WebCore {

void
FontPlatformData_enableScalingMonosizeFont(bool flag)
{
    gEnableScalingMonosizeFont = flag;
}

static WKC::WKCFontInfo*
_createWKCFont(const FontDescription& in_desc, const char* in_family)
{
    float size = floor(in_desc.computedSize()+0.5);
    int weight = WKC_FONT_WEIGHT_NORMAL;
    int family = WKC_FONT_FAMILY_NONE;
    bool italic = isItalic(in_desc.italic());

#if 0 // This seems to be not used in font peer.
    if (in_desc.firstFamily()==standardFamily) {
        family = WKC_FONT_FAMILY_STANDARD;
    } else if (in_desc.firstFamily()==serifFamily) {
        family = WKC_FONT_FAMILY_SERIF;
    } else if (in_desc.firstFamily()==sansSerifFamily) {
        family = WKC_FONT_FAMILY_SANSSERIF;
    } else if (in_desc.firstFamily()==pictographFamily) {
        family = WKC_FONT_FAMILY_PICTOGRAPH;
    } else if (in_desc.firstFamily()==monospaceFamily) {
        family = WKC_FONT_FAMILY_MONOSPACE;
    } else if (in_desc.firstFamily()==fantasyFamily) {
        family = WKC_FONT_FAMILY_FANTASY;
    } else if (in_desc.firstFamily()==cursiveFamily) {
        family = WKC_FONT_FAMILY_CURSIVE;
    } else {
        family = WKC_FONT_FAMILY_NONE;
    }
#endif

    auto fontWeight = in_desc.weight();

    if (fontWeight < FontSelectionValue(50))
        weight = WKC_FONT_WEIGHT_NORMAL;
    else if (fontWeight < FontSelectionValue(150))
        weight = WKC_FONT_WEIGHT_100;
    else if (fontWeight < FontSelectionValue(250))
        weight = WKC_FONT_WEIGHT_200;
    else if (fontWeight < FontSelectionValue(350))
        weight = WKC_FONT_WEIGHT_300;
    else if (fontWeight < FontSelectionValue(450))
        weight = WKC_FONT_WEIGHT_400;
    else if (fontWeight < FontSelectionValue(550))
        weight = WKC_FONT_WEIGHT_500;
    else if (fontWeight < FontSelectionValue(650))
        weight = WKC_FONT_WEIGHT_600;
    else if (fontWeight < FontSelectionValue(750))
        weight = WKC_FONT_WEIGHT_700;
    else if (fontWeight < FontSelectionValue(850))
        weight = WKC_FONT_WEIGHT_800;
    else if (fontWeight < FontSelectionValue(950))
        weight = WKC_FONT_WEIGHT_900;
    else
        weight = WKC_FONT_WEIGHT_NORMAL;

    return WKC::WKCFontInfo::create(size, weight, italic, in_desc.orientation()==FontOrientation::Horizontal, family, in_family);
}

FontPlatformData::FontPlatformData(const FontDescription& in_desc, const AtomString& in_family)
    : m_orientation(in_desc.orientation())
    , m_size(in_desc.computedSize())
{
    m_font = _createWKCFont(in_desc, in_family.string().utf8().data());
}

FontPlatformData::FontPlatformData(const FontPlatformData& other)
{
    platformDataAssign(other);
}

FontPlatformData& FontPlatformData::operator=(const FontPlatformData& other)
{
    // Check for self-assignment.
    if (this == &other)
        return *this;

    platformDataAssign(other);

    return *this;
}

FontPlatformData::~FontPlatformData()
{
    if (m_font) {
        delete m_font;
    }
}

void
FontPlatformData::platformDataAssign(const FontPlatformData& data)
{
    m_size = data.m_size;
    m_orientation = data.m_orientation;
    m_widthVariant = data.m_widthVariant;
    m_textRenderingMode = data.m_textRenderingMode;

    m_syntheticBold = data.m_syntheticBold;
    m_syntheticOblique = data.m_syntheticOblique;
    m_isColorBitmapFont = data.m_isColorBitmapFont;
    m_isHashTableDeletedValue = data.m_isHashTableDeletedValue;
    m_isSystemFont = data.m_isSystemFont;

    m_isEmoji = data.m_isEmoji;
    m_isSupplemental = data.m_isSupplemental;

    if (!data.m_font) {
        m_font = 0;
    } else if (data.m_isHashTableDeletedValue) {
        m_font = 0;
    } else {
        m_font = WKC::WKCFontInfo::copy(data.m_font);
        m_isEmoji = data.m_isEmoji;
        m_isSupplemental = data.m_isSupplemental;
    }
}

bool
FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    if (!m_font || !other.m_font) {
        return (m_font==other.m_font);
    }
    if (m_isHashTableDeletedValue || other.m_isHashTableDeletedValue) {
        return (m_font==other.m_font);
    }
    if (m_isEmoji != other.m_isEmoji)
        return false;
    if (m_isSupplemental != other.m_isSupplemental)
        return false;
    return m_font->isEqual(other.m_font);
}

unsigned
FontPlatformData::hash() const
{
    if (m_font == 0) {
        return ~0;
    } else if (m_isHashTableDeletedValue) {
        return 0xfffffffe;
    }

    unsigned char buf[384] = {0};
    unsigned char* p = buf;

    const int len = wkcFontSizeOfFontInstancePeer(m_font->font());
    ASSERT(len < sizeof(buf) - sizeof(int)*10);
    if (m_font->font()) {
        ::memcpy(p, m_font->font(), len);
    } else {
        ::memset(p, 0, len);
    }
    p+=len;

    *((int *)p) = m_syntheticBold ? 1 : 0;
    p+=sizeof(int);
    *((int *)p) = m_syntheticOblique ? 1: 0;
    p+=sizeof(int);
    *((int *)p) = m_isHashTableDeletedValue ? 1 : 0;
    p += sizeof(int);
    *((int *)p) = (int)m_orientation;
    p+=sizeof(int);
    ::memcpy(p, &m_size, sizeof(float));
    p+=sizeof(float);
    *((int *)p) = (int)m_widthVariant;
    p+=sizeof(int);
    *((int *)p) = (int)m_isSupplemental ? 1 : 0;
    p+=sizeof(int);
    *((int *)p) = (int)m_isEmoji ? 1 : 0;
    p+=sizeof(int);
    *((int *)p) = m_font->specificUnicodeChar();
    p+=sizeof(int);

    if (m_font->scale()!=1.f) {
        float v = m_font->requestSize();
        ::memcpy(p, &v, sizeof(float));
        p+=sizeof(float);
        *((int *)p) = m_font->weight();
        p+=sizeof(int);
        *((int *)p) = m_font->isItalic() ? 1 : 0;
        p+=sizeof(int);
    }
    return StringHasher::hashMemory(buf, p-buf);
}

#if !LOG_DISABLED
String FontPlatformData::description() const
{
    return String();
}
#endif

}

namespace WKC {

WKCFontInfo::WKCFontInfo(const char* familyName)
  : m_font(0)
  , m_scale(1.f)
  , m_iscale(1.f)
  , m_requestSize(0.f)
  , m_createdSize(0.f)
  , m_weight(0)
  , m_isItalic(false)
  , m_canScale(false)
  , m_ascent(0.f)
  , m_descent(0.f)
  , m_lineSpacing(0.f)
  , m_unicodeChar(0)
  , m_horizontal(true)
  , m_familyName(familyName)
{
}

WKCFontInfo::~WKCFontInfo()
{
    if (m_font)
        wkcFontDeletePeer(m_font);
}

WKCFontInfo*
WKCFontInfo::create(float size, int weight, bool italic, bool horizontal, int family, const char* familyName)
{
    WKCFontInfo* self = 0;
    self = new WKCFontInfo(familyName);
    if (!self->construct(size, weight, italic, horizontal, family)) {
        delete self;
        return 0;
    }
    return self;
}

bool
WKCFontInfo::construct(float size, int weight, bool italic, bool horizontal, int family)
{
    m_font = wkcFontNewPeer((int)size, weight, italic, horizontal, true, family, m_familyName.data());

    m_scale = 1.f;
    m_requestSize = size;
    m_createdSize = m_font ? wkcFontGetSizePeer(m_font) : m_requestSize;
    if (gEnableScalingMonosizeFont && m_createdSize!=m_requestSize) {
        m_scale = (float)m_requestSize / (float)m_createdSize;
    }
    m_iscale = 1.f / m_scale;

    m_weight = weight;
    m_isItalic = italic;
    if (m_font) {
        m_canScale = wkcFontCanScalePeer(m_font);
        m_ascent = (float)wkcFontGetAscentPeer(m_font);
        m_descent = (float)wkcFontGetDescentPeer(m_font);
        m_lineSpacing = (float)wkcFontGetLineSpacingPeer(m_font);
    }
    m_unicodeChar = 0;
    m_horizontal = horizontal;

    return true;
}

WKCFontInfo*
WKCFontInfo::copy(const WKCFontInfo* other)
{
    WKCFontInfo* self = 0;
    self = new WKCFontInfo(other->m_familyName.data());

    self->m_font = other->m_font ? wkcFontNewCopyPeer(other->m_font) : 0;
    self->m_scale = other->m_scale;
    self->m_iscale = other->m_iscale;
    self->m_requestSize = other->m_requestSize;
    self->m_createdSize = other->m_createdSize;
    self->m_weight = other->m_weight;
    self->m_isItalic = other->m_isItalic;
    self->m_canScale = other->m_canScale;
    self->m_ascent = other->m_ascent;
    self->m_descent = other->m_descent;
    self->m_lineSpacing = other->m_lineSpacing;
    self->m_unicodeChar = other->m_unicodeChar;
    self->m_horizontal = other->m_horizontal;

    return self;
}

bool
WKCFontInfo::isEqual(const WKCFontInfo* other)
{
    if (m_scale != other->m_scale ||
        m_iscale != other->m_iscale ||
        m_requestSize != other->m_requestSize ||
        m_createdSize != other->m_createdSize ||
        m_weight != other->m_weight ||
        m_isItalic != other->m_isItalic ||
        m_canScale != other->m_canScale ||
        m_ascent != other->m_ascent ||
        m_descent != other->m_descent ||
        m_lineSpacing != other->m_lineSpacing ||
        m_unicodeChar != other->m_unicodeChar ||
        m_horizontal != other->m_horizontal)
        return false;

    return wkcFontIsEqualPeer(m_font, other->m_font);
}

}
