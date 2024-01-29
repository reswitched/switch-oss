/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

// FIXME: Rename this file to WebEventIOS.mm after we upstream the iOS port and remove the PLATFORM(IOS)-guard.
#ifndef WebEventIOS_h
#define WebEventIOS_h

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE

typedef enum {
    WebEventMouseDown,
    WebEventMouseUp,
    WebEventMouseMoved,
    
    WebEventScrollWheel,
    
    WebEventKeyDown,
    WebEventKeyUp,
    
    WebEventTouchBegin,
    WebEventTouchChange,
    WebEventTouchEnd,
    WebEventTouchCancel
} WebEventType;

typedef enum {
    WebEventTouchPhaseBegan,
    WebEventTouchPhaseMoved,
    WebEventTouchPhaseStationary,
    WebEventTouchPhaseEnded,
    WebEventTouchPhaseCancelled
} WebEventTouchPhaseType;

// These enum values are copied directly from GSEvent for compatibility.
typedef enum {
    WebEventFlagMaskAlphaShift = 0x00010000,
    WebEventFlagMaskShift      = 0x00020000,
    WebEventFlagMaskControl    = 0x00040000,
    WebEventFlagMaskAlternate  = 0x00080000,
    WebEventFlagMaskCommand    = 0x00100000,
} WebEventFlagValues;
typedef unsigned WebEventFlags;

// These enum values are copied directly from GSEvent for compatibility.
typedef enum {
    WebEventCharacterSetASCII           = 0,
    WebEventCharacterSetSymbol          = 1,
    WebEventCharacterSetDingbats        = 2,
    WebEventCharacterSetUnicode         = 253,
    WebEventCharacterSetFunctionKeys    = 254,
} WebEventCharacterSet;

WEBCORE_EXPORT @interface WebEvent : NSObject {
@private
    WebEventType _type;
    CFTimeInterval _timestamp;
    
    CGPoint _locationInWindow;
    
    NSString *_characters;
    NSString *_charactersIgnoringModifiers;
    WebEventFlags _modifierFlags;
    BOOL _keyRepeating;
    BOOL _popupVariant; // FIXME: to be removed
    NSUInteger _keyboardFlags;
    uint16_t _keyCode;
    BOOL _tabKey;
    WebEventCharacterSet _characterSet;
    
    float _deltaX;
    float _deltaY;
    
    unsigned _touchCount;
    NSArray *_touchLocations;
    NSArray *_touchIdentifiers;
    NSArray *_touchPhases;
    
    BOOL _isGesture;
    float _gestureScale;
    float _gestureRotation;

    BOOL _wasHandled;
}

- (WebEvent *)initWithMouseEventType:(WebEventType)type
                           timeStamp:(CFTimeInterval)timeStamp
                            location:(CGPoint)point;

- (WebEvent *)initWithScrollWheelEventWithTimeStamp:(CFTimeInterval)timeStamp
                                           location:(CGPoint)point
                                              deltaX:(float)deltaX
                                              deltaY:(float)deltaY;

- (WebEvent *)initWithTouchEventType:(WebEventType)type
                           timeStamp:(CFTimeInterval)timeStamp
                            location:(CGPoint)point
                           modifiers:(WebEventFlags)modifiers
                          touchCount:(unsigned)touchCount
                      touchLocations:(NSArray *)touchLocations
                    touchIdentifiers:(NSArray *)touchIdentifiers
                         touchPhases:(NSArray *)touchPhases isGesture:(BOOL)isGesture
                        gestureScale:(float)gestureScale
                     gestureRotation:(float)gestureRotation;

// FIXME: this is deprecated. It will be removed when UIKit adopts the new one below.
- (WebEvent *)initWithKeyEventType:(WebEventType)type
                         timeStamp:(CFTimeInterval)timeStamp
                        characters:(NSString *)characters
       charactersIgnoringModifiers:(NSString *)charactersIgnoringModifiers
                         modifiers:(WebEventFlags)modifiers
                       isRepeating:(BOOL)repeating
                    isPopupVariant:(BOOL)popupVariant
                           keyCode:(uint16_t)keyCode
                          isTabKey:(BOOL)tabKey
                      characterSet:(WebEventCharacterSet)characterSet;

- (WebEvent *)initWithKeyEventType:(WebEventType)type
                         timeStamp:(CFTimeInterval)timeStamp
                        characters:(NSString *)characters
       charactersIgnoringModifiers:(NSString *)charactersIgnoringModifiers
                         modifiers:(WebEventFlags)modifiers
                       isRepeating:(BOOL)repeating
                         withFlags:(NSUInteger)flags
                           keyCode:(uint16_t)keyCode
                          isTabKey:(BOOL)tabKey
                      characterSet:(WebEventCharacterSet)characterSet;

@property(nonatomic, readonly) WebEventType type;
@property(nonatomic, readonly) CFTimeInterval timestamp;

// Mouse
@property(nonatomic, readonly) CGPoint locationInWindow;

// Keyboard
@property(nonatomic, readonly, retain) NSString *characters;
@property(nonatomic, readonly, retain) NSString *charactersIgnoringModifiers;
@property(nonatomic, readonly) WebEventFlags modifierFlags;
@property(nonatomic, readonly, getter = isKeyRepeating) BOOL keyRepeating;

// FIXME: this is deprecated. It will be removed when UIKit adopts the new initWithKeyEventType.
@property(nonatomic, readonly, getter = isPopupVariant) BOOL popupVariant;
@property(nonatomic, readonly) NSUInteger keyboardFlags;
@property(nonatomic, readonly) uint16_t keyCode;
@property(nonatomic, readonly, getter = isTabKey) BOOL tabKey;
@property(nonatomic, readonly) WebEventCharacterSet characterSet;

// Scroll Wheel
@property(nonatomic, readonly) float deltaX;
@property(nonatomic, readonly) float deltaY;

// Touch
@property(nonatomic, readonly) unsigned touchCount;
@property(nonatomic, readonly, retain) NSArray *touchLocations;
@property(nonatomic, readonly, retain) NSArray *touchIdentifiers;
@property(nonatomic, readonly, retain) NSArray *touchPhases;

// Gesture
@property(nonatomic, readonly) BOOL isGesture;
@property(nonatomic, readonly) float gestureScale;
@property(nonatomic, readonly) float gestureRotation;

@property(nonatomic) BOOL wasHandled;

@end

@interface WebIOSEvent : WebEvent
@end

#endif // TARGET_OS_IPHONE
#endif // WebEventIOS_h
