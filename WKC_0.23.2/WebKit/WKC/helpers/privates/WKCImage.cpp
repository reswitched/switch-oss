/*
 * Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCImage.h"
#include "helpers/privates/WKCImagePrivate.h"

#include "Image.h"

namespace WKC {

ImagePrivate::ImagePrivate(WebCore::Image* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

ImagePrivate::~ImagePrivate()
{
}

bool
ImagePrivate::isNull() const
{
    return m_webcore->isNull();
}

int
ImagePrivate::width() const
{
    return m_webcore->width();
}

int
ImagePrivate::height() const
{
    return m_webcore->height();
}

////////////////////////////////////////////////////////////////////////////////

Image::Image(ImagePrivate& parent)
    : m_private(parent)
{
}

Image::~Image()
{
}

bool
Image::isNull() const
{
    return m_private.isNull();
}

int
Image::width() const
{
    return m_private.width();
}

int
Image::height() const
{
    return m_private.height();
}

} // namespace
