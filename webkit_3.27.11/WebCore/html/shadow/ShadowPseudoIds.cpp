/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
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

#include "config.h"
#include "ShadowPseudoIds.h"

#include <wtf/NeverDestroyed.h>
#include <wtf/text/AtomString.h>

namespace WebCore {

namespace ShadowPseudoIds {

const AtomString& cue()
{
    static MainThreadNeverDestroyed<const AtomString> cue("cue", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (cue.isNull())
        cue.construct("cue", AtomString::ConstructFromLiteral);
#endif
    return cue;
}

const AtomString& fileSelectorButton()
{
    static MainThreadNeverDestroyed<const AtomString> fileSelectorButton("file-selector-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (fileSelectorButton.isNull())
        fileSelectorButton.construct("file-selector-button", AtomString::ConstructFromLiteral);
#endif
    return fileSelectorButton;
}

const AtomString& placeholder()
{
    static MainThreadNeverDestroyed<const AtomString> placeholder("placeholder", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (placeholder.isNull())
        placeholder.construct("placeholder", AtomString::ConstructFromLiteral);
#endif
    return placeholder;
}

const AtomString& webkitContactsAutoFillButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitContactsAutoFillButton("-webkit-contacts-auto-fill-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitContactsAutoFillButton.isNull())
        webkitContactsAutoFillButton.construct("-webkit-contacts-auto-fill-button", AtomString::ConstructFromLiteral);
#endif
    return webkitContactsAutoFillButton;
}

const AtomString& webkitCredentialsAutoFillButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitCredentialsAutoFillButton("-webkit-credentials-auto-fill-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitCredentialsAutoFillButton.isNull())
        webkitCredentialsAutoFillButton.construct("-webkit-credentials-auto-fill-button", AtomString::ConstructFromLiteral);
#endif
    return webkitCredentialsAutoFillButton;
}

const AtomString& webkitCreditCardAutoFillButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitCreditCardAutoFillButton("-webkit-credit-card-auto-fill-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitCreditCardAutoFillButton.isNull())
        webkitCreditCardAutoFillButton.construct("-webkit-credit-card-auto-fill-button", AtomString::ConstructFromLiteral);
#endif
    return webkitCreditCardAutoFillButton;
}

const AtomString& webkitStrongPasswordAutoFillButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitStrongPasswordAutoFillButton("-webkit-strong-password-auto-fill-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitStrongPasswordAutoFillButton.isNull())
        webkitStrongPasswordAutoFillButton.construct("-webkit-strong-password-auto-fill-button", AtomString::ConstructFromLiteral);
#endif
    return webkitStrongPasswordAutoFillButton;
}

const AtomString& webkitCapsLockIndicator()
{
    static MainThreadNeverDestroyed<const AtomString> webkitCapsLockIndicator("-webkit-caps-lock-indicator", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitCapsLockIndicator.isNull())
        webkitCapsLockIndicator.construct("-webkit-caps-lock-indicator", AtomString::ConstructFromLiteral);
#endif
    return webkitCapsLockIndicator;
}

const AtomString& webkitColorSwatch()
{
    static MainThreadNeverDestroyed<const AtomString> webkitColorSwatch("-webkit-color-swatch", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitColorSwatch.isNull())
        webkitColorSwatch.construct("-webkit-color-swatch", AtomString::ConstructFromLiteral);
#endif
    return webkitColorSwatch;
}

const AtomString& webkitColorSwatchWrapper()
{
    static MainThreadNeverDestroyed<const AtomString> webkitColorSwatchWrapper("-webkit-color-swatch-wrapper", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitColorSwatchWrapper.isNull())
        webkitColorSwatchWrapper.construct("-webkit-color-swatch-wrapper", AtomString::ConstructFromLiteral);
#endif
    return webkitColorSwatchWrapper;
}

const AtomString& webkitDatetimeEdit()
{
    static MainThreadNeverDestroyed<const AtomString> webkitDatetimeEdit("-webkit-datetime-edit", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitDatetimeEdit.isNull())
        webkitDatetimeEdit.construct("-webkit-datetime-edit", AtomString::ConstructFromLiteral);
#endif
    return webkitDatetimeEdit;
}

const AtomString& webkitDatetimeEditText()
{
    static MainThreadNeverDestroyed<const AtomString> webkitDatetimeEditText("-webkit-datetime-edit-text", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitDatetimeEditText.isNull())
        webkitDatetimeEditText.construct("-webkit-datetime-edit-text", AtomString::ConstructFromLiteral);
#endif
    return webkitDatetimeEditText;
}

const AtomString& webkitDatetimeEditFieldsWrapper()
{
    static MainThreadNeverDestroyed<const AtomString> webkitDatetimeEditFieldsWrapper("-webkit-datetime-edit-fields-wrapper", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitDatetimeEditFieldsWrapper.isNull())
        webkitDatetimeEditFieldsWrapper.construct("-webkit-datetime-edit-fields-wrapper", AtomString::ConstructFromLiteral);
#endif
    return webkitDatetimeEditFieldsWrapper;
}

const AtomString& webkitDateAndTimeValue()
{
    static MainThreadNeverDestroyed<const AtomString> webkitDateAndTimeValue("-webkit-date-and-time-value", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitDateAndTimeValue.isNull())
        webkitDateAndTimeValue.construct("-webkit-date-and-time-value", AtomString::ConstructFromLiteral);
#endif
    return webkitDateAndTimeValue;
}

const AtomString& webkitDetailsMarker()
{
    static MainThreadNeverDestroyed<const AtomString> webkitDetailsMarker("-webkit-details-marker", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitDetailsMarker.isNull())
        webkitDetailsMarker.construct("-webkit-details-marker", AtomString::ConstructFromLiteral);
#endif
    return webkitDetailsMarker;
}

const AtomString& webkitGenericCueRoot()
{
    static MainThreadNeverDestroyed<const AtomString> webkitGenericCueRoot("-webkit-generic-cue-root", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitGenericCueRoot.isNull())
        webkitGenericCueRoot.construct("-webkit-generic-cue-root", AtomString::ConstructFromLiteral);
#endif
    return webkitGenericCueRoot;
}

const AtomString& webkitInnerSpinButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitInnerSpinButton("-webkit-inner-spin-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitInnerSpinButton.isNull())
        webkitInnerSpinButton.construct("-webkit-inner-spin-button", AtomString::ConstructFromLiteral);
#endif
    return webkitInnerSpinButton;
}

const AtomString& webkitKeygenSelect()
{
    static MainThreadNeverDestroyed<const AtomString> webkitKeygenSelect("-webkit-keygen-select", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitKeygenSelect.isNull())
        webkitKeygenSelect.construct("-webkit-keygen-select", AtomString::ConstructFromLiteral);
#endif
    return webkitKeygenSelect;
}

const AtomString& webkitListButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitListButton("-webkit-list-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitListButton.isNull())
        webkitListButton.construct("-webkit-list-button", AtomString::ConstructFromLiteral);
#endif
    return webkitListButton;
}

const AtomString& webkitMediaSliderThumb()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaSliderThumb("-webkit-media-slider-thumb", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaSliderThumb.isNull())
        webkitMediaSliderThumb.construct("-webkit-media-slider-thumb", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaSliderThumb;
}

const AtomString& webkitMediaSliderContainer()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaSliderContainer("-webkit-media-slider-container", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaSliderContainer.isNull())
        webkitMediaSliderContainer.construct("-webkit-media-slider-container", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaSliderContainer;
}

const AtomString& webkitMediaTextTrackContainer()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaTextTrackContainer("-webkit-media-text-track-container", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaTextTrackContainer.isNull())
        webkitMediaTextTrackContainer.construct("-webkit-media-text-track-container", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaTextTrackContainer;
}

const AtomString& webkitMediaTextTrackDisplay()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaTextTrackDisplay("-webkit-media-text-track-display", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaTextTrackDisplay.isNull())
        webkitMediaTextTrackDisplay.construct("-webkit-media-text-track-display", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaTextTrackDisplay;
}

const AtomString& webkitMediaTextTrackDisplayBackdrop()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaTextTrackDisplay("-webkit-media-text-track-display-backdrop", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaTextTrackDisplay.isNull())
        webkitMediaTextTrackDisplay.construct("-webkit-media-text-track-display-backdrop", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaTextTrackDisplay;
}

const AtomString& webkitMediaTextTrackRegion()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaTextTrackRegion("-webkit-media-text-track-region", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaTextTrackRegion.isNull())
        webkitMediaTextTrackRegion.construct("-webkit-media-text-track-region", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaTextTrackRegion;
}

const AtomString& webkitMediaTextTrackRegionContainer()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMediaTextTrackRegionContainer("-webkit-media-text-track-region-container", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMediaTextTrackRegionContainer.isNull())
        webkitMediaTextTrackRegionContainer.construct("-webkit-media-text-track-region-container", AtomString::ConstructFromLiteral);
#endif
    return webkitMediaTextTrackRegionContainer;
}

const AtomString& webkitMeterBar()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMeterBar("-webkit-meter-bar", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMeterBar.isNull())
        webkitMeterBar.construct("-webkit-meter-bar", AtomString::ConstructFromLiteral);
#endif
    return webkitMeterBar;
}

const AtomString& webkitMeterInnerElement()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMeterInnerElement("-webkit-meter-inner-element", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMeterInnerElement.isNull())
        webkitMeterInnerElement.construct("-webkit-meter-inner-element", AtomString::ConstructFromLiteral);
#endif
    return webkitMeterInnerElement;
}

const AtomString& webkitMeterOptimumValue()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMeterOptimumValue("-webkit-meter-optimum-value", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMeterOptimumValue.isNull())
        webkitMeterOptimumValue.construct("-webkit-meter-optimum-value", AtomString::ConstructFromLiteral);
#endif
    return webkitMeterOptimumValue;
}

const AtomString& webkitMeterSuboptimumValue()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMeterSuboptimumValue("-webkit-meter-suboptimum-value", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMeterSuboptimumValue.isNull())
        webkitMeterSuboptimumValue.construct("-webkit-meter-suboptimum-value", AtomString::ConstructFromLiteral);
#endif
    return webkitMeterSuboptimumValue;
}

