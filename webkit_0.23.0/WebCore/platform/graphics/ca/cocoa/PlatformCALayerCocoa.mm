/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#import "PlatformCALayerCocoa.h"

#import "AnimationUtilities.h"
#import "BlockExceptions.h"
#import "FontAntialiasingStateSaver.h"
#import "GraphicsContext.h"
#import "GraphicsLayerCA.h"
#import "LengthFunctions.h"
#import "PlatformCAAnimationCocoa.h"
#import "PlatformCAFilters.h"
#import "QuartzCoreSPI.h"
#import "ScrollbarThemeMac.h"
#import "SoftLinking.h"
#import "TileController.h"
#import "TiledBacking.h"
#import "WebActionDisablingCALayerDelegate.h"
#import "WebCoreCALayerExtras.h"
#import "WebGLLayer.h"
#import "WebLayer.h"
#import "WebSystemBackdropLayer.h"
#import "WebTiledBackingLayer.h"
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/objc-auto.h>
#import <objc/runtime.h>
#import <wtf/CurrentTime.h>
#import <wtf/RetainPtr.h>

#if PLATFORM(IOS)
#import "WAKWindow.h"
#import "WKGraphics.h"
#import "WebCoreThread.h"
#import "WebTiledLayer.h"
#else
#import "ThemeMac.h"
#endif

#if ENABLE(FILTERS_LEVEL_2)
@interface CABackdropLayer : CALayer
@property BOOL windowServerAware;
@end
#endif

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

SOFT_LINK_CLASS(AVFoundation, AVPlayerLayer)

using namespace WebCore;

PassRefPtr<PlatformCALayer> PlatformCALayerCocoa::create(LayerType layerType, PlatformCALayerClient* owner)
{
    return adoptRef(new PlatformCALayerCocoa(layerType, owner));
}

PassRefPtr<PlatformCALayer> PlatformCALayerCocoa::create(void* platformLayer, PlatformCALayerClient* owner)
{
    return adoptRef(new PlatformCALayerCocoa(static_cast<PlatformLayer*>(platformLayer), owner));
}

static NSString * const platformCALayerPointer = @"WKPlatformCALayer";
PlatformCALayer* PlatformCALayer::platformCALayer(void* platformLayer)
{
    if (!platformLayer)
        return 0;

    // Pointer to PlatformCALayer is kept in a key of the CALayer
    PlatformCALayer* platformCALayer = nil;
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    platformCALayer = static_cast<PlatformCALayer*>([[static_cast<CALayer*>(platformLayer) valueForKey:platformCALayerPointer] pointerValue]);
    END_BLOCK_OBJC_EXCEPTIONS
    return platformCALayer;
}

static double mediaTimeToCurrentTime(CFTimeInterval t)
{
    return monotonicallyIncreasingTime() + t - CACurrentMediaTime();
}

// Delegate for animationDidStart callback
@interface WebAnimationDelegate : NSObject {
    PlatformCALayer* m_owner;
}

- (void)animationDidStart:(CAAnimation *)anim;
- (void)setOwner:(PlatformCALayer*)owner;

@end

@implementation WebAnimationDelegate

- (void)animationDidStart:(CAAnimation *)animation
{
#if PLATFORM(IOS)
    WebThreadLock();
#endif
    if (!m_owner)
        return;

    CFTimeInterval startTime;
    if (hasExplicitBeginTime(animation)) {
        // We don't know what time CA used to commit the animation, so just use the current time
        // (even though this will be slightly off).
        startTime = mediaTimeToCurrentTime(CACurrentMediaTime());
    } else
        startTime = mediaTimeToCurrentTime([animation beginTime]);

    CALayer *layer = m_owner->platformLayer();

    String animationKey;
    for (NSString *key in [layer animationKeys]) {
        if ([layer animationForKey:key] == animation) {
            animationKey = key;
            break;
        }
    }

    if (!animationKey.isEmpty())
        m_owner->animationStarted(animationKey, startTime);
}

