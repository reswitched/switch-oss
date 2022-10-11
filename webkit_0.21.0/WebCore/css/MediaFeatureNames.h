/*
 * Copyright (C) 2005, 2013 Apple Inc. All rights reserved.
 * Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
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
#ifndef MediaFeatureNames_h
#define MediaFeatureNames_h

#include <wtf/text/AtomicString.h>

namespace WebCore {
    namespace MediaFeatureNames {

#if ENABLE(VIEW_MODE_CSS_MEDIA)
#define CSS_MEDIAQUERY_VIEW_MODE(macro) macro(view_mode, "-webkit-view-mode")
#else
#define CSS_MEDIAQUERY_VIEW_MODE(macro)
#endif

#if ENABLE(WKC_DEVICE_MODE_CSS_MEDIA)
#define CSS_MEDIAQUERY_NINTENDO_SWITCH_DEVICE_MODE(macro) macro(nintendo_switch_device_mode, "-webkit-nintendo-switch-device-mode")
#else
#define CSS_MEDIAQUERY_NINTENDO_SWITCH_DEVICE_MODE(macro)
#endif

#if ENABLE(WKC_PERFORMANCE_MODE_CSS_MEDIA)
#define CSS_MEDIAQUERY_NINTENDO_SWITCH_PERFORMANCE_MODE(macro) macro(nintendo_switch_performance_mode, "-webkit-nintendo-switch-performance-mode")
#else
#define CSS_MEDIAQUERY_NINTENDO_SWITCH_PERFORMANCE_MODE(macro)
#endif

#define CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(macro) \
    macro(any_hover, "any-hover") \
    macro(any_pointer, "any-pointer") \
    macro(color, "color") \
    macro(color_index, "color-index") \
    macro(grid, "grid") \
    macro(monochrome, "monochrome") \
    macro(height, "height") \
    macro(hover, "hover") \
    macro(width, "width") \
    macro(orientation, "orientation") \
    macro(aspect_ratio, "aspect-ratio") \
    macro(device_aspect_ratio, "device-aspect-ratio") \
    macro(device_pixel_ratio, "-webkit-device-pixel-ratio") \
    macro(device_height, "device-height") \
    macro(device_width, "device-width") \
    macro(inverted_colors, "inverted-colors") \
    macro(max_color, "max-color") \
    macro(max_color_index, "max-color-index") \
    macro(max_aspect_ratio, "max-aspect-ratio") \
    macro(max_device_aspect_ratio, "max-device-aspect-ratio") \
    macro(max_device_pixel_ratio, "-webkit-max-device-pixel-ratio") \
    macro(max_device_height, "max-device-height") \
    macro(max_device_width, "max-device-width") \
    macro(max_height, "max-height") \
    macro(max_monochrome, "max-monochrome") \
    macro(max_width, "max-width") \
    macro(max_resolution, "max-resolution") \
    macro(min_color, "min-color") \
    macro(min_color_index, "min-color-index") \
    macro(min_aspect_ratio, "min-aspect-ratio") \
    macro(min_device_aspect_ratio, "min-device-aspect-ratio") \
    macro(min_device_pixel_ratio, "-webkit-min-device-pixel-ratio") \
    macro(min_device_height, "min-device-height") \
    macro(min_device_width, "min-device-width") \
    macro(min_height, "min-height") \
    macro(min_monochrome, "min-monochrome") \
    macro(min_width, "min-width") \
    macro(min_resolution, "min-resolution") \
    macro(pointer, "pointer") \
    macro(resolution, "resolution") \
    macro(transform_2d, "-webkit-transform-2d") \
    macro(transform_3d, "-webkit-transform-3d") \
    macro(transition, "-webkit-transition") \
    macro(animation, "-webkit-animation") \
    macro(video_playable_inline, "-webkit-video-playable-inline") \
    CSS_MEDIAQUERY_VIEW_MODE(macro) \
    CSS_MEDIAQUERY_NINTENDO_SWITCH_DEVICE_MODE(macro) \
    CSS_MEDIAQUERY_NINTENDO_SWITCH_PERFORMANCE_MODE(macro)

// end of macro

#ifndef CSS_MEDIAQUERY_NAMES_HIDE_GLOBALS
    #define CSS_MEDIAQUERY_NAMES_DECLARE(name, str) extern const AtomicString name##MediaFeature;
    CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(CSS_MEDIAQUERY_NAMES_DECLARE)
    #undef CSS_MEDIAQUERY_NAMES_DECLARE
#endif

        void init();

    } // namespace MediaFeatureNames
} // namespace WebCore

#endif // MediaFeatureNames_h
