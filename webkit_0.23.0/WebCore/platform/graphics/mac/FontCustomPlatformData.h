/*
 * Copyright (C) 2007 Apple Inc.
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

#include "TextFlags.h"
#include <CoreFoundation/CFBase.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RetainPtr.h>

typedef struct CGFont* CGFontRef;
typedef const struct __CTFontDescriptor* CTFontDescriptorRef;

namespace WebCore {

class FontDescription;
class FontFeatureSettings;
class FontPlatformData;
class SharedBuffer;

struct FontCustomPlatformData {
    WTF_MAKE_NONCOPYABLE(FontCustomPlatformData);
public:
#if CORETEXT_WEB_FONTS
    explicit FontCustomPlatformData(CTFontDescriptorRef fontDescriptor)
        : m_fontDescriptor(fontDescriptor)
#else
    explicit FontCustomPlatformData(CGFontRef cgFont)
        : m_cgFont(cgFont)
#endif
    {
    }

    ~FontCustomPlatformData();

    FontPlatformData fontPlatformData(const FontDescription&, bool bold, bool italic, const FontFeatureSettings& fontFaceFeatures, const FontVariantSettings& fontFaceVariantSettings);

    static bool supportsFormat(const String&);

#if CORETEXT_WEB_FONTS
    RetainPtr<CTFontDescriptorRef> m_fontDescriptor;
#else
    RetainPtr<CGFontRef> m_cgFont;
#endif
};

std::unique_ptr<FontCustomPlatformData> createFontCustomPlatformData(SharedBuffer&);

}

#endif
