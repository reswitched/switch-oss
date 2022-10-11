/*
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Igalia S.L.
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

#ifndef FontCustomPlatformData_h
#define FontCustomPlatformData_h

#if USE(CAIRO)

#include "TextFlags.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

typedef struct FT_FaceRec_*  FT_Face;
typedef struct _cairo_font_face cairo_font_face_t;

namespace WebCore {

class FontDescription;
class FontPlatformData;
class SharedBuffer;

struct FontCustomPlatformData {
    WTF_MAKE_NONCOPYABLE(FontCustomPlatformData);
public:
    FontCustomPlatformData(FT_Face, SharedBuffer&);
    ~FontCustomPlatformData();
    FontPlatformData fontPlatformData(const FontDescription&, bool bold, bool italic);
    static bool supportsFormat(const String&);

private:
    cairo_font_face_t* m_fontFace;
};

std::unique_ptr<FontCustomPlatformData> createFontCustomPlatformData(SharedBuffer&);

} // namespace WebCore

#endif // USE(CAIRO)

#endif // FontCustomPlatformData_h
