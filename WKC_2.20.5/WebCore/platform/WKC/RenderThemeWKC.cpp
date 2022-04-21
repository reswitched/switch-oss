/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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
#include "config.h"
#include <stdio.h>

#include "CSSValueKeywords.h"
#include "GraphicsContext.h"
#include "RenderTheme.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "HTMLSelectElement.h"
#include "Font.h"
#include "FontSelector.h"
#include "shadow/MediaControlElements.h"
#include "PaintInfo.h"
#include "FileList.h"
#include "StringTruncator.h"
#include "LocalizedStrings.h"
#include "UserAgentStyleSheets.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>
#if ENABLE(VIDEO)
# include <wkc/wkcmediapeer.h>
#endif
# include "RenderProgress.h"

namespace WebCore {

class RenderThemeWKC : public RenderTheme
{
public:
    RenderThemeWKC() : RenderTheme() { };
    virtual ~RenderThemeWKC() override;

public:
    static Ref<RenderTheme> create();

    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle&) const override { return true; }

    virtual bool supportsFocusRing(const RenderStyle&) const override;

    virtual bool paintCheckbox(const RenderObject&, const PaintInfo&, const IntRect&) override;
 
    virtual void setCheckboxSize(RenderStyle&) const override;

    virtual bool paintRadio(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void setRadioSize(RenderStyle&) const override;

    virtual void adjustRepaintRect(const RenderObject&, FloatRect&) override;

    virtual void adjustButtonStyle(RenderStyle&, const Element*) const override;
    virtual bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustTextFieldStyle(RenderStyle&, const Element*) const override;
    virtual bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual void adjustTextAreaStyle(RenderStyle&, const Element*) const override;
    virtual bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual void adjustSearchFieldStyle(RenderStyle&, const Element*) const override;
    virtual bool paintSearchField(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustSearchFieldCancelButtonStyle(RenderStyle&, const Element*) const override;
    virtual bool paintSearchFieldCancelButton(const RenderBox&, const PaintInfo&, const IntRect&) override;

    virtual bool paintSearchFieldDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustSearchFieldDecorationPartStyle(RenderStyle&, const Element*) const override;

    virtual void adjustSearchFieldResultsDecorationPartStyle(RenderStyle&, const Element*) const override;
    virtual bool paintSearchFieldResultsButton(const RenderBox&, const PaintInfo&, const IntRect&) override;

    virtual void adjustSliderTrackStyle(RenderStyle&, const Element*) const override;
    virtual bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustSliderThumbStyle(RenderStyle&, const Element*) const override;
    virtual bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustSliderThumbSize(RenderStyle&, const Element*) const override;

    virtual int minimumMenuListSize(const RenderStyle&) const override;

    virtual void adjustMenuListStyle(RenderStyle&, const Element*) const override;
    virtual bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual void adjustMenuListButtonStyle(RenderStyle&, const Element*) const override;
    virtual bool paintMenuListButtonDecorations(const RenderBox&, const PaintInfo&, const FloatRect&) override;

    virtual bool isControlStyled(const RenderStyle&, const RenderStyle& userAgentStyle) const override;

    virtual bool controlSupportsTints(const RenderObject&) const override;

    virtual Color platformFocusRingColor(OptionSet<StyleColor::Options>) const override;

    virtual Color systemColor(CSSValueID cssValueId, OptionSet<StyleColor::Options>) const override;

    virtual Color platformActiveSelectionBackgroundColor(OptionSet<StyleColor::Options>) const override;
    virtual Color platformInactiveSelectionBackgroundColor(OptionSet<StyleColor::Options>) const override;
    
    virtual Color platformActiveSelectionForegroundColor(OptionSet<StyleColor::Options>) const override;
    virtual Color platformInactiveSelectionForegroundColor(OptionSet<StyleColor::Options>) const override;

    virtual LengthBox popupInternalPaddingBox(const RenderStyle&) const override;

    // CSSs
    virtual String extraDefaultStyleSheet() override;
    virtual String extraQuirksStyleSheet() override;

    virtual void updateCachedSystemFontDescription(CSSValueID, FontCascadeDescription&) const override;

#if ENABLE(VIDEO)
    // Media controls
    virtual String mediaControlsStyleSheet() override;
    virtual String mediaControlsScript() override;

    virtual bool supportsClosedCaptioning() const override;
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const override;
    virtual bool usesMediaControlStatusDisplay() override;
    virtual bool usesMediaControlVolumeSlider() const override;
    virtual bool usesVerticalVolumeSlider() const  override { return true; }
    virtual double mediaControlsFadeInDuration() override;
    virtual Seconds mediaControlsFadeOutDuration() override;
    virtual String formatMediaControlsTime(float time) const override;
    virtual String formatMediaControlsCurrentTime(float currentTime, float duration) const override;
    virtual String formatMediaControlsRemainingTime(float currentTime, float duration) const override;
    
    // Returns the media volume slider container's offset from the mute button.
    virtual LayoutPoint volumeSliderOffsetFromMuteButton(const RenderBox&, const LayoutSize&) const override;

    virtual void adjustMediaControlStyle(RenderStyle&, const Element*) const override;
#endif

    // Returns the repeat interval of the animation for the progress bar.
    virtual Seconds animationRepeatIntervalForProgressBar(RenderProgress&) const override;
    // Returns the duration of the animation for the progress bar.
    virtual Seconds animationDurationForProgressBar(RenderProgress&) const override;
    virtual void adjustProgressBarStyle(RenderStyle&, const Element*) const override;
    virtual bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual bool shouldShowPlaceholderWhenFocused() const { return true; }
    // disable spin button
    virtual bool shouldHaveSpinButton(const HTMLInputElement&) const override { return false; }

    virtual void adjustInnerSpinButtonStyle(RenderStyle&, const Element*) const override;
    virtual bool paintInnerSpinButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual bool delegatesMenuListRendering() const override { return true; }
    virtual bool popsMenuByArrowKeys() const override { return false; }
    virtual bool popsMenuBySpaceOrReturn() const override { return true; }

    virtual String fileListNameForWidth(const FileList*, const FontCascade&, int width, bool multipleFilesAllowed) const override;

    static void resetVariables();

private:
    void close();

    bool supportsFocus(ControlPart) const;
};

// parameters
// it should be in skins...

static const int cPopupInternalPaddingLeft   = 4;
static const int cPopupInternalPaddingRight  = 4;
static const int cPopupInternalPaddingTop    = 3;
static const int cPopupInternalPaddingBottom = 3;

// implementations

RenderThemeWKC::~RenderThemeWKC()
{
}

RenderTheme& RenderTheme::singleton()
{
    WKC_DEFINE_STATIC_TYPE(RenderTheme*, gTheme, new RenderThemeWKC());
    return *gTheme;
}

bool RenderThemeWKC::isControlStyled(const RenderStyle& style, const RenderStyle& userAgentStyle) const
{
    if (style.appearance() == TextFieldPart || style.appearance() == TextAreaPart || style.appearance() == ListboxPart)
        return style.border() != userAgentStyle.border();

    return RenderTheme::isControlStyled(style, userAgentStyle);
}

void RenderThemeWKC::adjustRepaintRect(const RenderObject& o, FloatRect& r)
{
    switch (o.style().appearance()) {
        case MenulistPart: {
            r.setWidth(r.width() + 100);
            break;
        }
        default:
            break;
    }
}

bool RenderThemeWKC::controlSupportsTints(const RenderObject& o) const
{
    if (!isEnabled(o))
        return false;

    // Checkboxes only have tint when checked.
    if (o.style().appearance() == CheckboxPart)
        return isChecked(o);

    // For now assume other controls have tint if enabled.
    return true;
}

void RenderThemeWKC::updateCachedSystemFontDescription(CSSValueID propId, FontCascadeDescription& fontDescription) const
{
    int type = 0;
    float size = 0.f;

    switch (propId) {
    case CSSValueCaption:
        type = WKC_SYSTEMFONT_TYPE_CAPTION; break;
    case CSSValueIcon:
        type = WKC_SYSTEMFONT_TYPE_ICON; break;
    case CSSValueMenu:
        type = WKC_SYSTEMFONT_TYPE_MENU; break;
    case CSSValueMessageBox:
        type = WKC_SYSTEMFONT_TYPE_MESSAGE_BOX; break;
    case CSSValueSmallCaption:
        type = WKC_SYSTEMFONT_TYPE_SMALL_CAPTION; break;
    case CSSValueWebkitMiniControl:
        type = WKC_SYSTEMFONT_TYPE_WEBKIT_MINI_CONTROL; break;
    case CSSValueWebkitSmallControl:
        type = WKC_SYSTEMFONT_TYPE_WEBKIT_SMALL_CONTROL; break;
    case CSSValueWebkitControl:
        type = WKC_SYSTEMFONT_TYPE_WEBKIT_CONTROL; break;
    case CSSValueStatusBar:
        type = WKC_SYSTEMFONT_TYPE_STATUS_BAR; break;
    default:
        return;
    }
    size = wkcStockImageGetSystemFontSizePeer(type);
    const char* familyName = wkcStockImageGetSystemFontFamilyNamePeer(type);
    if (size && familyName) {
        fontDescription.setSpecifiedSize(size);
        fontDescription.setIsAbsoluteSize(true);
        fontDescription.setOneFamily(standardFamily);
        fontDescription.setOneFamily(familyName);
        fontDescription.setWeight(normalWeightValue());
        fontDescription.setItalic(normalItalicValue());
    }
}

bool RenderThemeWKC::supportsFocusRing(const RenderStyle& style) const
{
    // no themes can support drawing focus-rings...
    return false;
}

Color RenderThemeWKC::systemColor(CSSValueID cssValueId, OptionSet<StyleColor::Options> options) const
{
    int id = 0;

    switch (cssValueId) {
    case CSSValueActiveborder:
        id = WKC_SKINCOLOR_ACTIVEBORDER; break;
    case CSSValueActivecaption:
        id = WKC_SKINCOLOR_ACTIVECAPTION; break;
    case CSSValueAppworkspace:
        id = WKC_SKINCOLOR_APPWORKSPACE; break;
    case CSSValueBackground:
        id = WKC_SKINCOLOR_BACKGROUND; break;
    case CSSValueButtonface:
        id = WKC_SKINCOLOR_BUTTONFACE; break;
    case CSSValueButtonhighlight:
        id = WKC_SKINCOLOR_BUTTONHIGHLIGHT; break;
    case CSSValueButtonshadow:
        id = WKC_SKINCOLOR_BUTTONSHADOW; break;
    case CSSValueButtontext:
        id = WKC_SKINCOLOR_BUTTONTEXT; break;
    case CSSValueCaptiontext:
        id = WKC_SKINCOLOR_CAPTIONTEXT; break;
    case CSSValueGraytext:
        id = WKC_SKINCOLOR_GRAYTEXT; break;
    case CSSValueHighlight:
        id = WKC_SKINCOLOR_HIGHLIGHT; break;
    case CSSValueHighlighttext:
        id = WKC_SKINCOLOR_HIGHLIGHTTEXT; break;
    case CSSValueInactiveborder:
        id = WKC_SKINCOLOR_INACTIVEBORDER; break;
    case CSSValueInactivecaption:
        id = WKC_SKINCOLOR_INACTIVECAPTION; break;
    case CSSValueInactivecaptiontext:
        id = WKC_SKINCOLOR_INACTIVECAPTIONTEXT; break;
    case CSSValueInfobackground:
        id = WKC_SKINCOLOR_INFOBACKGROUND; break;
    case CSSValueInfotext:
        id = WKC_SKINCOLOR_INFOTEXT; break;
    case CSSValueMenu:
        id = WKC_SKINCOLOR_MENU; break;
    case CSSValueMenutext:
        id = WKC_SKINCOLOR_MENUTEXT; break;
    case CSSValueScrollbar:
        id = WKC_SKINCOLOR_SCROLLBAR; break;
    case CSSValueText:
        id = WKC_SKINCOLOR_TEXT; break;
    case CSSValueThreeddarkshadow:
        id = WKC_SKINCOLOR_THREEDDARKSHADOW; break;
    case CSSValueThreedface:
        id = WKC_SKINCOLOR_THREEDFACE; break;
    case CSSValueThreedhighlight:
        id = WKC_SKINCOLOR_THREEDHIGHLIGHTA; break;
    case CSSValueThreedlightshadow:
        id = WKC_SKINCOLOR_THREEDLIGHTSHADOW; break;
    case CSSValueThreedshadow:
        id = WKC_SKINCOLOR_THREEDSHADOW; break;
    case CSSValueWindow:
        id = WKC_SKINCOLOR_WINDOW; break;
    case CSSValueWindowframe:
        id = WKC_SKINCOLOR_WINDOWFRAME; break;
    case CSSValueWindowtext:
        id = WKC_SKINCOLOR_WINDOWTEXT; break;
    default:
        return RenderTheme::systemColor(cssValueId, options);

    }
    return wkcStockImageGetSkinColorPeer(id);
}

Color RenderThemeWKC::platformFocusRingColor(OptionSet<StyleColor::Options>) const
{
    return wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_FOCUSRING);
}

void RenderThemeWKC::setCheckboxSize(RenderStyle& style) const
{
    unsigned int w,dummy=0;
    // If the width and height are both specified, then we have nothing to do.
    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    float scale = wkcStockImageGetImageScalePeer();
    wkcStockImageGetSizePeer (WKC_IMAGE_CHECKBOX_UNCHECKED, &w, &dummy);
    float fw = (float)w * scale;
    if (style.width().isIntrinsicOrAuto())
        style.setWidth(Length(fw, Fixed));

    if (style.height().isAuto())
        style.setHeight(Length(fw, Fixed));
}

static void
_bitblt(void* ctx, int type, void* bitmap, int rowbytes, void* mask, int maskrowbytes, const WKCFloatRect* srcrect, const WKCFloatRect* destrect, int op)
{
    WKCPeerImage img = {0};

    img.fType = type;
    img.fBitmap = bitmap;
    img.fRowBytes = rowbytes;
    img.fMask = mask;
    img.fMaskRowBytes = maskrowbytes;
    WKCFloatRect_SetRect(&img.fSrcRect, srcrect);
    WKCFloatSize_Set(&img.fScale, 1, 1);
    WKCFloatSize_Set(&img.fiScale, 1, 1);
    WKCFloatPoint_Set(&img.fPhase, 0, 0);
    WKCFloatSize_Set(&img.fiTransform, 1, 1);

    wkcDrawContextBitBltPeer(ctx, &img, destrect, op);
}
// If skin size is larger than rect, need to reduce skin image for fitting the area.
static void
calcSkinRect(unsigned int in_skinWidth, unsigned int in_skinHeight, const IntRect& in_r, WKCFloatRect& out_dest)
{
    if (in_r.width() < in_skinWidth || in_r.height() < in_skinHeight) {
        if (in_r.width() * in_skinHeight < in_r.height() * in_skinWidth) { // in_r.width() / in_skinWidth < in_r.height() / in_skinHeight
            out_dest.fWidth = in_r.width();
            out_dest.fHeight = (float)in_skinHeight * in_r.width() / in_skinWidth;
        } else {
            out_dest.fWidth = (float)in_skinWidth * in_r.height() / in_skinHeight;
            out_dest.fHeight = in_r.height();
        }
    } else {
        out_dest.fWidth = in_skinWidth;
        out_dest.fHeight = in_skinHeight;
    }
    // centering skin
    out_dest.fX = in_r.x() + (in_r.width() - out_dest.fWidth) / 2; 
    out_dest.fY = in_r.y() + (in_r.height() - out_dest.fHeight) / 2; 
}

bool RenderThemeWKC::paintCheckbox(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    void *drawContext;
    WKCFloatRect src, dest;
    int index;
    unsigned int width, height;
    const unsigned char* image_buf;
    unsigned int rowbytes;

    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = 0;
    if (this->isEnabled(o)) {
        if (this->isPressed(o))
            index = WKC_IMAGE_CHECKBOX_PRESSED;
        else if (this->isFocused(o) || this->isHovered(o))
              index = this->isChecked(o) ? WKC_IMAGE_CHECKBOX_CHECKED_FOCUSED : WKC_IMAGE_CHECKBOX_UNCHECKED_FOCUSED;
        else
            index = this->isChecked(o) ? WKC_IMAGE_CHECKBOX_CHECKED : WKC_IMAGE_CHECKBOX_UNCHECKED;
    }
    else {
        index = this->isChecked(o) ? WKC_IMAGE_CHECKBOX_CHECKED_DISABLED : WKC_IMAGE_CHECKBOX_UNCHECKED_DISABLED;
    }
    
    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;

    rowbytes = width * 4;

    src.fX = 0; src.fY = 0;src.fWidth = (int) width; src.fHeight = (int) height;
    calcSkinRect(width, height, r, dest);
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}

void RenderThemeWKC::setRadioSize(RenderStyle& style) const
{
    // This is the same as checkboxes.
    setCheckboxSize(style);
}

bool RenderThemeWKC::paintRadio(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    void *drawContext;
    WKCFloatRect src, dest;
    int index;
    unsigned int width, height;
    const unsigned char* image_buf;
    unsigned int rowbytes;

    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = 0;
    if (this->isEnabled(o)) {
        index = this->isChecked(o) ? WKC_IMAGE_RADIO_CHECKED : WKC_IMAGE_RADIO_UNCHECKED;
    }
    else {
        index = this->isChecked(o) ? WKC_IMAGE_RADIO_CHECKED_DISABLED : WKC_IMAGE_RADIO_UNCHECKED_DISABLED;
    }
    
    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;

    rowbytes = width * 4;

    src.fX = 0; src.fY = 0;src.fWidth = (int) width; src.fHeight = (int) height;
    calcSkinRect(width, height, r, dest);
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}

bool RenderThemeWKC::supportsFocus(ControlPart part) const
{
    switch (part) {
    case PushButtonPart:
    case ButtonPart:
    case DefaultButtonPart:
    case RadioPart:
    case CheckboxPart:
    case TextFieldPart:
    case SearchFieldPart:
    case TextAreaPart:
        return true;
    default: // No for all others...
        return false;
    }
}

void RenderThemeWKC::adjustButtonStyle(RenderStyle& /*style*/, const Element* /*e*/) const
{
}

static void drawScalingBitmapPeer(const RenderObject& in_o, void* in_context, void* in_bitmap, int rowbytes, WKCSize *in_size, const WKCPoint *in_points, const WKCRect *in_destrect, int op)
{
    WKCFloatRect src, dest;
    const float scale = wkcStockImageGetImageScalePeer();

    // upper
    src.fX = in_points[0].fX; src.fY = 0; 
    src.fWidth = in_points[1].fX - in_points[0].fX;
    src.fHeight = in_points[0].fY;
    dest.fX = in_destrect->fX + in_points[0].fX * scale;
    dest.fY = in_destrect->fY;
    dest.fWidth = in_destrect->fWidth - in_points[0].fX * scale - (in_size->fWidth - in_points[1].fX) * scale;
    dest.fHeight = src.fHeight * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    // lower
    src.fX = in_points[2].fX; src.fY = in_points[2].fY;
    src.fWidth = in_points[3].fX - in_points[2].fX;
    src.fHeight = in_size->fHeight - in_points[2].fY;
    dest.fX = in_destrect->fX + in_points[2].fX * scale;
    dest.fY = in_destrect->fY + in_destrect->fHeight - src.fHeight * scale;
    dest.fWidth = in_destrect->fWidth - in_points[2].fX * scale - (in_size->fWidth - in_points[3].fX) * scale;
    dest.fHeight = src.fHeight * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    // left
    src.fX = 0; src.fY = in_points[0].fY;
    src.fWidth = in_points[0].fX;
    src.fHeight = in_points[2].fY - in_points[0].fY;
    dest.fX = in_destrect->fX; dest.fY = in_destrect->fY + in_points[0].fY * scale;
    dest.fWidth = src.fWidth * scale;
    dest.fHeight = in_destrect->fHeight - in_points[0].fY * scale - (in_size->fHeight - in_points[2].fY) * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    //right
    src.fX = in_points[1].fX; src.fY = in_points[1].fY;
    src.fWidth = in_size->fWidth - in_points[1].fX;
    src.fHeight = in_points[3].fY - in_points[1].fY;
    dest.fX = in_destrect->fX + in_destrect->fWidth - src.fWidth * scale;
    dest.fY = in_destrect->fY + in_points[1].fY * scale;
    dest.fWidth = src.fWidth * scale;
    dest.fHeight = in_destrect->fHeight - in_points[1].fY * scale - (in_size->fHeight - in_points[3].fY) * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    // center
    src.fX = in_points[0].fX; src.fY = in_points[0].fY;
    src.fWidth = in_points[3].fX - in_points[0].fX;
    src.fHeight = in_points[3].fY - in_points[0].fY;
    dest.fX = in_destrect->fX + in_points[0].fX * scale;
    dest.fY = in_destrect->fY + in_points[0].fY * scale;
    dest.fWidth = in_destrect->fWidth - in_points[0].fX * scale - (in_size->fWidth - in_points[3].fX) * scale;
    dest.fHeight = in_destrect->fHeight - in_points[0].fY * scale - (in_size->fHeight - in_points[3].fY) * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0)) {
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);
    }

    // top left corner
    src.fX = 0; src.fY = 0; src.fWidth = in_points[0].fX; src.fHeight = in_points[0].fY;
    dest.fX = in_destrect->fX; dest.fY = in_destrect->fY; dest.fWidth = src.fWidth * scale; dest.fHeight = src.fHeight * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    // top right
    src.fX = in_points[1].fX; src.fY = 0; src.fWidth = in_size->fWidth - in_points[1].fX; src.fHeight = in_points[0].fY;
    dest.fX = in_destrect->fX + in_destrect->fWidth - src.fWidth * scale; dest.fY = in_destrect->fY;
    dest.fWidth = src.fWidth * scale; dest.fHeight = src.fHeight * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    // bottom left
    src.fX = 0; src.fY = in_points[2].fY; src.fWidth = in_points[2].fX; src.fHeight = in_size->fHeight - in_points[2].fY;
    dest.fX = in_destrect->fX; dest.fY = in_destrect->fY + in_destrect->fHeight - src.fHeight * scale;
    dest.fWidth = src.fWidth * scale; dest.fHeight = src.fHeight * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);

    // bottom right corner
    src.fX = in_points[3].fX; src.fY = in_points[3].fY;
    src.fWidth = in_size->fWidth - in_points[3].fX; src.fHeight = in_size->fHeight - in_points[3].fY;
    dest.fX = in_destrect->fX + in_destrect->fWidth - src.fWidth * scale;
    dest.fY = in_destrect->fY + in_destrect->fHeight - src.fHeight * scale;
    dest.fWidth = src.fWidth * scale;
    dest.fHeight = src.fHeight * scale;
    if ((src.fWidth > 0) && (src.fHeight > 0) && (dest.fWidth > 0) && (dest.fHeight > 0))
        _bitblt (in_context, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, in_bitmap, rowbytes, 0, 0, &src, &dest, op);
}

