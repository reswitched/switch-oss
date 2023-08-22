/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2012 Apple Inc. All rights reserved.
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

#import "config.h"
#import "AXObjectCache.h"

#if HAVE(ACCESSIBILITY)

#import "AccessibilityObject.h"
#import "AccessibilityTable.h"
#import "RenderObject.h"
#import "WebAccessibilityObjectWrapperMac.h"
#import "WebCoreSystemInterface.h"

#import <wtf/PassRefPtr.h>

#ifndef NSAccessibilityLiveRegionChangedNotification
#define NSAccessibilityLiveRegionChangedNotification @"AXLiveRegionChanged"
#endif

#ifndef NSAccessibilityLiveRegionCreatedNotification 
#define NSAccessibilityLiveRegionCreatedNotification @"AXLiveRegionCreated"
#endif

#ifndef NSAccessibilityTextStateChangeTypeKey
#define NSAccessibilityTextStateChangeTypeKey @"AXTextStateChangeType"
#endif

#ifndef NSAccessibilityTextStateSyncKey
#define NSAccessibilityTextStateSyncKey @"AXTextStateSync"
#endif

#ifndef NSAccessibilityTextSelectionDirection
#define NSAccessibilityTextSelectionDirection @"AXTextSelectionDirection"
#endif

#ifndef NSAccessibilityTextSelectionGranularity
#define NSAccessibilityTextSelectionGranularity @"AXTextSelectionGranularity"
#endif

#ifndef NSAccessibilityTextSelectionChangedFocus
#define NSAccessibilityTextSelectionChangedFocus @"AXTextSelectionChangedFocus"
#endif

#ifndef NSAccessibilityTextEditType
#define NSAccessibilityTextEditType @"AXTextEditType"
#endif

#ifndef NSAccessibilityTextChangeValues
#define NSAccessibilityTextChangeValues @"AXTextChangeValues"
#endif

#ifndef NSAccessibilityTextChangeValue
#define NSAccessibilityTextChangeValue @"AXTextChangeValue"
#endif

#ifndef NSAccessibilityTextChangeValueLength
#define NSAccessibilityTextChangeValueLength @"AXTextChangeValueLength"
#endif

#ifndef NSAccessibilityTextChangeValueStartMarker
#define NSAccessibilityTextChangeValueStartMarker @"AXTextChangeValueStartMarker"
#endif

#ifndef NSAccessibilityTextChangeElement
#define NSAccessibilityTextChangeElement @"AXTextChangeElement"
#endif

#ifndef NSAccessibilitySelectedTextMarkerRangeAttribute
#define NSAccessibilitySelectedTextMarkerRangeAttribute @"AXSelectedTextMarkerRange"
#endif

// Very large strings can negatively impact the performance of notifications, so this length is chosen to try to fit an average paragraph or line of text, but not allow strings to be large enough to hurt performance.
static const NSUInteger AXValueChangeTruncationLength = 1000;

// The simple Cocoa calls in this file don't throw exceptions.