const AtomString& webkitMeterEvenLessGoodValue()
{
    static MainThreadNeverDestroyed<const AtomString> webkitMeterEvenLessGoodValue("-webkit-meter-even-less-good-value", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitMeterEvenLessGoodValue.isNull())
        webkitMeterEvenLessGoodValue.construct("-webkit-meter-even-less-good-value", AtomString::ConstructFromLiteral);
#endif
    return webkitMeterEvenLessGoodValue;
}

const AtomString& webkitPluginReplacement()
{
    static MainThreadNeverDestroyed<const AtomString> webkitPluginReplacement("-webkit-plugin-replacement", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitPluginReplacement.isNull())
        webkitPluginReplacement.construct("-webkit-plugin-replacement", AtomString::ConstructFromLiteral);
#endif
    return webkitPluginReplacement;
}

const AtomString& webkitProgressBar()
{
    static MainThreadNeverDestroyed<const AtomString> webkitProgressBar("-webkit-progress-bar", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitProgressBar.isNull())
        webkitProgressBar.construct("-webkit-progress-bar", AtomString::ConstructFromLiteral);
#endif
    return webkitProgressBar;
}

const AtomString& webkitProgressValue()
{
    static MainThreadNeverDestroyed<const AtomString> webkitProgressValue("-webkit-progress-value", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitProgressValue.isNull())
        webkitProgressValue.construct("-webkit-progress-value", AtomString::ConstructFromLiteral);
#endif
    return webkitProgressValue;
}

