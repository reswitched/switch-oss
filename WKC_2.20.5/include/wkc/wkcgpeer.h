/*
 *  wkcgpeer.h
 *
 *  Copyright(c) 2009-2017 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_G_PEER_H_
#define _WKC_G_PEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>

/**
   @file
   @brief graphics related peers.
   @attention
   wkcOffscreenXXXPeer, wkcDrawContextXXXPeer, wkcHWOffscreenXXXPeer and wkcLayerXXXPeers need not be implemented in cusotmers side. Use existing implementations as-is.
*/

/*@{*/

WKC_BEGIN_C_LINKAGE

// font peer

enum {
    WKC_FONT_WEIGHT_100 = 100,
    WKC_FONT_WEIGHT_200 = 200,
    WKC_FONT_WEIGHT_300 = 300,
    WKC_FONT_WEIGHT_400 = 400,
    WKC_FONT_WEIGHT_500 = 500,
    WKC_FONT_WEIGHT_600 = 600,
    WKC_FONT_WEIGHT_700 = 700,
    WKC_FONT_WEIGHT_800 = 800,
    WKC_FONT_WEIGHT_900 = 900,
    WKC_FONT_WEIGHT_NORMAL = WKC_FONT_WEIGHT_400,
    WKC_FONT_WEIGHT_BOLD = WKC_FONT_WEIGHT_700,
    WKC_FONT_WEIGHTS
};

enum {
    WKC_FONT_FAMILY_NONE = 0,
    WKC_FONT_FAMILY_STANDARD,
    WKC_FONT_FAMILY_SERIF,
    WKC_FONT_FAMILY_SANSSERIF,
    WKC_FONT_FAMILY_MONOSPACE,
    WKC_FONT_FAMILY_CURSIVE,
    WKC_FONT_FAMILY_FANTASY,
    WKC_FONT_FAMILY_PICTOGRAPH,
    WKC_FONT_FAMILIES,
};

enum {
    WKC_FONT_ENGINE_REGISTER_TYPE_MEMORY = 0,
    WKC_FONT_ENGINE_REGISTER_TYPE_FILE,
    WKC_FONT_ENGINE_REGISTER_TYPES
};

enum {
    WKC_FONT_FLAG_NONE          = 0x00000000,
    WKC_FONT_FLAG_OVERRIDE_BIDI = 0x00000001,
    WKC_FONT_FLAG_LTR           = 0x00000000,
    WKC_FONT_FLAG_RTL           = 0x00000010,
};

enum {
    WKC_FONT_DRAWINGMODE_INVISIBLE = 0x00000000,
    WKC_FONT_DRAWINGMODE_FILL      = 0x00000001,
    WKC_FONT_DRAWINGMODE_STROKE    = 0x00000002,
    WKC_FONT_DRAWINGMODE_CLIP      = 0x00000004,
};

/**
   @brief Type definition of memory allocation function pointer for font peer
   @details
   Call this function to allocate memory. A memory will be allocated from browser memory space.
*/
typedef void* (*fontPeerMalloc)(int size);
/**
   @brief Type definition of memory free function pointer for font peer
   @details
   Call this function to free memory allocated by fontPeerMalloc function.
*/
typedef void  (*fontPeerFree)(void *ptr);

/**
@brief Initializes font engine
@param heap allocated static memory (see wkcFontEngineHeapSize())
@param heap_len allocate static memory size (see wkcFontEngineHeapSize())
@param fontmalloc function pointer to memory allocation
@param fontfree function pointer to memory free
@param use_internal_font reserved
@retval !=false Success
@retval ==false Failed
@details
Write the necessary processes for initializing the font engine.
*/
WKC_PEER_API bool wkcFontEngineInitializePeer(void* heap, int heap_len, fontPeerMalloc fontmalloc, fontPeerFree fontfree, bool use_internal_font);
/**
@brief Finalizes font engine
@details
Write the necessary processes for finalizing the font engine.
*/
WKC_PEER_API void wkcFontEngineFinalizePeer(void);
/**
@brief Forcibly terminates font engine
@details
Write the necessary processes for forcibly terminating the font engine.
*/
WKC_PEER_API void wkcFontEngineForceTerminatePeer(void);
/**
@brief Queries whether the font engine is ready to draw.
@retval !=false The font engine is ready to draw
@retval ==false The font engine is not ready to draw
@details
Return whether the font engine is ready to draw.
*/
WKC_PEER_API bool wkcFontEngineIsReadyToDrawPeer(void);

WKC_PEER_API bool wkcFontEngineSupportsNonSpacingMarksPeer(void);

/**
@brief Queries whether the font engine can support the font data
@param in_data font data
@param in_len font data length
@retval !=false Supported
@retval ==false Not supported
@details
Return whether the font engine can support the font data.
*/
WKC_PEER_API bool wkcFontEngineCanSupportPeer(const unsigned char* in_data, unsigned int in_len);
/**
@brief Queries whether the font engine can support the font format
@param in_format format name
@param in_len length of in_format
@retval !=false Supported
@retval ==false Not supported
@details
Return whether the font engine can support the font format. in_format would be "truetype", "opentype", "woff" or else.
*/
WKC_PEER_API bool wkcFontEngineCanSupportByFormatNamePeer(const unsigned char* in_format, unsigned int in_len);
/**
@brief Register a font data to the font engine.
@param in_type register type
@li WKC_FONT_ENGINE_REGISTER_TYPE_MEMORY from memory
@li WKC_FONT_ENGINE_REGISTER_TYPE_FILE from file
@param in_data font data
@li in_type==WKC_FONT_ENGINE_REGISTER_TYPE_MEMORY memory pointer to the font data
@li in type==WKC_FONT_ENGINE_REGISTER_TYPE_FILE filename
@param in_len length of in_data
@retval register id
@details
Write the process to register font data to font engine that can be used additional font after this function calling.
*/
WKC_PEER_API int wkcFontEngineRegisterFontPeer(int in_type, const unsigned char* in_data, unsigned int in_len);
/**
@brief Unregister the font data
@param in_id register id
@details
Write the process to unregister font data specified in in_id.
*/
WKC_PEER_API void wkcFontEngineUnregisterFontPeer(int in_id);
/**
@brief Unregister all registered fonts.
@details
Write the process to unregister all registered fonts previously.
*/
WKC_PEER_API void wkcFontEngineUnregisterFontsPeer();
/**
@brief Get the font family name of a registered font
@param in_id register id
@param out_buf family name
@param in_buflen max length of out_buf
@retval !=fail Success
@retval ==fail Fail
@details
Get the font family name of a registered font specified in in_id. The family name will be used the argument of wkcFontNewPeer().
*/
WKC_PEER_API bool wkcFontEngineGetFamilyNamePeer(int in_id, char* out_buf, int in_buflen);
/**
@brief Set the font specified the fontID as primaty font
@param fontID register id
@retval !=fail Success
@retval ==fail Fail
@details
Deprecated.
*/
WKC_PEER_API bool wkcFontEngineSetPrimaryFontPeer(int fontID);
/**
@brief Queries static heap size for font engine.
@retval Required heap size in bytes.
@details
WKC will call this peer to query whether the font engine requires static heap memory.
If required, font engine should return required size of bytes of static heap.
WKC will allocate the statis memory with the size and pass it to the argument of wkcFontEngineInitializePeer().
If not require, please return some dummy value (!=0). 1024 would be appropriate.
*/
WKC_PEER_API int  wkcFontEngineHeapSizePeer(void);

/**
@brief Sets font scale for specified font
@retval success or not
@details
Sets font scale for specified font
*/
WKC_PEER_API bool wkcFontEngineSetFontScalePeer(int in_id, float in_scale);

