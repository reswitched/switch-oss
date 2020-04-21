/*
 *  WKCSkin.h
 *
 *  Copyright (c) 2010-2018 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef WKCSkin_h
#define WKCSkin_h

#include <wkc/wkcbase.h>

namespace WKC {

/*@{*/

/** @brief Skin image types */
enum SkinImage {
    /** @brief Check box is unchecked */
    ESkinImageCheckboxUnchecked = 0,
    /** @brief Check box is checked */
    ESkinImageCheckboxChecked,
    /** @brief Check box is unchecked, and disabled */
    ESkinImageCheckboxUncheckedDisabled,
    /** @brief Check box is checked, and disabled */
    ESkinImageCheckboxCheckedDisabled,
    /** @brief Check box is pressed */
    ESkinImageCheckboxPressed,
    /** @brief Check box is unchecked, and has focus */
    ESkinImageCheckboxUncheckedFocused,
    /** @brief Check box is checked, and has focus */
    ESkinImageCheckboxCheckedFocused,
    /** @brief Radio button is not selected */
    ESkinImageRadioUnchecked,
    /** @brief Radio button is selected */
    ESkinImageRadioChecked,
    /** @brief Radio button is not selected, and disabled */
    ESkinImageRadioUncheckedDisabled,
    /** @brief Radio button is selected, and disabled */
    ESkinImageRadioCheckedDisabled,
    /** @brief Radio button is pressed */
    ESkinImageRadioPressed,
    /** @brief Radio button is not selected, and has focus */
    ESkinImageRadioUncheckedFocused,
    /** @brief Radio button is selected, and has focus */
    ESkinImageRadioCheckedFocused,
    /** @brief Normal state of button */
    ESkinImageButton,
    /** @brief Button is disabled */
    ESkinImageButtonDisabled,
    /** @brief Button is pressed */
    ESkinImageButtonPressed,
    /** @brief Button is hovered over */
    ESkinImageButtonHovered,
    /** @brief Normal state of menu list button */
    ESkinImageMenuListButton,
    /** @brief Menu list button is disabled */
    ESkinImageMenuListButtonDisabled,
    /** @brief Menu list button is pressed */
    ESkinImageMenuListButtonPressed,
    /** @brief Menu list button has focus */
    ESkinImageMenuListButtonFocused,
    /** @brief Normal state of vertical scroll bar background */
    ESkinImageVScrollbarBackground,
    /** @brief Vertical scroll bar background is disabled */
    ESkinImageVScrollbarBackgroundDisabled,
    /** @brief Normal state of vertical scroll bar "Thumb" */
    ESkinImageVScrollbarThumb,
    /** @brief Vertical scroll bar "Thumb" is hovered over */
    ESkinImageVScrollbarThumbHovered,
    /** @brief Normal state of vertical scroll bar "Up" */
    ESkinImageVScrollbarUp,
    /** @brief Vertical scroll bar "Up" is disabled */
    ESkinImageVScrollbarUpDisabled,
    /** @brief Vertical scroll bar "Up" is hovered over */
    ESkinImageVScrollbarUpHovered,
    /** @brief Normal state of vertical scroll bar "Down" */
    ESkinImageVScrollbarDown,
    /** @brief Vertical scroll bar "Down" is disabled */
    ESkinImageVScrollbarDownDisabled,
    /** @brief Vertical scroll bar "Down" is hovered over */
    ESkinImageVScrollbarDownHovered,
    /** @brief Normal state of horizontal scroll bar background */
    ESkinImageHScrollbarBackground,
    /** @brief Horizontal scroll bar background is disabled */
    ESkinImageHScrollbarBackgroundDisabled,
    /** @brief Normal state of horizontal scroll bar "Thumb" */
    ESkinImageHScrollbarThumb,
    /** @brief Horizontal scroll bar "Thumb" is hovered over */
    ESkinImageHScrollbarThumbHovered,
    /** @brief Normal state of horizontal scroll bar "Left" */
    ESkinImageHScrollbarLeft,
    /** @brief Horizontal scroll bar "Left" is disabled */
    ESkinImageHScrollbarLeftDisabled,
    /** @brief Horizontal scroll bar "Left" is hovered over */
    ESkinImageHScrollbarLeftHovered,
    /** @brief Normal state of horizontal scroll bar "Right" */
    ESkinImageHScrollbarRight,
    /** @brief Horizontal scroll bar "Right" is disabled */
    ESkinImageHScrollbarRightDisabled,
    /** @brief Horizontal scroll bar "Right" is hovered over */
    ESkinImageHScrollbarRightHovered,
    /** @brief Normal state of scroll bar cross corner */
    ESkinImageScrollbarCrossCorner,
    /** @brief Scroll bar cross corner is disabled */
    ESkinImageScrollbarCrossCornerDisabled,
    /** @brief Normal state of vertical "Range" control */
    ESkinImageVRange,
    /** @brief Vertical "Range" control is disabled */
    ESkinImageVRangeDisabled,
    /** @brief Vertical "Range" control is pressed */
    ESkinImageVRangePressed,
    /** @brief Vertical "Range" control is hovered over */
    ESkinImageVRangeHovered,
    /** @brief Normal state of horizontal "Range" control */
    ESkinImageHRange,
    /** @brief Horizontal "Range" control is disabled */
    ESkinImageHRangeDisabled,
    /** @brief Horizontal "Range" control is pressed */
    ESkinImageHRangePressed,
    /** @brief Horizontal "Range" control is hovered over */
    ESkinImageHRangeHovered,
    /** @brief Search cancel button in search field */
    ESkinImageSearchFieldCancelButton,
    /** @brief Search result button in search field */
    ESkinImageSearchFieldResultButton,

