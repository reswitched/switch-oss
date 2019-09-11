/*
 * WKCPrefs.h
 *
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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
 */

#ifndef WKCPrefs_h
#define WKCPrefs_h

#include "WKCSkin.h"

namespace WKC {
class WKCWebFrame;
}

namespace WKC {

/*@{*/

    /**
       @brief Callback function that returns file selection string to draw
       @param path Path of selected file
       @param path_len Path length of selected file (number of characters)
       @param out_path Pointer to storage area of string to draw
       @param out_path_len Length of string to draw (number of characters)
       @param path_maxlen Maximum length that can be stored in out_path (number of characters)
    */
    typedef void (*ResolveFilenameForDisplayProc)(const unsigned short* path, const int path_len, unsigned short* out_path, int* out_path_len, const int path_maxlen);
    /**
       @brief Type definition of ForceNotifyScrollProc
       @param view Pointer to WebView
       @param scrollPoint Absolute coordinates of scroll target position
    */
    typedef void (*ForceNotifyScrollProc)(WKCWebFrame* frame, const WKCPoint& scrollPoint);

    /** @brief Structure for storing system string data */
    struct SystemStrings_ {
        /** @brief platform property of JavaScript Navigator object */
        const unsigned short* fNavigatorPlatform;
        /** @brief product property of JavaScript Navigator object */
        const unsigned short* fNavigatorProduct;
        /** @brief productSub property of JavaScript Navigator object */
        const unsigned short* fNavigatorProductSub;
        /** @brief vendor property of JavaScript Navigator object */
        const unsigned short* fNavigatorVendor;
        /** @brief vendorSub property of JavaScript Navigator object */
        const unsigned short* fNavigatorVendorSub;
        /** @brief System language */
        const unsigned short* fLanguage;
        /** @brief Button text of input element for which type="submit" is specified */
        const unsigned short* fButtonLabelSubmit;
        /** @brief Button text of input element for which type="reset" is specified */
        const unsigned short* fButtonLabelReset;
        /** @brief Button text of input element for which type="file" is specified */
        const unsigned short* fButtonLabelFile;
    };
    /** @brief Type definition of WKC::SystemStrings */
    typedef struct SystemStrings_ SystemStrings;

    /** @brief Encoding Detector Language */
    enum EncodingDetector {
        /** @brief No language sets */
        EEncodingDetectorNone               = 0x00000000,
        /** @brief Universal (UTF-8) */
        EEncodingDetectorUniversal          = 0x00000001,
        /** @brief Japanese */
        EEncodingDetectorJapanese           = 0x00000002,
        /** @brief Korean */
        EEncodingDetectorKorean             = 0x00000004,
        /** @brief Traditional Chinese */
        EEncodingDetectorTraditionalChinese = 0x00000008,
        /** @brief Simplified Chinese */
        EEncodingDetectorSimplifiedChinese  = 0x00000010,
        /** @brief Single-byte characher sets */
        EEncodingDetectorSBCS               = 0x00000020,

        /** @brief All language sets */
        EEncodingDetectorAll                = EEncodingDetectorUniversal | EEncodingDetectorJapanese | EEncodingDetectorKorean | EEncodingDetectorTraditionalChinese | EEncodingDetectorSimplifiedChinese | EEncodingDetectorSBCS,

        EEncodingDetectorEndOfEnum
    };

/*@}*/

}


namespace WKC {
/*@{*/
/** @brief Namespace of API layer of NetFront Browser NX WKCPrefs @n */
namespace WKCPrefs {
/*@{*/

    // network related
    /** @brief Proxy authentication type */
    enum ProxyAuth {
        /** @brief Basic authentication */
        Basic  = 0,
        /** @brief Digest authentication */
        Digest = 1,
        /** @brief NTLM authentication */
        NTLM   = 2,
        /** @brief No authentication */
        NONE   = 99
    };
    /** @brief Structure for storing screen device information */
    struct ScreenDeviceParams_ {
        /** @brief Screen device width */
        int fScreenWidth;
        /** @brief Screen device height */
        int fScreenHeight;
        /** @brief Width available for content on screen device */
        int fAvailableScreenWidth;
        /** @brief Height available for content on screen device */
        int fAvailableScreenHeight;
        /** @brief Screen color depth. Value that represents how many bits per pixel. */
        int fScreenDepth;
        /** @brief Color depth for individual R, G, and B. Value that represents how many bits per each element of RGB. Only one value can be set. */
        int fScreenDepthPerComponent;
        /** @brief Sets whether screen device is monochrome device. */
        bool fIsMonochrome;
    };
    /** @brief Type definition of WKC::ScreenDeviceParams */
    typedef struct ScreenDeviceParams_ ScreenDeviceParams;

