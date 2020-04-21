/*
 * Copyright (c) 2011, 2012, 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_VIEWPORTARGUMENTS_H_
#define _WKC_HELPERS_WKC_VIEWPORTARGUMENTS_H_

#include <wkc/wkcbase.h>

namespace WKC {

struct ViewportArguments {
    enum {
        ValueAuto = -1,
        ValueDeviceWidth = -2,
        ValueDeviceHeight = -3,
        ValuePortrait = -4,
        ValueLandscape = -5
    };

    ViewportArguments()
        : zoom(ValueAuto)
        , minZoom(ValueAuto)
        , maxZoom(ValueAuto)
        , width(ValueAuto)
        , height(ValueAuto)
        , orientation(ValueAuto)
        , userZoom(ValueAuto)
    {
    }

    float zoom;
    float minZoom;
    float maxZoom;
    float width;
    float height;
    float orientation;
    float userZoom;

    bool operator==(const ViewportArguments& other) const
    {
        return zoom == other.zoom
            && minZoom == other.minZoom
            && maxZoom == other.maxZoom
            && width == other.width
            && height == other.height
            && orientation == other.orientation
            && userZoom == other.userZoom;
    }
};

} // namespace

#endif // _WKC_HELPERS_WKC_VIEWPORTARGUMENTS_H_

