/*
 *  wkcpluginpeer.h
 *
 *  Copyright(c) 2009-2011 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_PLUGIN_PEER_H_
#define _WKC_PLUGIN_PEER_H_

#include <wkc/wkcbase.h>

/**
   @file
   @brief Plug-ins related peers.
 */

/*@{*/

WKC_BEGIN_C_LINKAGE

// plugins

enum {
    WKC_PLUGIN_QUIRK_WANTSMOZILLAUSERAGENT                    = 0x00000001,
    WKC_PLUGIN_QUIRK_DEFERFIRSTSETWINDOWCALL                  = 0x00000002,
    WKC_PLUGIN_QUIRK_THROTTLEINVALIDATE                       = 0x00000004,
    WKC_PLUGIN_QUIRK_REMOVEWINDOWLESSVIDEOPARAM               = 0x00000008,
    WKC_PLUGIN_QUIRK_THROTTLEWMUSERPLUSONEMESSAGES            = 0x00000010,
    WKC_PLUGIN_QUIRK_DONTUNLOADPLUGIN                         = 0x00000020,
    WKC_PLUGIN_QUIRK_DONTCALLWNDPROCFORSAMEMESSAGERECURSIVELY = 0x00000040,
    WKC_PLUGIN_QUIRK_HASMODALMESSAGELOOP                      = 0x00000080,
    WKC_PLUGIN_QUIRK_FLASHURLNOTIFYBUG                        = 0x00000100,
    WKC_PLUGIN_QUIRK_DONTCLIPTOZERORECTWHENSCROLLING          = 0x00000200,
    WKC_PLUGIN_QUIRK_DONTSETNULLWINDOWHANDLEONDESTROY         = 0x00000400,
    WKC_PLUGIN_QUIRK_DONTALLOWMULTIPLEINSTANCES               = 0x00000800,
    WKC_PLUGIN_QUIRK_REQUIRESGTKTOOLKIT                       = 0x00001000,
    WKC_PLUGIN_QUIRK_REQUIRESDEFAULTSCREENDEPTH               = 0x00002000,
};

enum {
    WKC_PLUGIN_TYPE_UNIX = 0,
    WKC_PLUGIN_TYPE_WIN,
    WKC_PLUGIN_TYPE_WINCE,
    WKC_PLUGIN_TYPE_MAC,
    WKC_PLUGIN_TYPE_GENERIC,
    WKC_PLUGIN_TYPES,
};

/** @brief Structure that stores plug-in information */
struct WKCPluginInfo_ {
    /**
       @brief Platform that plug-in supports
       @details
       Set the following values:
       @li WKC_PLUGIN_TYPE_UNIX
       @li WKC_PLUGIN_TYPE_WIN
       @li WKC_PLUGIN_TYPE_WINCE
       @li WKC_PLUGIN_TYPE_MAC
       @li WKC_PLUGIN_TYPE_GENERIC
    */
    int fType; // WKC_PLUGIN_TYPE_XXX
    /** @brief Plug-in name */
    char* fName;
    /** @brief Plug-in name length */
    int fNameLen;
    /** @brief Description of plug-in */
    char* fDescription;
    /** @brief Description length of plug-in */
    int fDescriptionLen;
    /**
       @brief MIME type information
       @details
       If the plug-in type is WKC_PLUGIN_TYPE_WIN, it is processed assuming that the structure is set as "mime1|mime2|mime3,,,".
       Other types are processed assuming that the structure is set as "mime:exts,exts,exts,,,:desc;mime2:exts,exts,exts,,,:desc2;" including extensions.
    */
    char* fMIMEDescription;
    /** @brief MIME type information length */
    int fMIMEDescriptionLen;
    /**
       @brief Extension information
       @details
       If the plug-in type is WKC_PLUGIN_TYPE_WIN, it is processed assuming that the structure is set as "ext1_1,ext1_2|ext2_1,ext2_2,,,|,,,".
       In other types, it is not necessary as it is set by fMIMEDescription.
    */
    char* fExtension;
    /** @brief Extension information length */
    int fExtensionLen;
    /**
       @brief Description of file (extension)
       @details
       If the type of a plug-in is WKC_PLUGIN_TYPE_WIN, it is processed assuming that the structure is set as "FileOpenName1|FileOpenName2|,,,".
       In other types, it is not necessary as it is set by fMIMEDescription.
    */
    char* fFileOpenName;
    /** Description length of file (extension) */
    int fFileOpenNameLen;
};
/** @brief Type definition of WKCPluginInfo */
typedef struct WKCPluginInfo_ WKCPluginInfo;