bool RenderThemeWKC::paintButton(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    void *drawContext;
    WKCSize img_size;
    int index;
    unsigned int width, height;
    const WKCPoint* points;
    const unsigned char* image_buf;
    unsigned int rowbytes;
    WKCRect rect;

    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = 0;
    if (this->isEnabled(o)) {
        index = WKC_IMAGE_BUTTON;
        if (this->isHovered(o) || this->isFocused(o))
              index = WKC_IMAGE_BUTTON_HOVERED;
        if (this->isPressed(o))
              index = WKC_IMAGE_BUTTON_PRESSED;
    }
    else {
        index = WKC_IMAGE_BUTTON_DISABLED;
    }

    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;
    points = wkcStockImageGetLayoutPointsPeer (index);
    if (!points)
          return false;
    
    img_size.fWidth = width;
    img_size.fHeight = height;
    rowbytes = width * 4;

    rect.fX = r.x(); rect.fY = r.y(); rect.fWidth = r.width(); rect.fHeight = r.height();

    drawScalingBitmapPeer (o, drawContext, (void *)image_buf, rowbytes, &img_size, points, &rect, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}

void RenderThemeWKC::adjustTextFieldStyle(RenderStyle& style, const Element* e) const
{
    if (style.hasBackgroundImage()) {
        style.resetBorder();
        const unsigned int defaultBorderColor = wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_TEXTFIELD_BORDER);
        const short defaultBorderWidth = 1;
        const BorderStyle defaultBorderStyle = BorderStyle::Solid;
        style.setBorderLeftWidth(defaultBorderWidth);
        style.setBorderLeftStyle(defaultBorderStyle);
        style.setBorderLeftColor(defaultBorderColor);
        style.setBorderRightWidth(defaultBorderWidth);
        style.setBorderRightStyle(defaultBorderStyle);
        style.setBorderRightColor(defaultBorderColor);
        style.setBorderTopWidth(defaultBorderWidth);
        style.setBorderTopStyle(defaultBorderStyle);    
        style.setBorderTopColor(defaultBorderColor);
        style.setBorderBottomWidth(defaultBorderWidth);
        style.setBorderBottomStyle(defaultBorderStyle);
        style.setBorderBottomColor(defaultBorderColor);
   }
 
}

