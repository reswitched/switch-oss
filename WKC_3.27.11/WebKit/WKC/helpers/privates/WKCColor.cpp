/*
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "Color.h"
#include "helpers/WKCColor.h"

#define PARENT() ((WebCore::Color *)m_parent)

namespace WKC {

Color::Color(ColorPrivate* parent)
    : m_parent((ColorPrivate *)new WebCore::Color())
{
    // copy struct
    *PARENT() = *(WebCore::Color *)parent;
}

Color::Color(const Color& color)
    : m_parent((ColorPrivate *)new WebCore::Color())
{
    // copy struct
    *PARENT() = *(WebCore::Color *)color.parent();
}

Color::Color(Color&& color)
    : m_parent((ColorPrivate *)new WebCore::Color())
{
    // move struct
    *PARENT() = WTFMove(*(WebCore::Color *)color.parent());
}

Color::Color()
    : m_parent((ColorPrivate *)new WebCore::Color())
{
}

Color::~Color()
{
    delete (WebCore::Color *)m_parent;
}

Color&
Color::operator=(const Color& orig)
{
    if (this!=&orig) {
        // copy struct
        *PARENT() = *(WebCore::Color *)orig.parent();
    }
    return *this;
}

Color&
Color::operator=(Color&& orig)
{
    // move struct
    *PARENT() = WTFMove(*(WebCore::Color *)orig.parent());
    return *this;
}

bool
Color::isValid() const
{
    return PARENT()->isValid();
}

bool
Color::isOpaque() const
{
    return PARENT()->isOpaque();
}

int
Color::red() const
{
    return (WebCore::PackedColor::RGBA(PARENT()->toColorTypeLossy<WebCore::SRGBA<uint8_t>>()).value >> 24) & 0xFF;
}

int
Color::green() const
{
    return (WebCore::PackedColor::RGBA(PARENT()->toColorTypeLossy<WebCore::SRGBA<uint8_t>>()).value >> 16) & 0xFF;
}

int
Color::blue() const
{
    return (WebCore::PackedColor::RGBA(PARENT()->toColorTypeLossy<WebCore::SRGBA<uint8_t>>()).value >> 8) & 0xFF;
}

int
Color::alpha() const
{
    return WebCore::PackedColor::RGBA(PARENT()->toColorTypeLossy<WebCore::SRGBA<uint8_t>>()).value & 0xFF;
}

RGBA32
Color::rgba() const
{
    return WebCore::PackedColor::RGBA(PARENT()->toColorTypeLossy<WebCore::SRGBA<uint8_t>>()).value;
}

ARGB32
Color::argb() const
{
    return WebCore::PackedColor::ARGB(PARENT()->toColorTypeLossy<WebCore::SRGBA<uint8_t>>()).value;
}

} // namespace
