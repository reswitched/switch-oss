/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_FeatureDefinesWKC_h
#define WTF_FeatureDefinesWKC_h

/* Use this file to list _all_ ENABLE() macros. Define the macros to be one of the following values:
 *  - "0" disables the feature by default. The feature can still be enabled for a specific port or environment.
 *  - "1" enables the feature by default. The feature can still be disabled for a specific port or environment.
 *
 * Use this file to define ENABLE() macros only. Do not use this file to define USE() or macros !
 *
 * Keep the file sorted by the name of the defines. As an exception you can change the order
 * to allow interdependencies between the default values.
 *
 * Below are a few potential commands to take advantage of this file running from the Source/WTF directory
 *
 * Get the list of feature defines: grep -o "ENABLE_\(\w\+\)" wtf/FeatureDefines.h | sort | uniq
 * Get the list of features enabled by default for a PLATFORM(XXX): gcc -E -dM -I. -DWTF_PLATFORM_XXX "wtf/Platform.h" | grep "ENABLE_\w\+ 1" | cut -d' ' -f2 | sort
 */

#define ENABLE_3D_TRANSFORMS 1
#define ENABLE_ACCELERATED_2D_CANVAS 0
#define ENABLE_ACCELERATED_OVERFLOW_SCROLLING 0
#define ENABLE_ALLOCATION_LOGGING 0
#define ENABLE_APNG 1
#define ENABLE_ASYNC_SCROLLING 0
#define ENABLE_CHANNEL_MESSAGING 1
#define ENABLE_COMPUTED_GOTO_OPCODES 0
#define ENABLE_CONTENT_EXTENSIONS 0
#define ENABLE_CONTEXT_MENUS 0
#define ENABLE_CSS3_TEXT 1
#define ENABLE_CSS_BOX_DECORATION_BREAK 1
#define ENABLE_CSS_COMPOSITING 1
#define ENABLE_CSS_DEVICE_ADAPTATION 1
#define ENABLE_CSS_IMAGE_ORIENTATION 0
#define ENABLE_CSS_IMAGE_RESOLUTION 0
#define ENABLE_CSS_SCROLL_SNAP 0
#define ENABLE_CSS_SELECTOR_JIT 0
#define ENABLE_CSS_TRAILING_WORD 0
#define ENABLE_CURSOR_SUPPORT 1
#define ENABLE_CUSTOM_SCHEME_HANDLER 0
#define ENABLE_DASHBOARD_SUPPORT 0
#define ENABLE_DATACUE_VALUE 0
#define ENABLE_DATALIST_ELEMENT 0
#define ENABLE_DATA_INTERACTION 0
#define ENABLE_DATE_AND_TIME_INPUT_TYPES 0
#define ENABLE_DEVICE_ORIENTATION 1
#define ENABLE_DFG_JIT 0
#define ENABLE_DOWNLOAD_ATTRIBUTE 1
#define ENABLE_DRAG_SUPPORT 1
#define ENABLE_ENCRYPTED_MEDIA 0
#define ENABLE_FILTERS_LEVEL_2 0
#define ENABLE_FTL_JIT 0
#define ENABLE_FTPDIR 0
#define ENABLE_FULLSCREEN_API 1
#define ENABLE_GAMEPAD 1
#define ENABLE_GEOLOCATION 0
#define ENABLE_GRAPHICS_CONTEXT_3D 0
#define ENABLE_INDEXED_DATABASE 0
#define ENABLE_INDEXED_DATABASE_IN_WORKERS 0
#define ENABLE_INPUT_TYPE_COLOR 1
#define ENABLE_INPUT_TYPE_COLOR_POPOVER 0
#define ENABLE_INPUT_TYPE_DATE 0
#define ENABLE_INPUT_TYPE_DATETIMELOCAL 0
#define ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE 0 // see http://lists.webkit.org/pipermail/webkit-dev/2013-January/023404.html
#define ENABLE_INPUT_TYPE_MONTH 0
#define ENABLE_INPUT_TYPE_TIME 0
#define ENABLE_INPUT_TYPE_WEEK 0
#define ENABLE_INSPECTOR_ALTERNATE_DISPATCHERS 0
// NOTE: ENABLE_INTL is temporary disabled. If enable it, we need to support ICU basically build (data, common, and i18n)
#define ENABLE_INTL 0
#define ENABLE_INTL_NUMBER_FORMAT_TO_PARTS 0
#define ENABLE_INTL_PLURAL_RULES 0
#define ENABLE_JAVASCRIPT_I18N_API 0
#define ENABLE_JIT 0
#define ENABLE_KEYBOARD_CODE_ATTRIBUTE 0
#define ENABLE_KEYBOARD_KEY_ATTRIBUTE 0
#define ENABLE_LAYOUT_FORMATTING_CONTEXT 0
#define ENABLE_LEGACY_CSS_VENDOR_PREFIXES 0
#define ENABLE_LEGACY_ENCRYPTED_MEDIA 0
#define ENABLE_LETTERPRESS 0
#define ENABLE_MASM_PROBE 0
#define ENABLE_MATHML 0
#define ENABLE_MEDIA_CAPTURE 0
#define ENABLE_MEDIA_CONTROLS_SCRIPT 1
#define ENABLE_MEDIA_SOURCE 1
#define ENABLE_MEDIA_STATISTICS 0
#define ENABLE_MEDIA_STREAM 0
#define ENABLE_METER_ELEMENT 1
#define ENABLE_MHTML 0
#define ENABLE_MOUSE_CURSOR_SCALE 0
#define ENABLE_MOUSE_FORCE_EVENTS 0
#define ENABLE_NAVIGATOR_CONTENT_UTILS 0
#define ENABLE_NETSCAPE_PLUGIN_API 0
#define ENABLE_NOTIFICATIONS 0
#define ENABLE_OPCODE_STATS 0
#define ENABLE_OPENTYPE_VERTICAL 0
#define ENABLE_ORIENTATION_EVENTS 0
#define ENABLE_PAN_SCROLLING 0
#define ENABLE_PAYMENT_REQUEST 0
#define ENABLE_POINTER_LOCK 0
#define ENABLE_POISON 0
#define ENABLE_PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC 0
#define ENABLE_QUOTA 0
#define ENABLE_REGEXP_TRACING 0
#define ENABLE_REMOTE_INSPECTOR 1
#define ENABLE_RESOLUTION_MEDIA_QUERY 1
#define ENABLE_RUBBER_BANDING 0
#define ENABLE_SAMPLING_PROFILER 0
#define ENABLE_SERVICE_CONTROLS 0
#define ENABLE_SERVICE_WORKER 0
#define ENABLE_SMOOTH_SCROLLING 0
#define ENABLE_SPEECH_SYNTHESIS 0
#define ENABLE_SPELLCHECK 0
#define ENABLE_STREAMS_API 1
#define ENABLE_SUBTLE_CRYPTO 1
#define ENABLE_SVG_FONTS 1
#define ENABLE_TEXT_AUTOSIZING 0
#define ENABLE_TEXT_CARET 1
#define ENABLE_TEXT_SELECTION 1
#define ENABLE_THREADING_GENERIC 1
#define ENABLE_TOUCH_EVENTS 1
// We enable this even in release build for ease of debugging.
#define ENABLE_TREE_DEBUGGING 1
#define ENABLE_VIDEO 1
#define ENABLE_VIDEO_TRACK 1
#define ENABLE_WEBASSEMBLY 0
#define ENABLE_WEBGL 0
#define ENABLE_WEBGL2 0
// only for Safari
#define ENABLE_WEB_ARCHIVE 0
#define ENABLE_WEB_AUDIO 1
#define ENABLE_WIRELESS_PLAYBACK_TARGET 0
#define ENABLE_WKC_COMPOSITED_FIXED_ELEMENTS 1
#define ENABLE_WKC_DEVICE_MODE_CSS_MEDIA 1
#define ENABLE_WKC_FORCE_FIXED_ELEMENTS_NONFIXED_LAYOUT 1
#define ENABLE_WKC_HTTPCACHE 1
#define ENABLE_WKC_PERFORMANCE_MODE_CSS_MEDIA 1
#define ENABLE_WKC_WEB_NFC 0
#define ENABLE_WRITE_BARRIER_PROFILING 0
#define ENABLE_XSLT 0

#endif /* WTF_FeatureDefinesWKC_h */
