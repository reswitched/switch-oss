/*
 * Copyright (C) 2014 Apple Inc.  All rights reserved.
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

#ifndef CoreGraphicsSPI_h
#define CoreGraphicsSPI_h

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

#if USE(APPLE_INTERNAL_SDK)

#include <CoreGraphics/CGFontCache.h>
#include <CoreGraphics/CoreGraphicsPrivate.h>

#else
struct CGFontHMetrics {
    int ascent;
    int descent;
    int lineGap;
    int maxAdvanceWidth;
    int minLeftSideBearing;
    int minRightSideBearing;
};

struct CGFontDescriptor {
    CGRect bbox;
    CGFloat ascent;
    CGFloat descent;
    CGFloat capHeight;
    CGFloat italicAngle;
    CGFloat stemV;
    CGFloat stemH;
    CGFloat avgWidth;
    CGFloat maxWidth;
    CGFloat missingWidth;
    CGFloat leading;
    CGFloat xHeight;
};

typedef const struct CGColorTransform* CGColorTransformRef;

typedef enum {
    kCGCompositeCopy = 1,
    kCGCompositeSover = 2,
} CGCompositeOperation;

enum {
    kCGFontRenderingStyleAntialiasing = 1 << 0,
    kCGFontRenderingStyleSmoothing = 1 << 1,
    kCGFontRenderingStyleSubpixelPositioning = 1 << 2,
    kCGFontRenderingStyleSubpixelQuantization = 1 << 3,
    kCGFontRenderingStylePlatformNative = 1 << 9,
    kCGFontRenderingStyleMask = 0x20F,
};
typedef uint32_t CGFontRenderingStyle;

enum {
    kCGFontAntialiasingStyleUnfiltered = 0 << 7,
    kCGFontAntialiasingStyleFilterLight = 1 << 7,
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101100
    kCGFontAntialiasingStyleUnfilteredCustomDilation = (8 << 7),
#endif
};
typedef uint32_t CGFontAntialiasingStyle;

enum {
    kCGImageCachingTransient = 1,
    kCGImageCachingTemporary = 3,
};
typedef uint32_t CGImageCachingFlags;

#if PLATFORM(COCOA)
typedef struct CGSRegionEnumeratorObject* CGSRegionEnumeratorObj;
typedef struct CGSRegionObject* CGSRegionObj;
typedef struct CGSRegionObject* CGRegionRef;
#endif

#ifdef CGFLOAT_IS_DOUBLE
#define CGRound(value) round((value))
#define CGFloor(value) floor((value))
#define CGCeiling(value) ceil((value))
#define CGFAbs(value) fabs((value))
#else
#define CGRound(value) roundf((value))
#define CGFloor(value) floorf((value))
#define CGCeiling(value) ceilf((value))
#define CGFAbs(value) fabsf((value))
#endif

static inline CGFloat CGFloatMin(CGFloat a, CGFloat b) { return isnan(a) ? b : ((isnan(b) || a < b) ? a : b); }

typedef struct CGFontCache CGFontCache;

#if PLATFORM(MAC)
typedef uint32_t CGSConnectionID;
typedef uint32_t CGSWindowID;
typedef uint32_t CGSWindowCount;
typedef CGSWindowID *CGSWindowIDList;

enum {
    kCGSWindowCaptureNominalResolution = 0x0200,
    kCGSCaptureIgnoreGlobalClipShape = 0x0800,
};
typedef uint32_t CGSWindowCaptureOptions;
#endif

#endif // USE(APPLE_INTERNAL_SDK)

WTF_EXTERN_C_BEGIN

CGColorRef CGColorTransformConvertColor(CGColorTransformRef, CGColorRef, CGColorRenderingIntent);
CGColorTransformRef CGColorTransformCreate(CGColorSpaceRef, CFDictionaryRef attributes);

CGAffineTransform CGContextGetBaseCTM(CGContextRef);
CGCompositeOperation CGContextGetCompositeOperation(CGContextRef);
CGColorRef CGContextGetFillColorAsColor(CGContextRef);
CGFloat CGContextGetLineWidth(CGContextRef);
bool CGContextGetShouldSmoothFonts(CGContextRef);
void CGContextSetBaseCTM(CGContextRef, CGAffineTransform);
void CGContextSetCTM(CGContextRef, CGAffineTransform);
void CGContextSetCompositeOperation(CGContextRef, CGCompositeOperation);
void CGContextSetShouldAntialiasFonts(CGContextRef, bool shouldAntialiasFonts);
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101100
void CGContextSetFontDilation(CGContextRef, CGSize);
void CGContextSetFontRenderingStyle(CGContextRef, CGFontRenderingStyle);
#endif

CFStringRef CGFontCopyFamilyName(CGFontRef);
bool CGFontGetDescriptor(CGFontRef, CGFontDescriptor*);
bool CGFontGetGlyphAdvancesForStyle(CGFontRef, const CGAffineTransform* , CGFontRenderingStyle, const CGGlyph[], size_t count, CGSize advances[]);
void CGFontGetGlyphsForUnichars(CGFontRef, const UniChar[], CGGlyph[], size_t count);
const CGFontHMetrics* CGFontGetHMetrics(CGFontRef);
const char* CGFontGetPostScriptName(CGFontRef);
bool CGFontIsFixedPitch(CGFontRef);
void CGFontSetShouldUseMulticache(bool);

void CGImageSetCachingFlags(CGImageRef, CGImageCachingFlags);
CGImageCachingFlags CGImageGetCachingFlags(CGImageRef);

CGDataProviderRef CGPDFDocumentGetDataProvider(CGPDFDocumentRef);

CGFontAntialiasingStyle CGContextGetFontAntialiasingStyle(CGContextRef);
void CGContextSetFontAntialiasingStyle(CGContextRef, CGFontAntialiasingStyle);

#if PLATFORM(COCOA)
CGSRegionEnumeratorObj CGSRegionEnumerator(CGRegionRef);
CGRect* CGSNextRect(const CGSRegionEnumeratorObj);
CGError CGSReleaseRegionEnumerator(const CGSRegionEnumeratorObj);
#endif

#if PLATFORM(WIN)
CGFontCache* CGFontCacheGetLocalCache();
void CGFontCacheSetShouldAutoExpire(CGFontCache*, bool);
void CGFontCacheSetMaxSize(CGFontCache*, size_t);
#endif

#if PLATFORM(MAC)
CGSConnectionID CGSMainConnectionID(void);
CFArrayRef CGSHWCaptureWindowList(CGSConnectionID cid, CGSWindowIDList windowList, CGSWindowCount windowCount, CGSWindowCaptureOptions options);
CGError CGSSetConnectionProperty(CGSConnectionID, CGSConnectionID ownerCid, CFStringRef key, CFTypeRef value);
CGError CGSCopyConnectionProperty(CGSConnectionID, CGSConnectionID ownerCid, CFStringRef key, CFTypeRef *value);
#endif

WTF_EXTERN_C_END

#endif // CoreGraphicsSPI_h