/**
@brief Create new font instance
@param in_size font size in pixel
@param in_weight font weight.@n
see http://www.w3.org/wiki/CSS/Properties/font-weight
@li WKC_FONT_WEIGHT_100
@li WKC_FONT_WEIGHT_200
@li WKC_FONT_WEIGHT_300
@li WKC_FONT_WEIGHT_400
@li WKC_FONT_WEIGHT_500
@li WKC_FONT_WEIGHT_600
@li WKC_FONT_WEIGHT_700
@li WKC_FONT_WEIGHT_800
@li WKC_FONT_WEIGHT_900
@param in_italic italic
@param in_horizontal font orientation is horizontal or vertical
@param in_verticalright text orientation is vertical-right or upright
@param in_family abstract font family.@n
see http://www.w3.org/Style/Examples/007/fonts.en.html
@li WKC_FONT_FAMILY_STANDARD standard font
@li WKC_FONT_FAMILY_SERIF serif font
@li WKC_FONT_FAMILY_SANSSERIF sans-serif font
@li WKC_FONT_FAMILY_MONOSPACE monospace font
@li WKC_FONT_FAMILY_CURSIVE cursive font
@li WKC_FONT_FAMILY_FANTASY fantasy font
@li WKC_FONT_FAMILY_PICTOGRAPH pictograph font
@param in_familyname font family name
@li "systemfont" last resort font
@li "-webkit-XXX" abstract font family name with -webkit prefix
@li other font family name specified in css, registered font, etc
@li null use in_family
@retval created font instance
@details
Creates a font instance specified in parameteres.
@attention
If you want to support webfont correctly, in_familyname should be treated carefully.@n
With webfont, WKC will call this peer first with family name specified in CSS, but not registered yet.
At this point, this peer should return null. Then WKC wil try to register the webfont to font engine
and call this peer again with actual font name obtained by wkcFontEngineGetFamilyNamePeer().
So this peer should return registered webfont.
@attention
If in_familyname equal to "systemfont", this peer MUST return some pre-allocated font and MUST NOT return null
to prevent crashing.
*/
WKC_PEER_API void* wkcFontNewPeer(int in_size, int in_weight, bool in_italic, bool in_horizontal, bool in_verticalright, int in_family, const char* in_familyname);
/**
@brief Create copy of font instance
@param in_font original font
@retval newly created font instance
@details
Creates copy of the in_font.
*/
WKC_PEER_API void* wkcFontNewCopyPeer(void* in_font);
/**
@brief Delete font instance
@param in_font font instance
@details
Deletes font instance.
*/
WKC_PEER_API void wkcFontDeletePeer(void* in_font);
/**
@brief Return size of font instance
@param in_font font instance
@retval size of font instance
@details
Returns size of font instance itself.
*/
WKC_PEER_API int wkcFontSizeOfFontInstancePeer(void* in_font);

/**
@brief Return whether the font can support scaling
@param in_font font instance
@retval !=false Can support
@retval ==false Cannot support
@details
Returns whether the font can be scaled while drawing. If it can support, wkcFontDrawTextPeer() will called with appropriate scale value.
If not, WKC will scale internally.
*/
WKC_PEER_API bool wkcFontCanScalePeer(void* in_font);
/**
@brief Return whether the font can support drawing complex text.
@param in_font font instance
@retval !=false Can support
@retval ==false Cannot support
@details
Return whether the font can support drawing complex (eg. RtL, ligature, etc) text.
If it can support, wkcFontDrawTextPeer() will be called just with corresponding flag and all complex text drawings are up to font engine.
if not, WKC will try to draw complex text internally and tiny.
*/
WKC_PEER_API bool wkcFontCanSupportDrawComplexPeer(void* in_font);
/**
@brief Reserved for future implementations.
@param in_font font instance
@retval !=false Can support
@retval ==false Cannot support
@details
Reserved for future implementations.
*/
WKC_PEER_API bool wkcFontCanTransformPeer(void* in_font);
/**
@brief Determine whether the both font instance is same
@param in_font_1 font instance 1
@param in_font_2 font instance 2
@retval !=false same
@retval ==false not same
@details
Determine whether the both 2 of fonts are same instances.
*/
WKC_PEER_API bool wkcFontIsEqualPeer(void* in_font_1, void* in_font_2);
/**
@brief Reserved for future implementations.
@param in_font font instance
@param in_a matrix
@param in_b matrix
@param in_c matrix
@param in_d matrix
@param in_e matrix
@param in_f matrix
@retval !=false Can support
@retval ==false Cannot support
@details
Reserved for future implementations.
*/
WKC_PEER_API void wkcFontSetMatrixPeer(void* in_font, float in_a, float in_b, float in_c, float in_d, float in_e, float in_f);
/**
@brief Get the size of the font
@param in_font font instance
@retval font size in pixel
@details
Returns the font size in pixel.
*/
WKC_PEER_API int wkcFontGetSizePeer(void* in_font);
/**
@brief Get the ascent of the font
@param in_font font instance
@retval ascent in pixel
@details
Returns the ascent of the font in pixel.
*/
WKC_PEER_API float wkcFontGetAscentPeer(void* in_font);
/**
@brief Get the descent of the font
@param in_font font instance
@retval descent in pixel
@details
Returns the descent of the font in pixel.
*/
WKC_PEER_API float wkcFontGetDescentPeer(void* in_font);
/**
@brief Get the line-spacing of the font
@param in_font font instance
@retval line spacing in pixel
@details
Returns the line-spacing of the font in pixel.
*/
WKC_PEER_API float wkcFontGetLineSpacingPeer(void* in_font);
/**
@brief Get the line-gap of the font
@param in_font font instance
@retval line gap in pixel
@details
Returns the line-gap of the font in pixel.
*/
WKC_PEER_API float wkcFontGetLineGapPeer(void* in_font);
/**
@brief Get the x-height of the font
@param in_font font instance
@retval x-height in pixel
@details
Returns the x-height of the font in pixel.
*/
WKC_PEER_API float wkcFontGetXHeightPeer(void* in_font);
/**
@brief Get the average character width of the font
@param in_font font instance
@retval average character width in pixel
@details
Returns the average character width of the font in pixel.
*/
WKC_PEER_API float wkcFontGetAverageCharWidthPeer(void* in_font);
/**
@brief Get the max character width of the font
@param in_font font instance
@retval max character width in pixel
@details
Returns the max character width of the font in pixel.
*/
WKC_PEER_API float wkcFontGetMaxCharWidthPeer(void* in_font);
/**
@brief Returns whether the font is fixed-sized font
@param in_font font instance
@retval !=false fixed size font
@retval ==false variable size font
@details
Returns whether the font is fixed-sized font or not.
*/
WKC_PEER_API bool wkcFontIsFixedFontPeer(void* in_font);
/**
@brief Get text width of the string
@param in_font font instance
@param in_flags flags
@li WKC_FONT_FLAG_NONE none 
@li WKC_FONT_FLAG_OVERRIDE_BIDI override unicode bidi property
@li WKC_FONT_FLAG_LTR Left to Right
@li WKC_FONT_FLAG_RTL Right to Left
@param in_str string
@param in_strlen length of string
@param out_clip_width clipping width (may be bigger than text width. eg: italic font)
@retval text width
@details
Returns the text width and clipping width of the string.
*/
WKC_PEER_API float wkcFontGetTextWidthPeer(void* in_font, int in_flags, const unsigned short* in_str, int in_strlen, float* out_clip_width);
/**
                                 void* in_offscreen, const WKCSize* in_offscreensize,
                                 int in_rowbytes, int in_fmt,
                                 const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip,
                                 int in_drawingmode, unsigned int in_color, unsigned int in_strokecolor, int in_strokestyle, float in_strokethickness, float in_zoomlevel)
@brief Draws text onto offscreen
@param in_font font instance
@param in_flags flags
@li WKC_FONT_FLAG_NONE none
@li WKC_FONT_FLAG_OVERRIDE_BIDI override unicode bidi property
@li WKC_FONT_FLAG_LTR Left to Right
@li WKC_FONT_FLAG_RTL Right to Left
@param in_str string
@param in_strlen length of string
@param in_offscreen offscreen
@param in_offscreensize offscreen size
@param in_rowbytes row-bytes of offscreen
@param in_fmt format of offscreen
@param in_textbox bounding box of string
@param in_clip clipping rect for drawing (may be smaller than in_textbox)
@param in_drawingmode drawing mode: or-ed parameters of following items
@li WKC_FONT_DRAWINGMODE_INVISIBLE invisible
@li WKC_FONT_DRAWINGMODE_FILL fill
@li WKC_FONT_DRAWINGMODE_STROKE stroke
@li WKC_FONT_DRAWINGMODE_CLIP clip
@param in_color color (ARGB32 format)
@param in_strokecolor color (ARGB32 format)
@param in_strokestyle stroke style
@li WKC_STROKESTYLE_NO no
@li WKC_STROKESTYLE_SOLID solid
@li WKC_STROKESTYLE_DOTTED dotted
@li WKC_STROKESTYLE_DASHED dashed
@param in_strokethickness stroke thickness
@param in_zoomlevel zooming level
@retval actual width of drawn string
@details
Draws text with specified font onto the off-screen.
@remarks
Even if the in_font is created with vertical mode, this peer should write text horizontally.
WKC will rotate the drawn text to vertical.@n
In this case, if the in_str contains CJK characters, these characters should be
written with rotated.
*/
WKC_PEER_API float wkcFontDrawTextPeer(void* in_font, int in_flags, const unsigned short* in_str, int in_strlen,
                                 void* in_offscreen, const WKCSize* in_offscreensize,
                                 int in_rowbytes, int in_fmt,
                                 const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip,
                                 int in_drawingmode, unsigned int in_color, unsigned int in_strokecolor, int in_strokestyle, float in_strokethickness,
                                 float in_zoomlevel);