static void
_setBorder(GraphicsContext& context, const Color& color, float thickness)
{
    context.setStrokeColor(color);
    context.setStrokeThickness(thickness);
    context.setStrokeStyle(SolidStroke);
}

bool RenderThemeWKC::paintTextField(const RenderObject& o, const PaintInfo& i, const FloatRect& r)
{
    Color backgroundColor;
    const Color defaultBorderColor(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_TEXTFIELD_BORDER));
    bool ret = false;

    i.context().save();

    _setBorder(i.context(), defaultBorderColor, 1.0);
    if (o.style().hasBackground()) {
        backgroundColor = o.style().visitedDependentColor(CSSPropertyBackgroundColor);
    } else {
        backgroundColor = Color(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_TEXTFIELD_BACKGROUND));
    }

    if (!o.style().hasBackgroundImage()) {
        i.context().setFillColor(backgroundColor);
        i.context().drawRect(r);
    } else {
        ret = true;
    }

    i.context().restore();

    return ret;
}

void RenderThemeWKC::adjustTextAreaStyle(RenderStyle& style, const Element* e) const
{
    adjustTextFieldStyle(style, e);
}

bool RenderThemeWKC::paintTextArea(const RenderObject& o, const PaintInfo& info, const FloatRect& r)
{
    return paintTextField(o, info, r);
}