/**
@brief Initializes plug-in
@retval true  Succeeded
@retval false Failed
@details
Performs the necessary processes for initializing the plug-in.
*/
WKC_PEER_API bool wkcPluginInitializePeer(void);
/**
@brief Finalizes plug-in
@details
Performs the necessary processes for finalizing the plug-in.
*/
WKC_PEER_API void wkcPluginFinalizePeer(void);
/**
@brief Forcibly terminates plug-in
@details
Performs the forced termination process of the plug-in.
*/
WKC_PEER_API void wkcPluginForceTerminatePeer(void);

/**
@brief Sets path where plug-in module is set up
@param in_path Setting location path of plug-in module
@details
Performs the process for handling the path (not including the file name) where the plug-in module specified by in_path is set up.
*/
WKC_PEER_API void wkcPluginSetPluginPathPeer(const char* in_path);
/**
@brief Gets path where plug-in module is set up
@retval Setting location path of plug-in module
@details
The path (not including the file name) where the plug-in module is set up must be returned as a return value.
*/
WKC_PEER_API const char* wkcPluginGetPluginPathPeer(void);
/**
@brief Gets number of plug-in modules
@retval Number of plug-in modules
@details
Counts the number of plug-ins that exist in the path and returns it as a return value.
*/
WKC_PEER_API int wkcPluginGetPluginsCountPeer(void);
/**
@brief Gets full path of plug-in module
@param in_index File index
@retval Plug-in module full path
@details
The full path including the file name of the plug-in module must be returned as a return value.
Only the number returned by wkcPluginGetPluginsCountPeer is called.
*/
WKC_PEER_API const char* wkcPluginGetPluginFullPathPeer(int in_index);
/**
@brief Sets process for specified plug-in MIME type
@param in_mime MIME type
@retval Symbol of process to set
@details
Use the following symbols for the value to return. For setting this, multiple values can be specified using OR.
@li WKC_PLUGIN_QUIRK_WANTSMOZILLAUSERAGENT
@li WKC_PLUGIN_QUIRK_DEFERFIRSTSETWINDOWCALL
@li WKC_PLUGIN_QUIRK_THROTTLEINVALIDATE
@li WKC_PLUGIN_QUIRK_REMOVEWINDOWLESSVIDEOPARAM
@li WKC_PLUGIN_QUIRK_THROTTLEWMUSERPLUSONEMESSAGES
@li WKC_PLUGIN_QUIRK_DONTUNLOADPLUGIN
@li WKC_PLUGIN_QUIRK_DONTCALLWNDPROCFORSAMEMESSAGERECURSIVELY
@li WKC_PLUGIN_QUIRK_HASMODALMESSAGELOOP
@li WKC_PLUGIN_QUIRK_FLASHURLNOTIFYBUG
@li WKC_PLUGIN_QUIRK_DONTCLIPTOZERORECTWHENSCROLLING
@li WKC_PLUGIN_QUIRK_DONTSETNULLWINDOWHANDLEONDESTROY
@li WKC_PLUGIN_QUIRK_DONTALLOWMULTIPLEINSTANCES
@li WKC_PLUGIN_QUIRK_REQUIRESGTKTOOLKIT
@li WKC_PLUGIN_QUIRK_REQUIRESDEFAULTSCREENDEPTH
*/
WKC_PEER_API unsigned int wkcPluginGetQuirksInfoPeer(const char* in_mime);
/**
@brief Gets plug-in information
@param in_id Parameter ID of information to get
@param out_value Information to get
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the process for returning information of the parameter ID specified for the browser plug-in.
Since the ID obtained by in_id corresponds to the variable of NPN_GetValue, refer to it for more information.
*/
WKC_PEER_API bool wkcPluginGetStaticValuePeer(int in_id, void* out_value);

