/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2007 Pioneer Research Center USA, Inc.
 * Copyright (C) 2010 Igalia S.L.
 * All rights reserved.
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

#ifndef FontPlatformDataFreeType_h
#define FontPlatformDataFreeType_h

#include "FontCache.h"
#include "FontDescription.h"
#include "GlyphBuffer.h"
#include "HarfBuzzFace.h"
#include "OpenTypeVerticalData.h"
#include "RefPtrCairo.h"
#include "SharedBuffer.h"
#include "TextFlags.h"
#include <wtf/Forward.h>
#include <wtf/HashFunctions.h>

typedef struct _FcFontSet FcFontSet;
class HarfBuzzFace;

namespace WebCore {

class FontPlatformData {
public:
    FontPlatformData(WTF::HashTableDeletedValueType)
        : m_fallbacks(0)
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_scaledFont(hashTableDeletedFontValue())
        , m_orientation(Horizontal)
        { }

    FontPlatformData()
        : m_fallbacks(0)
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_scaledFont(0)
        , m_orientation(Horizontal)
        { }

    FontPlatformData(FcPattern*, const FontDescription&);
    FontPlatformData(cairo_font_face_t*, const FontDescription&, bool bold, bool italic);
    FontPlatformData(float size, bool bold, bool italic);
    FontPlatformData(const FontPlatformData&);
    FontPlatformData(const FontPlatformData&, float size);

    ~FontPlatformData();

    HarfBuzzFace* harfBuzzFace() const;

    bool isFixedPitch();
    float size() const { return m_size; }
    void setSize(float size) { m_size = size; }
    bool syntheticBold() const { return m_syntheticBold; }
    bool syntheticOblique() const { return m_syntheticOblique; }
    void setSyntheticOblique(bool);
    bool hasCompatibleCharmap();

    FontOrientation orientation() const { return m_orientation; }
    void setOrientation(FontOrientation);
    PassRefPtr<SharedBuffer> openTypeTable(uint32_t table) const;
    PassRefPtr<OpenTypeVerticalData> verticalData() const;

    cairo_scaled_font_t* scaledFont() const { return m_scaledFont; }

    unsigned hash() const
    {
        return PtrHash<cairo_scaled_font_t*>::hash(m_scaledFont);
    }

    bool operator==(const FontPlatformData&) const;
    FontPlatformData& operator=(const FontPlatformData&);
    bool isHashTableDeletedValue() const
    {
        return m_scaledFont == hashTableDeletedFontValue();
    }

#ifndef NDEBUG
    String description() const;
#endif

    RefPtr<FcPattern> m_pattern;
    mutable FcFontSet* m_fallbacks; // Initialized lazily.
    float m_size;
    bool m_syntheticBold;
    bool m_syntheticOblique;
    bool m_fixedWidth;
    cairo_scaled_font_t* m_scaledFont;
    mutable RefPtr<HarfBuzzFace> m_harfBuzzFace;

private:
    void buildScaledFont(cairo_font_face_t*);
    static cairo_scaled_font_t* hashTableDeletedFontValue() { return reinterpret_cast<cairo_scaled_font_t*>(-1); }

    FontOrientation m_orientation;
};

}

#endif // FontPlatformDataFreeType_h