void RenderThemeWKC::adjustSearchFieldStyle(RenderStyle& style, const Element* e) const
{
    adjustTextFieldStyle(style, e);
}

bool RenderThemeWKC::paintSearchField(const RenderObject& o, const PaintInfo& info, const IntRect& r)
{
    return paintTextField(o, info, r);
}

void RenderThemeWKC::adjustSearchFieldCancelButtonStyle(RenderStyle& style, const Element*) const
{
    unsigned int width=0, height=0;
    float scale = wkcStockImageGetImageScalePeer();
    wkcStockImageGetSizePeer(WKC_IMAGE_SEARCHFIELD_CANCELBUTTON, &width, &height);
    const FloatSize size((float)width*scale, (float)height*scale);
    style.setWidth(Length(size.width(), Fixed));
    style.setHeight(Length(size.height(), Fixed));
}

bool RenderThemeWKC::paintSearchFieldCancelButton(const RenderBox& o, const PaintInfo& i, const IntRect& r)
{
    void *drawContext;
    int index;
    unsigned int width, height;
    const WKCPoint* points;
    const unsigned char* image_buf;
    unsigned int rowbytes;
    WKCFloatRect src, dest;
    
    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = WKC_IMAGE_SEARCHFIELD_CANCELBUTTON;
    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;
    points = wkcStockImageGetLayoutPointsPeer (index);
    if (!points)
          return false;
    
    rowbytes = width * 4;

    src.fX = 0; src.fY = 0;src.fWidth = (int) width; src.fHeight = (int) height;
    calcSkinRect(width, height, r, dest);
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}