/**
@brief Reads plug-in module
@param in_path Plug-in module full path
@retval Plug-in instance
@details
The plug-in instance specified by in_path must be returned as a return value.
The set plug-in instance is passed as in_plugin, an argument of individual wkcPluginxxxPeer().
*/
WKC_PEER_API void* wkcPluginLoadPeer(const char* in_path);
/**
@brief Gets plug-in function
@param in_plugin Plug-in instance
@param in_symbol Plug-in function name
@retval Pointer of plug-in function
@details
The function pointer specified by in_symbol of the plug-in specified by in_plugin must be returned as a return value.
*/
WKC_PEER_API void* wkcPluginGetSymbolPeer(void* in_plugin, const char* in_symbol);
/**
@brief Releases plug-in
@param in_plugin Plug-in instance
@details
Performs the release process of the plug-in instance.
*/
WKC_PEER_API void wkcPluginUnloadPeer(void* in_plugin);
/**
@brief Gets plug-in information
@param in_plugin Plug-in instance
@param out_info Plug-in information
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Set the information of plug-in specified by out_info, and return out_info as a return value.
Note that WKCPluginInfo::fName, WKCPluginInfo::fDescription, WKCPluginInfo::fMIMEDescription, WKCPluginInfo::fExtension and WKCPluginInfo::fFileOpenName that are passed as arguments can be NULL. In addition, the length of each information string even if these are NULL must be set and returned as a return value.
*/
WKC_PEER_API bool wkcPluginGetInfoPeer(void* in_plugin, WKCPluginInfo* out_info);
/**
@brief Gets plug-in type
@param in_plugin Plug-in instance
@retval WKC_PLUGIN_TYPE_UNIX
@retval WKC_PLUGIN_TYPE_WIN
@retval WKC_PLUGIN_TYPE_WINCE
@retval WKC_PLUGIN_TYPE_MAC
@retval WKC_PLUGIN_TYPE_GENERIC
@details
The symbol (WKC_PLUGIN_TYPE_xxx) of the platform supported by the specified plug-ins must be returned as a return value.
*/
WKC_PEER_API int wkcPluginGetTypePeer(void* in_plugin);

enum {
    WKC_PLUGINWINDOW_HIDEVENT_KEYDOWN = 0,
    WKC_PLUGINWINDOW_HIDEVENT_KEYUP,
    WKC_PLUGINWINDOW_HIDEVENT_MOUSEDOWN,
    WKC_PLUGINWINDOW_HIDEVENT_MOUSEUP,
    WKC_PLUGINWINDOW_HIDEVENT_MOUSEMOVE,
    WKC_PLUGINWINDOW_HIDEVENT_MOUSEOUT,
    WKC_PLUGINWINDOW_HIDEVENT_MOUSEOVER,
    WKC_PLUGINWINDOW_HIDEVENTS
};

enum {
    WKC_PLUGINWINDOW_MODIFIER_NONE    = 0x00000000,
    WKC_PLUGINWINDOW_MODIFIER_CONTROL = 0x00000001,
    WKC_PLUGINWINDOW_MODIFIER_ALT     = 0x00000002,
    WKC_PLUGINWINDOW_MODIFIER_SHIFT   = 0x00000004,
    WKC_PLUGINWINDOW_MODIFIER_META    = 0x00000008,
    WKC_PLUGINWINDOW_MODIFIERS,
};

enum {
    WKC_PLUGINWINDOW_BUTTON_NONE = 0,
    WKC_PLUGINWINDOW_BUTTON_LEFT,
    WKC_PLUGINWINDOW_BUTTON_MIDDLE,
    WKC_PLUGINWINDOW_BUTTON_RIGHT,
    WKC_PLUGINWINDOW_BUTTONS
};

/** @brief Reserved for future extension */
typedef void(*wkcPluginWindowNotifyMouseEventProc)(void* in_self, int in_type, int in_x, int in_y, int in_button, int in_modifiers);
/** @brief Reserved for future extension */
typedef void(*wkcPluginWindowNotifyKeyEventProc)(void* in_self, int in_type, int in_key, int in_modifiers);
/**
   @brief Gives notification of NPEvent for plug-in instance
   @details
   Set in_opaque notified by wkcPluginWindowNewPeer() for in_self.
   In addition, set the NPEvent-format event for in_npevent.
*/
typedef bool(*wkcPluginWindowNotifyNPEventProc)(void* in_self, void* in_npevent);
/**
   @brief Notifies that plug-in are destroyed for WebKit
   @details
   Set in_opaque notified by wkcPluginWindowNewPeer() for in_self.
*/
typedef void(*wkcPluginWindowNotifyDestroyProc)(void* in_self);