- (void)animationDidStop:(CAAnimation *)animation finished:(BOOL)finished
{
#if PLATFORM(IOS)
    WebThreadLock();
#endif
    UNUSED_PARAM(finished);

    if (!m_owner)
        return;
    
    CALayer *layer = m_owner->platformLayer();

    String animationKey;
    for (NSString *key in [layer animationKeys]) {
        if ([layer animationForKey:key] == animation) {
            animationKey = key;
            break;
        }
    }

    if (!animationKey.isEmpty())
        m_owner->animationEnded(animationKey);
}

- (void)setOwner:(PlatformCALayer*)owner
{
    m_owner = owner;
}

@end

void PlatformCALayerCocoa::setOwner(PlatformCALayerClient* owner)
{
    PlatformCALayer::setOwner(owner);
    
    // Change the delegate's owner if needed
    if (m_delegate)
        [static_cast<WebAnimationDelegate*>(m_delegate.get()) setOwner:this];        
}

static NSString *toCAFilterType(PlatformCALayer::FilterType type)
{
    switch (type) {
    case PlatformCALayer::Linear: return kCAFilterLinear;
    case PlatformCALayer::Nearest: return kCAFilterNearest;
    case PlatformCALayer::Trilinear: return kCAFilterTrilinear;
    default: return 0;
    }
}

PlatformCALayer::LayerType PlatformCALayerCocoa::layerTypeForPlatformLayer(PlatformLayer* layer)
{
    if ([layer isKindOfClass:getAVPlayerLayerClass()] || [layer isKindOfClass:objc_getClass("WebVideoContainerLayer")])
        return LayerTypeAVPlayerLayer;

    if ([layer isKindOfClass:[WebGLLayer class]])
        return LayerTypeWebGLLayer;

    return LayerTypeCustom;
}

PlatformCALayerCocoa::PlatformCALayerCocoa(LayerType layerType, PlatformCALayerClient* owner)
    : PlatformCALayer(layerType, owner)
    , m_customAppearance(GraphicsLayer::NoCustomAppearance)
{
    Class layerClass = Nil;
    switch (layerType) {
    case LayerTypeLayer:
    case LayerTypeRootLayer:
        layerClass = [CALayer class];
        break;
    case LayerTypeScrollingLayer:
        // Scrolling layers only have special behavior with PlatformCALayerRemote.
        // fallthrough
    case LayerTypeWebLayer:
        layerClass = [WebLayer class];
        break;
    case LayerTypeSimpleLayer:
    case LayerTypeTiledBackingTileLayer:
        layerClass = [WebSimpleLayer class];
        break;
    case LayerTypeTransformLayer:
        layerClass = [CATransformLayer class];
        break;
#if ENABLE(FILTERS_LEVEL_2)
    case LayerTypeBackdropLayer:
        layerClass = [CABackdropLayer class];
        break;
    case LayerTypeLightSystemBackdropLayer:
        layerClass = [WebLightSystemBackdropLayer class];
        break;
    case LayerTypeDarkSystemBackdropLayer:
        layerClass = [WebDarkSystemBackdropLayer class];
        break;
#else
    case LayerTypeBackdropLayer:
    case LayerTypeLightSystemBackdropLayer:
    case LayerTypeDarkSystemBackdropLayer:
        ASSERT_NOT_REACHED();
        layerClass = [CALayer class];
        break;
#endif
    case LayerTypeWebTiledLayer:
        ASSERT_NOT_REACHED();
        break;
    case LayerTypeTiledBackingLayer:
    case LayerTypePageTiledBackingLayer:
        layerClass = [WebTiledBackingLayer class];
        break;
    case LayerTypeAVPlayerLayer:
        layerClass = getAVPlayerLayerClass();
        break;
    case LayerTypeWebGLLayer:
        // We don't create PlatformCALayerCocoas wrapped around WebGLLayers.
        ASSERT_NOT_REACHED();
        break;
    case LayerTypeShapeLayer:
        layerClass = [CAShapeLayer class];
        // fillColor defaults to opaque black.
        break;
    case LayerTypeCustom:
        break;
    }

    if (layerClass)
        m_layer = adoptNS([(CALayer *)[layerClass alloc] init]);

#if ENABLE(FILTERS_LEVEL_2) && PLATFORM(MAC)
    if (layerType == LayerTypeBackdropLayer)
        [(CABackdropLayer*)m_layer.get() setWindowServerAware:NO];
#endif

    commonInit();
}