WKC_PEER_API void wkcFontSetNotifyNoMemoryProcPeer(void(*in_proc)());

WKC_PEER_API bool wkcFontGetGlyphsForCharactersPeer(void* in_font, const unsigned short* in_str, int in_strlen, unsigned short* out_glyphs);

// skin peer

enum {
    WKC_IMAGE_CHECKBOX_UNCHECKED = 0,
    WKC_IMAGE_CHECKBOX_CHECKED,
    WKC_IMAGE_CHECKBOX_UNCHECKED_DISABLED,
    WKC_IMAGE_CHECKBOX_CHECKED_DISABLED,
    WKC_IMAGE_CHECKBOX_PRESSED,
    WKC_IMAGE_CHECKBOX_UNCHECKED_FOCUSED,
    WKC_IMAGE_CHECKBOX_CHECKED_FOCUSED,
    WKC_IMAGE_RADIO_UNCHECKED,
    WKC_IMAGE_RADIO_CHECKED,
    WKC_IMAGE_RADIO_UNCHECKED_DISABLED,
    WKC_IMAGE_RADIO_CHECKED_DISABLED,
    WKC_IMAGE_RADIO_PRESSED,
    WKC_IMAGE_RADIO_UNCHECKED_FOCUSED,
    WKC_IMAGE_RADIO_CHECKED_FOCUSED,
    WKC_IMAGE_BUTTON,
    WKC_IMAGE_BUTTON_DISABLED,
    WKC_IMAGE_BUTTON_PRESSED,
    WKC_IMAGE_BUTTON_HOVERED,
    WKC_IMAGE_MENU_LIST_BUTTON,
    WKC_IMAGE_MENU_LIST_BUTTON_DISABLED,
    WKC_IMAGE_MENU_LIST_BUTTON_PRESSED,
    WKC_IMAGE_MENU_LIST_BUTTON_FOCUSED,
    WKC_IMAGE_V_SCROLLBAR_BACKGROUND,
    WKC_IMAGE_V_SCROLLBAR_BACKGROUND_DISABLED,
    WKC_IMAGE_V_SCROLLBAR_THUMB,
    WKC_IMAGE_V_SCROLLBAR_THUMB_HOVERED,
    WKC_IMAGE_V_SCROLLBAR_UP,
    WKC_IMAGE_V_SCROLLBAR_UP_DISABLED,
    WKC_IMAGE_V_SCROLLBAR_UP_HOVERED,
    WKC_IMAGE_V_SCROLLBAR_DOWN,
    WKC_IMAGE_V_SCROLLBAR_DOWN_DISABLED,
    WKC_IMAGE_V_SCROLLBAR_DOWN_HOVERED,
    WKC_IMAGE_H_SCROLLBAR_BACKGROUND,
    WKC_IMAGE_H_SCROLLBAR_BACKGROUND_DISABLED,
    WKC_IMAGE_H_SCROLLBAR_THUMB,
    WKC_IMAGE_H_SCROLLBAR_THUMB_HOVERED,
    WKC_IMAGE_H_SCROLLBAR_LEFT,
    WKC_IMAGE_H_SCROLLBAR_LEFT_DISABLED,
    WKC_IMAGE_H_SCROLLBAR_LEFT_HOVERED,
    WKC_IMAGE_H_SCROLLBAR_RIGHT,
    WKC_IMAGE_H_SCROLLBAR_RIGHT_DISABLED,
    WKC_IMAGE_H_SCROLLBAR_RIGHT_HOVERED,
    WKC_IMAGE_SCROLLBAR_CROSS_CORNER,
    WKC_IMAGE_SCROLLBAR_CROSS_CORNER_DISABLED,
    WKC_IMAGE_V_RANGE,
    WKC_IMAGE_V_RANGE_DISABLED,
    WKC_IMAGE_V_RANGE_PRESSED,
    WKC_IMAGE_V_RANGE_HOVERED,
    WKC_IMAGE_H_RANGE,
    WKC_IMAGE_H_RANGE_DISABLED,
    WKC_IMAGE_H_RANGE_PRESSED,
    WKC_IMAGE_H_RANGE_HOVERED,
    WKC_IMAGE_SEARCHFIELD_CANCELBUTTON,
    WKC_IMAGE_SEARCHFIELD_RESULTBUTTON,

    WKC_IMAGE_MAX_NUM
};

enum {
    // system colors
    WKC_SKINCOLOR_ACTIVEBORDER = 0,
    WKC_SKINCOLOR_ACTIVECAPTION,
    WKC_SKINCOLOR_APPWORKSPACE,
    WKC_SKINCOLOR_BACKGROUND,
    WKC_SKINCOLOR_BUTTONFACE,
    WKC_SKINCOLOR_BUTTONHIGHLIGHT,
    WKC_SKINCOLOR_BUTTONSHADOW,
    WKC_SKINCOLOR_BUTTONTEXT,
    WKC_SKINCOLOR_CAPTIONTEXT,
    WKC_SKINCOLOR_GRAYTEXT,
    WKC_SKINCOLOR_HIGHLIGHT,
    WKC_SKINCOLOR_HIGHLIGHTTEXT,
    WKC_SKINCOLOR_INACTIVEBORDER,
    WKC_SKINCOLOR_INACTIVECAPTION,
    WKC_SKINCOLOR_INACTIVECAPTIONTEXT,
    WKC_SKINCOLOR_INFOBACKGROUND,
    WKC_SKINCOLOR_INFOTEXT,
    WKC_SKINCOLOR_MENU,
    WKC_SKINCOLOR_MENUTEXT,
    WKC_SKINCOLOR_SCROLLBAR,
    WKC_SKINCOLOR_TEXT,
    WKC_SKINCOLOR_THREEDDARKSHADOW,
    WKC_SKINCOLOR_THREEDFACE,
    WKC_SKINCOLOR_THREEDHIGHLIGHTA,
    WKC_SKINCOLOR_THREEDLIGHTSHADOW,
    WKC_SKINCOLOR_THREEDSHADOW,
    WKC_SKINCOLOR_WINDOW,
    WKC_SKINCOLOR_WINDOWFRAME,
    WKC_SKINCOLOR_WINDOWTEXT,

    // text selection colors
    WKC_SKINCOLOR_ACTIVESELECTIONFOREGROUND,
    WKC_SKINCOLOR_ACTIVESELECTIONBACKGROUND,
    WKC_SKINCOLOR_INACTIVESELECTIONFOREGROUND,
    WKC_SKINCOLOR_INACTIVESELECTIONBACKGROUND,

    // focus ring
    WKC_SKINCOLOR_FOCUSRING,

    // text-field
    WKC_SKINCOLOR_TEXTFIELD_BORDER,
    WKC_SKINCOLOR_TEXTFIELD_BACKGROUND,

    // progress bar
    WKC_SKINCOLOR_PROGRESSBAR_BORDER,
    WKC_SKINCOLOR_PROGRESSBAR_BODY,
    WKC_SKINCOLOR_PROGRESSBAR_BACKGROUND,
    WKC_SKINCOLOR_PROGRESSBAR_BACKGROUND_DISABLED,

    // range / slider
    WKC_SKINCOLOR_RANGE_BORDER,
    WKC_SKINCOLOR_RANGE_BACKGROUND_DISABLED,

    WKC_SKINCOLOR_MAX_NUM
};

/** @brief Structure for storing skin image */
struct wkcSkinImage_ {
    /** @brief Image size */
    WKCSize fSize;
    /**
       @brief Coordinates of image cutoff point
       @details
       For information about coordinate descriptions, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
    */
    WKCPoint fPoints[4];
    /**
       @brief Image data
       @details
       rowbytes must be fSize.fWidth * type (3 or 4)
    */
    void* fBitmap;
};
/** @brief Type definition of wkcSkinImage */
typedef struct wkcSkinImage_  wkcSkinImage;

