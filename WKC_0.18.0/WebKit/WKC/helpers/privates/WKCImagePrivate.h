/*
 * Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_IMAGE_H_
#define _WKC_HELPERS_PRIVATE_IMAGE_H_

#include "helpers/WKCImage.h"

namespace WebCore { 
class Image;
} // namespace

namespace WKC {

class ImageWrap : public Image {
WTF_MAKE_FAST_ALLOCATED;
public:
    ImageWrap(ImagePrivate& parent) : Image(parent) {}
    ~ImageWrap() {}
};

class ImagePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ImagePrivate(WebCore::Image*);
    ~ImagePrivate();

    WebCore::Image* webcore() const { return m_webcore; }
    Image& wkc() { return m_wkc; }

    bool isNull() const;

    int width() const;
    int height() const;

private:
    WebCore::Image* m_webcore;
    ImageWrap m_wkc;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_IMAGE_H_