namespace WebCore {

void AXObjectCache::detachWrapper(AccessibilityObject* obj, AccessibilityDetachmentType)
{
    [obj->wrapper() detach];
    obj->setWrapper(nullptr);
}

void AXObjectCache::attachWrapper(AccessibilityObject* obj)
{
    RetainPtr<WebAccessibilityObjectWrapper> wrapper = adoptNS([[WebAccessibilityObjectWrapper alloc] initWithAccessibilityObject:obj]);
    obj->setWrapper(wrapper.get());
}

static BOOL axShouldRepostNotificationsForTests = false;

void AXObjectCache::setShouldRepostNotificationsForTests(bool value)
{
    axShouldRepostNotificationsForTests = value;
}

static void AXPostNotificationWithUserInfo(id object, NSString *notification, id userInfo)
{
    NSAccessibilityPostNotificationWithUserInfo(object, notification, userInfo);
    // To simplify monitoring for notifications in tests, repost as a simple NSNotification instead of forcing test infrastucture to setup an IPC client and do all the translation between WebCore types and platform specific IPC types and back
    if (UNLIKELY(axShouldRepostNotificationsForTests))
        [object accessibilityPostedNotification:notification userInfo:userInfo];
}

void AXObjectCache::postPlatformNotification(AccessibilityObject* obj, AXNotification notification)
{
    if (!obj)
        return;
    
    // Some notifications are unique to Safari and do not have NSAccessibility equivalents.
    NSString *macNotification;
    switch (notification) {
        case AXActiveDescendantChanged:
            // An active descendant change for trees means a selected rows change.
            if (obj->isTree())
                macNotification = NSAccessibilitySelectedRowsChangedNotification;
            
            // When a combobox uses active descendant, it means the selected item in its associated
            // list has changed. In these cases we should use selected children changed, because
            // we don't want the focus to change away from the combobox where the user is typing.
            else if (obj->isComboBox())
                macNotification = NSAccessibilitySelectedChildrenChangedNotification;
            else
                macNotification = NSAccessibilityFocusedUIElementChangedNotification;                
            break;
        case AXAutocorrectionOccured:
            macNotification = @"AXAutocorrectionOccurred";
            break;
        case AXFocusedUIElementChanged:
            macNotification = NSAccessibilityFocusedUIElementChangedNotification;
            break;
        case AXLayoutComplete:
            macNotification = @"AXLayoutComplete";
            break;
        case AXLoadComplete:
            macNotification = @"AXLoadComplete";
            break;
        case AXInvalidStatusChanged:
            macNotification = @"AXInvalidStatusChanged";
            break;
        case AXSelectedChildrenChanged:
            if (is<AccessibilityTable>(*obj) && downcast<AccessibilityTable>(*obj).isExposableThroughAccessibility())
                macNotification = NSAccessibilitySelectedRowsChangedNotification;
            else
                macNotification = NSAccessibilitySelectedChildrenChangedNotification;
            break;
        case AXSelectedTextChanged:
            macNotification = NSAccessibilitySelectedTextChangedNotification;
            break;
        case AXCheckedStateChanged:
        case AXValueChanged:
            macNotification = NSAccessibilityValueChangedNotification;
            break;
        case AXLiveRegionCreated:
            macNotification = NSAccessibilityLiveRegionCreatedNotification;
            break;
        case AXLiveRegionChanged:
            macNotification = NSAccessibilityLiveRegionChangedNotification;
            break;
        case AXRowCountChanged:
            macNotification = NSAccessibilityRowCountChangedNotification;
            break;
        case AXRowExpanded:
            macNotification = NSAccessibilityRowExpandedNotification;
            break;
        case AXRowCollapsed:
            macNotification = NSAccessibilityRowCollapsedNotification;
            break;
        case AXElementBusyChanged:
            macNotification = @"AXElementBusyChanged";
            break;
        case AXExpandedChanged:
            macNotification = @"AXExpandedChanged";
            break;
        case AXMenuClosed:
            macNotification = (id)kAXMenuClosedNotification;
            break;
        case AXMenuListItemSelected:
            macNotification = (id)kAXMenuItemSelectedNotification;
            break;
        case AXMenuOpened:
            macNotification = (id)kAXMenuOpenedNotification;
            break;
        default:
            return;
    }
    
    // NSAccessibilityPostNotification will call this method, (but not when running DRT), so ASSERT here to make sure it does not crash.
    // https://bugs.webkit.org/show_bug.cgi?id=46662
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    ASSERT([obj->wrapper() accessibilityIsIgnored] || true);
#pragma clang diagnostic pop

    AXPostNotificationWithUserInfo(obj->wrapper(), macNotification, nil);
}

void AXObjectCache::postTextStateChangePlatformNotification(AccessibilityObject* object, const AXTextStateChangeIntent& intent, const VisibleSelection& selection)
{
    if (!object)
        object = rootWebArea();

    if (!object)
        return;

    NSMutableDictionary *userInfo = [[NSMutableDictionary alloc] initWithCapacity:5];
    if (m_isSynchronizingSelection)
        [userInfo setObject:[NSNumber numberWithBool:YES] forKey:NSAccessibilityTextStateSyncKey];
    if (intent.type != AXTextStateChangeTypeUnknown) {
        [userInfo setObject:[NSNumber numberWithInt:intent.type] forKey:NSAccessibilityTextStateChangeTypeKey];
        switch (intent.type) {
        case AXTextStateChangeTypeSelectionMove:
        case AXTextStateChangeTypeSelectionExtend:
            [userInfo setObject:[NSNumber numberWithInt:intent.selection.direction] forKey:NSAccessibilityTextSelectionDirection];
            switch (intent.selection.direction) {
            case AXTextSelectionDirectionUnknown:
                break;
            case AXTextSelectionDirectionBeginning:
            case AXTextSelectionDirectionEnd:
            case AXTextSelectionDirectionPrevious:
            case AXTextSelectionDirectionNext:
                [userInfo setObject:[NSNumber numberWithInt:intent.selection.granularity] forKey:NSAccessibilityTextSelectionGranularity];
                break;
            case AXTextSelectionDirectionDiscontiguous:
                break;
            }
            break;
        case AXTextStateChangeTypeUnknown:
        case AXTextStateChangeTypeEdit:
            break;
        }
        if (intent.selection.focusChange)
            [userInfo setObject:[NSNumber numberWithBool:intent.selection.focusChange] forKey:NSAccessibilityTextSelectionChangedFocus];
    }
    if (!selection.isNone()) {
        if (id textMarkerRange = [object->wrapper() textMarkerRangeFromVisiblePositions:selection.visibleStart() endPosition:selection.visibleEnd()])
            [userInfo setObject:textMarkerRange forKey:NSAccessibilitySelectedTextMarkerRangeAttribute];
    }

    if (id wrapper = object->wrapper())
        [userInfo setObject:wrapper forKey:NSAccessibilityTextChangeElement];

    AXPostNotificationWithUserInfo(rootWebArea()->wrapper(), NSAccessibilitySelectedTextChangedNotification, userInfo);
    AXPostNotificationWithUserInfo(object->wrapper(), NSAccessibilitySelectedTextChangedNotification, userInfo);

    [userInfo release];
}

static NSDictionary *textReplacementChangeDictionary(AccessibilityObject* object, AXTextEditType type, const String& string, const VisiblePosition& position)
{
    NSString *text = (NSString *)string;
    NSUInteger length = [text length];
    if (!length)
        return nil;
    NSMutableDictionary *change = [[NSMutableDictionary alloc] initWithCapacity:4];
    [change setObject:[NSNumber numberWithInt:type] forKey:NSAccessibilityTextEditType];
    if (length > AXValueChangeTruncationLength) {
        [change setObject:[NSNumber numberWithInt:length] forKey:NSAccessibilityTextChangeValueLength];
        text = [text substringToIndex:AXValueChangeTruncationLength];
    }
    [change setObject:text forKey:NSAccessibilityTextChangeValue];
    if (position.isNotNull()) {
        if (id textMarker = [object->wrapper() textMarkerForVisiblePosition:position])
            [change setObject:textMarker forKey:NSAccessibilityTextChangeValueStartMarker];
    }
    return [change autorelease];
}

void AXObjectCache::postTextStateChangePlatformNotification(AccessibilityObject* object, AXTextEditType type, const String& text, const VisiblePosition& position)
{
    if (!text.length())
        return;

    postTextReplacementPlatformNotification(object, AXTextEditTypeUnknown, "", type, text, position);
}

void AXObjectCache::postTextReplacementPlatformNotification(AccessibilityObject* object, AXTextEditType deletionType, const String& deletedText, AXTextEditType insertionType, const String& insertedText, const VisiblePosition& position)
{
    if (!object)
        object = rootWebArea();

    if (!object)
        return;

    NSMutableDictionary *userInfo = [[NSMutableDictionary alloc] initWithCapacity:4];
    [userInfo setObject:@(AXTextStateChangeTypeEdit) forKey:NSAccessibilityTextStateChangeTypeKey];

    NSMutableArray *changes = [[NSMutableArray alloc] initWithCapacity:2];
    if (NSDictionary *change = textReplacementChangeDictionary(object, deletionType, deletedText, position))
        [changes addObject:change];
    if (NSDictionary *change = textReplacementChangeDictionary(object, insertionType, insertedText, position))
        [changes addObject:change];
    if (changes.count)
        [userInfo setObject:changes forKey:NSAccessibilityTextChangeValues];
    [changes release];

    if (id wrapper = object->wrapper())
        [userInfo setObject:wrapper forKey:NSAccessibilityTextChangeElement];

    AXPostNotificationWithUserInfo(rootWebArea()->wrapper(), NSAccessibilityValueChangedNotification, userInfo);
    AXPostNotificationWithUserInfo(object->wrapper(), NSAccessibilityValueChangedNotification, userInfo);

    [userInfo release];
}

void AXObjectCache::frameLoadingEventPlatformNotification(AccessibilityObject* axFrameObject, AXLoadingEvent loadingEvent)
{
    if (!axFrameObject)
        return;
    
    if (loadingEvent == AXLoadingFinished && axFrameObject->document() == axFrameObject->topDocument())
        postPlatformNotification(axFrameObject, AXLoadComplete);
}

void AXObjectCache::platformHandleFocusedUIElementChanged(Node*, Node*)
{
    wkAccessibilityHandleFocusChanged();
    // AXFocusChanged is a test specific notification name and not something a real AT will be listening for
    if (UNLIKELY(axShouldRepostNotificationsForTests))
        [rootWebArea()->wrapper() accessibilityPostedNotification:@"AXFocusChanged" userInfo:nil];
}

void AXObjectCache::handleScrolledToAnchor(const Node*)
{
}

}

#endif // HAVE(ACCESSIBILITY)