/** @brief Structure for storing resource image */
struct wkcSkinResourceImage_ {
    /** @brief Resource name */
    const char* fName;
    /** @brief Image size */
    WKCSize fSize;
    /**
       @brief Image data
       @details
       rowbytes must be fSize.fWidth * 4
    */
    void* fBitmap;
};
/** @brief Type definition of wkcSkinResourceImage */
typedef struct wkcSkinResourceImage_ wkcSkinResourceImage;

enum {
    WKC_SYSTEMFONT_TYPE_CAPTION = 0,
    WKC_SYSTEMFONT_TYPE_ICON,
    WKC_SYSTEMFONT_TYPE_MENU,
    WKC_SYSTEMFONT_TYPE_MESSAGE_BOX,
    WKC_SYSTEMFONT_TYPE_SMALL_CAPTION,
    WKC_SYSTEMFONT_TYPE_WEBKIT_MINI_CONTROL,
    WKC_SYSTEMFONT_TYPE_WEBKIT_SMALL_CONTROL,
    WKC_SYSTEMFONT_TYPE_WEBKIT_CONTROL,
    WKC_SYSTEMFONT_TYPE_STATUS_BAR,
    
    WKC_SYSTEMFONT_TYPES
};

/** @brief Structure for storing skin data */
struct wkcSkin_ {
    /** @brief Array of structures for storing skin image data */
    wkcSkinImage fImages[WKC_IMAGE_MAX_NUM];
    /** @brief List of structures for storing resource image data */
    wkcSkinResourceImage** fResourceImages;
    /** @brief scale of each skin image */
    float fImageScale;
    /** @brief Array for storing color information for each color identifier */
    unsigned int fColors[WKC_SKINCOLOR_MAX_NUM];
    /** @brief Array for storing size for each font type identifier */
    float fSystemFontSize[WKC_SYSTEMFONT_TYPES];
    /** @brief Array for storing family name for each font type identifier */
    const char* fSystemFontFamilyName[WKC_SYSTEMFONT_TYPES];

    /** @brief Default style sheet */
    const char* fDefaultStyleSheet;
    /** @brief Compatibility mode style sheet */
    const char* fQuirksStyleSheet;
};
/** @brief Type definition of wkcSkin */
typedef struct wkcSkin_ wkcSkin;

/**
@brief Gets skin image bitmap data
@param in_image_id Skin image type
@return skin image bitmap data

@details
Returns the bitmap to be stored in wkcSkin::fImages[] that supports in_image_id.@n
For information about types, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
*/
WKC_PEER_API const unsigned char* wkcStockImageGetBitmapPeer(int in_image_id);
/**
@brief Gets skin image size
@param in_image_id Skin image type
@param out_width Image width
@param out_height Image height
@details
Returns the image size to be stored in wkcSkin::fImages[] that supports in_image_id.@n
For information about types, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
*/
WKC_PEER_API void wkcStockImageGetSizePeer(int in_image_id, unsigned int *out_width, unsigned int *out_height);
/**
@brief Gets scale of skin images
@retval  scale
@details
Returns the scale of skin images
*/
WKC_PEER_API float wkcStockImageGetImageScalePeer(void);
/**
@brief Gets image cutoff point coordinates of skin image
@param in_image_id Skin image type
@return image cutoff point coordinates
@details
Returns the image cutoff coordinates to be stored in wkcSkin::fImages[] that supports in_image_id.@n
For information about types, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
*/
WKC_PEER_API const WKCPoint* wkcStockImageGetLayoutPointsPeer(int in_image_id);
/**
@brief Gets color information
@param in_color_id Color type
@return 32-bit value of color
@details
Returns 32-bit value that supports in_color_id.@n
For information about types, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
*/
WKC_PEER_API unsigned int wkcStockImageGetSkinColorPeer(int in_color_id);
/**
@brief Gets font size used for control
@param in_control_type Control type
@return font size
@details
Returns the font size used for controls that support in_control_type.@n
For information about types, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
*/
WKC_PEER_API float wkcStockImageGetSystemFontSizePeer(int in_control_type);
/**
@brief Gets font family name used for control
@param in_control_type Control type
@return font family name
@details
Returns the font family name used for controls that support in_control_type.@n
For information about types, see the section "UI Implementation - Skin" in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
*/
WKC_PEER_API const char* wkcStockImageGetSystemFontFamilyNamePeer(int in_control_type);
/**
@brief Registers skin
@param in_skin
@details
The skin structure set by an application is passed.
*/
WKC_PEER_API void wkcStockImageRegisterSkinPeer(const wkcSkin* in_skin);
/**
@brief Gets default style sheet
@return the default mode style sheet string
@details
Returns the default style sheet string.
*/
WKC_PEER_API const char* wkcStockImageGetDefaultStyleSheetPeer(void);
/**
@brief Gets compatibility mode style sheet
@return the compatibility mode style sheet string
@details
Returns the compatibility mode style sheet string.
*/
WKC_PEER_API const char* wkcStockImageGetQuirksStyleSheetPeer(void);
/**
@brief Gets resource image
@param in_name Image name
@param out_width Image width
@param out_height Image height
@return Resource image bitmap data
@details
Returns the resource image.
*/
WKC_PEER_API const unsigned char* wkcStockImageGetPlatformResourceImagePeer(const char* in_name, unsigned int* out_width, unsigned int* out_height);


// graphics peer

// Below peers need not to implement in customer side. Use existing implementations.

enum {
//    WKC_OFFSCREEN_TYPE_RGBA5650 = 0, // deprecated
//    WKC_OFFSCREEN_TYPE_ARGB8888, // deprecated
    WKC_OFFSCREEN_TYPE_POLYGON = 2,
    WKC_OFFSCREEN_TYPE_CAIRO16,
    WKC_OFFSCREEN_TYPE_CAIRO32,
    WKC_OFFSCREEN_TYPE_CAIROSURFACE,

    // for internal use only
    WKC_OFFSCREEN_TYPE_A8,
    WKC_OFFSCREEN_TYPE_IMAGEBUF,
    WKC_OFFSCREEN_TYPE_ABGR8888,

    WKC_OFFSCREEN_TYPE_TRANSPLAYER,

    WKC_OFFSCREEN_TYPES,
};

enum {
    WKC_COMPOSITEOPERATION_CLEAR = 1,
    WKC_COMPOSITEOPERATION_COPY,
    WKC_COMPOSITEOPERATION_SOURCEOVER,
    WKC_COMPOSITEOPERATION_SOURCEIN,
    WKC_COMPOSITEOPERATION_SOURCEOUT,
    WKC_COMPOSITEOPERATION_SOURCEATOP,
    WKC_COMPOSITEOPERATION_DESTINATIONOVER,
    WKC_COMPOSITEOPERATION_DESTINATIONIN,
    WKC_COMPOSITEOPERATION_DESTINATIONOUT,
    WKC_COMPOSITEOPERATION_DESTINATIONATOP,
    WKC_COMPOSITEOPERATION_XOR,
    WKC_COMPOSITEOPERATION_PLUSDARKER,
    WKC_COMPOSITEOPERATION_HIGHLIGHT,
    WKC_COMPOSITEOPERATION_PLUSLIGHTER,
    WKC_COMPOSITEOPERATION_TEXT,
    WKC_COMPOSITEOPERATIONS
};

enum {
    WKC_LINECAP_NONE = 0,
    WKC_LINECAP_BUTTCAP,
    WKC_LINECAP_ROUNDCAP,
    WKC_LINECAP_SQUARECAP,
    WKC_LINECAPS
};

enum {
    WKC_LINEJOIN_NONE = 0,
    WKC_LINEJOIN_MITERJOIN,
    WKC_LINEJOIN_ROUNDJOIN,
    WKC_LINEJOIN_BEVELJOIN,
    WKC_LINEJOINS
};

enum {
    WKC_STROKESTYLE_NO = 0,
    WKC_STROKESTYLE_SOLID,
    WKC_STROKESTYLE_DOTTED,
    WKC_STROKESTYLE_DASHED,
    WKC_STROKESTYLES
};

enum {
    WKC_FILLRULE_WINDING,
    WKC_FILLRULE_EVENODD,
    WKC_FILLRULES
};