PlatformCALayerCocoa::PlatformCALayerCocoa(PlatformLayer* layer, PlatformCALayerClient* owner)
    : PlatformCALayer(layerTypeForPlatformLayer(layer), owner)
    , m_customAppearance(GraphicsLayer::NoCustomAppearance)
{
    m_layer = layer;
    commonInit();
}

void PlatformCALayerCocoa::commonInit()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    // Save a pointer to 'this' in the CALayer
    [m_layer setValue:[NSValue valueWithPointer:this] forKey:platformCALayerPointer];
    
    // Clear all the implicit animations on the CALayer
    if (m_layerType == LayerTypeAVPlayerLayer || m_layerType == LayerTypeWebGLLayer || m_layerType == LayerTypeScrollingLayer || m_layerType == LayerTypeCustom)
        [m_layer web_disableAllActions];
    else
        [m_layer setDelegate:[WebActionDisablingCALayerDelegate shared]];

    // So that the scrolling thread's performance logging code can find all the tiles, mark this as being a tile.
    if (m_layerType == LayerTypeTiledBackingTileLayer)
        [m_layer setValue:@YES forKey:@"isTile"];

    if (usesTiledBackingLayer()) {
        WebTiledBackingLayer* tiledBackingLayer = static_cast<WebTiledBackingLayer*>(m_layer.get());
        TileController* tileController = [tiledBackingLayer createTileController:this];

        m_customSublayers = std::make_unique<PlatformCALayerList>(tileController->containerLayers());
    }

    END_BLOCK_OBJC_EXCEPTIONS
}

PassRefPtr<PlatformCALayer> PlatformCALayerCocoa::clone(PlatformCALayerClient* owner) const
{
    LayerType type;
    switch (layerType()) {
    case LayerTypeTransformLayer:
        type = LayerTypeTransformLayer;
        break;
    case LayerTypeAVPlayerLayer:
        type = LayerTypeAVPlayerLayer;
        break;
    case LayerTypeShapeLayer:
        type = LayerTypeShapeLayer;
        break;
    case LayerTypeLayer:
    default:
        type = LayerTypeLayer;
        break;
    };
    RefPtr<PlatformCALayer> newLayer = PlatformCALayerCocoa::create(type, owner);
    
    newLayer->setPosition(position());
    newLayer->setBounds(bounds());
    newLayer->setAnchorPoint(anchorPoint());
    newLayer->setTransform(transform());
    newLayer->setSublayerTransform(sublayerTransform());
    newLayer->setContents(contents());
    newLayer->setMasksToBounds(masksToBounds());
    newLayer->setDoubleSided(isDoubleSided());
    newLayer->setOpaque(isOpaque());
    newLayer->setBackgroundColor(backgroundColor());
    newLayer->setContentsScale(contentsScale());
    newLayer->setCornerRadius(cornerRadius());
    newLayer->copyFiltersFrom(*this);
    newLayer->updateCustomAppearance(customAppearance());

    if (type == LayerTypeAVPlayerLayer) {
        ASSERT([newLayer->platformLayer() isKindOfClass:getAVPlayerLayerClass()]);
        ASSERT([platformLayer() isKindOfClass:getAVPlayerLayerClass()]);

        AVPlayerLayer* destinationPlayerLayer = static_cast<AVPlayerLayer *>(newLayer->platformLayer());
        AVPlayerLayer* sourcePlayerLayer = static_cast<AVPlayerLayer *>(platformLayer());
        dispatch_async(dispatch_get_main_queue(), ^{
            [destinationPlayerLayer setPlayer:[sourcePlayerLayer player]];
        });
    }
    
    if (type == LayerTypeShapeLayer)
        newLayer->setShapeRoundedRect(shapeRoundedRect());

    return newLayer;
}

