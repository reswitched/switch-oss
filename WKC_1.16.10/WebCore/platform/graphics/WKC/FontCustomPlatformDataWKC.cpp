/*
 * Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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

#include "FontCustomPlatformData.h"
#include "FontPlatformDataWKC.h"

#include "CString.h"
#include "FontDescription.h"
#include "SharedBuffer.h"
#include "WOFFFileFormat.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>


namespace WebCore {

FontCustomPlatformData::FontCustomPlatformData(SharedBuffer* buf)
    : m_buffer(buf)
    , m_registeredId(0)
{
    ::memset(m_familyName, 0, sizeof(m_familyName));
    m_buffer->ref();
}

FontCustomPlatformData::~FontCustomPlatformData()
{
    if (m_registeredId>=0)
        wkcFontEngineUnregisterFontPeer(m_registeredId);
    if (m_buffer)
        m_buffer->deref();
}

FontCustomPlatformData*
FontCustomPlatformData::create(SharedBuffer* buf)
{
    FontCustomPlatformData* self = 0;
    self = new FontCustomPlatformData(buf);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
FontCustomPlatformData::construct()
{
    m_registeredId = wkcFontEngineRegisterFontPeer(WKC_FONT_ENGINE_REGISTER_TYPE_MEMORY, reinterpret_cast<const unsigned char *>(m_buffer->data()), m_buffer->size());
    if (m_registeredId<0)
        return false;

    bool ret = wkcFontEngineGetFamilyNamePeer(m_registeredId, m_familyName, sizeof(m_familyName));
    if (!ret)
        return false;

    return true;
}

std::unique_ptr<FontCustomPlatformData>
createFontCustomPlatformData(SharedBuffer& buffer, const String&)
{
    const unsigned char* data = reinterpret_cast<const unsigned char *>(buffer.data());
    const unsigned int len = buffer.size();
    if (!data || !len)
        return nullptr;

    if (!wkcFontEngineCanSupportPeer(data, len))
        return nullptr;

    FontCustomPlatformData* self = 0;
    self = FontCustomPlatformData::create(&buffer);
    return std::unique_ptr<FontCustomPlatformData>(self);
}

FontPlatformData
FontCustomPlatformData::fontPlatformData(const FontDescription& fontDescription, bool bold, bool italic)
{
    FontDescription desc;

    int size = fontDescription.computedPixelSize();
    FontRenderingMode renderingMode = fontDescription.renderingMode();
    FontOrientation orientation = fontDescription.orientation();

    desc.setComputedSize(size);
    desc.setWeight(bold ? boldWeightValue() : normalWeightValue());
    desc.setItalic(italic ? italicValue() : normalItalicValue());
    desc.setRenderingMode(renderingMode);
    desc.setOrientation(orientation);

    FontPlatformData fd(desc, m_familyName);
    if (fd.font() && fd.font()->font())
        return fd;
    else
        return FontPlatformData(desc, "systemfont");
}

bool
FontCustomPlatformData::supportsFormat(const WTF::String& format)
{
#if ENABLE(SVG_FONTS)
    if (format=="svg" || format=="svgz")
        return true;
#endif

    return wkcFontEngineCanSupportByFormatNamePeer(reinterpret_cast<const unsigned char*>(format.utf8().data()), format.utf8().length());
}


} // namespace