void RenderThemeWKC::adjustSearchFieldResultsDecorationPartStyle(RenderStyle& style, const Element* e) const
{
    unsigned int width=0, height=0;
    float scale = wkcStockImageGetImageScalePeer();
    wkcStockImageGetSizePeer(WKC_IMAGE_SEARCHFIELD_RESULTBUTTON, &width, &height);
    const FloatSize size((float)width*scale, (float)height*scale);
    style.setWidth(Length(size.width(), Fixed));
    style.setHeight(Length(size.height(), Fixed));
}

bool RenderThemeWKC::paintSearchFieldResultsButton(const RenderBox& o, const PaintInfo& i, const IntRect& r)
{
    void *drawContext;
    WKCSize img_size;
    int index;
    unsigned int width, height;
    const WKCPoint* points;
    const unsigned char* image_buf;
    unsigned int rowbytes;
    WKCRect rect;
    
    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = WKC_IMAGE_SEARCHFIELD_RESULTBUTTON;
    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;
    points = wkcStockImageGetLayoutPointsPeer (index);
    if (!points)
          return false;
    
    img_size.fWidth = width;
    img_size.fHeight = height;
    rowbytes = width * 4;

    rect.fX = r.x(); rect.fY = r.y(); rect.fWidth = r.width(); rect.fHeight = r.height();

    drawScalingBitmapPeer (o, drawContext, (void *)image_buf, rowbytes, &img_size, points, &rect, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}

void RenderThemeWKC::adjustSearchFieldDecorationPartStyle(RenderStyle& style, const Element*) const
{
}

bool RenderThemeWKC::paintSearchFieldDecorations(const RenderObject&, const PaintInfo&, const IntRect&)
{
    return false;
}

int RenderThemeWKC::minimumMenuListSize(const RenderStyle& style) const 
{
    // same as safari.
    int fontsize = style.computedFontPixelSize();
    if (fontsize >= 13) {
        return 9;
    } else if (fontsize >= 11) {
        return 5;
    }
    return 0;
}

void RenderThemeWKC::adjustMenuListStyle(RenderStyle& style, const Element* e) const
{
    style.resetBorder();
    style.resetPadding();
    style.setHeight(Length(Auto));
    style.setWhiteSpace(WhiteSpace::Pre);
}

bool RenderThemeWKC::paintMenuList(const RenderObject& o, const PaintInfo& i, const FloatRect& r)
{
    paintTextField(o, i, r);
    return paintMenuListButtonDecorations(downcast<RenderBox>(o), i, r);
}

void RenderThemeWKC::adjustMenuListButtonStyle(RenderStyle& style, const Element* e) const
{
    style.resetPadding();
    style.setLineHeight(RenderStyle::initialLineHeight());
}

bool RenderThemeWKC::paintMenuListButtonDecorations(const RenderBox& o, const PaintInfo& i, const FloatRect& r)
{
    void *drawContext;
    WKCFloatRect src, dest;
    int index;
    unsigned int width, height;
    float swidth, sheight;
    float scale;
    const WKCPoint* points;
    const unsigned char* image_buf;
    const int menulistPadding = 0;
    const int minimumHeight = 8;
    unsigned int rowbytes;
    int tmp_height;

    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = 0;
    if (this->isEnabled(o)) {
        if (this->isPressed(o))
              index = WKC_IMAGE_MENU_LIST_BUTTON_PRESSED;
        else if (this->isFocused(o) || this->isHovered(o))
              index = WKC_IMAGE_MENU_LIST_BUTTON_FOCUSED;
        else
              index = WKC_IMAGE_MENU_LIST_BUTTON;
    } else {
        index = WKC_IMAGE_MENU_LIST_BUTTON_DISABLED;
    }

    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;
    points = wkcStockImageGetLayoutPointsPeer (index);
    if (!points)
          return false;

    rowbytes = width * 4;

    scale = wkcStockImageGetImageScalePeer();
    swidth = (float)width * scale;
    sheight = (float)height * scale;

    // center
    if (r.height() <= minimumHeight) { 
        src.fX = 0; src.fY = points[1].fY; 
        src.fWidth = width; 
        src.fHeight = points[2].fY - points[1].fY;
        dest.fX = r.x() + r.width() - width;
        dest.fY = r.y() + (r.height() - src.fHeight*scale)/2;
        dest.fWidth = swidth;
        dest.fHeight = src.fHeight;
        _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);
        return false; // only paint the cneter
    }

    // upper + lower + center
    if (r.height() <= (points[3].fY - points[0].fY + 2 * menulistPadding) * scale) {
        // upper
        src.fX = 0;
        src.fY = points[0].fY;
        src.fWidth = width;
        src.fHeight = points[1].fY - points[0].fY;
        dest.fX = r.x() + r.width() - swidth;
        dest.fY = r.y() + menulistPadding;
        dest.fWidth = swidth;
        tmp_height = (r.height() - (points[2].fY - points[1].fY))*scale;
        dest.fHeight = tmp_height / 2;
        if (dest.fHeight > 0)
            _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

        // center
        src.fY += src.fHeight;
        src.fHeight = points[2].fY - points[1].fY;
        dest.fY += dest.fHeight;
        dest.fHeight = src.fHeight*scale;
        _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

        // lower
        src.fY += src.fHeight;
        src.fHeight = points[3].fY - points[2].fY;
        dest.fY += dest.fHeight;
        dest.fHeight = tmp_height / 2 + tmp_height % 2;
        if (dest.fHeight > 0)
            _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);
        
        return false;
    }
    // paint all
    // top
    float dy = 0;
    src.fX = 0; src.fY = 0;
    src.fWidth = width;
    src.fHeight = points[0].fY;
    dest.fX = r.x() + r.width() - swidth;
    dest.fY = r.y() + menulistPadding; 
    dest.fWidth = swidth;
    dest.fHeight = src.fHeight * scale;
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);
    dy += dest.fHeight;

    // upper
    src.fY += src.fHeight;
    src.fHeight = points[1].fY - points[0].fY;
    dest.fY += dest.fHeight;
    tmp_height = r.height() - ((int)height - points[3].fY + points[0].fY + points[2].fY - points[1].fY) * scale;
    dest.fHeight = tmp_height / 2;
    if (dest.fHeight > 0) {
        _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);
        dy += dest.fHeight;
    }

    // center
    src.fY += src.fHeight;
    src.fHeight = points[2].fY - points[1].fY;
    dest.fY += dest.fHeight;
    float fl = tmp_height / 2 + tmp_height % 2;
    float fb = ((int) height - points[3].fY)*scale;
    fb = (fl>0) ? fl+fb : fb;
    float fr = r.height() - dy - fb;
    dest.fHeight = fr > src.fHeight*scale ? fr : src.fHeight*scale;
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);
    dy += dest.fHeight;

    // lower
    src.fY += src.fHeight;
    src.fHeight = points[3].fY - points[2].fY;
    dest.fY += dest.fHeight;
    dest.fHeight = tmp_height / 2 + tmp_height % 2;
    if (dest.fHeight > 0) {
        if (dest.fY + dest.fHeight + ((int) height - points[3].fY)*scale < r.height())
            dest.fHeight = r.height() - ((int) height - points[3].fY)*scale - dest.fY;
        _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);
        dy += dest.fHeight;
    }
    
    // bottom
    src.fY += src.fHeight;
    src.fHeight = (int) height - points[3].fY;
    dest.fY += dest.fHeight;
    dest.fHeight = src.fHeight * scale;
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}