PlatformCALayerCocoa::~PlatformCALayerCocoa()
{
    [m_layer setValue:nil forKey:platformCALayerPointer];
    
    // Remove the owner pointer from the delegate in case there is a pending animationStarted event.
    [static_cast<WebAnimationDelegate*>(m_delegate.get()) setOwner:nil];

    if (usesTiledBackingLayer())
        [static_cast<WebTiledBackingLayer *>(m_layer.get()) invalidate];
}

void PlatformCALayerCocoa::animationStarted(const String& animationKey, CFTimeInterval beginTime)
{
    if (m_owner)
        m_owner->platformCALayerAnimationStarted(animationKey, beginTime);
}

void PlatformCALayerCocoa::animationEnded(const String& animationKey)
{
    if (m_owner)
        m_owner->platformCALayerAnimationEnded(animationKey);
}

void PlatformCALayerCocoa::setNeedsDisplay()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setNeedsDisplay];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setNeedsDisplayInRect(const FloatRect& dirtyRect)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setNeedsDisplayInRect:dirtyRect];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::copyContentsFromLayer(PlatformCALayer* layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    CALayer* caLayer = layer->m_layer.get();
    if ([m_layer contents] != [caLayer contents])
        [m_layer setContents:[caLayer contents]];
    else
        [m_layer setContentsChanged];
    END_BLOCK_OBJC_EXCEPTIONS
}

PlatformCALayer* PlatformCALayerCocoa::superlayer() const
{
    return platformCALayer([m_layer superlayer]);
}

void PlatformCALayerCocoa::removeFromSuperlayer()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer removeFromSuperlayer];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setSublayers(const PlatformCALayerList& list)
{
    // Short circuiting here avoids the allocation of the array below.
    if (!list.size()) {
        removeAllSublayers();
        return;
    }

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    NSMutableArray* sublayers = [[NSMutableArray alloc] init];
    for (size_t i = 0; i < list.size(); ++i)
        [sublayers addObject:list[i]->m_layer.get()];

    [m_layer setSublayers:sublayers];
    [sublayers release];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::removeAllSublayers()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setSublayers:nil];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::appendSublayer(PlatformCALayer& layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    ASSERT(m_layer != layer.m_layer);
    [m_layer addSublayer:layer.m_layer.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::insertSublayer(PlatformCALayer& layer, size_t index)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    ASSERT(m_layer != layer.m_layer);
    [m_layer insertSublayer:layer.m_layer.get() atIndex:index];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::replaceSublayer(PlatformCALayer& reference, PlatformCALayer& layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    ASSERT(m_layer != layer.m_layer);
    [m_layer replaceSublayer:reference.m_layer.get() with:layer.m_layer.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::adoptSublayers(PlatformCALayer& source)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setSublayers:[source.m_layer.get() sublayers]];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::addAnimationForKey(const String& key, PlatformCAAnimation& animation)
{
    // Add the delegate
    if (!m_delegate) {
        WebAnimationDelegate* webAnimationDelegate = [[WebAnimationDelegate alloc] init];
        m_delegate = adoptNS(webAnimationDelegate);
        [webAnimationDelegate setOwner:this];
    }
    
    CAPropertyAnimation* propertyAnimation = static_cast<CAPropertyAnimation*>(downcast<PlatformCAAnimationCocoa>(animation).platformAnimation());
    if (![propertyAnimation delegate])
        [propertyAnimation setDelegate:static_cast<id>(m_delegate.get())];

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer addAnimation:propertyAnimation forKey:key];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::removeAnimationForKey(const String& key)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer removeAnimationForKey:key];
    END_BLOCK_OBJC_EXCEPTIONS
}

PassRefPtr<PlatformCAAnimation> PlatformCALayerCocoa::animationForKey(const String& key)
{
    CAPropertyAnimation* propertyAnimation = static_cast<CAPropertyAnimation*>([m_layer animationForKey:key]);
    if (!propertyAnimation)
        return 0;
    return PlatformCAAnimationCocoa::create(propertyAnimation);
}

void PlatformCALayerCocoa::setMask(PlatformCALayer* layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setMask:layer ? layer->platformLayer() : nil];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayerCocoa::isOpaque() const
{
    return [m_layer isOpaque];
}

void PlatformCALayerCocoa::setOpaque(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setOpaque:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatRect PlatformCALayerCocoa::bounds() const
{
    return [m_layer bounds];
}

void PlatformCALayerCocoa::setBounds(const FloatRect& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setBounds:value];
    
    if (requiresCustomAppearanceUpdateOnBoundsChange())
        updateCustomAppearance(m_customAppearance);

    END_BLOCK_OBJC_EXCEPTIONS
}

FloatPoint3D PlatformCALayerCocoa::position() const
{
    CGPoint point = [m_layer position];
    return FloatPoint3D(point.x, point.y, [m_layer zPosition]);
}

void PlatformCALayerCocoa::setPosition(const FloatPoint3D& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setPosition:CGPointMake(value.x(), value.y())];
    [m_layer setZPosition:value.z()];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatPoint3D PlatformCALayerCocoa::anchorPoint() const
{
    CGPoint point = [m_layer anchorPoint];
    float z = 0;
    z = [m_layer anchorPointZ];
    return FloatPoint3D(point.x, point.y, z);
}

void PlatformCALayerCocoa::setAnchorPoint(const FloatPoint3D& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setAnchorPoint:CGPointMake(value.x(), value.y())];
    [m_layer setAnchorPointZ:value.z()];
    END_BLOCK_OBJC_EXCEPTIONS
}