const AtomString& webkitProgressInnerElement()
{
    static MainThreadNeverDestroyed<const AtomString> webkitProgressInnerElement("-webkit-progress-inner-element", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitProgressInnerElement.isNull())
        webkitProgressInnerElement.construct("-webkit-progress-inner-element", AtomString::ConstructFromLiteral);
#endif
    return webkitProgressInnerElement;
}

const AtomString& webkitSearchDecoration()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSearchDecoration("-webkit-search-decoration", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSearchDecoration.isNull())
        webkitSearchDecoration.construct("-webkit-search-decoration", AtomString::ConstructFromLiteral);
#endif
    return webkitSearchDecoration;
}

const AtomString& webkitSearchResultsButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSearchResultsButton("-webkit-search-results-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSearchResultsButton.isNull())
        webkitSearchResultsButton.construct("-webkit-search-results-button", AtomString::ConstructFromLiteral);
#endif
    return webkitSearchResultsButton;
}

const AtomString& webkitSearchResultsDecoration()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSearchResultsDecoration("-webkit-search-results-decoration", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSearchResultsDecoration.isNull())
        webkitSearchResultsDecoration.construct("-webkit-search-results-decoration", AtomString::ConstructFromLiteral);
#endif
    return webkitSearchResultsDecoration;
}