void RenderThemeWKC::adjustSliderTrackStyle(RenderStyle& style, const Element* element) const
{
    RenderTheme::adjustSliderTrackStyle(style, element);
}

bool RenderThemeWKC::paintSliderTrack(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    int width = r.width();
    int height = r.height();
    IntRect rect(r);
    Color backgroundColor;
    const Color defaultBorderColor(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_RANGE_BORDER));

    if (o.style().appearance() == SliderHorizontalPart) {
        rect.setHeight(4);
        rect.move(0, (height - rect.height())/2);
    } else if (o.style().appearance() == SliderVerticalPart) {
        rect.setWidth(4);
        rect.move((width - rect.width())/2, 0);
    } else {
        return false;
    }

    i.context().save();

    _setBorder(i.context(), defaultBorderColor, 1.0);
    if (!this->isEnabled(o)) {
        backgroundColor = Color(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_RANGE_BACKGROUND_DISABLED));
    } else {
        backgroundColor = o.style().visitedDependentColor(CSSPropertyBackgroundColor);
    }
    i.context().setFillColor(backgroundColor);
    i.context().drawRect(rect);

    i.context().restore();

    return false;
}

void RenderThemeWKC::adjustSliderThumbStyle(RenderStyle& style, const Element* e) const
{
    int index = 0;
    if (style.appearance() == SliderThumbHorizontalPart) {
        index = WKC_IMAGE_H_RANGE;
    } else if (style.appearance() == SliderThumbVerticalPart) {
        index = WKC_IMAGE_V_RANGE;
    } else {
        return;
    }

    unsigned int width=0, height=0;
    float scale = wkcStockImageGetImageScalePeer();
    wkcStockImageGetSizePeer(index, &width, &height);
    const FloatSize size((float)width*scale, (float)height*scale);
    style.setWidth(Length(size.width(), Fixed));
    style.setHeight(Length(size.height(), Fixed));

    style.setBoxShadow(nullptr);
}

bool RenderThemeWKC::paintSliderThumb(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    void *drawContext;
    WKCFloatRect src, dest;
    int index;
    unsigned int width, height;
    const unsigned char* image_buf;
    unsigned int rowbytes;
    
    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    index = 0;
    if (o.style().appearance() == SliderThumbHorizontalPart) {
        if (this->isEnabled(o)) {
            index = WKC_IMAGE_H_RANGE;
            if (this->isHovered(o) || this->isFocused(o))
                index = WKC_IMAGE_H_RANGE_HOVERED;
            if (this->isPressed(o))
                index = WKC_IMAGE_H_RANGE_PRESSED;
        } else {
            index = WKC_IMAGE_H_RANGE_DISABLED;
        }
    } else if (o.style().appearance() == SliderThumbVerticalPart) {
        if (this->isEnabled(o)) {
            index = WKC_IMAGE_V_RANGE;
            if (this->isHovered(o) || this->isFocused(o))
                index = WKC_IMAGE_V_RANGE_HOVERED;
            if (this->isPressed(o))
                index = WKC_IMAGE_V_RANGE_PRESSED;
        } else {
            index = WKC_IMAGE_V_RANGE_DISABLED;
        }
    } else {
        return false;
    }

    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;
    
    rowbytes = width * 4;

    src.fX = 0; src.fY = 0;src.fWidth = (int) width; src.fHeight = (int) height;
    calcSkinRect(width, height, r, dest);
    _bitblt (drawContext, WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASTRUEALPHA | WKC_IMAGETYPE_FLAG_FORSKIN, (void *)image_buf, rowbytes, 0, 0, &src, &dest, WKC_COMPOSITEOPERATION_SOURCEOVER);

    return false;
}