#define WKC_DRAWTEXT_NONE             (0x00000000)
#define WKC_DRAWTEXT_COMPLEX          (0x00000001)
#define WKC_DRAWTEXT_COMPLEX_RTL      (0x00000002)
#define WKC_DRAWTEXT_COMPLEX_TINY_RTL (0x00000004)
#define WKC_DRAWTEXT_OVERRIDE_BIDI    (0x00000010)

#define WKC_IMAGETYPE_ARGB8888           0x00000000
#define WKC_IMAGETYPE_RGB565             0x00000001
#define WKC_IMAGETYPE_TEXTURE            0x00000004
#define WKC_IMAGETYPE_TYPEMASK           0x0000ffff
#define WKC_IMAGETYPE_FLAG_HASALPHA      0x00010000
#define WKC_IMAGETYPE_FLAG_HASTRUEALPHA  0x00020000
#define WKC_IMAGETYPE_FLAG_FORSKIN       0x00100000
#define WKC_IMAGETYPE_FLAG_STEREO_M      0x00000000
#define WKC_IMAGETYPE_FLAG_STEREO_L      0x01000000
#define WKC_IMAGETYPE_FLAG_STEREO_R      0x02000000
#define WKC_IMAGETYPE_FLAG_STEREOMASK    0x0f000000

WKC_PEER_API void* wkcOffscreenNewPeer(int in_type, void* in_bitmap, int in_rowbytes, const WKCSize* in_size);
WKC_PEER_API bool wkcOffscreenIsAcceleratedPeer(void* in_offscreen);
WKC_PEER_API void* wkcTextureNewPeer(const WKCSize* in_size);
WKC_PEER_API void wkcTextureSetImagePeer(void* tex ,void* in_image);
WKC_PEER_API void wkcTextureClearImagePeer(void* tex ,void* in_rect);
WKC_PEER_API void wkcOffscreenDeletePeer(void* in_offscreen);
WKC_PEER_API void wkcTextureDeletePeer(void* in_texture);
WKC_PEER_API void wkcOffscreenForceTerminatePeer(void);
WKC_PEER_API void wkcOffscreenResizePeer(void* in_offscreen, const WKCSize* in_size);
WKC_PEER_API void wkcOffscreenGetSizePeer(void* in_offscreen, WKCSize* out_size);
WKC_PEER_API void wkcOffscreenScrollPeer(void* in_offscreen, const WKCRect* in_rect, const WKCSize* in_diff);
WKC_PEER_API void wkcOffscreenSetOpticalZoomPeer(void* in_offscreen, float in_zoomlevel, const WKCFloatPoint* in_offset);
WKC_PEER_API void wkcOffscreenSetUseInterpolationForImagePeer(void* in_offscreen, bool in_flag);
WKC_PEER_API void wkcOffscreenSetUseAntiAliasForPolygonPeer(void* in_offscreen, bool in_flag);
WKC_PEER_API void* wkcOffscreenBitmapPeer(void* in_offscreen, int* out_rowbytes);
WKC_PEER_API void wkcOffscreenSetScrollPositionPeer(void* in_offscreen, const WKCPoint* in_pos);
WKC_PEER_API void wkcOffscreenBeginPaintPeer(void* in_offscreen);
WKC_PEER_API void wkcOffscreenEndPaintPeer(void* in_offscreen);

enum {
    WKC_OFFSCREEN_FLUSH_FOR_READPIXELS = 0,
    WKC_OFFSCREEN_FLUSH_FOR_DRAW,
    WKC_OFFSCREEN_FLUSH_PURPOSES
};
WKC_PEER_API void wkcOffscreenFlushPeer(void* in_offscreen, int purpos);

WKC_PEER_API void wkcOffscreenGetPixelsPeer(void* in_offscreen, void* out_pixels, const WKCRect *in_rect);
WKC_PEER_API void wkcOffscreenPutPixelsPeer(void* in_offscreen, void* in_pixels, const WKCRect *in_rect);

WKC_PEER_API bool wkcOffscreenCreateGlyphCachePeer(int in_format, void* in_cache, const WKCSize* in_size);
WKC_PEER_API void wkcOffscreenDeleteGlyphCachePeer(void);
WKC_PEER_API void wkcOffscreenClearGlyphCachePeer(void);
WKC_PEER_API bool wkcOffscreenCreateImageCachePeer(int in_format, void* in_cache, const WKCSize* in_size);
WKC_PEER_API void wkcOffscreenDeleteImageCachePeer(void);
WKC_PEER_API void wkcOffscreenClearImageCachePeer(void);
#ifdef USE_WKC_CAIRO
WKC_PEER_API bool wkcOffscreenIsErrorPeer(void* in_offscreen);
#endif

WKC_PEER_API int wkcOffscreenGetRenderingTargetPeer(void* in_offscreen);

typedef struct WKCPeerFont_ {
    void* fFont;
    void* fFontId;
    int fRequestedSize;
    int fCreatedSize;
    int fWeight;
    bool fItalic;
    float fScale;
    float fiScale;
    bool fCanScale;
    bool fHorizontal;
} WKCPeerFont;

typedef struct WKCPeerImage_ {
    int fType;
    void* fBitmap;
    int fRowBytes;
    void* fMask;
    int fMaskRowBytes;
    WKCFloatRect fSrcRect;
    WKCFloatSize fScale;
    WKCFloatSize fiScale;
    WKCFloatPoint fPhase;
    WKCFloatSize fiTransform;
    bool fRepeatX;
    bool fRepeatY;

    // for image-char
    void* fFontId;
    unsigned int fChar;

    // pointer to the offscreen storing the image on GPU if there is one
    void* fOffscreen;  // for canvas

    void* fTexture; // for images

    bool fBlendWithAlpha; // use an alpha coefficient when blitting
    unsigned int fAlpha; // the alpha coefficient to be used when blitting if fBlendWithAlpha
} WKCPeerImage;

#define WKCPEERGRADIENT_MAXSTOPS (32)

typedef struct WKCPeerGradient_ {
    void* fSelf;
    bool fRadial;
    WKCFloatPoint fPoint0;
    WKCFloatPoint fPoint1;
    float fMatrix[4];
    int fStops;
    float fStop[WKCPEERGRADIENT_MAXSTOPS];
    float fR0;
    float fR1;
    void (*fGetColorProc)(void* self, float in_value, float* out_r, float* out_g, float* out_b, float* out_a);
} WKCPeerGradient;

enum {
    WKC_PATTERN_IMAGE = 0,
    WKC_PATTERN_IMAGE_CHAR,
    WKC_PATTERN_GRADIENT,
    WKC_PATTERNS
};

typedef struct WKCPeerPattern_ {
    int fType;
    union Patterns {
        WKCPeerImage fImage;
        WKCPeerGradient fGradient;
    } u;
} WKCPeerPattern;

WKC_PEER_API void wkcDrawContextInitializePeer(void);
WKC_PEER_API void wkcDrawContextFinalizePeer(void);
WKC_PEER_API void wkcDrawContextForceTerminatePeer(void);

WKC_PEER_API void* wkcDrawContextNewPeer(void* in_offscreen);
WKC_PEER_API void wkcDrawContextDeletePeer(void* in_context);

WKC_PEER_API void wkcDrawContextSaveStatePeer(void* in_context);
WKC_PEER_API void wkcDrawContextRestoreStatePeer(void* in_context);
WKC_PEER_API bool wkcDrawContextCanHandleStereoViewPeer(void* in_context);

WKC_PEER_API void wkcDrawContextSetStrokeColorPeer(void* in_context, unsigned int in_color);
WKC_PEER_API void wkcDrawContextSetStrokeThicknessPeer(void* in_context, float in_thickness);
WKC_PEER_API void wkcDrawContextSetStrokeStylePeer(void* in_context, int in_style);
WKC_PEER_API void wkcDrawContextSetLineDashPeer(void* in_context, float* in_dashes, int in_dashes_size, float in_offset);
WKC_PEER_API void wkcDrawContextSetPenStylePeer(void* in_context, unsigned int in_color, float in_thickness, int in_style);
WKC_PEER_API void wkcDrawContextSetFillColorPeer(void* in_context, unsigned int in_color);
WKC_PEER_API int wkcDrawContextGetFillRulePeer(void* in_context);
WKC_PEER_API void wkcDrawContextSetFillRulePeer(void* in_context, int in_full_rule);
WKC_PEER_API void wkcDrawContextSetLineCapPeer(void* in_context, int in_cap);
WKC_PEER_API void wkcDrawContextSetLineJoinPeer(void* in_context, int in_join);
WKC_PEER_API void wkcDrawContextSetMiterLimitPeer(void* in_context, float in_limit);
WKC_PEER_API void wkcDrawContextSetAlphaPeer(void* in_context, int in_alpha);
WKC_PEER_API void wkcDrawContextSetPatternPeer(void* in_context, const WKCPeerPattern* in_pattern);
WKC_PEER_API void wkcDrawContextSetDrawAccuratePeer(void* in_context, bool in_flag);
WKC_PEER_API void wkcDrawContextSetTextDrawingModePeer(void* in_context, int in_mode);