/** @brief Structure that stores callback function for notifying plug-in of event */
struct WKCPluginWindowEventHandlers_ {
    /** @brief Reserved for future extension */
    wkcPluginWindowNotifyMouseEventProc fMouseEvent;
    /** @brief Reserved for future extension */
    wkcPluginWindowNotifyKeyEventProc fKeyEvent;
    /**
       @brief Gives notification of NPEvent for plug-in instance
       @details
       For more information, see wkcPluginWindowNotifyNPEventProc.
    */
    wkcPluginWindowNotifyNPEventProc fNPEvent;
    /**
       @brief Notifies that plug-in are destroyed for WebKit
       @details
       For more information, see wkcPluginWindowNotifyDestroyProc.
    */
    wkcPluginWindowNotifyDestroyProc fDestroyProc;
};
/** @brief Type definition of WKCPluginWindowEventHandlers */
typedef struct WKCPluginWindowEventHandlers_ WKCPluginWindowEventHandlers;

/**
@brief Generates plug-in window
@param in_windowed Window mode
@param in_needsxembed XEmbed usage setting
@param in_handlers Pointer to structure that stores event handler
@param in_opaque Pointer to data to be passed to event handler
@retval Plug-in instance
@details
Generates the individual window generation process, according to the argument information, and return the plug-in instance as a return value.
If in_window is true, window mode is applied. If in_window is false, windowless mode is applied.
Set the notified in_opaque for in_self, an argument of individual event handler.
The set plug-in instance is passed as in_window, an argument of wkcPluginWindowxxxPeer().
in_needsxembed is not used in Windows environment.
*/
WKC_PEER_API void* wkcPluginWindowNewPeer(bool in_windowed, bool in_needsxembed, const WKCPluginWindowEventHandlers* in_handlers, void* in_opaque);
/**
@brief Deletes plug-in window
@param in_window Plug-in instance
@details
The window of the specified plug-in instance must be deleted.
*/
WKC_PEER_API void wkcPluginWindowDeletePeer(void* in_window);
/**
@brief Gets plug-in information that plug-in is notified
@param in_window Plug-in instance
@param out_windowtype Plug-in type
@retval Window pointer
@details
The window type and pointer used when the plug-in draws must be returned as return values.
The values set for out_windowtype are as follows:
@li out_window == 1 Window mode (NPWindowTypeWindow)
@li out_window == 2 Windowless mode (NPWindowTypeDrawable)
*/
WKC_PEER_API void* wkcPluginWindowGetNPWindowPeer(void* in_window, int* out_windowtype);
/**
@brief Sets plug-in visibility
@param in_window Plug-in instance
@param in_visible Visibility setting
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the process for setting plug-in visibility.
*/
WKC_PEER_API bool wkcPluginWindowSetVisiblePeer(void* in_window, bool in_visible);
/**
@brief Sets drawing area
@param in_window Plug-in instance
@param in_rect Drawing area
@param in_clip Drawable clipping area
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the process for setting the drawing area of specified plug-in.
*/
WKC_PEER_API bool wkcPluginWindowSetRectPeer(void* in_window, const WKCRect* in_rect, const WKCRect* in_clip);
/**
@brief Gives notification of invalid area for plug-in window
@param in_window Plug-in instance
@param in_rect Rectangular area to set
@details
Performs the process for setting the invalid rectangular area where specified plug-in does not draw.
*/
WKC_PEER_API void wkcPluginWindowNotifyInvalRectPeer(void* in_window, const WKCRect* in_rect);
/**
@brief Gives notification of drawing process for plug-in window
@param in_window Plug-in instance
@param in_framerect Plug-in area
@param in_rect Drawing area
@details
Performs the drawing process of specified plug-in.
This function is called in windowless mode.
*/
WKC_PEER_API void wkcPluginWindowNotifyPaintPeer(void* in_window, const WKCRect* in_framerect, const WKCRect* in_rect);
/**
@brief Gives notification of drawing process for plug-in window
@param in_window Plug-in instance
@details
Performs the drawing process of specified plug-in.
This function is called in windowless mode.
*/
WKC_PEER_API void wkcPluginWindowNotifyForceRedrawPeer(void* in_window);
/**
@brief (TBD)
@param in_window Plug-in instance
@details
(TBD)
*/
WKC_PEER_API void wkcPluginWindowNotifySuspendPeer(void* in_window);
/**
@brief (TBD)
@param in_window Plug-in instance
@details
(TBD)
*/
WKC_PEER_API void wkcPluginWindowNotifyResumePeer(void* in_window);

/**
@brief Gets information for specified plug-in
@param in_window Plug-in instance
@param in_id Parameter ID of information to get
@param out_value Information for in_id
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the process for returning information for the parameter ID specified for the specified plug-in.
Since the ID obtained by in_id corresponds to the variable of NPN_GetValue, refer to it for more information.
*/
WKC_PEER_API bool wkcPluginWindowGetValuePeer(void* in_window, int in_id, void* out_value);