const AtomString& webkitSearchCancelButton()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSearchCancelButton("-webkit-search-cancel-button", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSearchCancelButton.isNull())
        webkitSearchCancelButton.construct("-webkit-search-cancel-button", AtomString::ConstructFromLiteral);
#endif
    return webkitSearchCancelButton;
}

const AtomString& webkitSliderRunnableTrack()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSliderRunnableTrack("-webkit-slider-runnable-track", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSliderRunnableTrack.isNull())
        webkitSliderRunnableTrack.construct("-webkit-slider-runnable-track", AtomString::ConstructFromLiteral);
#endif
    return webkitSliderRunnableTrack;
}

const AtomString& webkitSliderThumb()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSliderThumb("-webkit-slider-thumb", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSliderThumb.isNull())
        webkitSliderThumb.construct("-webkit-slider-thumb", AtomString::ConstructFromLiteral);
#endif
    return webkitSliderThumb;
}

const AtomString& webkitSliderContainer()
{
    static MainThreadNeverDestroyed<const AtomString> webkitSliderContainer("-webkit-slider-container", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitSliderContainer.isNull())
        webkitSliderContainer.construct("-webkit-slider-container", AtomString::ConstructFromLiteral);
#endif
    return webkitSliderContainer;
}

const AtomString& webkitTextfieldDecorationContainer()
{
    static MainThreadNeverDestroyed<const AtomString> webkitTextfieldDecorationContainer("-webkit-textfield-decoration-container", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitTextfieldDecorationContainer.isNull())
        webkitTextfieldDecorationContainer.construct("-webkit-textfield-decoration-container", AtomString::ConstructFromLiteral);
#endif
    return webkitTextfieldDecorationContainer;
}

const AtomString& webkitValidationBubble()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubble("-webkit-validation-bubble", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubble.isNull())
        webkitValidationBubble.construct("-webkit-validation-bubble", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubble;
}

const AtomString& webkitValidationBubbleArrowClipper()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleArrowClipper("-webkit-validation-bubble-arrow-clipper", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleArrowClipper.isNull())
        webkitValidationBubbleArrowClipper.construct("-webkit-validation-bubble-arrow-clipper", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleArrowClipper;
}

const AtomString& webkitValidationBubbleArrow()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleArrow("-webkit-validation-bubble-arrow", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleArrow.isNull())
        webkitValidationBubbleArrow.construct("-webkit-validation-bubble-arrow", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleArrow;
}

const AtomString& webkitValidationBubbleMessage()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleMessage("-webkit-validation-bubble-message", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleMessage.isNull())
        webkitValidationBubbleMessage.construct("-webkit-validation-bubble-message", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleMessage;
}

const AtomString& webkitValidationBubbleIcon()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleIcon("-webkit-validation-bubble-icon", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleIcon.isNull())
        webkitValidationBubbleIcon.construct("-webkit-validation-bubble-icon", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleIcon;
}

const AtomString& webkitValidationBubbleTextBlock()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleTextBlock("-webkit-validation-bubble-text-block", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleTextBlock.isNull())
        webkitValidationBubbleTextBlock.construct("-webkit-validation-bubble-text-block", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleTextBlock;
}

const AtomString& webkitValidationBubbleHeading()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleHeading("-webkit-validation-bubble-heading", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleHeading.isNull())
        webkitValidationBubbleHeading.construct("-webkit-validation-bubble-heading", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleHeading;
}

const AtomString& webkitValidationBubbleBody()
{
    static MainThreadNeverDestroyed<const AtomString> webkitValidationBubbleBody("-webkit-validation-bubble-body", AtomString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (webkitValidationBubbleBody.isNull())
        webkitValidationBubbleBody.construct("-webkit-validation-bubble-body", AtomString::ConstructFromLiteral);
#endif
    return webkitValidationBubbleBody;
}

} // namespace ShadowPseudoId

} // namespace WebCore