WKC_PEER_API void wkcDrawContextSetShouldAntialiasPeer(void* in_context, int in_shouldantialias);
WKC_PEER_API void wkcDrawContextSetShadowPeer(void* in_context, int in_shadow_api, const WKCFloatRect *in_css_box_edge, const WKCFloatRect *in_css_box_hole, const WKCFloatSize *in_css_box_radii, const WKCFloatSize* in_shadow_offset, float in_shadow_blur, unsigned int in_shadow_color, bool in_shadows_ignore_transforms);
WKC_PEER_API void wkcDrawContextClearShadowPeer(void* in_context);
WKC_PEER_API void wkcDrawContextBeginTransparencyLayerPeer(void* in_context, unsigned char in_alpha);
WKC_PEER_API void wkcDrawContextEndTransparencyLayerPeer(void* in_context);

WKC_PEER_API void wkcDrawContextSetCompositeOperationPeer(void* in_context, int in_op);

WKC_PEER_API void wkcDrawContextSetMatrixPeer(void* in_context, float in_a, float in_b, float in_c, float in_d, float in_e, float in_f);
WKC_PEER_API void wkcDrawContextSetInvertMatrixPeer(void* in_context, float in_a, float in_b, float in_c, float in_d, float in_e, float in_f);

WKC_PEER_API void wkcDrawContextClipPeer(void* in_context, const WKCFloatRect* in_rect);
WKC_PEER_API void wkcDrawContextClipPathPeer(void* in_context, const void* in_path);
WKC_PEER_API void wkcDrawContextCanvasClipPeer(void* in_context, const void* in_path);
WKC_PEER_API void wkcDrawContextClipToImageBufferPeer(void* in_context, const WKCFloatRect* in_rect, void* in_image, int in_rowbytes, const WKCFloatRect* in_imagerect);
WKC_PEER_API void wkcDrawContextClipOutPathPeer(void* in_context, const void* in_path);
WKC_PEER_API void wkcDrawContextClipOutRectPeer(void* in_context, const WKCFloatRect* in_rect);
WKC_PEER_API void wkcDrawContextClipOutEllipseInRectPeer(void* in_context, const WKCFloatRect* in_rect);

WKC_PEER_API void wkcDrawContextDrawLinePeer(void* in_context, const WKCFloatPoint* in_pos1, const WKCFloatPoint* in_pos2);
WKC_PEER_API void wkcDrawContextDrawLineForTextPeer(void* in_context, const WKCFloatPoint* in_origin, float in_width, int in_printing);
WKC_PEER_API void wkcDrawContextDrawLineForMisspellingOrBadGrammarPeer(void* in_context, const WKCFloatPoint* in_origin, float in_width, int in_grammar);
WKC_PEER_API void wkcDrawContextDrawEllipsePeer(void* in_context, const WKCFloatRect* in_rect);
WKC_PEER_API void wkcDrawContextStrokeArcPeer(void* in_context, const WKCFloatRect* in_rect, int in_startangle, int in_anglespan);
WKC_PEER_API void wkcDrawContextDrawTextPeer(void* in_context, const unsigned short* in_str, int in_len, const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip, WKCPeerFont* in_font, int in_flag);
WKC_PEER_API void wkcDrawContextBitBltPeer(void* in_context, const WKCPeerImage* in_image, const WKCFloatRect* in_destrect, int in_op);
WKC_PEER_API void wkcDrawContextBlitPatternPeer(void* in_context, const WKCPeerImage* in_image, const WKCFloatRect* in_destrect, int in_op);

WKC_PEER_API void wkcDrawContextDrawConvexPolygonPeer(void* in_context, int in_npoints, const WKCFloatPoint* in_points, int in_shouldantialias);
WKC_PEER_API void wkcDrawContextDrawPolygonPeer(void* in_context, int in_npoints, const WKCFloatPoint* in_points);
WKC_PEER_API void wkcDrawContextDrawPolylinePeer(void* in_context, int in_npoints, const WKCFloatPoint* in_points, bool in_closed, bool in_drawjoin);
WKC_PEER_API void wkcDrawContextClearPolygonPeer(void* in_context, int in_npoints, const WKCFloatPoint* in_points);
WKC_PEER_API void wkcDrawContextClipPolygonPeer(void* in_context, int in_npoints, const WKCFloatPoint* in_points);
WKC_PEER_API void wkcDrawContextClipOutPolygonPeer(void* in_context, int in_npoints, const WKCFloatPoint* in_points);
WKC_PEER_API void wkcDrawContextClearClipPolygonPeer(void* in_context);
WKC_PEER_API void wkcDrawContextCanvasClipPathBeginPeer(void* in_context);
WKC_PEER_API void wkcDrawContextCanvasClipPathEndPeer(void* in_context);

WKC_PEER_API void wkcDrawContextDrawRectPeer(void* in_context, const WKCFloatRect* in_rect);
WKC_PEER_API void wkcDrawContextFillRectPeer(void* in_context, const WKCFloatRect* in_rect, unsigned int in_color);
WKC_PEER_API void wkcDrawContextClearRectPeer(void* in_context, const WKCFloatRect* in_rect);
WKC_PEER_API void wkcDrawContextClearPeer(void* in_context);
WKC_PEER_API void wkcDrawContextStrokeRectPeer(void* in_context, const WKCFloatRect* in_rect);

WKC_PEER_API void wkcDrawContextFlushPeer(void* in_context);

WKC_PEER_API void* wkcDrawContextGetOffscreenPeer(void* in_context);
WKC_PEER_API int wkcDrawContextGetOffscreenTypePeer(void* in_context);

WKC_PEER_API void wkcDrawContextSetEmojiGlyphsFolderPeer(const char* in_path);
WKC_PEER_API int wkcDrawContextIsEmojiSequencePeer(const unsigned short* in_str, int in_len);
WKC_PEER_API float wkcDrawContextGetEmojiWidthPeer(void* in_font, const unsigned short* in_str, int in_len, float* out_clipwidth);
WKC_PEER_API void wkcDrawContextClearEmojiCachePeer(void);

#ifdef USE_WKC_CAIRO
typedef void *(*wkcCreateEngineContextProc)(void* in_peer_context);
typedef void (*wkcDestroyEngineContextProc)(void* in_engine_context);
typedef void *(*wkcGetPeerContextProc)(void* in_engine_context);
WKC_PEER_API bool wkcDrawContextSetCallbacksPeer(wkcCreateEngineContextProc in_createproc, wkcDestroyEngineContextProc in_destroyproc, wkcGetPeerContextProc in_getproc);
WKC_PEER_API void wkcDrawContextAddAllocatedObjectPeer(void* in_obj, void (*in_destroy_func)(void* ));
WKC_PEER_API void wkcDrawContextRemoveAllocatedObjectPeer(const void* in_obj);
WKC_PEER_API void wkcDrawContextForceTerminateAllocatedObjectsPeer(void);
WKC_PEER_API void wkcDrawContextSetOpticalZoomPeer(void* in_context, float in_zoomlevel, const WKCFloatPoint* in_offset);
WKC_PEER_API bool wkcDrawContextIsForOffscreenPeer(void* in_context);
WKC_PEER_API bool wkcDrawContextIsForLayerPeer(void* in_context);
WKC_PEER_API void wkcDrawContextSetForLayerPeer(void* in_context, bool in_forlayer);
WKC_PEER_API void* wkcDrawContextCairoSurfaceNewPeer(int in_width, int in_height);
WKC_PEER_API void wkcDrawContextCairoSurfaceDeletePeer(void* in_surface);
WKC_PEER_API void wkcDrawContextCairoSurfaceGetSizePeer(void* in_surface, int* out_width, int* out_height);
WKC_PEER_API bool wkcDrawContextCairoSurfaceIsImagePeer(void* in_surface);
WKC_PEER_API bool wkcDrawContextIsErrorPeer(void* in_context);
WKC_PEER_API bool wkcDrawContexCairoIsUseFilterNearestPeer(void);
WKC_PEER_API void wkcDrawContexCairoSetUseFilterNearestPeer(bool in_use);
# define WKC_CAIRO_ADD_OBJECT(obj, type) wkcDrawContextAddAllocatedObjectPeer((obj), (void (*)(void*))type##_destroy)
# define WKC_CAIRO_REMOVE_OBJECT(obj) wkcDrawContextRemoveAllocatedObjectPeer((obj))
# define WKC_CAIRO_FORCE_TERMINATE() wkcDrawContextForceTerminateAllocatedObjectsPeer()
#endif /* USE_WKC_CAIRO */

