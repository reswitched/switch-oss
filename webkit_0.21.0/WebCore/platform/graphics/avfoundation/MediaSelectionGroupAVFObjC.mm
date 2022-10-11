/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#import "config.h"
#import "MediaSelectionGroupAVFObjC.h"

#if ENABLE(VIDEO_TRACK)

#import "Language.h"
#import "SoftLinking.h"
#import <AVFoundation/AVAsset.h>
#import <AVFoundation/AVMediaSelectionGroup.h>
#import <AVFoundation/AVPlayerItem.h>
#import <objc/runtime.h>
#import <wtf/HashMap.h>
#import <wtf/HashSet.h>
#import <wtf/text/WTFString.h>

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

SOFT_LINK_CLASS(AVFoundation, AVMediaSelectionGroup)
SOFT_LINK_CLASS(AVFoundation, AVMediaSelectionOption)

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
#include <MediaAccessibility/MediaAccessibility.h>
#include "MediaAccessibilitySoftLink.h"
#endif

namespace WebCore {

PassRefPtr<MediaSelectionOptionAVFObjC> MediaSelectionOptionAVFObjC::create(MediaSelectionGroupAVFObjC& group, AVMediaSelectionOption *option)
{
    return adoptRef(new MediaSelectionOptionAVFObjC(group, option));
}

MediaSelectionOptionAVFObjC::MediaSelectionOptionAVFObjC(MediaSelectionGroupAVFObjC& group, AVMediaSelectionOption *option)
    : m_group(&group)
    , m_mediaSelectionOption(option)
{
}

void MediaSelectionOptionAVFObjC::setSelected(bool selected)
{
    if (!m_group)
        return;

    if (selected == this->selected())
        return;

    m_group->setSelectedOption(selected ? this : nullptr);
}

bool MediaSelectionOptionAVFObjC::selected() const
{
    if (!m_group)
        return false;
    return this == m_group->selectedOption();
}

int MediaSelectionOptionAVFObjC::index() const
{
    if (!m_group)
        return 0;

    return [[m_group->avMediaSelectionGroup() options] indexOfObject:m_mediaSelectionOption.get()];
}

PassRefPtr<MediaSelectionGroupAVFObjC> MediaSelectionGroupAVFObjC::create(AVPlayerItem *item, AVMediaSelectionGroup *group, const Vector<String>& characteristics)
{
    return adoptRef(new MediaSelectionGroupAVFObjC(item, group, characteristics));
}

MediaSelectionGroupAVFObjC::MediaSelectionGroupAVFObjC(AVPlayerItem *item, AVMediaSelectionGroup *group, const Vector<String>& characteristics)
    : m_playerItem(item)
    , m_mediaSelectionGroup(group)
    , m_selectionTimer(*this, &MediaSelectionGroupAVFObjC::selectionTimerFired)
{
    updateOptions(characteristics);
}

MediaSelectionGroupAVFObjC::~MediaSelectionGroupAVFObjC()
{
    for (auto& option : m_options.values())
        option->clearGroup();
}

void MediaSelectionGroupAVFObjC::updateOptions(const Vector<String>& characteristics)
{
    RetainPtr<NSSet> newAVOptions = adoptNS([[NSSet alloc] initWithArray:[getAVMediaSelectionGroupClass() playableMediaSelectionOptionsFromArray:[m_mediaSelectionGroup options]]]);
    RetainPtr<NSMutableSet> oldAVOptions = adoptNS([[NSMutableSet alloc] initWithCapacity:m_options.size()]);
    for (auto& avOption : m_options.keys())
        [oldAVOptions addObject:avOption];

    RetainPtr<NSMutableSet> addedAVOptions = adoptNS([newAVOptions mutableCopy]);
    [addedAVOptions minusSet:oldAVOptions.get()];

    RetainPtr<NSMutableSet> removedAVOptions = adoptNS([oldAVOptions mutableCopy]);
    [removedAVOptions minusSet:newAVOptions.get()];

    for (AVMediaSelectionOption* removedAVOption in removedAVOptions.get()) {
        if (m_selectedOption && removedAVOption == m_selectedOption->avMediaSelectionOption())
            m_selectedOption = nullptr;

        m_options.remove(removedAVOption);
    }

    AVMediaSelectionOption* selectedOption = [m_playerItem selectedMediaOptionInMediaSelectionGroup:m_mediaSelectionGroup.get()];

    for (AVMediaSelectionOption* addedAVOption in addedAVOptions.get()) {
        RefPtr<MediaSelectionOptionAVFObjC> addedOption = MediaSelectionOptionAVFObjC::create(*this, addedAVOption);
        if (addedAVOption == selectedOption)
            m_selectedOption = addedOption.get();
        m_options.set(addedAVOption, addedOption.release());
    }

    if (!m_shouldSelectOptionAutomatically)
        return;

    RetainPtr<NSMutableArray> nsLanguages = adoptNS([[NSMutableArray alloc] initWithCapacity:userPreferredLanguages().size()]);
    for (auto& language : userPreferredLanguages())
        [nsLanguages addObject:(NSString*)language];
    NSArray* filteredOptions = [getAVMediaSelectionGroupClass() mediaSelectionOptionsFromArray:[m_mediaSelectionGroup options] filteredAndSortedAccordingToPreferredLanguages:nsLanguages.get()];

    if (![filteredOptions count] && characteristics.isEmpty())
        return;

    // If no options match our language selection, search for matching characteristics across all the group's options
    if (![filteredOptions count])
        filteredOptions = [m_mediaSelectionGroup options];

    RetainPtr<NSMutableArray> nsCharacteristics = adoptNS([[NSMutableArray alloc] initWithCapacity:characteristics.size()]);
    for (auto& characteristic : characteristics)
        [nsCharacteristics addObject:(NSString *)characteristic];

    NSArray* optionsWithCharacteristics = [getAVMediaSelectionGroupClass() mediaSelectionOptionsFromArray:filteredOptions withMediaCharacteristics:nsCharacteristics.get()];
    if (optionsWithCharacteristics && [optionsWithCharacteristics count])
        filteredOptions = optionsWithCharacteristics;

    if (![filteredOptions count])
        return;

    AVMediaSelectionOption* preferredOption = [filteredOptions objectAtIndex:0];
    if (m_selectedOption && m_selectedOption->avMediaSelectionOption() == preferredOption)
        return;

    ASSERT(m_options.contains(preferredOption));
    m_selectedOption = m_options.get(preferredOption);
    m_selectionTimer.startOneShot(0);
}

void MediaSelectionGroupAVFObjC::setSelectedOption(MediaSelectionOptionAVFObjC* option)
{
    if (m_selectedOption == option)
        return;

    m_shouldSelectOptionAutomatically = false;
    m_selectedOption = option;
    if (m_selectionTimer.isActive())
        m_selectionTimer.stop();
    m_selectionTimer.startOneShot(0);
}

void MediaSelectionGroupAVFObjC::selectionTimerFired()
{
    [m_playerItem selectMediaOption:(m_selectedOption ? m_selectedOption->avMediaSelectionOption() : nil) inMediaSelectionGroup:m_mediaSelectionGroup.get()];
}

}

#endif