/**
@brief Locks offscreen
@param in_window Plug-in instance
@param out_fmt Format
@param out_rowbytes Number of bytes in horizontal direction of drawing buffer
@param out_size Number of bytes of drawing buffer
@retval Pointer to drawing buffer
@details
Performs the processes for locking the offscreen for writing and returning the drawing buffer as a return value.
WKC_IMAGETYPE_XXX must be returned to out_fmt.
*/
WKC_PEER_API void* wkcPluginWindowLockOffscreenPeer(void* in_window, int* out_fmt, int* out_rowbytes, WKCSize* out_size);
/**
@brief Unlocks offscreen
@param in_window Plug-in instance
@param in_offscreen Pointer to offscreen
@details
Performs the process for unlocking the specified offscreen.
*/
WKC_PEER_API void wkcPluginWindowUnlockOffscreenPeer(void* in_window, void* in_offscreen);

/**
@brief Gives notification of mouse event
@param in_window Plug-in instance
@param in_type Mouse event type
@param in_x X coordinate of mouse
@param in_y Y coordinate of mouse
@param in_button Mouse button type
@param in_modifiers Modifier
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the mouse event process for the specified plug-in.
The mouse coordinate represents the coordinate inside the plug-in drawing area.
This function is called when the plug-in is in windowless mode.
The types notified by in_type are the following symbols:
@li WKC_PLUGINWINDOW_HIDEVENT_MOUSEDOWN Mouse press event
@li WKC_PLUGINWINDOW_HIDEVENT_MOUSEUP Mouse release event
@li WKC_PLUGINWINDOW_HIDEVENT_MOUSEMOVE Mouse cursor move event
@n

The types of buttons notified by in_button are the following symbols:
@li WKC_PLUGINWINDOW_BUTTON_NONE No button
@li WKC_PLUGINWINDOW_BUTTON_LEFT Mouse left button
@li WKC_PLUGINWINDOW_BUTTON_MIDDLE Mouse center button
@li WKC_PLUGINWINDOW_BUTTON_RIGHT Mouse right button
@n

The keys notified by in_modifiers are as follows:
@li WKC_PLUGINWINDOW_MODIFIER_NONE None
@li WKC_PLUGINWINDOW_MODIFIER_CONTROL Ctrl key
@li WKC_PLUGINWINDOW_MODIFIER_ALT Alt key
@li WKC_PLUGINWINDOW_MODIFIER_SHIFT Shift key
@li WKC_PLUGINWINDOW_MODIFIER_META Meta key
*/
WKC_PEER_API bool wkcPluginWindowNotifyMouseEventPeer(void* in_window, int in_type, int in_x, int in_y, int in_button, int in_modifiers);
/**
@brief Gives notification of key event
@param in_window Plug-in instance
@param in_type Key event type
@param in_key Key type
@param in_modifiers Modifier
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the key event process for the specified plug-in.
This function is called when the plug-in is in windowless mode.
For example, the type of key events notified by in_type is as follows:
(Note that the following are not all. For more information, see WKCEnumKeys.h.)
@li EKey0
@li EKey1
@li EKey2
@li EKey3
@li EKey4
@li EKey5
@li EKey6
*/
WKC_PEER_API bool wkcPluginWindowNotifyKeyboardEventPeer(void* in_window, int in_type, int in_key, int in_modifiers);

/**
@brief Sets environment-dependent instance
@param in_instance1 Environment-dependent instance 1
@param in_instance2 Environment-dependent instance 2
@details
Performs the process for setting the environment-dependent instance.
This argument corresponds to that of WKC::WKCWebKitSetPluginInstances().
Depending on the environment, set them individually and use if needed.
*/
WKC_PEER_API void wkcPluginWindowSetInstancesPeer(void* in_instance1, void* in_instance2);

// for XP_UNIX
/**
@brief Gets window system information
@param in_window Plug-in instance
@param out_display Window handle
@param out_visual Visual type
@param out_depth Window depth (plain number)
@param out_colormap Color map ID
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Only for XP_UNIX@n
The process for returning window system information must be performed.
*/
WKC_PEER_API bool wkcPluginWindowX11GetWSIInfoPeer(void* in_window, void** out_display, void** out_visual, int* out_depth, void** out_colormap);

WKC_END_C_LINKAGE

/*@}*/

#endif // _WKC_PLUGIN_PEER_H_