    /**
       @brief Initializes WKC::WKCPref namespace
       @return None
    */
    WKC_API void initialize();
    /**
       @brief Forcibly terminates WKC::WKCPref namespace
       @return None
    */
    WKC_API void forceTerminate();
    /**
       @brief Forcibly terminates WKC::WKCPref namespace
       @return None
    */
    WKC_API void finalize();

    /**
       @brief Cache-related settings
       @param min_dead_resource Minimum value of cache resource size of past pages
       @param max_dead_resource Maximum value of cache resource size of past pages
       @param total Maximum value of cache
       @return None
       @details
       Sets the size of cache maintained by the browser engine. total is the combined value of the current page cache resources and past page cache resources, and cache data is deleted when cache resources grow larger than this value. max_dead_resource is the maximum value for the amount of cache resources of past pages in the entire cache, and min_dead_resource is the minimum. 
    */
    WKC_API void setCacheCapacities(unsigned int min_dead_resource, unsigned int max_dead_resource, unsigned int total);
    /**
       @brief Sets timer that deletes decoded cache data
       @param interval  Time until it is deleted (sec)
       @return None
       @details
       This timer is set when the data decoded by the browser engine is no longer referenced, and the decoded data is deleted when it times out. This must be set to 0 if the timer will not be used.
    */
    WKC_API void setDeadDecodedDataDeletionInterval(double interval);
    /**
       @brief Sets delay period before deletion of cache inside browser.
       @param delay Delay period before cache deletion (sec)
       @return None
       @details
       Sets the delay period until the decoded data cache inside the browser is deleted.
       If the period from the time it was last accessed until it will be deleted is shorter than the set time, then it will not be deleted.
    */
    WKC_API void setMinDelayBeforeLiveDecodedPruneCaches(double delay);
    /**
       @brief Enables / disables the memory cache
       @param enable Whether proxy is used @n
       - != false Use
       - == false Do not use
       @return None
       @details
       Enables / disables the memory cache.
    */
    WKC_API void setEnableMemoryCache(bool enable);
    /**
       @brief Sets the proxy.
       @param enable Whether proxy is used @n
       - != false Use
       - == false Do not use
       @param host Pointer to host address string
       @param port Port number
       @param isHTTP10 Proxy only supports HTTP 1.0
       @param proxyuser Pointer user id string
       @param proxypass Pointer to password string
       @param filters Proxy filters string
       @return None
       @attention
       SSL Proxy is not supported
    */
    WKC_API void setProxy(bool enable, const char* host, int port, bool isHTTP10 = false, const char* proxyuser = 0, const char* proxypass = 0, const char* filters = 0);
    /**
       @brief Sets SSL protocol parameters
       @param isEnableSSL2 Enable SSL2.0 or not
       @param isEnableSSL3 Enable SSL3.0 or not
       @param isEnableTLS10 Enable TLS1.0 or not
       @param isEnableTLS11 Enable TLS1.1 or not
       @param isEnableTLS12 Enable TLS1.2 or not
       @param isEnableTLS13 Enable TLS1.3 or not
       @return None
       @details
       Enables / disables each SSL protocols.
    */
    WKC_API void setSSLProtocols(bool isEnableSSL2, bool isEnableSSL3, bool isEnableTLS10, bool isEnableTLS11, bool isEnableTLS12, bool isEnableTLS13);
    /**
       @brief Enables / disables SSL online cert check
       @param isEnableOCSP Enable OCSP or not
       @param isEnableCRLDP Enable CRLDP or not
       @return None
       @details
       Enables / disables SSL online cert check
    */
    WKC_API void setSSLEnableOnlineCertChecks(bool isEnableOCSP, bool isEnableCRLDP);
    /**
    @brief Sets maximum number of HTTP connections
    @param number Maximum number of HTTP connections @n
    @return None
    @details
    By calling this function, the maximum value for the number of HTTP connections the browser engine will use can be changed.@n
    It is 5 by default.
    @attention
    - The system must be able to establish a minimum of 3 HTTP connections.
    - The following 2 HTTP connections are reserved: Synchronous XHR, OCSP.
    - Specifying 2 or less will not be applied.
    - Total value of "MaxHTTPConnections + MaxWebSocketConnections" must not exceed the maximum number of TCP connections available to the system.
    - It must never be set during communication.
    */
    WKC_API void setMaxHTTPConnections(long number);
    /**
    @brief Sets maximum number of HTTP pipeline connections
    @param number Maximum number of HTTP pipeline connections @n
    @return None
    @details
    By calling this function, the maximum value for the number of HTTP pipeline connections the browser engine will use can be changed.@n
    It is 8 by default.
    */
    WKC_API void setMaxHTTPPipelineRequests(long number);
    /**
       @brief Sets maximum number of WebSocket connections
       @param number Maximum number of WebSocket connections @n
       @return None
       @details
       By calling this function, the maximum value for the number of WebSocket connections the browser engine will use can be changed.@n
       It is 3 by default.
       @attention
       - The system must be able to establish a minimum of 2 WebSocket connections.
       - The WebInspector connection is reserved.
       - Specifying 1 or less will not be applied.
       - Total value of "MaxHTTPConnections + MaxWebSocketConnections" must not exceed the maximum number of TCP connections available to the system.
       - It must never be set during communication.
    */
    WKC_API void setMaxWebSocketConnections(long number);
    /**
       @brief Sets maximum number of cookies that can be saved internally
       @param number Maximum number of cookies @n
       @return None
       @details
       Internal cookie data is shared by the browser engine. @n
       By calling this function, the maximum number of cookies held by the browser engine can be changed.@n
       The default value is 20 cookies.
       @attention
       The behavior cannot be guaranteed if this is set during communication.
    */
    WKC_API void setMaxCookieEntries(long number);
    /**
       @brief Set timeout value of the DNS cache expire
       @param sec Timeout value
       - == -1 no timeout
       - >= 0 second
       @return None
       @details
       Set DNS cache timeout value.\n
       The default value is 5 * 60 (5 seconds)
    */
    WKC_API void setDNSCacheTimeout(int sec);
    /**
       @brief Set timeout value of the server response
       @param sec Timeout value
       - == -1 no timeout
       - >= 0 second
       @return None
       @details
       Set server response timeout value.\n
       The default value is 20 sec.
    */
    WKC_API void setServerResponseTimeout(int sec);
    /**
       @brief Set timeout value of the connection establish
       @param sec Timeout value
       - == -1 no timeout
       - >= 0 second
       @return None
       @details
       Set the timeout value of connection establish.\n
       The default value is 20 sec.
    */
    WKC_API void setConnectTimeout(int sec);
    /**
       @brief Sets Accept-encoding header for each HTTP requests.
       @param encodings Encoding names.
       @return None
       @details
       Set the value of Accept-encoding headers.
    */
    WKC_API void setAcceptEncoding(const char* encodings);
    /**
       @brief Enables site access permission callback to be allowed
       @param enable Distinguishes whether to enable@n
       - != false Enable
       - == false Disable
       @return None
       @details
       Enables callback that inquires apps whether to allow site access, using a filter for hazardous sites, etc.@n
       It is disabled by default.
    */
    WKC_API void setHarmfulSiteFilter(bool enable);
    /**
       @brief Enables Do Not Track.
       @param enable Distinguishes whether to enable@n
       - != false Enable
       - == false Disable
       @return None
       @details
       Enables Do Not Track.  NX will send 'DNT:' HTTP header.@n
       It is disabled by default.
    */
    WKC_API void setDoNotTrack(bool enable);
    /**
       @brief Performs HTTP redirect inside WebKit, not on HTTP layer
       @param enable Distinguishes whether to enable@n
       - != false Enable
       - == false Disable
       @return None
       @details
       Enables callback that asks apps whether to allow site access, using a filter for hazardous sites, etc., and also enables it that asks apps whether to allow site access to the HTTP redirect destination.
       It is disabled by default.
    */
    WKC_API void setRedirectInWebKit(bool enable);
    /**
    @brief Enable HTTP/2
    @param enable Distinguishes whether to enable@n
    - != false Enable
    - == false Disable
    @return None
    @details
    Enables HTTP/2
    It is enabled by default.
    */
    WKC_API void setUseHTTP2(bool enable);
    /**
    @brief Sets stack size
    @param stack_size Stack size
    @retval None
    @details
    The stack size set by this function is used as the stack size for the stack overflow detection process inside the browser engine and that notified by WKC::WKCMemoryEventHandler::notifyStackOverflow().
    @attention
    Always call and set this when creating the browser instance.
    */
    WKC_API void setStackSize(unsigned int stack_size);
    /**
       @brief Sets screen device information
       @param params Screen device information
       @retval None
       @details
       Sets the screen size, effective screen size, one of the RGB color depths, pixel color depth, and whether it is a monochrome device as screen device information.@n
       Always set this when initializing the browser. If it is necessary to change the device information, this API may be used to reset it.
    */
    WKC_API void setScreenDeviceParams(const ScreenDeviceParams& params);
    /**
       @brief Sets system strings
       @param strings System strings
       @retval None
       @details
       The strings set by this function are copied internally.
       @attention
       Always call and set this when creating the browser instance. @n
       If this function is not called, or if NULL is set for a member of WKC::SystemStrings and called, then the following values will be set as default values for system strings. @n
       - platform property of JavaScript Navigator object (WKC::SystemStrings::fNavigatorPlatform) … "" (empty string)
       - product property of JavaScript Navigator object (WKC::SystemStrings::fNavigatorProduct) … "" (empty string)
       - productSub property of JavaScript Navigator object (WKC::SystemStrings::fNavigatorProductSub) … "" (empty string)
       - vendor property of JavaScript Navigator object (WKC::SystemStrings::fNavigatorVendor) … "" (empty string)
       - vendorSub property of JavaScript Navigator object (WKC::SystemStrings::fNavigatorVendorSub) … "" (empty string)
       - System language (WKC::SystemStrings::fLanguage) … " en "
       - Button text of input element for which type="submit" is specified (WKC::SystemStrings::fButtonLabelSubmit) … "Submit"
       - Button text of input element for which type="reset" is specified (WKC::SystemStrings::fButtonLabelReset) … "Reset"
       - Button text of input element for which type="file" is specified (WKC::SystemStrings::fButtonLabelFile) … "Choose File"
    */
    WKC_API void setSystemStrings(const WKC::SystemStrings* strings);
    /**
       @brief Sets system string
       @param key Key string
       @param text Localized string (UTF-8)
       @retval "!= false" Setting succeeded
       @retval "== false" Setting failed
       @details
       Sets localized strings independently. Refer available keys in LocalizedKeysWKC.cpp.
    */
    WKC_API bool setSystemString(const unsigned char* key, const unsigned char* text);
    /**
       @brief Sets engine thread information
       @param thread_id Thread ID
       @param stack_base Stack base address
       @retval None
       @details
       Sets ID and stack back address of thread that calls a browser engine API.@n
       This API does not have to be called on platforms where the thread and stack base address can be obtained by the porting layer alone.@n
       @attention
       On platforms other than those where the thread and stack base address can be obtained by the porting layer alone, this must always be called and set when creating the browser instance.
    */
    WKC_API void setThreadInfo(void* thread_id, void* stack_base);
    /**
       @brief Sets language set for which character encoding is auto-recognized
       @param languageSetFlag Language set for auto-recognition
       @retval None
       @details
       Enables auto-recognition of the character encoding for the language set set here. If WKC::EEncodingDetectorNone is specified, auto-recognition is not performed.
       @attention
       There are two auto-recognition processes: WKC auto-recognition and WebKit built-in auto-recognition. The character encoding this function specifies is for WKC auto-recognition.
    */
    WKC_API void setEncodingDetectorLanguageSet(int languageSetFlag);
    /**
       @brief Sets primary font data
       @param fontID ID of font data
       @retval "!= false" Setting succeeded
       @retval "== false" Setting failed
       @details
       Sets the font data with the specified ID as primary font data.@n
       When generating glyphs, its search is centered on the primary font data. In other cases, it searches other registered font data.@n
    */
    WKC_API bool setPrimaryFont(int fontID);
    /**
       @brief Sets whether to scale Bitmapfont glyphs on the WebCore side
       @param flag Distinguishes whether to scale@n
       - != false Scale on WebCore side
       - == false Do not scale on WebCore side
       @retval None
       @details
       API for future extension
    */
    WKC_API void setEnableScalingMonosizeFont(bool flag);
    /**
       @brief Sets glyph to use when getting average font width.
       @param glyph Glyph (code number in unicode)
       @retval None
       @details
       Sets glyph to use when getting average font width.@n
       When this is not set, it uses the average width taken from the font itself.
    **/
    WKC_API void setGlyphForAverageWidth(const unsigned int glyph);
    /**
      @brief Sets emoji glyphs folder path.
      @param path Path to emoji glyphs folder
      @retval None
      @details
      Sets emoji glyphs folder path.@n
    **/
    WKC_API void setEmojiGlyphsFolderPath(const char* path);
    /**
       @brief Sets function to resolve drawn string of input element for which type="file" is specified
       @param proc Pointer to function for resolving the display string
       @retval None
       @details
       Registers the function that generates the string to be displayed by the input element for which type="file" is specified. Depending on the drawing size, not all of the strings returned by the registered function may be visible.@n
       The arguments of WKC::ResolveFilenameForDisplayProc() are a pointer to the full path of the file specified from the application, the length of the file path, a pointer to the display text storage area, the length of the display text, and the maximum length of the display text storage area. There is no return value. This callback function assumes that the display text and its length to be stored in the area passed in argument. The unit for the maximum length is in number of characters.
    */
    WKC_API void setResolveFilenameForDisplayProc(WKC::ResolveFilenameForDisplayProc proc);
    /**
       @brief Sets function for browser engine scroll process notifications
       @param proc Pointer to function for scroll process notifications
       @retval None
       @details
       Registers a function for browser engine scrolling process notifications.@n
       The registered function is called when the browser engine scrolling process occurs.@n
       The registered function is called while displaying the top of the page, and executing scrollto(0,0) even if the actual scrolling process has not occurred.
    */
    WKC_API void setForceNotifyScrollProc(WKC::ForceNotifyScrollProc proc);
    /**
       @brief Sets skin
       @param skin Pointer to WKCSkin
       @retval None
       @details
       The actual skin indicated by the pointer to the skin image data held by WKC::WKCSkin must not be released until the browser terminates.
    */
    WKC_API void registerSkin(const WKC::WKCSkin* skin);
    /**
       @brief Sets the media player controls style sheet
       @param css Pointer to css
       @retval None
       @details
       The actual css indicated by the pointer to the media player controls css must not be released until the browser terminates.
    */
    WKC_API void registerMediaControlsStyleSheet(const char* css);
    /**
       @brief Sets the media player controls JavaScript
       @param js Pointer to JavaScript
       @retval None
       @details
       The actual js indicated by the pointer to the media player controls js must not be released until the browser terminates.
    */
    WKC_API void registerMediaControlsJavaScript(const char* js);
    /**
       @brief Sets parameters of HTTP cache.
       @param enable Enables / disables HTTP cache
       @param limitTotalContentsSize Limit size of total contents size of caches
       @param maxContentSize Max size of each content size
       @param minContentSize Minimum size of each content size
       @param limitEntries Limit numbers of entries in cache
       @param limitEntryInfoSize Limit size of resource information of each entry in cache
       @param filePath Directory to store HTTP caches
       @retval None
       @details
       Sets parameters of HTTP cache.
    */
    WKC_API void setHTTPCache(bool enable, long long limitTotalContentsSize, long maxContentSize, long minContentSize, int limitEntries, int limitEntryInfoSize, const char *filePath);
    