// HW peer

enum {
    WKC_HWOFFSCREEN_FRAMEBUFFER_TYPE_OFFSCREEN = 0,
    WKC_HWOFFSCREEN_FRAMEBUFFER_TYPE_IMAGEBUF,
    WKC_HWOFFSCREEN_FRAMEBUFFER_TYPE_TRANSPLAYER,
    WKC_HWOFFSCREEN_FRAMEBUFFER_TYPES
};

enum {
    WKC_HWOFFSCREEN_COLORBUFFER = 0,
    WKC_HWOFFSCREEN_STENCILBUFFER,

    WKC_HWOFFSCREEN_COLORBUFFER_L,
    WKC_HWOFFSCREEN_COLORBUFFER_R,

    WKC_HWOFFSCREEN_RADIALGRADIENT,

    WKC_HWOFFSCREEN_BUFFERS
};

enum {
    WKC_HWOFFSCREEN_FORMAT_A4 = 0,
    WKC_HWOFFSCREEN_FORMAT_A8,
    WKC_HWOFFSCREEN_FORMAT_4444,
    WKC_HWOFFSCREEN_FORMAT_8888,
    WKC_HWOFFSCREEN_FORMATS
};

enum {
    WKC_HWOFFSCREEN_TEXTURETYPE_GLYPH = 0,
    WKC_HWOFFSCREEN_TEXTURETYPE_IMAGE,
    WKC_HWOFFSCREEN_TEXTURETYPES
};

enum {
    WKC_HWOFFSCREEN_PRIMITIVE_TRIANGLE = 0,
    WKC_HWOFFSCREEN_PRIMITIVE_POLYGON,
    WKC_HWOFFSCREEN_PRIMITIVE_LINE,
    WKC_HWOFFSCREEN_PRIMITIVE_LINE_DASHED,
    WKC_HWOFFSCREEN_PRIMITIVES,
};

enum {
    WKC_IMAGEBUFFER_TYPE_DEFAULT = 0,
    WKC_IMAGEBUFFER_TYPE_TRANSPLAYER,
    WKC_IMAGEBUFFER_TYPE_IMAGEBUF,
    WKC_IMAGEBUFFER_TYPES
};

typedef void (*wkcHWOffscreenLockHWProc)(void* in_opaque);
typedef void (*wkcHWOffscreenUnlockHWProc)(void* in_opaque);

typedef struct WKCHWOffscreenParams_ {
    wkcHWOffscreenLockHWProc fLockProc;
    wkcHWOffscreenUnlockHWProc fUnlockProc;

    bool fEnable;
    bool fEnableForImagebuffer;

    unsigned int fScreenWidth;
    unsigned int fScreenHeight;
} WKCHWOffscreenParams;