void RenderThemeWKC::adjustSliderThumbSize(RenderStyle& style, const Element*) const
{
    int index = 0;
    if (style.appearance() == SliderThumbHorizontalPart) {
        index = WKC_IMAGE_H_RANGE;
    } else if (style.appearance() == SliderThumbVerticalPart) {
        index = WKC_IMAGE_V_RANGE;
    } else {
        return;
    }

    unsigned int width=0, height=0;
    float scale = wkcStockImageGetImageScalePeer();
    wkcStockImageGetSizePeer(index, &width, &height);

    style.setWidth(Length((float)width*scale, Fixed));
    style.setHeight(Length((float)height*scale, Fixed));
}

Color RenderThemeWKC::platformActiveSelectionBackgroundColor(OptionSet<StyleColor::Options>) const
{
    return wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_ACTIVESELECTIONBACKGROUND);
}

Color RenderThemeWKC::platformInactiveSelectionBackgroundColor(OptionSet<StyleColor::Options>) const
{
    return wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_INACTIVESELECTIONBACKGROUND);
}

Color RenderThemeWKC::platformActiveSelectionForegroundColor(OptionSet<StyleColor::Options>) const
{
    return wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_ACTIVESELECTIONFOREGROUND);
}

Color RenderThemeWKC::platformInactiveSelectionForegroundColor(OptionSet<StyleColor::Options>) const
{
    return wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_INACTIVESELECTIONFOREGROUND);
}

LengthBox RenderThemeWKC::popupInternalPaddingBox(const RenderStyle& style) const
{ 
    int leftWidth = 0;
    int rightWidth = 0;

    switch (style.appearance()) {
    case MenulistPart: // default
        leftWidth += 1;
        rightWidth += 1;
        // fall through
    case MenulistButtonPart:
        leftWidth += cPopupInternalPaddingLeft;
        {
            unsigned int w=0, h=0;
            float scale = wkcStockImageGetImageScalePeer();
            wkcStockImageGetSizePeer(WKC_IMAGE_MENU_LIST_BUTTON, &w, &h);
            w *= scale;

            rightWidth += cPopupInternalPaddingRight;
            rightWidth += w;
        }
        break;
    case ButtonPart:
        leftWidth = 1;
        rightWidth = 1;
        break;
    default:
        break;
    }

    return {
        cPopupInternalPaddingTop,
        rightWidth,
        cPopupInternalPaddingBottom,
        leftWidth
    };
}

typedef void (*ResolveFilenameForDisplayProc)(const unsigned short* path, const int path_len, unsigned short* out_path, int* out_path_len, const int path_maxlen);
WKC_DEFINE_GLOBAL_TYPE_ZERO(ResolveFilenameForDisplayProc, gResolveFilenameForDisplayProc);

String RenderThemeWKC::fileListNameForWidth(const FileList* fileList, const FontCascade& fc, int width, bool multipleFilesAllowed) const
{
    if (width <= 0)
        return String();

    String str;
    unsigned short filenameBuffer[MAX_PATH] = {0};

    if (1 < fileList->length()) {
        str = multipleFileUploadText(fileList->length());
        return StringTruncator::rightTruncate(str, width, fc);
    } else {
        if (gResolveFilenameForDisplayProc != 0) {
            String file;
            if (fileList->isEmpty()) {
                file = String();
            } else {
                file = fileList->item(0)->path();
            }
            int len = 0;
            int cur_len = file.length();
            (*gResolveFilenameForDisplayProc)(file.charactersWithNullTermination().data(), cur_len, filenameBuffer, &len, MAX_PATH);
            str = String(filenameBuffer, len);
        } else {
            if (fileList->isEmpty()) {
                str = fileButtonNoFileSelectedLabel();
            } else {
                str = fileList->item(0)->path();
            }
        }
        return StringTruncator::centerTruncate(str, width, fc);
    }
}

void
RenderTheme_SetResolveFilenameForDisplayProc(WebCore::ResolveFilenameForDisplayProc proc)
{
    gResolveFilenameForDisplayProc = proc;
}

// CSSs
String RenderThemeWKC::extraDefaultStyleSheet()
{
    return String(wkcStockImageGetDefaultStyleSheetPeer());
}

String RenderThemeWKC::extraQuirksStyleSheet()
{
    return String(wkcStockImageGetQuirksStyleSheetPeer());
}

#if ENABLE(VIDEO)
    // Media controls
bool RenderThemeWKC::supportsClosedCaptioning() const
{
    return false;
}
bool RenderThemeWKC::hasOwnDisabledStateHandlingFor(ControlPart) const
{
    return false;
}
bool RenderThemeWKC::usesMediaControlStatusDisplay()
{
    return false;
}
bool RenderThemeWKC::usesMediaControlVolumeSlider() const
{
    return true;
}
double RenderThemeWKC::mediaControlsFadeInDuration() { return 0.1; }
Seconds RenderThemeWKC::mediaControlsFadeOutDuration() { return 300_ms; }

String RenderThemeWKC::formatMediaControlsTime(float in_time) const
{
    return RenderTheme::formatMediaControlsTime(in_time);
}

String RenderThemeWKC::formatMediaControlsCurrentTime(float cur, float duration) const
{
    return RenderTheme::formatMediaControlsCurrentTime(cur, duration);
}

String RenderThemeWKC::formatMediaControlsRemainingTime(float cur, float duration) const
{
    return RenderTheme::formatMediaControlsRemainingTime(cur, duration);
}

LayoutPoint RenderThemeWKC::volumeSliderOffsetFromMuteButton(const RenderBox& o, const LayoutSize& size) const
{
    return RenderTheme::volumeSliderOffsetFromMuteButton(o, size);
}

void RenderThemeWKC::adjustMediaControlStyle(RenderStyle&, const Element*) const
{
}

String RenderThemeWKC::mediaControlsStyleSheet()
{
    const char* css = wkcMediaPlayerControlsGetStyleSheetPeer();
    if (css) {
        return String(css);
    } else {
        return String();
    }
}

String RenderThemeWKC::mediaControlsScript()
{
    const char* js = wkcMediaPlayerControlsGetScriptPeer();
    if (js) {
        return String(js);
    } else {
        return String();
    }
}

#endif

// Ugh!: it should be in skin parameter!
// 111005 ACCESS Co.,Ltd.
static const Seconds gFrameRateForProgressBarAnimation { 33_ms };
static const Seconds gDurationForProgressBarAnimation { 2.0 };

