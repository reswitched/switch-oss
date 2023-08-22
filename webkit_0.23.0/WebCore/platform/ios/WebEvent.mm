/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebEvent.h"

#import "KeyEventCocoa.h"
#import <wtf/Assertions.h>

using namespace WebCore;

#if PLATFORM(IOS)

#import "WAKAppKitStubs.h"

@implementation WebEvent

@synthesize type = _type;
@synthesize timestamp = _timestamp;
@synthesize wasHandled = _wasHandled;

- (WebEvent *)initWithMouseEventType:(WebEventType)type
                           timeStamp:(CFTimeInterval)timeStamp
                            location:(CGPoint)point
{
    self = [super init];
    if (!self)
        return nil;
    
    _type = type;
    _timestamp = timeStamp;

    _locationInWindow = point;
    
    return self;
}

- (WebEvent *)initWithScrollWheelEventWithTimeStamp:(CFTimeInterval)timeStamp
                                           location:(CGPoint)point
                                              deltaX:(float)deltaX
                                              deltaY:(float)deltaY
{
    self = [super init];
    if (!self)
        return nil;
    
    _type = WebEventScrollWheel;
    _timestamp = timeStamp;

    _locationInWindow = point;
    _deltaX = deltaX;
    _deltaY = deltaY;
    
    return self;
}

- (WebEvent *)initWithTouchEventType:(WebEventType)type
                           timeStamp:(CFTimeInterval)timeStamp
                            location:(CGPoint)point
                           modifiers:(WebEventFlags)modifiers
                          touchCount:(unsigned)touchCount
                      touchLocations:(NSArray *)touchLocations
                    touchIdentifiers:(NSArray *)touchIdentifiers
                         touchPhases:(NSArray *)touchPhases isGesture:(BOOL)isGesture
                        gestureScale:(float)gestureScale
                     gestureRotation:(float)gestureRotation
{
    self = [super init];
    if (!self)
        return nil;

    _type = type;
    _timestamp = timeStamp;
    _modifierFlags = modifiers;

    // FIXME: <rdar://problem/7185284> TouchEvents may be in more than one window some day.
    _locationInWindow = point;
    _touchCount = touchCount;
    _touchLocations = [touchLocations copy];
    _touchIdentifiers = [touchIdentifiers copy];
    _touchPhases = [touchPhases copy];
    _isGesture = isGesture;
    _gestureScale = gestureScale;
    _gestureRotation = gestureRotation;

    return self;
}

static int windowsKeyCodeForCharCodeIOS(unichar charCode)
{
    // iPhone Specific Cases
    // <rdar://7709408>: We get 10 ('\n') from UIKit when using the software keyboard
    if (charCode == 10)
        return 0x0D;

    // General Case
    return windowsKeyCodeForCharCode(charCode);
}

- (WebEvent *)initWithKeyEventType:(WebEventType)type
                         timeStamp:(CFTimeInterval)timeStamp
                        characters:(NSString *)characters
       charactersIgnoringModifiers:(NSString *)charactersIgnoringModifiers
                         modifiers:(WebEventFlags)modifiers
                       isRepeating:(BOOL)repeating
                    isPopupVariant:(BOOL)popupVariant
                           keyCode:(uint16_t)keyCode
                          isTabKey:(BOOL)tabKey
                      characterSet:(WebEventCharacterSet)characterSet
{
    self = [super init];
    if (!self)
        return nil;
    
    _type = type;
    _timestamp = timeStamp;

    _characters = [characters retain];
    _charactersIgnoringModifiers = [charactersIgnoringModifiers retain];
    _modifierFlags = modifiers;
    _keyRepeating = repeating;
    _popupVariant = popupVariant;
    _tabKey = tabKey;
    _characterSet = characterSet;

    if (keyCode)
        _keyCode = windowsKeyCodeForKeyCode(keyCode);

    // NOTE: this preserves the original semantics which used the
    // characters string for the keyCode. This should be changed in iOS 4.0 to
    // allow the client to explicitly specify a keyCode, otherwise default to
    // mapping the characters string to a keyCode.
    // e.g. add an 'else' before this 'if'.
    if ([charactersIgnoringModifiers length] == 1) {
        unichar ch = [charactersIgnoringModifiers characterAtIndex:0];
        _keyCode = windowsKeyCodeForCharCodeIOS(ch);
    }

    return self;
}