WKC_PEER_API bool wkcHWOffscreenInitializePeer(void);
WKC_PEER_API void wkcHWOffscreenFinalizePeer(void);
WKC_PEER_API void wkcHWOffscreenForceTerminatePeer(void);
WKC_PEER_API void wkcHWOffscreenSuspendPeer(void);
WKC_PEER_API bool wkcHWOffscreenResumePeer(void);
WKC_PEER_API void wkcHWOffscreenSetParamsPeer(const WKCHWOffscreenParams* in_procs, void* in_opaque);
WKC_PEER_API bool wkcHWOffscreenIsEnabledForImageBufPeer(void);
WKC_PEER_API int wkcHWOffscreenGetMaxVertexBufferSizePeer(void);
WKC_PEER_API int wkcHWOffscreenGetMaxIndexBufferSizePeer(void);
WKC_PEER_API bool wkcHWOffscreenCreateGlyphCachePeer(int in_format, void* in_cache, const WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenDeleteGlyphCachePeer(void);
WKC_PEER_API bool wkcHWOffscreenCreateImageCachePeer(int in_format, void* in_cache, const WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenDeleteImageCachePeer(void);
WKC_PEER_API void* wkcHWOffscreenCreateFrameBufferPeer(int in_type, void* in_targettexture, const WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenDeleteFrameBufferPeer(void* self);
WKC_PEER_API unsigned int wkcHWOffscreenCreateGLTexturePeer(WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenUploadGLTextureContentsPeer(unsigned int in_tid, const void* in_image, WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenClearGLTextureContentsPeer(unsigned int in_tid, WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenDeleteGLTexturePeer(unsigned int in_tid);
WKC_PEER_API void wkcHWOffscreenLockHWPeer(void* self);
WKC_PEER_API void wkcHWOffscreenUnlockHWPeer(void* self);
WKC_PEER_API void wkcHWOffscreenUpdateTexturePeer(void* self, int in_type, void* in_bitmap, const WKCSize* in_size);
WKC_PEER_API void wkcHWOffscreenUploadGlyphTexturePeer(void* self, void* in_bitmap, const WKCSize* in_size, int in_entry);
WKC_PEER_API void wkcHWOffscreenDeleteGlyphTexturePeer(void* self, int in_entry);
WKC_PEER_API void wkcHWOffscreenDrawElementsPeer(void* self, int in_target, int in_type, void* in_vertexes, int in_vertexeslen, unsigned short* in_indices, int in_nindices);
WKC_PEER_API void wkcHWOffscreenDrawLinesPeer(void* self, int in_target, int in_type, void* in_vertexes, int in_vertexeslen, unsigned short* in_indices, int in_nindices,float in_thickness, int in_lineStyle, float in_lineLength, float* in_colors);
WKC_PEER_API void wkcHWOffscreenDrawLineCapPeer(void* self, int in_target, int in_type, void* in_vertexes, int in_vertexeslen, unsigned short* in_indices, int in_nindices);
WKC_PEER_API void wkcHWOffscreenScrollPeer(void* self, int in_target, const WKCFloatRect* in_rect, const WKCFloatSize* in_diff);
WKC_PEER_API void wkcHWOffscreenDrawFrameBufferPeer(void* self, void* in_framebuffer, unsigned int in_npoints, const WKCFloatPoint* in_points, const WKCPeerPattern* in_pt);
WKC_PEER_API void wkcHWOffscreenDrawTexturePeer(void* self, unsigned int in_texid, WKCSize* in_size, unsigned int in_npoints, const WKCFloatPoint* in_points, const WKCPeerPattern* in_pt);
WKC_PEER_API void wkcHWOffscreenReadPixelsPeer(void* self, int in_target, void* out_bitmap, int in_fmt, const WKCRect* in_rect);
WKC_PEER_API void wkcHWOffscreenWritePixelsPeer(void* self, int in_target, const void* in_bitmap, int in_fmt, const WKCRect* in_rect);
WKC_PEER_API void wkcHWOffscreenClearFrameBufferPeer(void* self);
WKC_PEER_API void wkcHWOffscreenSetMatrixPeer(void* self, int in_target, float in_m0, float in_m1, float in_m2, float in_m3, float in_m4, float in_m5);
WKC_PEER_API void wkcHWOffscreenSetClipRectUsagePeer(void* self, bool in_use_clip_rect, const WKCFloatRect *in_clip_rect);
WKC_PEER_API void wkcHWOffscreenSetClipBufferUsagePeer(void *self, int in_clip_b_pass);
WKC_PEER_API void wkcHWOffscreenClipCollapsePeer(void *self);
WKC_PEER_API void wkcHWOffscreenSetStencilUsagePeer(void* self, int in_stencil_usage);
WKC_PEER_API void wkcHWOffscreenBeginPaintPeer(void* self);
WKC_PEER_API void wkcHWOffscreenEndPaintPeer(void* self);
WKC_PEER_API void wkcHWOffscreenEnableCacheTexturePeer(void* self, int in_type, bool in_enable);
WKC_PEER_API void wkcHWOffscreenSetCurGlyphCacheTexturePeer(void* self, int in_entry);
WKC_PEER_API void wkcHWOffscreenSetCompositeOpPeer(void* self, const int in_op);
WKC_PEER_API int wkcHWOffscreenGetRenderTargetPeer(void* self);
WKC_PEER_API int wkcHWOffscreenDrawShadow1Peer(void* self, void *in_shadow, int in_type, void* in_vertexes, int in_vertexeslen, unsigned short* in_indices, int in_nindices, int in_bbox_x, int in_bbox_y, unsigned int in_bbox_w, unsigned int in_bbox_h, float in_blur_level, const float *in_shadow_color, const float *in_shape_color, int in_n_iter);
WKC_PEER_API void wkcHWOffscreenDrawShadow2Peer(void* self, void *in_shadow, int in_bbox_x, int in_bbox_y, unsigned int in_bbox_w, unsigned int in_bbox_h, float in_blur_level, const float *in_shadow_color, const float *in_shape_color, const WKCFloatRect *in_css_box_edge, const WKCFloatRect *in_css_box_hole, const WKCFloatSize *in_css_box_radii);
WKC_PEER_API void wkcHWUploadGradientPatternPeer(void *self, const WKCPeerPattern *in_gradient_pattern);
WKC_PEER_API void wkcHWOffscreenDumpPixelsPeer(void* self, const char* in_s);
WKC_PEER_API void wkcHWOffscreenDrawLayerPeer(const unsigned int in_texture, const float* in_mvp, const WKCFloatSize& in_size, const bool in_backface_on, const float in_opacity, const WKCRect& in_clip, const bool in_yflip, const bool in_is_mask);
WKC_PEER_API void wkcHWOffscreenDrawRootLayerPeer(void);
WKC_PEER_API unsigned int wkcHWOffscreenDrawMaskLayerPeer(const unsigned int in_layer_texture, const WKCFloatSize& in_size, const unsigned int in_mask_texture, const float in_mask_opacity);
WKC_PEER_API void wkcHWOffscreenInitLayerDrawingPeer(void);
WKC_PEER_API void wkcHWOffscreenFinishLayerDrawingPeer(void);
WKC_PEER_API void wkcHWOffscreenFinishUpdateScreenPeer(void);
WKC_PEER_API void wkcHWOffscreenUpdateScreenPeer(void);

enum {
    WKC_LAYER_TYPE_NONE = 0,
    WKC_LAYER_TYPE_OFFSCREEN,
    WKC_LAYER_TYPE_3DCANVAS,
    WKC_LAYER_TYPE_MEDIA,
    WKC_LAYER_TYPES
};

// from WKC/WebKit/WKC/helpers/WKCHelpersEnumsBSDS.h
enum {
    WKC_LAYER_BLENDMODE_NORMAL = 1,
    WKC_LAYER_BLENDMODE_MULTIPLY,
    WKC_LAYER_BLENDMODE_SCREEN,
    WKC_LAYER_BLENDMODE_DARKEN,
    WKC_LAYER_BLENDMODE_LIGHTEN,
    WKC_LAYER_BLENDMODE_OVERLAY,
    WKC_LAYER_BLENDMODE_COLORDODGE,
    WKC_LAYER_BLENDMODE_COLORBURN,
    WKC_LAYER_BLENDMODE_HARDLIGHT,
    WKC_LAYER_BLENDMODE_SOFTLIGHT,
    WKC_LAYER_BLENDMODE_DIFFERENCE,
    WKC_LAYER_BLENDMODE_EXCLUSION,
    WKC_LAYER_BLENDMODE_HUE,
    WKC_LAYER_BLENDMODE_SATURATION,
    WKC_LAYER_BLENDMODE_COLOR,
    WKC_LAYER_BLENDMODE_LUMINOSITY,
    WKC_LAYER_BLENDMODE_PLUSDARKER,
    WKC_LAYER_BLENDMODE_PLUSLIGHTER
};

typedef bool (*wkcLayerTextureMakeProc)(void* in_layer, int in_width, int in_height, int in_bpp, void** out_bitmap, int* out_rowbytes, int* out_width, int* out_height, void** out_opaque_texture, bool is_3dcanvas);
typedef void (*wkcLayerTextureDeleteProc)(void *in_layer, void* in_bitmap);
typedef void (*wkcLayerTextureUpdateProc)(void *in_opaque, int in_width, int in_height, void *in_bitmap, int in_texture);
typedef void (*wkcLayerDidChangeParentProc)(void *in_layer);
typedef bool (*wkcLayerCanAllocateProc)(int in_width, int in_height, int blendmode);
typedef wkcLayerTextureMakeProc wkcLayerTextureChangeProc;
typedef void (*wkcLayerDidChangeParentProc)(void *in_layer);
typedef void (*wkcLayerAttachedGlTexturesProc)(void *in_layer, int* in_textures, int in_num);

WKC_PEER_API void wkcLayerInitializePeer(void);
WKC_PEER_API void wkcLayerFinalizePeer(void);
WKC_PEER_API void wkcLayerForceTerminatePeer(void);
WKC_PEER_API void wkcLayerInitializeCallbacksPeer(wkcLayerTextureMakeProc in_texture_maker_proc, wkcLayerTextureDeleteProc in_texture_deleter_proc, wkcLayerTextureUpdateProc in_texture_updater_proc, wkcLayerTextureChangeProc in_texture_changer_proc, wkcLayerDidChangeParentProc in_did_change_parent_proc, wkcLayerCanAllocateProc in_can_allocate_proc, wkcLayerAttachedGlTexturesProc in_attached_gl_textures_proc);

WKC_PEER_API void* wkcLayerNewPeer(int in_type, const WKCSize* in_size, float in_opticalzoomlevel);
WKC_PEER_API void wkcLayerDeletePeer(void* in_layer);
WKC_PEER_API void wkcLayerAttachGLTexturesPeer(void* in_layer, int* in_textures, int in_num);
WKC_PEER_API int wkcLayerGetTypePeer(void* in_layer);
WKC_PEER_API int wkcLayerGetTexturePeer(void* in_layer);
WKC_PEER_API void* wkcLayerGetOffscreenPeer(void* in_layer);
WKC_PEER_API void wkcLayerSetTexturePeer(void* in_layer, int in_texture, unsigned int in_width, unsigned int in_height);
WKC_PEER_API void* wkcLayerGetDrawContextPeer(void* in_layer);
WKC_PEER_API void wkcLayerReleaseDrawContextPeer(void* in_layer, void* in_dc);
WKC_PEER_API void wkcLayerDidDisplayPeer(void* in_layer);
WKC_PEER_API void wkcLayerUploadBitmapPeer(void* in_layer, int in_fmt, const void* in_bitmap, int in_rowbytes, const WKCSize* in_size);
WKC_PEER_API bool wkcLayerIsNeededYFlipPeer(void* in_layer);
WKC_PEER_API void wkcLayerGetAllocatedSizePeer(void* in_layer, int* out_width, int* out_height);
WKC_PEER_API void wkcLayerGetOriginalSizePeer(void* in_layer, int* out_width, int* out_height);
WKC_PEER_API bool wkcLayerResizePeer(void* in_layer, const WKCSize* in_size);
WKC_PEER_API bool wkcLayerSetOpticalZoomPeer(void* in_layer, float in_zoom_level);
WKC_PEER_API void wkcLayerDidAttachToTreePeer(void* in_layer);
WKC_PEER_API void wkcLayerDidDetachFromTreePeer(void* in_layer);
WKC_PEER_API bool wkcLayerCanAllocatePeer(int in_width, int in_height, int in_blendmode);
WKC_PEER_API void* wkcLayerGetOpaqueTexturePeer(void* in_layer);
WKC_PEER_API void wkcLayerSetOpaqueTexturePeer(void* in_layer, void* in_opaque_texture);

WKC_PEER_API void wkcDisplaySetScreenSizePeer(const WKCSize* in_size);
WKC_PEER_API void wkcDisplayGetScreenSizePeer(WKCSize* out_size);

WKC_END_C_LINKAGE

/*@}*/

#endif // _WKC_G_PEER_H_