Seconds RenderThemeWKC::animationRepeatIntervalForProgressBar(RenderProgress& p) const
{
    return gFrameRateForProgressBarAnimation;
}

Seconds RenderThemeWKC::animationDurationForProgressBar(RenderProgress& p) const
{
    if (p.isDeterminate())
        return gFrameRateForProgressBarAnimation;
    return gDurationForProgressBarAnimation;
}

void RenderThemeWKC::adjustProgressBarStyle(RenderStyle&, const Element*) const
{
}

bool RenderThemeWKC::paintProgressBar(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    if (!o.isProgress())
        return true;

    Color backgroundColor;
    const Color defaultBorderColor(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_PROGRESSBAR_BORDER));
    const Color bodyColor(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_PROGRESSBAR_BODY));

    i.context().save();

    _setBorder(i.context(), defaultBorderColor, 1.0);
    if (!this->isEnabled(o)) {
        backgroundColor = Color(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_PROGRESSBAR_BACKGROUND_DISABLED));
    } else if (o.style().hasBackground()) {
        backgroundColor = o.style().visitedDependentColor(CSSPropertyBackgroundColor);
    } else {
        backgroundColor = Color(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_PROGRESSBAR_BACKGROUND));
    }
    if (!o.style().hasBackgroundImage()) {
        i.context().setFillColor(backgroundColor);
        i.context().drawRect(r);
    }

    const RenderProgress& renderProgress = (const RenderProgress&)o;
    IntRect vr = IntRect(0,0,0,0);
    IntRect vrr = IntRect(0,0,0,0);
    if (renderProgress.isDeterminate()) {
        vr = r;
        vr.expand(-2,-2);
        vr.move(1,1);
        int w = vr.width() * renderProgress.position();
        vr.setWidth(w);
    } else {
        vr = r;
        vr.expand(-2,-2);
        int mx = vr.maxX() - 1;
        int x = vr.width() * renderProgress.animationProgress();
        vr.move(x, 1);
        int w = vr.width() * 0.25;
        vr.setWidth(w);
        if (vr.maxX() > mx) {
            vrr = r;
            vrr.expand(-2,-2);
            vrr.move(1,1);
            vrr.setWidth(vr.maxX() - mx);
            vr.expand(-vrr.width(), 0);
        }
    }

    _setBorder(i.context(), Color(), 0.0);
    i.context().setFillColor(bodyColor);
    i.context().fillRect(vr);
    if (!vrr.isEmpty()) {
        i.context().fillRect(vrr);
    }

    i.context().restore();

    return true;
}

void RenderThemeWKC::adjustInnerSpinButtonStyle(RenderStyle& style, const Element*) const
{
    int width = style.computedFontPixelSize();
    if (width <= 0)
        width = 18;
    style.setWidth(Length(width, Fixed));
    style.setMinWidth(Length(width, Fixed));
}

static bool _paintSpinButton(const RenderObject& o, const PaintInfo& i, const IntRect& r, int index, bool up)
{
    void *drawContext;
    WKCSize img_size;
    unsigned int width, height;
    const WKCPoint* points;
    const unsigned char* image_buf;
    unsigned int rowbytes;
    WKCRect rect;
    
    drawContext = i.context().platformContext();
    if (!drawContext)
          return false;

    image_buf = wkcStockImageGetBitmapPeer (index);
    if (!image_buf)
          return false;

    wkcStockImageGetSizePeer (index, &width, &height);
    if (width == 0 || height == 0)
          return false;
    points = wkcStockImageGetLayoutPointsPeer (index);
    if (!points)
          return false;
    
    img_size.fWidth = width;
    img_size.fHeight = height;
    rowbytes = width * 4;

    rect.fX = r.x(); rect.fY = r.y(); rect.fWidth = r.width(); rect.fHeight = r.height();

    drawScalingBitmapPeer (o, drawContext, (void *)image_buf, rowbytes, &img_size, points, &rect, WKC_COMPOSITEOPERATION_SOURCEOVER);

    int cw = rect.fWidth - points[0].fX - (img_size.fWidth - points[3].fX);
    int ch = rect.fHeight - points[0].fY - (img_size.fHeight - points[3].fY);
    int cx0 = rect.fX + points[0].fX;
    int cy0 = rect.fY + points[0].fY;
    int cx1 = cx0 + cw;
    int cy1 = cy0 + ch;
    int cx = cx0 + cw / 2;

    Path path;
    if (up) {
        path.moveTo(FloatPoint(cx0, cy1));
        path.addLineTo(FloatPoint(cx, cy0));
        path.addLineTo(FloatPoint(cx1, cy1));
        path.addLineTo(FloatPoint(cx0, cy1));
    } else {
        path.moveTo(FloatPoint(cx0, cy0));
        path.addLineTo(FloatPoint(cx, cy1));
        path.addLineTo(FloatPoint(cx1, cy0));
        path.addLineTo(FloatPoint(cx0, cy0));
    }
    Color c(wkcStockImageGetSkinColorPeer(WKC_SKINCOLOR_BUTTONTEXT));
    i.context().save();
    i.context().setFillColor(c);
    i.context().fillPath(path);
    i.context().restore();

    return false;
}

bool RenderThemeWKC::paintInnerSpinButton(const RenderObject& o, const PaintInfo& i, const IntRect& r)
{
    IntRect r1(r), r2(r);
    r1.setHeight(r1.height()/2);
    r2.setHeight(r2.height()/2);
    r2.move(0, r1.height());

    int index = 0;
    if (isEnabled(o)) {
        index = WKC_IMAGE_BUTTON;
        if (isHovered(o) && isSpinUpButtonPartHovered(o))
            index = WKC_IMAGE_BUTTON_HOVERED;
        if (isPressed(o) && isSpinUpButtonPartPressed(o))
            index = WKC_IMAGE_BUTTON_PRESSED;
    } else {
        index = WKC_IMAGE_BUTTON_DISABLED;
    }

    _paintSpinButton(o, i, r1, index, true);

    index = 0;
    if (isEnabled(o)) {
        index = WKC_IMAGE_BUTTON;
        if (isHovered(o) && !isSpinUpButtonPartHovered(o))
            index = WKC_IMAGE_BUTTON_HOVERED;
        if (isPressed(o) && !isSpinUpButtonPartPressed(o))
            index = WKC_IMAGE_BUTTON_PRESSED;
    } else {
        index = WKC_IMAGE_BUTTON_DISABLED;
    }

    _paintSpinButton(o, i, r2, index, false);

    return true;
}

}