- (WebEvent *)initWithKeyEventType:(WebEventType)type
                         timeStamp:(CFTimeInterval)timeStamp
                        characters:(NSString *)characters
       charactersIgnoringModifiers:(NSString *)charactersIgnoringModifiers
                         modifiers:(WebEventFlags)modifiers
                       isRepeating:(BOOL)repeating
                         withFlags:(NSUInteger)flags
                           keyCode:(uint16_t)keyCode
                          isTabKey:(BOOL)tabKey
                      characterSet:(WebEventCharacterSet)characterSet
{
    self = [super init];
    if (!self)
        return nil;
    
    _type = type;
    _timestamp = timeStamp;

    _characters = [characters retain];
    _charactersIgnoringModifiers = [charactersIgnoringModifiers retain];
    _modifierFlags = modifiers;
    _keyRepeating = repeating;
    _keyboardFlags = flags;
    _tabKey = tabKey;
    _characterSet = characterSet;
    
    if (keyCode)
        _keyCode = windowsKeyCodeForKeyCode(keyCode);

    // NOTE: this preserves the original semantics which used the 
    // characters string for the keyCode. This should be changed in iOS 4.0 to
    // allow the client to explicitly specify a keyCode, otherwise default to 
    // mapping the characters string to a keyCode.
    // e.g. add an 'else' before this 'if'.
    if ([charactersIgnoringModifiers length] == 1) {
        unichar ch = [charactersIgnoringModifiers characterAtIndex:0];
        _keyCode = windowsKeyCodeForCharCodeIOS(ch);
    }

    return self;
}

- (void)dealloc
{
    [_characters release];
    [_charactersIgnoringModifiers release];

    [_touchLocations release];
    [_touchIdentifiers release];
    [_touchPhases release];
    
    [super dealloc];
}

- (NSString *)_typeDescription
{
    switch (_type) {
        case WebEventMouseDown:
            return @"WebEventMouseDown";
        case WebEventMouseUp:
            return @"WebEventMouseUp";
        case WebEventMouseMoved:
            return @"WebEventMouseMoved";
        case WebEventScrollWheel:
            return @"WebEventScrollWheel";
        case WebEventKeyDown:
            return @"WebEventKeyDown";
        case WebEventKeyUp:
            return @"WebEventKeyUp";
        case WebEventTouchBegin:
            return @"WebEventTouchBegin";
        case WebEventTouchChange:
            return @"WebEventTouchChange";
        case WebEventTouchEnd:
            return @"WebEventTouchEnd";
        case WebEventTouchCancel:
            return @"WebEventTouchCancel";
        default:
            ASSERT_NOT_REACHED();
    }
    return @"Unknown";
}

- (NSString *)_modiferFlagsDescription
{
    switch (_modifierFlags) {
        case WebEventMouseDown:
            return @"WebEventMouseDown";
        case WebEventMouseUp:
            return @"WebEventMouseUp";
        case WebEventMouseMoved:
            return @"WebEventMouseMoved";
        case WebEventScrollWheel:
            return @"WebEventScrollWheel";
        case WebEventKeyDown:
            return @"WebEventKeyDown";
        case WebEventKeyUp:
            return @"WebEventKeyUp";
        case WebEventTouchBegin:
            return @"WebEventTouchBegin";
        case WebEventTouchChange:
            return @"WebEventTouchChange";
        case WebEventTouchEnd:
            return @"WebEventTouchEnd";
        case WebEventTouchCancel:
            return @"WebEventTouchCancel";
        default:
            ASSERT_NOT_REACHED();
    }
    return @"Unknown";
}

- (NSString *)_characterSetDescription
{
    switch (_characterSet) {
        case WebEventCharacterSetASCII:
            return @"WebEventCharacterSetASCII";
        case WebEventCharacterSetSymbol:
            return @"WebEventCharacterSetSymbol";
        case WebEventCharacterSetDingbats:
            return @"WebEventCharacterSetDingbats";
        case WebEventCharacterSetUnicode:
            return @"WebEventCharacterSetUnicode";
        case WebEventCharacterSetFunctionKeys:
            return @"WebEventCharacterSetFunctionKeys";
        default:
            ASSERT_NOT_REACHED();
    }
    return @"Unknown";
}

- (NSString *)_touchLocationsDescription:(NSArray *)locations
{
    BOOL shouldAddComma = NO;
    NSMutableString *description = [NSMutableString string];
    for (NSValue *value in locations) {
        CGPoint point = [value pointValue];
        [description appendFormat:@"%@(%f, %f)", (shouldAddComma ? @", " : @""), point.x, point.y];
        shouldAddComma = YES;
    }
    return description;
}

- (NSString *)_touchIdentifiersDescription
{
    BOOL shouldAddComma = NO;
    NSMutableString *description = [NSMutableString string];
    for (NSNumber *identifier in _touchIdentifiers) {
        [description appendFormat:@"%@%u", (shouldAddComma ? @", " : @""), [identifier unsignedIntValue]];
        shouldAddComma = YES;
    }
    return description;
}