    /**
       @brief Sets online state
       @param online Sets online / offline
       @details
       Sets online state.
    */
    WKC_API void setOnLine(bool online);
    /**
       @brief Sets the effective angle for horizontal focus movement
       @param angle Sets the effective angle from the horizontal direction (between 0 and 90 degrees)
       @details
        Sets the effective angle for horizontal focus movement.
    */
    WKC_API void setEffectiveAngleForHorizontalFocusNavigation(unsigned int angle);
    /**
       @brief Sets the effective angle for vertical focus movement
       @param angle Sets the effective angle from the vertical direction (between 0 and 90 degrees)
       @details
        Sets the effective angle for vertical focus movement.
    */
    WKC_API void setEffectiveAngleForVerticalFocusNavigation(unsigned int angle);
    /**
       @brief Sets the effective angle for diagonal focus movement
       @param minAngle Sets the minimum effective angle from the horizontal direction (between 0 and 90 degrees)
       @param maxAngle Sets the maximum effective angle from the horizontal direction (between 0 and 90 degrees)
       @details
        Sets the effective angle for diagonal focus movement.
    */
    WKC_API void setEffectiveAngleForDiagonalFocusNavigation(unsigned int minAngle, unsigned int maxAngle);
    /**
      @brief Sets the webdatabase path.
      @param path Path of the webdatabase storage
      @details
      Sets the webdatabase path.
    */
    WKC_API void setDatabasesDirectory(const char* path);

    /**
      @brief Sets whether to use neaqrest-neighbor filter on drawing
      @param flag Use or nor
      @details
      Sets whether to use neaqrest-neighbor filter on drawing.
      If not set (default), use bilinear filter.
    */
    WKC_API void setUseNearestFilter(bool flag);

    /**
      @brief activate WebGL
      @details
      Activates WebGL functions.
    */
    WKC_API void activateWebGL();

    WKC_API void setProhibitsScrollingEnabled(bool enabled);

    WKC_API void setUseDollarVM(bool enabled);
/*@}*/

} // namespace

/*@}*/

} // namespace

#endif // WKCPrefs_h