    /** @brief Total number of skin types and states */
    ESkinImages
};

/**
   @brief Skin color types
   @details
   For information about system colors, see "CSS Color Module Level 3" in @ref reference.
 */
enum SkinColor {
    // system colors
    /** @brief ActiveBorder color */
    ESkinColorActiveBorder = 0,
    /** @brief ActiveCaption color */
    ESkinColorActiveCaption,
    /** @brief AppWorkspace color */
    ESkinColorAppWorkSpace,
    /** @brief Background color */
    ESkinColorBackground,
    /** @brief ButtonFace color */
    ESkinColorButtonFace,
    /** @brief ButtonHighlight color */
    ESkinColorButtonHighlight,
    /** @brief ButtonShadow color */
    ESkinColorButtonShadow,
    /** @brief ButtonText color */
    ESkinColorButtonText,
    /** @brief CaptionText color */
    ESkinColorCaptionText,
    /** @brief GrayText color */
    ESkinColorGrayText,
    /** @var WKC::ESkinColorHighlight */
    ESkinColorHighlight,
    /** @brief HighlightText color */
    ESkinColorHighlightText,
    /** @brief Inactive border color */
    ESkinColorInactiveBorder,
    /** @brief InactiveCaption color */
    ESkinColorInactiveCaption,
    /** @brief InactiveCaptionText color */
    ESkinColorInactiveCaptionText,
    /** @brief InfoBackground color */
    ESkinColorInfoBackground,
    /** @brief InfoText color */
    ESkinColorInfoText,
    /** @brief Menu color */
    ESkinColorMenu,
    /** @brief MenuText color */
    ESkinColorMenuText,
    /** @brief Scrollbar color */
    ESkinColorScrollbar,
    /** @brief Text color */
    ESkinColorText,
    /** @brief ThreeDDarkShadow color */
    ESkinColorThreeDDarkShadow,
    /** @brief ThreeDFace color */
    ESkinColorThreeDFace,
    /** @brief ThreeDHighlight color */
    ESkinColorThreeDHighlight,
    /** @brief ThreeDLightShadow color */
    ESkinColorThreeDLightShadow,
    /** @brief ThreeDShadow color */
    ESkinColorThreeDShadow,
    /** @brief Window color */
    ESkinColorWindow,
    /** @brief WindowFrame color */
    ESkinColorWindowFrame,
    /** @brief WindowText color */
    ESkinColorWindowText,

    // text selection colors
    /** @brief Foreground color of active selection */
    ESkinColorActiveSelectionForeground,
    /** @brief Background color of active selection */
    ESkinColorActiveSelectionBackground,
    /** @brief Foreground color of inactive selection */
    ESkinColorInactiveSelectionForeground,
    /** @brief Background color of inactive selection */
    ESkinColorInactiveSelectionBackground,

    // focus ring
    /** @brief Focus frame color */
    ESkinColorFocusRing,

    // text-field
    /** @brief Border color when drawing text fields, text areas, or list boxes */
    ESkinColorTextfieldBorder,
    /** @brief Background color when drawing text fields, text areas, or list boxes (not supported) */
    ESkinColorTextfieldBackground,