- (NSString *)_touchPhaseDescription:(WebEventTouchPhaseType)phase
{
    switch (phase) {
        case WebEventTouchPhaseBegan:
            return @"WebEventTouchPhaseBegan";
        case WebEventTouchPhaseMoved:
            return @"WebEventTouchPhaseMoved";
        case WebEventTouchPhaseStationary:
            return @"WebEventTouchPhaseStationary";
        case WebEventTouchPhaseEnded:
            return @"WebEventTouchPhaseEnded";
        case WebEventTouchPhaseCancelled:
            return @"WebEventTouchPhaseCancelled";
        default:
            ASSERT_NOT_REACHED();
    }
    return @"Unknown";
}

- (NSString *)_touchPhasesDescription
{
    BOOL shouldAddComma = NO;
    NSMutableString *description = [NSMutableString string];
    for (NSNumber *phase in _touchPhases) {
        [description appendFormat:@"%@%@", (shouldAddComma ? @", " : @""), [self _touchPhaseDescription:static_cast<WebEventTouchPhaseType>([phase unsignedIntValue])]];
        shouldAddComma = YES;
    }
    return description;
}

- (NSString *)_eventDescription
{
    switch (_type) {
        case WebEventMouseDown:
        case WebEventMouseUp:
        case WebEventMouseMoved:
            return [NSString stringWithFormat:@"location: (%f, %f)", _locationInWindow.x, _locationInWindow.y];
        case WebEventScrollWheel:
            return [NSString stringWithFormat:@"location: (%f, %f) deltaX: %f deltaY: %f", _locationInWindow.x, _locationInWindow.y, _deltaX, _deltaY];
        case WebEventKeyDown:
        case WebEventKeyUp:
            return [NSString stringWithFormat:@"chars: %@ charsNoModifiers: %@ flags: %d repeating: %d keyboardFlags: %lu keyCode %d, isTab: %d charSet: %@", _characters, _charactersIgnoringModifiers, _modifierFlags, _keyRepeating, static_cast<unsigned long>(_keyboardFlags), _keyCode, _tabKey, [self _characterSetDescription]];
        case WebEventTouchBegin:
        case WebEventTouchChange:
        case WebEventTouchEnd:
        case WebEventTouchCancel:
            return [NSString stringWithFormat:@"location: (%f, %f) count: %d locations: %@ identifiers: %@ phases: %@ isGesture: %d scale: %f rotation: %f", _locationInWindow.x, _locationInWindow.y, _touchCount, [self _touchLocationsDescription:_touchLocations], [self _touchIdentifiersDescription], [self _touchPhasesDescription], (_isGesture ? 1 : 0), _gestureScale, _gestureRotation];
        default:
            ASSERT_NOT_REACHED();
    }
    return @"Unknown";
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"%@ type: %@ - %@", [super description], [self _typeDescription], [self _eventDescription]];
}

- (CGPoint)locationInWindow
{
    ASSERT_WITH_MESSAGE(_type == WebEventMouseDown || _type == WebEventMouseUp || _type == WebEventMouseMoved || _type == WebEventScrollWheel
                        // FIXME: <rdar://problem/7185284> TouchEvents may be in more than one window some day.
                        || _type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel
                        , "WebEventType: %d", _type);
    return _locationInWindow;
}

- (NSString *)characters
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return [[_characters retain] autorelease];
}

- (NSString *)charactersIgnoringModifiers
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return [[_charactersIgnoringModifiers retain] autorelease];
}

- (WebEventFlags)modifierFlags
{
    return _modifierFlags;
}

- (BOOL)isKeyRepeating
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return _keyRepeating;
}

// FIXME: to be removed
- (BOOL)isPopupVariant
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return _popupVariant;
}

- (NSUInteger)keyboardFlags
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return _keyboardFlags;
}

- (uint16_t)keyCode
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return _keyCode;
}

- (BOOL)isTabKey
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return _tabKey;
}

- (WebEventCharacterSet)characterSet
{
    ASSERT(_type == WebEventKeyDown || _type == WebEventKeyUp);
    return _characterSet;
}

- (float)deltaX
{
    ASSERT(_type == WebEventScrollWheel);
    return _deltaX;
}

- (float)deltaY
{
    ASSERT(_type == WebEventScrollWheel);
    return _deltaY;
}

// Touch
- (unsigned)touchCount
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _touchCount;
}

- (NSArray *)touchLocations
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _touchLocations;
}

- (NSArray *)touchIdentifiers
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _touchIdentifiers;
}

- (NSArray *)touchPhases
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _touchPhases;
}

// Gesture
- (BOOL)isGesture
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _isGesture;
}

- (float)gestureScale
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _gestureScale;
}

- (float)gestureRotation
{
    ASSERT(_type == WebEventTouchBegin || _type == WebEventTouchChange || _type == WebEventTouchEnd || _type == WebEventTouchCancel);
    return _gestureRotation;
}

@end

#endif // PLATFORM(IOS)
