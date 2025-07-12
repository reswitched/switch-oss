/*
 * Copyright (C) 2007 Apple Computer, Kevin Ollivier.  All rights reserved.
 * Copyright (c) 2008, Google Inc. All rights reserved.
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
#include "ImageWKC.h"

#include "BitmapImage.h"

#include <wkc/wkcgpeer.h>

namespace WebCore {

void
BitmapImage::invalidatePlatformData()
{
}

Ref<Image>
Image::loadPlatformResource(const char* name)
{
    const unsigned char* bitmap = 0;
    unsigned int width = 0;
    unsigned int height = 0;

    bitmap = wkcStockImageGetPlatformResourceImagePeer(name, &width, &height);
    if (bitmap) {
        RefPtr<Image> image = PlatformImageHelper::createImage(bitmap, width, height);
        if (!image)
            CRASH_WITH_NO_MEMORY();
        return image.releaseNonNull();
    }
    return BitmapImage::create();
}

float
Image::platformResourceScale()
{
    return wkcStockImageGetImageScalePeer();
}


PlatformImageHelper::PlatformImageHelper(PlatformImagePtr image)
    : m_image(WTFMove(image))
{
    ASSERT(m_image);
}

RefPtr<Image>
PlatformImageHelper::createImage(const unsigned char* bitmap, size_t width, size_t height)
{
#if USE(WKC_CAIRO)
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(const_cast<unsigned char*>(bitmap), CAIRO_FORMAT_ARGB32, width, height, stride));
    if (cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS)
        BitmapImage::create();
    return BitmapImage::create(WTFMove(surface));
#else
    BitmapImage::create();
#endif
}


int
PlatformImageHelper::bpp() const
{
    return 4;
}

size_t
PlatformImageHelper::rowbytes() const
{
#if USE(WKC_CAIRO)
    return cairo_image_surface_get_stride(m_image.get());
#else
    return 0;
#endif
}

unsigned char*
PlatformImageHelper::bitmap() const
{
#if USE(WKC_CAIRO)
    return cairo_image_surface_get_data(m_image.get());
#else
    return nullptr;
#endif
}


}