TransformationMatrix PlatformCALayerCocoa::transform() const
{
    return [m_layer transform];
}

void PlatformCALayerCocoa::setTransform(const TransformationMatrix& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setTransform:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

TransformationMatrix PlatformCALayerCocoa::sublayerTransform() const
{
    return [m_layer sublayerTransform];
}

void PlatformCALayerCocoa::setSublayerTransform(const TransformationMatrix& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setSublayerTransform:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setHidden(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setHidden:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setBackingStoreAttached(bool)
{
    // We could throw away backing store here with setContents:nil.
}

bool PlatformCALayerCocoa::backingStoreAttached() const
{
    return true;
}

void PlatformCALayerCocoa::setGeometryFlipped(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setGeometryFlipped:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayerCocoa::isDoubleSided() const
{
    return [m_layer isDoubleSided];
}

void PlatformCALayerCocoa::setDoubleSided(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setDoubleSided:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayerCocoa::masksToBounds() const
{
    return [m_layer masksToBounds];
}

void PlatformCALayerCocoa::setMasksToBounds(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setMasksToBounds:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayerCocoa::acceleratesDrawing() const
{
    return [m_layer acceleratesDrawing];
}

void PlatformCALayerCocoa::setAcceleratesDrawing(bool acceleratesDrawing)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setAcceleratesDrawing:acceleratesDrawing];
    END_BLOCK_OBJC_EXCEPTIONS
}

CFTypeRef PlatformCALayerCocoa::contents() const
{
    return [m_layer contents];
}

void PlatformCALayerCocoa::setContents(CFTypeRef value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setContents:static_cast<id>(const_cast<void*>(value))];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setContentsRect(const FloatRect& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setContentsRect:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setMinificationFilter(FilterType value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setMinificationFilter:toCAFilterType(value)];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setMagnificationFilter(FilterType value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setMagnificationFilter:toCAFilterType(value)];
    END_BLOCK_OBJC_EXCEPTIONS
}

Color PlatformCALayerCocoa::backgroundColor() const
{
    return [m_layer backgroundColor];
}

void PlatformCALayerCocoa::setBackgroundColor(const Color& value)
{
    CGFloat components[4];
    value.getRGBA(components[0], components[1], components[2], components[3]);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setBackgroundColor:color.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setBorderWidth(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setBorderWidth:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setBorderColor(const Color& value)
{
    if (value.isValid()) {
        CGFloat components[4];
        value.getRGBA(components[0], components[1], components[2], components[3]);

        RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
        RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));

        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [m_layer setBorderColor:color.get()];
        END_BLOCK_OBJC_EXCEPTIONS
    } else {
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [m_layer setBorderColor:nil];
        END_BLOCK_OBJC_EXCEPTIONS
    }
}

float PlatformCALayerCocoa::opacity() const
{
    return [m_layer opacity];
}

void PlatformCALayerCocoa::setOpacity(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setOpacity:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setFilters(const FilterOperations& filters)
{
    PlatformCAFilters::setFiltersOnLayer(platformLayer(), filters);
}

void PlatformCALayerCocoa::copyFiltersFrom(const PlatformCALayer& sourceLayer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setFilters:[sourceLayer.platformLayer() filters]];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayerCocoa::filtersCanBeComposited(const FilterOperations& filters)
{
    // Return false if there are no filters to avoid needless work
    if (!filters.size())
        return false;
    
    for (unsigned i = 0; i < filters.size(); ++i) {
        const FilterOperation* filterOperation = filters.at(i);
        switch (filterOperation->type()) {
        case FilterOperation::REFERENCE:
            return false;
        case FilterOperation::DROP_SHADOW:
            // FIXME: For now we can only handle drop-shadow is if it's last in the list
            if (i < (filters.size() - 1))
                return false;
            break;
        default:
            break;
        }
    }

    return true;
}

#if ENABLE(CSS_COMPOSITING)
void PlatformCALayerCocoa::setBlendMode(BlendMode blendMode)
{
    PlatformCAFilters::setBlendingFiltersOnLayer(platformLayer(), blendMode);
}
#endif

void PlatformCALayerCocoa::setName(const String& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setName:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setSpeed(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setSpeed:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setTimeOffset(CFTimeInterval value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setTimeOffset:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

float PlatformCALayerCocoa::contentsScale() const
{
    return [m_layer contentsScale];
}

void PlatformCALayerCocoa::setContentsScale(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setContentsScale:value];
#if PLATFORM(IOS)
    [m_layer setRasterizationScale:value];

    if (m_layerType == LayerTypeWebTiledLayer) {
        // This will invalidate all the tiles so we won't end up with stale tiles with the wrong scale in the wrong place,
        // see <rdar://problem/9434765> for more information.
        static NSDictionary *optionsDictionary = [[NSDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithBool:YES], kCATiledLayerRemoveImmediately, nil];
        [(CATiledLayer *)m_layer.get() setNeedsDisplayInRect:[m_layer bounds] levelOfDetail:0 options:optionsDictionary];
    }
#endif
    END_BLOCK_OBJC_EXCEPTIONS
}

float PlatformCALayerCocoa::cornerRadius() const
{
    return [m_layer cornerRadius];
}

void PlatformCALayerCocoa::setCornerRadius(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setCornerRadius:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setEdgeAntialiasingMask(unsigned mask)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer setEdgeAntialiasingMask:mask];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatRoundedRect PlatformCALayerCocoa::shapeRoundedRect() const
{
    ASSERT(m_layerType == LayerTypeShapeLayer);
    if (m_shapeRoundedRect)
        return *m_shapeRoundedRect;

    return FloatRoundedRect();
}

void PlatformCALayerCocoa::setShapeRoundedRect(const FloatRoundedRect& roundedRect)
{
    ASSERT(m_layerType == LayerTypeShapeLayer);
    m_shapeRoundedRect = std::make_unique<FloatRoundedRect>(roundedRect);

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    Path shapePath;
    shapePath.addRoundedRect(roundedRect);
    [(CAShapeLayer *)m_layer setPath:shapePath.platformPath()];
    END_BLOCK_OBJC_EXCEPTIONS
}

WindRule PlatformCALayerCocoa::shapeWindRule() const
{
    ASSERT(m_layerType == LayerTypeShapeLayer);

    NSString *fillRule = [(CAShapeLayer *)m_layer fillRule];
    if ([fillRule isEqualToString:@"even-odd"])
        return RULE_EVENODD;

    return RULE_NONZERO;
}

void PlatformCALayerCocoa::setShapeWindRule(WindRule windRule)
{
    ASSERT(m_layerType == LayerTypeShapeLayer);

    switch (windRule) {
    case RULE_NONZERO:
        [(CAShapeLayer *)m_layer setFillRule:@"non-zero"];
        break;
    case RULE_EVENODD:
        [(CAShapeLayer *)m_layer setFillRule:@"even-odd"];
        break;
    }
}

Path PlatformCALayerCocoa::shapePath() const
{
    ASSERT(m_layerType == LayerTypeShapeLayer);

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    return Path(CGPathCreateMutableCopy([(CAShapeLayer *)m_layer path]));
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayerCocoa::setShapePath(const Path& path)
{
    ASSERT(m_layerType == LayerTypeShapeLayer);

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [(CAShapeLayer *)m_layer setPath:path.platformPath()];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayerCocoa::requiresCustomAppearanceUpdateOnBoundsChange() const
{
    return m_customAppearance == GraphicsLayer::ScrollingShadow;
}

void PlatformCALayerCocoa::updateCustomAppearance(GraphicsLayer::CustomAppearance appearance)
{
    if (m_customAppearance == appearance)
        return;

    m_customAppearance = appearance;

#if ENABLE(RUBBER_BANDING)
    switch (appearance) {
    case GraphicsLayer::NoCustomAppearance:
    case GraphicsLayer::LightBackdropAppearance:
    case GraphicsLayer::DarkBackdropAppearance:
        ScrollbarThemeMac::removeOverhangAreaBackground(platformLayer());
        ScrollbarThemeMac::removeOverhangAreaShadow(platformLayer());
        break;
    case GraphicsLayer::ScrollingOverhang:
        ScrollbarThemeMac::setUpOverhangAreaBackground(platformLayer());
        break;
    case GraphicsLayer::ScrollingShadow:
        ScrollbarThemeMac::setUpOverhangAreaShadow(platformLayer());
        break;
    }
#endif
}

TiledBacking* PlatformCALayerCocoa::tiledBacking()
{
    if (!usesTiledBackingLayer())
        return nullptr;

    WebTiledBackingLayer *tiledBackingLayer = static_cast<WebTiledBackingLayer *>(m_layer.get());
    return [tiledBackingLayer tiledBacking];
}

#if PLATFORM(IOS)
bool PlatformCALayer::isWebLayer()
{
    BOOL result = NO;
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    result = [m_layer isKindOfClass:[WebLayer self]];
    END_BLOCK_OBJC_EXCEPTIONS
    return result;
}

void PlatformCALayer::setBoundsOnMainThread(CGRect bounds)
{
    CALayer *layer = m_layer.get();
    dispatch_async(dispatch_get_main_queue(), ^{
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [layer setBounds:bounds];
        END_BLOCK_OBJC_EXCEPTIONS
    });
}

void PlatformCALayer::setPositionOnMainThread(CGPoint position)
{
    CALayer *layer = m_layer.get();
    dispatch_async(dispatch_get_main_queue(), ^{
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [layer setPosition:position];
        END_BLOCK_OBJC_EXCEPTIONS
    });
}

void PlatformCALayer::setAnchorPointOnMainThread(FloatPoint3D value)
{
    CALayer *layer = m_layer.get();
    dispatch_async(dispatch_get_main_queue(), ^{
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [layer setAnchorPoint:CGPointMake(value.x(), value.y())];
        [layer setAnchorPointZ:value.z()];
        END_BLOCK_OBJC_EXCEPTIONS
    });
}

void PlatformCALayer::setTileSize(const IntSize& tileSize)
{
    if (m_layerType != LayerTypeWebTiledLayer)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [static_cast<WebTiledLayer*>(m_layer.get()) setTileSize:tileSize];
    END_BLOCK_OBJC_EXCEPTIONS
}
#endif // PLATFORM(IOS)

PlatformCALayer::RepaintRectList PlatformCALayer::collectRectsToPaint(CGContextRef context, PlatformCALayer* platformCALayer)
{
    __block double totalRectArea = 0;
    __block unsigned rectCount = 0;
    __block RepaintRectList dirtyRects;
    
    platformCALayer->enumerateRectsBeingDrawn(context, ^(CGRect rect) {
        if (++rectCount > webLayerMaxRectsToPaint)
            return;
        
        totalRectArea += rect.size.width * rect.size.height;
        dirtyRects.append(rect);
    });
    
    FloatRect clipBounds = CGContextGetClipBoundingBox(context);
    double clipArea = clipBounds.width() * clipBounds.height();
    
    if (rectCount >= webLayerMaxRectsToPaint || totalRectArea >= clipArea * webLayerWastedSpaceThreshold) {
        dirtyRects.clear();
        dirtyRects.append(clipBounds);
    }
    
    return dirtyRects;
}

void PlatformCALayer::drawLayerContents(CGContextRef context, WebCore::PlatformCALayer* platformCALayer, RepaintRectList& dirtyRects)
{
    WebCore::PlatformCALayerClient* layerContents = platformCALayer->owner();
    if (!layerContents)
        return;
    
#if PLATFORM(IOS)
    WKSetCurrentGraphicsContext(context);
#endif
    
    CGContextSaveGState(context);
    
    // We never use CompositingCoordinatesBottomUp on Mac.
    ASSERT(layerContents->platformCALayerContentsOrientation() == GraphicsLayer::CompositingCoordinatesTopDown);
    
#if PLATFORM(IOS)
    FontAntialiasingStateSaver fontAntialiasingState(context, [platformCALayer->platformLayer() isOpaque]);
    fontAntialiasingState.setup([WAKWindow hasLandscapeOrientation]);
#else
    [NSGraphicsContext saveGraphicsState];
    
    // Set up an NSGraphicsContext for the context, so that parts of AppKit that rely on
    // the current NSGraphicsContext (e.g. NSCell drawing) get the right one.
    NSGraphicsContext* layerContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
    [NSGraphicsContext setCurrentContext:layerContext];
#endif
    
    GraphicsContext graphicsContext(context);
    graphicsContext.setIsCALayerContext(true);
    graphicsContext.setIsAcceleratedContext(platformCALayer->acceleratesDrawing());
    
    if (!layerContents->platformCALayerContentsOpaque()) {
        // Turn off font smoothing to improve the appearance of text rendered onto a transparent background.
        graphicsContext.setShouldSmoothFonts(false);
        graphicsContext.setAntialiasedFontDilationEnabled(true);
    }
    
#if PLATFORM(MAC)
    // It's important to get the clip from the context, because it may be significantly
    // smaller than the layer bounds (e.g. tiled layers)
    ThemeMac::setFocusRingClipRect(CGContextGetClipBoundingBox(context));
#endif
    
    for (const auto& rect : dirtyRects) {
        GraphicsContextStateSaver stateSaver(graphicsContext);
        graphicsContext.clip(rect);
        
        layerContents->platformCALayerPaintContents(platformCALayer, graphicsContext, rect);
    }
    
#if PLATFORM(IOS)
    fontAntialiasingState.restore();
#else
    ThemeMac::setFocusRingClipRect(FloatRect());
    
    [NSGraphicsContext restoreGraphicsState];
#endif
    
    // Re-fetch the layer owner, since <rdar://problem/9125151> indicates that it might have been destroyed during painting.
    layerContents = platformCALayer->owner();
    ASSERT(layerContents);
    
    CGContextRestoreGState(context);
    
    // Always update the repaint count so that it's accurate even if the count itself is not shown. This will be useful
    // for the Web Inspector feeding this information through the LayerTreeAgent.
    int repaintCount = layerContents->platformCALayerIncrementRepaintCount(platformCALayer);
    
    if (!platformCALayer->usesTiledBackingLayer() && layerContents && layerContents->platformCALayerShowRepaintCounter(platformCALayer))
        drawRepaintIndicator(context, platformCALayer, repaintCount, nullptr);
}

CGRect PlatformCALayer::frameForLayer(const PlatformLayer* tileLayer)
{
    return [tileLayer frame];
}

PassRefPtr<PlatformCALayer> PlatformCALayerCocoa::createCompatibleLayer(PlatformCALayer::LayerType layerType, PlatformCALayerClient* client) const
{
    return PlatformCALayerCocoa::create(layerType, client);
}

void PlatformCALayerCocoa::enumerateRectsBeingDrawn(CGContextRef context, void (^block)(CGRect))
{
    wkCALayerEnumerateRectsBeingDrawnWithBlock(m_layer.get(), context, block);
}