    // progress bar
    /** @brief Border color when drawing progress bar */
    ESkinColorProgressbarBorder,
    /** @brief Bar body color when drawing progress bar */
    ESkinColorProgressbarBody,
    /** @brief Background color when drawing progress bar */
    ESkinColorProgressbarBackground,
    /** @brief Background color in disabled state when drawing progress bar  */
    ESkinColorProgressbarBackgroundDisabled,

    // range / slider background
    /** @brief Border color of range control */
    ESkinColorRangeBorder,
    /** @brief Border color of disabled range control */
    ESkinColorRangeBackgroundDisabled,

    /** @brief Total number of skin colors */
    ESkinColors
};

/** @brief System font type */ 
enum SystemFontType {
    /** @brief Font used when font: caption; is specified */
    ESystemFontTypeCaption = 0,
    /** @brief Font used when font: icon; is specified */
    ESystemFontTypeIcon,
    /** @brief Font used when font: menu; is specified */
    ESystemFontTypeMenu,
    /** @brief Font used when font: message-box; is specified */
    ESystemFontTypeMessageBox,
    /** @brief Font used when font: small-caption; is specified */
    ESystemFontTypeSmallCaption,
    /** @brief Font used when font: -webkit-mini-control; is specified */
    ESystemFontTypeWebkitMiniControl,
    /** @brief Font used when font: -webkit-small-control; is specified */
    ESystemFontTypeWebkitSmallControl,
    /** @brief Font used when font: -webkit-control; is specified */
    ESystemFontTypeWebkitControl,
    /** @brief Font used when font: status-bar; is specified */
    ESystemFontTypeStatusBar,
    
    /** @brief Total number of system font types */
    ESystemFontTypes
};

/** @brief Structure for storing skin image data */
struct WKCSkinImage_ {
    /** @brief Skin image size. */
    WKCSize fSize;
    /** @brief Array of coordinate data for specifying four points within a skin image */
    WKCPoint fPoints[4];
    /**
       @brief Pointer to image data of skin image
       @details
       bitmap format: ARGB8888@n
       (lsb) AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB (msb)@n
       rowbytes must be fSize.fWidth * 4@n
    */
    void* fBitmap;
};
/** @brief Type definition of WKC::WKCSkinImage */
typedef struct WKCSkinImage_ WKCSkinImage;

/** @brief Structure for storing resource image data */
struct WKCSkinResourceImage_ {
    /** @brief resource name. */
    const char* fName;
    /** @brief Skin image size. */
    WKCSize fSize;
    /**
       @brief Pointer to image data of skin image
       @details
       bitmap format: ARGB8888@n
       (lsb) AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB (msb)@n
       rowbytes must be fSize.fWidth * 4@n
    */
    void* fBitmap;
};
/** @brief Type definition of WKC::WKCSkinResourceImage */
typedef struct WKCSkinResourceImage_ WKCSkinResourceImage;

/** @brief Structure for storing skin data */
struct WKCSkin_ {
    /**
       @brief Array of structures used to store skin image data
       @details
       Array is the size of WKC::ESkinImages.
     */
    WKCSkinImage fImages[ESkinImages];
    /**
       @brief scale of each skin images
       @details
       Scale of each skin images.
       If no scale required, it should be 1.
     */
    float fImageScale;
    // color format: ARGB8888
    /**
       @brief Array of structures used to store skin colors
       @details
       Array is the size of WKC::ESkinColors.
    */
    unsigned int fColors[ESkinColors];
    /**
       @brief Array for storing system font sizes
       @details
       Array is the size of WKC::ESystemFontTypes.
    */
    float fSystemFontSize[ESystemFontTypes];
    /**
       @brief Array for storing system font family name
       @details
       Array is the size of WKC::ESystemFontTypes.
    */
    const char* fSystemFontFamilyName[ESystemFontTypes];
    /** @brief Default style sheet */
    const char* fDefaultStyleSheet;
    /** @brief Quirks style sheet */
    const char* fQuirksStyleSheet;
    /** @brief List of resource images */
    WKCSkinResourceImage** fResourceImages;
};
/** @brief Type definition of WKC::WKCSkin */
typedef struct WKCSkin_ WKCSkin;

/*@}*/

} // namespace

#endif // WKCSkin_h
