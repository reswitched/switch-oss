/*
 * WKCWebView.h
 *
 * Copyright (c) 2010-2020 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCWebView_h
#define WKCWebView_h

// class definition

#include "WKCVersion.h"

#include "WKCEnums.h"
#include "helpers/WKCSharedStringHash.h"
#include "helpers/WKCHelpersEnums.h"

#include <stddef.h>

#include <wkc/wkcfileprocs.h>
#include <wkc/wkcmediaprocs.h>
#include <wkc/wkcpasteboardprocs.h>
#include <wkc/wkcthreadprocs.h>

#include <wkcglobalwrapper.h>

// prototypes
namespace WKC {
class Element;
class Frame;
class Node;
class HistoryItem;
class Page;
class ResourceHandle;
class String;
class HitTestResult;

class WKCWebView;
class WKCClientBuilders;
class WKCWebFrame;
class WKCWebViewPrivate;
class WKCWebInspector;
class WKCHitTestResult;
class WKCWebElementInfo;
class WKCSettings;
class WKCNetworkEventHandler;
class WKCWebViewPrefs;
class WKCMemoryEventHandler;
class WKCTimerEventHandler;

typedef struct CompositionUnderline_ CompositionUnderline;
}

// callbacks
/*@{*/

/** @brief Namespace of API layer of NetFront Browser NX WKC */
namespace WKC {
    /** @brief Type definition of FileSystemProcs. For more information, see the section "Structure - WKCFileProcs_" in "NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Peer API Reference". */
    typedef WKCFileProcs FileSystemProcs;
    typedef WKCMediaPlayerProcs MediaPlayerProcs;
    /** @brief Type definition of PasteboardProcs. For more information, see the section "Structure - WKCPasteboardProcs_" in "NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Peer API Reference". */
    typedef WKCPasteboardProcs PasteboardProcs;
    typedef WKCThreadProcs ThreadProcs;

    /** @brief Structure for storing client certificate informations */
    struct clientCertInfo_ {
        /** @brief Issuer */
        const char* issuer;
        /** @brief Subject */
        const char* subject;
        /** @brief Not before */
        const char* notbefore;
        /** @brief Not after */
        const char* notafter;
        /** @brief Serial number */
        const char* serialNumber;
    };
    /** @brief Type definition of WKC::clientCertInfo */
    typedef struct clientCertInfo_ clientCertInfo;

    /** @brief Structure for storing hardware offscreen settings */
    struct HWOffscreenDeviceParams_ {
        /** @brief Pointer to lock function passed to hardware offscreen */
        void (*fLockProc)(void* in_opaque);
        /** @brief Pointer to unlock function passed to hardware offscreen */
        void (*fUnlockProc)(void* in_opaque);
        /** @brief Parameter passed to hardware offscreen */
        bool fEnable;
        /** @brief Parameter passed to hardware offscreen */
        bool fEnableForImagebuffer;
        /** @brief Parameter passed to hardware offscreen */
        unsigned int fScreenWidth;
        /** @brief Parameter passed to hardware offscreen */
        unsigned int fScreenHeight;
    };
    /** @brief Type definition of WKC::HWOffscreenDeviceParams */
    typedef struct HWOffscreenDeviceParams_ HWOffscreenDeviceParams;

    /** @brief EPUB related structures */
    typedef struct WKCEPUBDataNav_ WKCEPUBDataNav;
    typedef struct WKCEPUBDataTocItem_ {
        unsigned int fIndex;
        const unsigned char* fToc;
        const unsigned char* fTitle;
        const unsigned char* fUri;
        const unsigned char* fType;
    } WKCEPUBDataTocItem;
    struct WKCEPUBDataNav_ {
        const unsigned char* fType;
        unsigned int fTocItems;
        WKCEPUBDataTocItem** fItems;

        unsigned int fChildItems;
        WKCEPUBDataNav** fChildren;
    };
    typedef struct WKCEPUBDataSpine_ {
        const unsigned char* fSpine;
        const unsigned char* fProperties;
        int fLinear;
    } WKCEPUBDataSpine;
    enum {
        EPageProgressDirectionDefault,
        EPageProgressDirectionLtR,
        EPageProgressDirectionRtL,
        EPageProgressDirections = 0x7fffffff
    };
    typedef struct WKCEPUBStringArray_ {
        int fItems;
        const unsigned char** fStrings;
    } WKCEPUBStringArray;
    typedef struct WKCEPUBData_ {
        WKCEPUBStringArray fTitle;
        WKCEPUBStringArray fCreator;
        WKCEPUBStringArray fContributor;
        WKCEPUBStringArray fLanguage;
        WKCEPUBStringArray fRights;
        WKCEPUBStringArray fPublisher;
        const unsigned char* fDate;
        const unsigned char* fModified;
        const unsigned char* fNavUri;

        unsigned int fPageProgressionDirection;

        unsigned int fSpineItems;
        WKCEPUBDataSpine* fSpines;

        unsigned int fNavItems;
        WKCEPUBDataNav** fNavs;
    } WKCEPUBData;

    typedef struct LayerCallbacks_ {
        bool (*fTextureMakeProc)(void* in_layer, int in_width, int in_height, int in_bpp, void** out_bitmap, int* out_rowbytes, int* out_width, int* out_height, void** out_opaque_texture, bool is_3dcanvas);
        void (*fTextureDeleteProc)(void *in_layer, void* in_bitmap);
        void (*fTextureUpdateProc)(void *in_opaque, int in_width, int in_height, void *in_bitmap, int in_texture);
        bool (*fTextureChangeProc)(void* in_layer, int in_width, int in_height, int in_bpp, void** out_bitmap, int* out_rowbytes, int* out_width, int* out_height, void** out_opaque_texture, bool is_3dcanvas);
        void (*fDidChangeParentProc)(void *in_layer);
        bool (*fCanAllocateProc)(int in_width, int in_height, int in_blendmode);
        void (*fAttachedGlTexturesProc)(void* in_layer, int* in_textures, int in_num);
    } LayerCallbacks;

    enum {
        EDeviceModeConsole = 0,
        EDeviceModeHandheld,
        EDeviceModes
    };

    enum {
        EPerformanceMode0 = 0,
        EPerformanceMode1,
        EPerformanceModes
    };

    typedef void (*SetThreadPriorityProc)(const char* in_name, int* out_priority, int* out_core);
    typedef void (*FontNoMemoryProc)();
    typedef void (*GetCurrentTimeProc)(wkc_int64* out_posixtime);
    /**
    @brief callback to determine if it is possible to save a cache data to the disk
    @param in_url url
    @param in_current_age The time that has passed since the cache's response was generated by, or successfully validated with, the origin server
    @param in_freshness_lifetime The length of time between the cache's response's generation by the origin server and its expiration time
    @param in_original_content_size The size of the content
    @param in_aligned_content_size The size of the content rounded up to a multiple of some power of 2
    @param in_mime_type mime type
    @retval "!= false" Can be cached to the disk
    @retval "== false" Cannot be cached to the disk
    */
    typedef bool (*CanCacheToDiskProc)(const char* in_url, double in_current_age, double in_freshness_lifetime, long long in_original_content_size, long long in_aligned_content_size, const char* in_mime_type);
    /**
    @brief callback to determine whether to allow connection to an address
    @param in_address IPv4 address in network byte order
    @retval "!= false" Connection to in_address is allowed
    @retval "== false" Connection to in_address is not allowed
    */
    typedef bool (*ConnectionFilteringProc)(wkc_uint32 in_address);
    /**
    @brief Checks whether to accept cookies
    @param income if inbound cookie, set true.
    @param url Requested/requesting url
    @param firstparty_host String of host name of first-party cookies
    @param cookie_domain String of domain name of cookies
    @retval "!= false" Accept
    @retval "== false" Do not accept
    @details
    Compares the first party host and domain accompanied with cookies for the received cookies, and determines whether to allow it.
    */
    typedef bool (*WillAcceptCookieProc)(bool income, const char* url, const char* firstparty_host, const char* cookie_domain);
    /**
    @brief curl debug callback
    @param curl_handle Pointer to curl handle
    @param type Type of the data (equal to curl_infotype)
    @param data Data from curl, which will not be zero terminated
    @param size Size of the data
    */
    typedef void (*CurlDebugProc)(void* curl_handle, int type, char* data, size_t size);

    typedef struct WebGLTextureCallbacks_ {
        bool (*fTextureMakeProc)(int num, unsigned int* out_textures);
        void (*fTextureDeleteProc)(int num, unsigned int* in_textures);
        bool (*fTextureChangeProc)(int num, unsigned int* inout_textures);
    } WebGLTextureCallbacks;
}

/*@}*/

// APIs
namespace WKC {

/*@{*/

/**
@brief Initializes browser engine
@param memory Pointer to heap memory for browser engine
@param physical_memory_size Size of physical memory for browser engine (bytes)
@param virtual_memory_size Size of virtual memory for browser engine (bytes)
@param font_memory Pointer to heap memory for font engine 
@param font_memory_size Size of heap memory for font engine (bytes)
@param memory_event_handler Reference to WKC::WKCMemoryEventHandler
@param timer_event_handler Reference to WKC::WKCTimerEventHandler
@retval "!= false" Succeeded in initializing
@retval "== false" Failed to initialize
@details
The heap memory for the browser engine must be set to a value of 10MB or higher.@n
If virtual_memory_size is not zero, the browser engine runs on the virtual memory system.
In that case, memory, physical_memory_size and virtual_memory_size must be aligned on your system page size.
The heap memory for the font engine must be set to an appropriate value of 256KB or higher, or 0(in case disable font engine).
Considering trade-offs with performance, 512KB is the recommended value for the size.
If font_memory and its size is 0, the font engine become disabled and no text drawn.
@attention
The method of setting heap memory for the font engine may change in the future.
*/
WKC_API bool WKCWebKitInitialize(void* memory, size_t physical_memory_size, size_t virtual_memory_size, void* font_memory, size_t font_memory_size, WKCMemoryEventHandler& memory_event_handler, WKCTimerEventHandler& timer_event_handler);
/**
@brief Terminates browser engine
@return None
*/
WKC_API void WKCWebKitFinalize();
/**
@brief Forcibly terminates the browser engine
@return None
@details
This function performs the processes required for forcibly terminating the browser engine all at once.
It must only be called if there is insufficient memory, etc. in the browser engine, and the browser engine cannot be terminated except by force.
In normal situations when terminating the browser engine, you should follow the normal procedure and call WKC::WKCWebKitFinalize().
*/
WKC_API void WKCWebKitForceTerminate();
/**
@brief Forcibly terminates browser engine in order to terminate application
@return None
@details
This function performs the processes required for forcibly terminating the browser engine all at once.
Call it to forcibly terminate the browser engine, and consequently, the application.
In normal situations when terminating the browser engine, you should follow the normal procedure and call WKC::WKCWebKitFinalize().
*/
WKC_API void WKCWebKitForceFinalize();
/**
@brief Reset Maximum heap usage
@return None
@details
This function reset the Maximum heap usage.
*/
WKC_API void WKCWebKitResetMaxHeapUsage();
/**
@brief Gets amount of browser engine heap available
@retval unsigned int Heap memory available (bytes)
@deprecated
This function will be removed at NX 4.0. Use WKC::Heap::GetAvailableSize().
*/
WKC_API unsigned int WKCWebKitAvailableMemory();
/**
@brief Gets maximum size of browser engine heap that can be allocated
@retval unsigned int Maximum size that can be allocated (bytes)
@deprecated
This function will be removed at NX 4.0. Use WKC::Heap::GetMaxAvailableBlockSize().
*/
WKC_API unsigned int WKCWebKitMaxAvailableBlock();
/**
@brief Executes JS garbage collector
@param is_now
- != false Do GC immediately
- == false Just request GC
@return None
@details
This function calls GC in JS. In regard to call timing, it must be called by the application context.
*/
WKC_API void WKCWebKitRequestGarbageCollect(bool is_now = false);
/**
@brief Gets recommended size of heap memory for font engine
@return Recommended size (bytes)
@details
This function gets the recommend font_memory_size value to pass to WKC::WKCWebKitInitialize() or WKC::WKCWebKitResumeFont().
*/
WKC_API unsigned int WKCWebKitFontHeapSize();
/**
@brief Suspends font engine
@retval "!= false" Succeeded in suspending
@retval "== false" Failed to suspend
@details
Sends suspend request to font engine.
When suspending is successful, the font engine heap can be detached, which makes it available for other uses if necessary.
*/
WKC_API bool WKCWebKitSuspendFont();
/**
@brief Resumes font engine
@param font_memory Pointer to heap memory for font engine 
@param font_memory_size Size of heap memory for font engine (bytes)
@return None
@details
Provides heap memory for the font engine, so the font engine can operate.
*/
WKC_API void WKCWebKitResumeFont(void* font_memory, unsigned int font_memory_size);
/**
@brief Registers font data (passed in memory)
@param memPtr Address of font data mapped to ROM or RAM
@param len Length of font data mapped to ROM or RAM
@return ID of registered font data
@retval ==-1 Failed to register
@retval >= 0 Succeeded in registering
@details
Registers font data that exists in memory in the browser engine.@n
Using font data that is mapped to memory improves performance. Memory allocation and release must be performed on the application side.@n
By default, the font data that is registered first is set as the primary font.@n
A maximum of 16 font items can be registered.@n
*/
WKC_API int WKCWebKitRegisterFontOnMemory(const unsigned char* memPtr, unsigned int len);
/**
@brief Registers font data (passed in file).
@param filePath File path of font data
@return ID of registered font data
@retval ==-1 Failed to register
@retval >= 0 Succeeded in registering
@details
Registers font data that exists as files in the browser engine.@n
By default, the font data that is registered first is set as the primary font.@n
A maximum of 16 font items can be registered.@n
*/
WKC_API int WKCWebKitRegisterFontInFile(const char* filePath);
/**
@brief Deletes all registered font data
@return None
@details
Deletes all registered font data. Normally, this is called within WebKitFinalize().@n
*/
WKC_API void WKCWebKitUnregisterFonts();

/**
@brief Sets font scale for specified font
@return Success or not
@details
Sets font scale for specified font.@n
*/
WKC_API bool WKCWebKitSetFontScale(int id, float scale);

/**
@brief Sets a callback for a memory shortage in font engine
@param proc callback
@details
Sets a callback for memory shortage in font engine. the callback is called when a memory allocation failed in font engine.
*/
WKC_API void WKCWebKitSetFontNoMemoryCallback(FontNoMemoryProc proc);

/**
@brief Wakes up browser engine
@param in_timer Timer passed by WKCTimerEventHandler::requestWakeUp()
@param in_data Data passed by WKCTimerEventHandler::requestWakeUp()
@return None
@details
Wakes up the browser engine that is waiting for timer wakeup using WKCTimerEventHandler::requestWakeUp().
*/
WKC_API void WKCWebKitWakeUp(void* in_timer, void* in_data);
/**
@brief Get current tick count
@return Current tick count
@details
Get current tick count
*/
WKC_API unsigned int WKCWebKitGetTickCount();

/**
@brief Sets a proc for getting current time.
@param in_proc Callback of getting current time.
@details
Sets a proc for getting current time.
*/
WKC_API void WKCWebkitRegisterGetCurrentTimeCallback(WKC::GetCurrentTimeProc in_proc);

/**
@brief Sets hardware offscreen
@param params Setting value
@param opaque WKC::HWOffscreenDeviceParams::fLockProc, User data passed when calling WKC::HWOffscreenDeviceParams::fUnlockProc
@details
When using hardware offscreen, this function and Graphics Peer must be implemented appropriately.
*/
WKC_API void WKCWebKitSetHWOffscreenDeviceParams(const HWOffscreenDeviceParams* params, void* opaque);

/**
@brief Sets layer callbacks
@param callbacks Callbacks
@details
Sets layer callbacks. This function is needed for specific targets.
*/
WKC_API void WKCWebKitSetLayerCallbacks(const LayerCallbacks* callbacks);

/**
@brief Sets WebGL texture callbacks
@param callbacks Callbacks
@details
Sets WebGL texture creation related callbacks.
If the application have to manage OpenGL texture creation/destruction,
then register callbacks with this API.
Or the textures will be created/destroyed inside the engine.
This function must be called before loading any WebGL contents.
*/
WKC_API void WKCWebKitSetWebGLTextureCallbacks(const WebGLTextureCallbacks* callbacks);

/**
@brief Gets layer infos
@param layer Layer
@param texture Texture id
@param width Width of exture
@param height Height of texture
@param need_yflip Needs yflip
@param offscreen Offscreen
@param offscreenwidth Offscreen width
@param offscreenheight Offscreen height
@param is_3dcanvas whether 3d canvas or not
@details
Obtains layer infos
*/
WKC_API void WKCWebKitGetLayerProperties(void* layer, void** opaque_texture, int* width, int* height, bool* need_yflip, void** offscreen, int*offscreenwidth, int* offscreenheight, bool* is_3dcanvas);

/**
@brief Create an offscreen
@param format The pixel format of the offscreen
@param bitmap Actual memory to be drawn
@param rowbytes Bytes in a row
@param size Size of the offscreen
@return new offscreen or null
@details
Create new offscreen. Only for obtain the drawcontext by WKCWebKitDrawContextNew().
*/
WKC_API void* WKCWebKitOffscreenNew(OffscreenFormat format, void* bitmap, int rowbytes, const WKCSize* size);
/**
@brief Delete an offscreen
@param offscreen Offscrren created by WKCWebKitOffscreenNew()
@details
Delete the given offscreen.
*/
WKC_API void WKCWebKitOffscreenDelete(void* offscreen);
/**
@brief Obtain the state of an offscreen
@param offscreen Offscrren created by WKCWebKitOffscreenNew()
@return whether or not the offscreen is in error state.
@details
Return whether or not the given offscreen is in error state. If true, the offscreen should be deleted and re-created.
*/
WKC_API bool WKCWebKitOffscreenIsError(void* offscreen);
/**
@brief Create the drawcontext of an offscreen
@param offscreen Offscrren created by WKCWebKitOffscreenNew()
@return new drawcontext or null
@details
Create new drawcontext. Only for use as an argument of WKC::GraphicsLayer::paint() and clear().
*/
WKC_API void* WKCWebKitDrawContextNew(void* offscreen);
/**
@brief Delete a drawcontext
@param context Drawcontext created by WKCWebKitDrawContextNew()
@details
Delete the given drawcontext.
*/
WKC_API void WKCWebKitDrawContextDelete(void* context);
/**
@brief Obtain the state of a drawcontext
@param context Drawcontext created by WKCWebKitDrawContextNew()
@return whether or not the drawcontext is in error state.
@details
Return whether or not the given drawcontext is in error state. If true, the context should be deleted and re-created.
*/
WKC_API bool WKCWebKitDrawContextIsError(void* context);

/**
@brief Sets plug-in instances
@param instance1 Environment-dependent instance 1
@param instance2 Environment-dependent instance 2
@retval None
@details
Registers application-side instance information that will be required when generating plug-ins.
For more information, see the Peer-side implementations since the values that should be set are different for every environment and Peer implementation.
*/
WKC_API void WKCWebKitSetPluginInstances(void* instance1, void* instance2);
/**
@brief Registers trusted ROOT CA certificate
@param cert Pointer to buffer for ROOT CA certificate to register
@param cert_len Length of data to register
@retval != 0 void pointer of registered certificate. Use when unregistering (WKC::WKCWebKitSSLUnregisterCert()) certificates individually.
@retval == 0 Failed to register
@details
Registers trusted ROOT CA certificate.
- Data format must be X.509 v3 certificate data (ASN.1 definition)
- Data must be PEM encoded
- One item of data may include one certificate
- Multiple ROOT CA certificates may be registered by calling this multiple times
@attention
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API void* WKCWebKitSSLRegisterRootCA(const char* cert, int cert_len);
WKC_API void* WKCWebKitSSLRegisterRootCAByDER(const char* cert, int cert_len);
/**
@brief Unregisters ROOT CA certificate
@param certid void pointer of registered certificate
@retval < 0 Failed to register
@retval == 0 Succeeded in registering
@details
Unregisters a registered ROOT CA certificate.
@attention
certid must be the value obtained by WKC::WKCWebKitSSLRegisterRootCA().
*/
WKC_API int   WKCWebKitSSLUnregisterRootCA(void* certid);
/**
@brief Unregisters all ROOT CA certificates
@details
Unregisters all registered ROOT CA certificates.
*/
WKC_API void  WKCWebKitSSLRootCADeleteAll(void);
/**
@brief Registers trusted CRL certificate
@param crl CRL certificate data to register
@param crl_len Length of CRL certificate data
@retval != 0 void pointer of registered certificate. Used when unregistering (WKC::WKCWebKitSSLUnregisterCRL()) certificates individually.
@retval == 0 Failed to register
@details
Registers trusted CRL certificate.
- Data format must be X.509 v3 certificate data (ASN.1 definition)
- Data must be PEM encoded
- One item of data may include one CRL
- Multiple CRL certificates may be registered by calling this multiple times
@attention
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API void* WKCWebKitSSLRegisterCRL(const char* crl, int crl_len);
/**
@brief Unregisters CRL certificate
@param crlid void pointer of registered certificate
@retval < 0 Failed to register
@retval == 0 Succeeded in registering
@details
Unregisters a registered CRL certificate.
@attention
crlid must be the value obtained by WKC::WKCWebKitSSLRegisterCRL().
*/
WKC_API int   WKCWebKitSSLUnregisterCRL(void* crlid);
/**
@brief Unregisters all CRL certificates
@details
Unregisters all registered CRL certificates.
*/
WKC_API void  WKCWebKitSSLCRLDeleteAll(void);
/**
@brief Registers Client certificate
@param pkcs12 Client certificate consisting of private key and certificate
@param pkcs12_len Length of Client certificate
@param pass Private key password
@param pass_len Length of private key
@retval != 0 void pointer of registered Client certificate. Used when unregistering (WKC::WKCWebKitSSLUnregisterClientCert()) certificates individually.
@retval == 0 Failed to register
@details
Registers Client certificate.
- Data format must be PKCS#12 formatted combination of private and certificate.
- Data must be PEM encoded
- One item of data may include one client certificate
- Multiple Client certificates may be registered by calling this multiple times.
@attention
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API void* WKCWebKitSSLRegisterClientCert(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len);
WKC_API void* WKCWebKitSSLRegisterClientCertByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len);
/**
@brief Unregisters Client certificate
@param certid void pointer of registered Client certificate
@retval < 0 Failed to register
@retval == 0 Succeeded in registering
@details
Unregisters a registered Client certificate.
@attention
certid must be the value obtained by WKC::WKCWebKitSSLRegisterClientCert().
*/
WKC_API int   WKCWebKitSSLUnregisterClientCert(void* certid);
/**
@brief Unregisters all Client certificates
@details
Unregisters all registered Client certificates.
*/
WKC_API void  WKCWebKitSSLClientCertDeleteAll(void);
/**
@brief Registers certificate in black list
@param issuerCommonName CommonName or organizationalUnitName of issuer of abused certificate
@param SerialNumber SerialNumber of abused certificate. Must be a contiguous sequence in hexadecimal format, and even-numbered in length._
@retval true Succeeded in registering
@retval "== false" Failed to register
@details
Registers issuerCommonName and SerialNumber of certificate in question to black list.@n
Used when checking whether the path of the certificate received includes corresponding certificate.
@attention
Becomes effective on next connection.@n
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API bool  WKCWebKitSSLRegisterBlackCert(const char* issuerName, const char* SerialNumber);
/**
@brief Registers certificate in black list by DER
@param cert certificate data to register
@param crl_len Length of certificate data
@retval true Succeeded in registering
@retval "== false" Failed to register
@details
Get access to DER format certificate in its raw form to register issuer(CommonName or OrganizationalUnit) and serial of certificate to black list.@n
@attention
Becomes effective on next connection.@n
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API bool  WKCWebKitSSLRegisterBlackCertByDER(const char* cert, int cert_len);
/**
@brief Unregisters all black listed certificates
@details
Unregisters all black listed certificates.
*/
WKC_API void  WKCWebKitSSLBlackCertDeleteAll(void);
/**
@brief Registers certificate in untrusted list by DER
@param cert certificate data to register
@param crl_len Length of certificate data
@retval true Succeeded in registering
@retval "== false" Failed to register
@details
Get access to DER format certificate in its raw form to register issuer(CommonName or OrganizationalUnit) and serial of certificate to untrusted list.@n
@attention
Becomes effective on next connection.@n
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API bool  WKCWebKitSSLRegisterUntrustedCertByDER(const char* cert, int cert_len);
/**
@brief Registers information such as OID of trusted ROOT certificate to allow EV SSL
@param issuerCommonName CommonName or organizationalUnitName of ROOT certificate issuer
@param OID EV SSL OID of ROOT certificate issuer
@param sha1FingerPrint SHA1 FingerPrint of ROOT certificate. Must be in the format that connects hexadecimal numbers with ':'.
@param SerialNumber SerialNumber of ROOT certificate. Must be a contiguous sequence in hexadecimal format, and even-numbered in length.
@retval true Succeeded in registering
@retval "== false" Failed to register
@details
Registers EV SSL OID of trusted ROOT certificate to allow EV SSL, etc.
@attention
Becomes effective on next connection.@n
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API bool  WKCWebKitSSLRegisterEVSSLOID(const char *issuerCommonName, const char *OID, const char *sha1FingerPrint, const char *SerialNumber);
/**
@brief Unregisters all EV-SSL OID
@details
Unregisters all EV-SSL OID.
*/
WKC_API void  WKCWebKitSSLEVSSLOIDDeleteAll(void);
/**
@brief Sets to unconditionally allow SSL communication with server
@param host_w_port Server host name and port number concatenated with ":"
@retval None
@details
Sets to allow SSL communication with some server unconditionally.@n
Used when user intentionally allows SSL communication even though certificates that are sent are invalid.
@attention
Becomes effective on next connection.@n
This API must be called after WKC::WKCWebView::WKCWebKitInitialize is called.
*/
WKC_API void  WKCWebKitSSLSetAllowServerHost(const char *host_w_port);
/**
@brief Sets file access callback
@param procs Callback structure for file access
@retval None
@details
Registers callback to set processing for the platform file system.
All WKC::FileSystemProcs member callbacks must be implemented.
The registered file access process is used for processing uploaded files, loading %file:/// content, SSL, database processing, plug-in, or media player.
@sa @ref bbb-filecontrol
*/
WKC_API void WKCWebKitSetFileSystemProcs(const WKC::FileSystemProcs* procs);
WKC_API void WKCWebKitSetMediaPlayerProcs(const WKC::MediaPlayerProcs* procs);
/**
@brief Sets pasteboard (clipboard) access callback
@param procs Callback structure for pasteboard access
@retval None
@details
Registers callback to set processing for the platform pasteboard.
All WKC::PasteboardProcs member callbacks must be implemented.
*/
WKC_API void WKCWebKitSetPasteboardProcs(const WKC::PasteboardProcs* procs);
WKC_API void WKCWebKitSetThreadProcs(const WKC::ThreadProcs* procs);
/**
@brief get server certificate chian
@param in_url url
@param out_num number of certificate in chain
@retval certificates
@details
Get server certificate chian for url.
@attention
certificates MUST free by WKCWebKitSSLFreeServerCertChain()
*/
WKC_API const char** WKCWebKitSSLGetServerCertChain(const char* in_url, int& out_num);

/**
@brief free server certificate chian memory
@param chain the pointer of chain
@param num number of certificates
@details
free memory of server certificate chian.
*/
WKC_API void WKCWebKitSSLFreeServerCertChain(const char** chain, int num);

/**
@brief Sets font glyph cache
@param format Cache format 
@param cache Cache data
@param size Cache size
@retval true Succeeded
@retval "== false" Failed
@details
The format, cache, and size are passed to wkcOffscreenCreateGlyphCachePeer(), the engine does not affect that content.
*/
WKC_API bool WKCWebKitSetGlyphCache(int format, void* cache, const WKCSize* size);
/**
@brief Sets image cache
@param format Cache format 
@param cache Cache data
@param size Cache size
@retval "!=false" Succeeded
@retval "== false" Failed
@details
The format, cache, and size are passed to wkcOffscreenCreateImageCachePeer(), the engine does not affect that content.
*/
WKC_API bool WKCWebKitSetImageCache(int format, void* cache, const WKCSize* size);
/**
@brief Returns is in crashing mode
@retval "!=false" Now in crashing
@retval "== false" Not crashing
@details
Returns is in crashing mode.
*/
WKC_API bool WKCWebKitIsMemoryCrashing();
/**
@brief Specifies resource path for webAudio
@param path path of webAudio resources
*/
WKC_API void WKCWebKitSetWebAudioResourcePath(const char* path);
/**
@brief Specifies resource path for WebInspector
@param path path of WebInspector resources
*/
WKC_API void WKCWebKitSetWebInspectorResourcePath(const char* path);
/**
@brief Starts WebInspector server
@param addr listening interface address
@param port listening port
@param modalcycle modal cycle callback function
@param opaque opaque parameter for modalcycleproc
@retval "!= false" Succeeded to start server
@retval "== false" Failed to start server
*/
WKC_API bool WKCWebKitStartWebInspector(const char* addr, int port, bool(*modalcycle)(void*), void* opaque);
/**
@brief Stops WebInspector server
*/
WKC_API void WKCWebKitStopWebInspector();

/**
@brief Clears cookies
@return None
@details
Internal cookie data is shared by the browser engine. @n
Calling this function clears all of the cookie data inside the browser engine.
@attention
It may be called at any time when clearing cookies that were arbitrarily saved by the user. However, the reproduction of pages cannot be guaranteed after cookies are cleared.@n
If the application wants to clear cookies other than those intended by the user, it should be avoided while browsing a page.
*/
WKC_API void WKCWebKitClearCookies(void);
/**
@brief Serializing cookies
@param buff Buffer for data to load
@param bufflen Length of buffer for data to load
@return write length
@details
Loads serialized cookies.
@attention
- If buff is null, just return buffer length to write.
*/
WKC_API int  WKCWebKitCookieSerialize(char* buff, int bufflen);
/**
@brief Deserializing cookies
@param buff Buffer for data to register
@param restart Specify true when restarting the engine due to insufficient memory, etc.
@details
Registers serialized cookies.
@attention
- buff must be termited by "\0".
*/
WKC_API void WKCWebKitCookieDeserialize(const char* buff, bool restart);
/**
@brief Gets cookies of specified uri
@param uri URI to get cookies
@param buf Buffer for storing cookies
@param len Max length of buffer
@retval Length of cookies string
@details
Gets cookies of specified uri, which the cookies is null terminated string.
@attention
- If buf is null, just return buffer length to write.
*/
WKC_API int WKCWebKitCookieGet(const char* uri, char* buf, unsigned int len);
/**
@brief Sets cookies of specified uri
@param uri URI to set cookies
@param cookie Cookies string
@details
Sets cookies of specified uri
*/
WKC_API void WKCWebKitCookieSet(const char* uri, const char* cookie);
/**
@brief Gets number of current WebSocket connections
@retval ">= 0" Number of current WebSocket connections
@retval -1 Faild
@details
Gets number of current WebSocket connections
*/
WKC_API int WKCWebKitCurrentWebSocketConnectionsNum(void);
/**
@brief Sets callback function for DNS prefetch
@retval None
@details
Sets callback function for DNS prefetch. The function will be called for pre-fetching DNS entry.
*/
WKC_API void WKCWebKitSetDNSPrefetchProc(void(*requestprefetchproc)(const char*), void* resolverlocker);
/**
@brief Sets resolved DNS entry to network peer
@retval None
@details
Sets resolved DNS entry to network peer.
*/
WKC_API void WKCWebKitCachePrefetchedDNSEntry(const char* hostname, const unsigned char* ipaddr);

/**
@brief Get the number of sockets
@retval Number of sockets
*/
WKC_API int WKCWebKitGetNumberOfSockets(void);
/** @brief Structure that contains the socket statistics */
struct SocketStatistics_ {
    /** @brief Socket FD */
    int fFd;
    /** @brief Number of bytes received */
    unsigned int fRecvBytes;
    /** @brief Number of bytes sent */
    unsigned int fSendBytes;
};
/** @brief Type definition of WKC::SocketStatistics */
typedef struct SocketStatistics_ SocketStatistics;
/**
@brief Get the statistics of sockets
@param in_numberOfArray Number of array
@param out_statistics Statistics of sockets
@retval Number of statistics
*/
WKC_API int WKCWebKitGetSocketStatistics(int in_numberOfArray, SocketStatistics* out_statistics);

/**
@brief Set IsNetworkAvailable callback in network peer
@param in_proc callback
@retval None
@details
Set IsNetworkAvailable callback in network peer.
*/
WKC_API void WKCWebKitSetIsNetworkAvailableProc(bool(*in_proc)(void));

/**
@brief Resets GPU
@details
Resets GPU.
*/
WKC_API void WKCWebKitResetGPU();

/**
@brief Set device mode used for custom media query.
@param in_mode Device mode (EDeviceModeConsole or EDeviceModeHandheld)
@details
Set device mode.
*/
WKC_API void WKCWebKitSetDeviceMode(int in_mode);

/**
@brief Set performance mode used for custom media query.
@param in_mode Performance mode (EPerformanceMode0 or EPerformanceMode1)
@details
Set performance mode.
*/
WKC_API void WKCWebKitSetPerformanceMode(int in_mode);

/**
@brief Cancel dragging mode of range input element.
@details
Cancel dragging mode of range input element.
*/
WKC_API void WKCWebKitCancelRangeInputDragging();

/**
@brief Sets a callback to determine if it is possible to save a cache data to the disk
@param in_proc callback
@details
Sets a callback to determine if it is possible to save a cache data to the disk.
*/
WKC_API void WKCWebKitSetCanCacheToDiskCallback(CanCacheToDiskProc in_proc);

/**
@brief Sets a callback to determine whether to allow connection to an address
@param in_proc callback
@details
Sets a callback to determine whether to allow connection to in_address.
*/
WKC_API void WKCWebKitSetConnectionFilteringCallback(ConnectionFilteringProc in_proc);

/**
@brief Sets a callback to determine whether to accept cookies
@param in_proc callback
@details
Sets a callback to determine whether to accept cookies.
*/
WKC_API void WKCWebKitSetWillAcceptCookieCallback(WillAcceptCookieProc in_proc);

/**
@brief Sets a curl debug callback
*/
WKC_API void WKCWebKitSetCurlDebugCallback(CurlDebugProc in_proc);

/**
@brief Dump HTTP Cache info.
@details
Dump HTTP Cache info.
@attention
- This API is debug use only.
*/
WKC_API void WKCWebKitDumpHTTPCacheList();

WKC_API void WKCWebKitDisplayWasRefreshed();

/**
@brief Initialize Log Channels.
@param logLevelString initialize string
@details
Initialize Log Channels by logLevelString.
See WebCore/platform/Logging.h and WTF/wtf/Assertions.cpp for how to specify the settings.
WTFInitializeLogChannelStatesFromString() in Assertions.cpp is a parser.
Logging.h has a Channel names.
*/
WKC_API void WKCWebKitInitializeLogChannelsIfNecessary(const char* logLevelString);

/** @brief Class that corresponds to the content display screen of the browser. */
class WKC_API WKCWebView
{
    friend class WKCWebFrame;
    friend class WKCWebViewPrefs;

public:
    // life and death
    /**
       @brief Generates WebView
       @param builders Reference to WKCClientBuilders
       @retval WKCWebView* Pointer to WebView
       @details
       Generates WKC::WKCWebView and returns its pointer as a return value. @n
       @attention
       WKC::WKCWebView::deleteWKCWebView() must be called to discard the generated WebView.
    */
    static WKCWebView* create(WKCClientBuilders& builders);
    /**
       @brief Discards WebView
       @param self Pointer to WebView
       @return None
       @details
       Discards the WebView generated by WKC::WKCWebView::create().
    */
    static void deleteWKCWebView(WKCWebView *self);

    /**
       @brief Notifies of forced termination of browser engine
       @return None
       @details
       Notifies the WKCWebView class instance of forced termination of the browser engine.
       When there are multiple instances of the WKCWebView class, this must be called for each individual instance.
       It must only be called if there is insufficient memory, etc. in the browser engine, and the browser engine cannot be terminated except by force.
       Always call this function before calling WKC::WKCWebKitForceTerminate().
    */
    void notifyForceTerminate();

    // off-screen draw
    /**
       @brief Sets offscreen 
       @param format Offscreen format type
       @param bitmap Pointer to offscreen memory
       @param rowbytes Number of bytes per line of offscreen
       @param desktopsize Initial value for desktop size
       @param viewsize Initial value for view size (fixed layout size)
       @param fixedlayout Setting for using fixed layout size
       @param needslayout Needs layout
       - != false Use fixed layout size.
       - == false Do not use fixed layout size.
       @retval "!= false" Succeeded
       @retval "== false" Failed
    */
    bool setOffscreen(WKC::OffscreenFormat format, void* bitmap, int rowbytes, const WKCSize& offscreensize, const WKCSize& viewsize, bool fixedlayout, const WKCSize* const desktopsize = 0, bool needslayout = true);
    /**
       @brief Changes view size
       @param size View size
       @return None
       @details
       View size is used as the layout size. The setting for the initial value specified by WKCWebView::setOffscreen() due to page transitions is remodified by the engine. 
    */
    void notifyResizeViewSize(const WKCSize& size);
    /**
       @brief Changes desktop size
       @param size Desktop size
       @param resizeevent
       - != false Send resize event
       - == false Do not send resize event
       @return None
       @details
       Desktop size is used as the size for drawing. The setting for the initial value specified by WKCWebView::setOffscreen() due to page transitions is remodified by the engine. 
    */
    void notifyResizeDesktopSize(const WKCSize& size, bool resizeevent = true);
    /**
       @brief Requests engine layout
       @param force @n
       - != false  Forced layout@n
       - == false  Normal layout@n
       @return None
    */
    void notifyRelayout(bool force = false);
    void notifyPaintOffscreenFrom(const WKCRect& rect, const WKCPoint& p);
    /**
       @brief Requests drawing of offscreen
       @param rect Rectangle in which to draw
       @return None
    */
    void notifyPaintOffscreen(const WKCRect& rect);
#ifdef USE_WKC_CAIRO
    /**
       @brief Requests drawing to specified context
       @param rect Rectangle in which to draw
       @param context Context to draw
       @return None
    */
    void notifyPaintToContext(const WKCRect& rect, void* context);
#endif
    /**
       @brief Requests scrolling of offscreen
       @param rect Rectangle to scroll
       @param diff Scroll amount
       @return None
       @details
       This function must always be called within the WKC::ChromeClientWKC::scroll() callback. @n 
       And, the rectToScroll and scrollDelta notified by WKC::ChromeClientWKC::scroll() must be specified for the rect and diff to be specified as arguments.
    */
    void notifyScrollOffscreen(const WKCRect& rect, const WKCSize& diff);

    // events
    /**
       @brief Sends key press event
       @param key Key type
       @param modifiers Modifier key types
       @param in_autorepeat Auto repeat or not
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
       @details
       In this API and in WKC::WKCWebView::notifyKeyRelease(), sending WKC::EKeyTab moves the focus forward.
       Sending WKC::EKeyTab and WKC::EModifierShift moves the focus backward.
       @attention
       If false is set for the return value of WKC::ChromeClientWKC::tabsToLinks(), the focus will not move even when WKC::EKeyTab is sent.@n
       And if true is returned, following WKC::WKCWebView::notifyKeyChar() should not be sent.
    */
    bool notifyKeyPress(WKC::Key key, WKC::Modifier modifiers, bool in_autorepeat=false);
    /**
       @brief Sends key release event
       @param key Key type
       @param modifiers Modifier key types
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
       @details
       In this API and in WKC::WKCWebView::notifyKeyPress(), sending WKC::EKeyTab moves the focus forward.
       Sending WKC::EKeyTab and WKC::EModifierShift moves the focus backward.
       @attention
       If false is set for the return value of WKC::ChromeClientWKC::tabsToLinks(), the focus will not move even when WKC::EKeyTab is sent.
    */
    bool notifyKeyRelease(WKC::Key key, WKC::Modifier modifiers);
    /**
       @brief Sends character input event
       @param in_char Character code (unicode)
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
    */
    bool notifyKeyChar(unsigned int in_char);
    /**
       @brief TBD
       @retval TBD
    */
    bool notifyIMEComposition(const unsigned short* in_string, WKC::CompositionUnderline* in_underlines, unsigned int in_underlineNum, unsigned int in_cursorPosition, unsigned int in_selectionEnd, bool in_confirm);
    /**
       @brief Sends access key event
       @param in_char Character code (unicode)
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
    */
    bool notifyAccessKey(unsigned int in_char);
    /**
       @brief Sends mouse down event
       @param pos Position where event occurred
       @param button Type of mouse button
       - WKC::EMouseButtonLeft Left mouse button
       - WKC::EMouseButtonMiddle Middle mouse button
       - WKC::EMouseButtonRight Right mouse button
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
       @details
       WKC::EMouseButtonNone is not used.
    */
    bool notifyMouseDown(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers);
    /**
       @brief Sends mouse up event.
       @param pos Position where event occurred
       @param button Type of mouse button
       - WKC::EMouseButtonLeft Left mouse button
       - WKC::EMouseButtonMiddle Middle mouse button
       - WKC::EMouseButtonRight Right mouse button
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
       @details
       WKC::EMouseButtonNone is not used.
    */
    bool notifyMouseUp(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers);
    /**
       @brief Sends mouse move event
       @param pos Position where event occurred
       @param button Type of mouse button
       - WKC::EMouseButtonLeft Left mouse button
       - WKC::EMouseButtonMiddle Middle mouse button
       - WKC::EMouseButtonRight Right mouse button
       - WKC::EMouseButtonNone  No mouse button
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
    */
    bool notifyMouseMove(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers);
    /**
       @brief Tests mouse move event to affect content changing
       @param pos Position where event occurred
       @param button Type of mouse button
       - WKC::EMouseButtonLeft Left mouse button
       - WKC::EMouseButtonMiddle Middle mouse button
       - WKC::EMouseButtonRight Right mouse button
       - WKC::EMouseButtonNone  No mouse button
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @param contentChanged Content will be changed
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
    */
    bool notifyMouseMoveTest(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers, bool& contentChanged);
    /**
       @brief Sends mouse double-click event
       @param pos Position where event occurred
       @param button Type of mouse button
       - WKC::EMouseButtonLeft Left mouse button
       - WKC::EMouseButtonMiddle Middle mouse button
       - WKC::EMouseButtonRight Right mouse button
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
       @details
       WKC::EMouseButtonNone is not used.
    */
    bool notifyMouseDoubleClick(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers);
    /**
       @brief Sends mouse wheel event
       @param pos Position where event occurred
       @param diff Wheel move amount
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
    */
    bool notifyMouseWheel(const WKCPoint& pos, const WKCSize& diff, WKC::Modifier modifiers);
    /**
    @brief Forcely set mouse press state
    @param pressed pressed or not
    */
    void notifySetMousePressed(bool pressed);
    /**
    @brief Notifies mouse capture is lost
    */
    void notifyLostMouseCapture();
    /** @brief Structure that stores touch coordinate information */
    struct TouchPoint_ {
        /** @brief ID for identifier */
        int fId;
        /** @brief Touch state */
        int fState;
        /** @brief Touched coordinate */
        WKCPoint fPoint;
    };
    /** @brief Type definition of WKC::TouchPoint */
    typedef struct TouchPoint_ TouchPoint;
    /**
       @brief Sends touch event.
       @param type Event type
       - WKC::ETouchTypeStart Start touch
       - WKC::ETouchTypeMove Move touch
       - WKC::ETouchTypeEnd End touch
       - WKC::ETouchTypeCancel Cancel touch
       @param points List of touch information
       @param npoints Length of list
       @param modifiers Modifier key types
       - WKC::EModifierNone  No modifier key
       - WKC::EModifierCtrl  CTRL key
       - WKC::EModifierShift  SHIFT key
       - WKC::EModifierAlt   ALT key
       @retval "!= false" The event is consumed
       @retval "== false" The event is not consumed
    */
    bool notifyTouchEvent(int type, const TouchPoint* points, int npoints, WKC::Modifier modifiers);
    bool hasTouchEventHandlers();
    /**
       @brief Sends scroll event
       @param scrolltype Type of scroll
       @retval "!= false" Succeeded
       @retval "== false" Failed
    */
    bool notifyScroll(WKC::ScrollType scrolltype);
    /**
       @brief Activates focus frame
       @return None
    */
    void notifyFocusIn();
    /**
       @brief Releases active frame
       @return None
    */
    void notifyFocusOut();
    /**
       @brief Set focused element
       @param element Focused element
       @retval "!=false" Succeeded
       @retval "==NULL" Failed
       @details
       Set focused element
    */
    bool setFocusedElement(WKC::Element* element);
    /**
       @brief Requests suspension of browser engine.
       @return None
    */
    void notifySuspend();
    /**
       @brief Requests resumption of browser engine.
       @return None
    */
    void notifyResume();
    /**
       @brief Notifies scroll position is changed
       @return None
    */
    void notifyScrollPositionChanged();

    /**
       @brief Sends scroll event
       @param dx Move amount in x direction
       @param dy Move amount in y direction
       @retval "!= false" Succeeded
       @retval "== false" Failed
    */
    bool notifyScroll(int dx, int dy);
    /**
       @brief Sends scroll event
       @param x x coordinate of scroll position
       @param y y coordinate of scroll position
       @retval "!= false" Succeeded
       @retval "== false" Failed
    */
    bool notifyScrollTo(int x, int y);
    /**
       @brief Gets scroll position
       @param pos Reference to scroll position coordinates
       @return None
    */
    void scrollPosition(WKCPoint& pos);
    /**
       @brief Gets maximum scroll position
       @param pos Reference to scroll position coordinates
       @return None
    */
    void maximumScrollPosition(WKCPoint& pos) const;
    /**
       @brief Gets minimum scroll position
       @param pos Reference to scroll position coordinates
       @return None
    */
    void minimumScrollPosition(WKCPoint& pos) const;
    /**
       @brief Gets content size
       @param size Reference to content size
       @return None
    */
    void contentsSize(WKCSize& size);

    /**
      @brief Initialize gamepads
      @param None
      @return None
      @details
      Initialize gamepads
    */
    static void initializeGamepads();

    /**
      @brief Notify connect gamepad
      @param id identifier string of gamepad
      @param naxes number of axes.
      @param nbuttons number of buttons
      @return index of gamepad
      @details
      notify connect gamepads.
    */
    static int connectGamepad(const WKC::String& id, int naxes, int nbuttons);

    /**
      @brief Notify connect gamepad
      @param index index of gamepad when connectGamepad() returned.
      @return None
      @details
      notify disconnect gamepads.
    */
    static void disconnectGamepad(int index);

    /**
      @brief Update gamepad value
      @param index index of gamepad when connectGamepad() returned.
      @param timestamp timestamp
      @param naxes number of axes
      @param axes axes
      @param nbuttons number of buttons
      @param buttons buttons
      @return !=false succeeded
      @return ==false failed
      @details
      Update Gamepad value
    */
    static bool updateGamepadValue(int index, long long timestamp, int naxes, const double* axes, int nbuttons, const double* buttons);

    // APIs
    /**
       @brief Gets title of display content
       @retval "const unsigned short*" Pointer to title string
    */
    const unsigned short* title();
    /**
       @brief Gets URL of display content
       @retval "const char*" Pointer to URL string
    */
    const char* uri();

    /**
       @brief Gets WKC::Page instance.
       @retval Pointer to WKC::Page instance
    */
    WKC::Page* core();
    /**
       @brief Gets settings
       @retval WKC::WKCSettings* Pointer to WKC::WKCSettings
    */
    WKC::WKCSettings* settings();

    /**
       @brief Checks whether backward movement in transition history is allowed
       @retval "!= false" Movement allowed
       @retval "== false" Movement not allowed
    */

    bool canGoBack();
    /**
       @brief Checks whether forward or backward movement in transition history is allowed 
       @param steps Movement amount @n
       - > 0 Forward movement amount @n
       - < 0 Backward movement amount @n
       @retval "!= false" Movement allowed
       @retval "== false" Movement not allowed
       @details
       Always returns !=false when 0 is entered for steps.
    */
    bool canGoBackOrForward(int steps);
    /**
       @brief Checks whether forward movement in transition history is allowed
       @retval "!= false" Movement allowed
       @retval "== false" Movement not allowed
    */
    bool canGoForward();
    /**
       @brief Moves backward in transition history.
       @retval "!= false" Succeeded
       @retval "== false" Failed
       @attention
       After calling this function, functions that perform history transitions must not be called until FrameLoaderClient::saveViewStateToItem() is called.
    */
    bool goBack();
    /**
       @brief Moves forward or backward in transition history
       @param steps Movement amount @n
       - > 0 Forward movement amount @n
       - < 0 Backward movement amount @n
       - = 0 No movement or reloading occurred @n
       @return None
       @details
       Before calling this function, transitions must be performed after confirming that the target history item exists, using WKC::WKCWebView::canGoBackOrForward().
       @attention
       After calling this function, functions that perform history transitions must not be called until FrameLoaderClient::saveViewStateToItem() is called.
    */
    void goBackOrForward(int steps);
    /**
       @brief Moves forward in transition history
       @retval "!= false" Succeeded
       @retval "== false" Failed
       @attention
       After calling this function, functions that perform history transitions must not be called until FrameLoaderClient::saveViewStateToItem() is called.
    */
    bool goForward();

    /**
       @brief Stops loading page
       @return None
    */
    void stopLoading();
    /**
       @brief Reloads page
       @return None
    */
    void reload();
    /**
       @brief Reloads page from network
       @return None
    */
    void reloadBypassCache();
    /**
       @brief Gets page
       @param uri Pointer to URL string
       @param referrer Referer
       @retval "!= false" Succeeded
       @retval "== false" Failed
    */
    bool loadURI(const char* uri, const char* referrer = 0);
    /**
       @brief Gets page
       @param content Pointer to content string
       @param mimetype Pointer to mime type string. If 0, it is handled as text/html.
       @param encoding Pointer to encoding type string. If 0, it is handled as UTF-8.
       @param base_uri Pointer to base URI string
       @return None
       @details
       Currently, the only data that can be used as content is text.
    */
    void loadString(const char* content, const unsigned short* mimetype, const unsigned short* encoding, const char* base_uri);
    /**
       @brief Gets page
       @param content Pointer to content string (UTF-8)
       @param base_uri Pointer to base URI string
       @return None
       @details
       Displays page with content string as "text/html" content.
    */
    void loadHTMLString(const char* content, const char* base_uri);

    /**
       @brief Searches text
       @param text Pointer to search string
       @param case_sensitive Case sensitivity @n
       - != false Case sensitive @n
       - == false Not case sensitive @n
       @param forward Search direction @n
       - != false Forward @n
       - == false Backward @n
       @param wrap Consider word wrapping @n
       - != false Consider word wrapping @n
       - == false Do not consider word wrapping @n
       @retval "!= false" Search results found
       @retval "== false" No search results
    */
    bool searchText(const unsigned short* text, bool case_sensitive, bool forward, bool wrap);
    /**
       @brief Finds multiple text matches
       @param string Pointer to search string
       @param case_sensitive Case sensitivity @n
       - != false Case sensitive @n
       - == false Not case sensitive @n
       @param limit Upper limit for search results @n
       - 0 Unlimited @n
       - number Upper limit @n
       @retval "unsigned int" Number of individual text matches
       @details
       Not supported.
    */
    unsigned int markTextMatches(const unsigned short* string, bool case_sensitive, unsigned int limit);
    /**
       @brief Sets search string highlight display
       @param highlight
       - != false Highlight
       - == false Do not highlight
       @return None
    */
    void setHighlightTextMatches(bool highlight);
    /**
       @brief Clears search string
       @return None
       @details
       Not supported.
    */
    void unmarkTextMatches();

    /**
       @brief Gets main frame
       @retval WKCWebFrame* Pointer to web frame of main frame
    */
    WKCWebFrame* mainFrame();
    /**
       @brief Gets frame that has focus
       @return WKCWebFrame* Pointer to web frame of frame that has focus
    */
    WKCWebFrame* focusedFrame();

    /**
       @brief Executes user JavaScript
       @param script Pointer to JavaScript string (UTF-8)
       @return None
    */
    void executeScript(const char* script);

    /**
       @brief Confirms whether range is selected
       @retval "!= false" Range is selected
       @retval fase No range is selected
    */
    bool hasSelection();
    /**
       @brief Clears selected range
       @return None
    */
    void clearSelection();
    /**
       @brief Gets BoundingBox in selected range
       @param textonly Type of elements to select @n
       - != false Only elements that contain text
       - == false All elements
       @param useSelectionHeight Method of specifying height (only valid when textonly is !=false) @n
       - != false Use height of selected area
       - == false Use height of text
       @return WKCRect object to BoundingBox
    */
    WKCRect selectionBoundingBox(bool textonly, bool useSelectionHeight);
    /**
       @brief Gets text in selected range
       @return Pointer to text string (NULL terminated, with linefeed codes)
    */
    const unsigned short* selectionText();
    /**
       @brief Selects all
       @return None
    */
    void selectAll();
    /**
       @brief Gets whether the caret is at the beginning of the text field
       @retval "!= false" Caret is at the beginning of the text field
       @retval "== false" Caret is not at the beginning of the text field
       @details
       Gets whether the caret is at the beginning of the text field.
       The text field may be input, textarea, or contenteditable.
    */
    bool isCaretAtBeginningOfTextField() const;

    /**
       @brief Checks if mime type can be displayed
       @param mime_type Pointer to mime type string
       @retval "!= false" Can be displayed
       @retval "== false" Cannot be displayed
    */
    bool canShowMimeType(const unsigned short* mime_type);

    /**
       @brief Gets zoom ratio
       @retval float Zoom ratio
    */
    float zoomLevel();
    /**
       @brief Zooms content in/out
       @param zoom_level Zoom ratio @n
       - < 1.0 Zoom out display
       - == 1.0 Original size
       - > 1.0 Zoom in display
       @return float Zoom ratio that was set
    */
    float setZoomLevel(float zoom_level);
    /**
       @brief Zooms content in
       @param ratio Zoom ratio
       @return None
       @details
       Adds 'ratio' ratio to WKC::WKCWebView::zoomLevel() ratio and zooms in the display.
    */
    void zoomIn(float ratio);
    /**
       @brief Zooms content out
       @param ratio Zoom ratio
       @return None
       @details
       Subtracts 'ratio' ratio from WKC::WKCWebView::zoomLevel() ratio and zooms out the display.
    */
    void zoomOut(float ratio);
    /**
       @brief Gets text only zoom ratio
       @retval float Zoom ratio
    */
    float textOnlyZoomLevel();
    /**
       @brief Zooms texts in/out
       @param zoom_level Zoom ratio @n
       - < 1.0 Zoom out display
       - == 1.0 Original size
       - > 1.0 Zoom in display
       @return float Zoom ratio that was set
    */
    float setTextOnlyZoomLevel(float zoom_level);
    /**
       @brief Zooms texts in
       @param ratio Zoom ratio
       @return None
       @details
       Adds 'ratio' ratio to WKC::WKCWebView::textOnlyZoomLevel() ratio and zooms in the display.
    */
    void textOnlyZoomIn(float ratio);
    /**
       @brief Zooms texts out
       @param ratio Zoom ratio
       @return None
       @details
       Subtracts 'ratio' ratio from WKC::WKCWebView::textOnlyZoomLevel() ratio and zooms out the display.
    */
    void textOnlyZoomOut(float ratio);
    /**
       @brief Checks full content zoom in/out setting
       @retval "!= false" Zoom full content in/out
       @retval "== false" Zoom text in/out
    */
    bool fullContentZoom();
    /**
       @brief Zoom method setting
       @param full_content_zoom
       - != false Full zoom in
       - == false Zoom in text only
       @return None
    */
    void setFullContentZoom(bool full_content_zoom);

    /**
       @brief Gets optical zoom ratio
       @return Optical zoom ratio
       @details 
       @ref bbb-opticalzoom 
    */
    float opticalZoomLevel() const;
    /**
       @brief Gets optical zoom offset value
       @retval Optical zoom offset value
       @details
       @ref bbb-opticalzoom 
    */
    const WKCFloatPoint& opticalZoomOffset() const;
    /**
       @brief Sets optical zoom ratio
       @param zoom_level Zoom ratio
       @param offset Optical zoom offset value
       @retval Optical zoom ratio in which offset value is calculated
       @details
       @ref bbb-opticalzoom 
    */
    float setOpticalZoom(float zoom_level, const WKCFloatPoint& offset);

    /**
       @brief Gets view size
       @param size Reference to view size
       @return None
       @details
       Gets view size.
    */
    void viewSize(WKCSize& size) const;

    /**
       @brief Gets character encoding
       @retval "const unsigned short*" Pointer to character code string
    */
    const unsigned short* encoding();
    /**
       @brief Sets custom character encoding
       @param encoding Pointer to character code string
       @retval None
    */
    void setCustomEncoding(const unsigned short* encoding);
    /**
       @brief Gets custom character encoding
       @retval "const unsigned short*" Pointer to character code string
    */
    const unsigned short* customEncoding();

    /**
       @brief Gets loading status
       @retval WKC::LoadStatus Content loading status
    */
    WKC::LoadStatus loadStatus();
    /**
       @brief Gets loading progress status
       @return (TBD) implement description 
    */
    double progress();

    /**
       @brief Do hit-test for node
       @param node Target node
       @param result Reference to WKC::HitTestResult
       @retval "!=false" Succeeded
       @retval "==false" Failed
       @details
       Do hit-test for node.
    */
    bool hitTestResultForNode(const WKC::Node* node, WKC::HitTestResult& result);

    /**
       @brief Enter compositig mode
       @details
       Enter compositig mode.
    */
    void enterCompositingMode();

    // caches
    /**
       @brief Gets current cached size
       @param dead_resource Cache resource size of past pages
       @param live_resource Cache resource size of current page
       @details
       Gets size of cached data held by the browser engine.
       The size of cached data for past pages is set for dead_resource, and the size of cached data for the current page is set for live_resource.
    */
    static void cachedSize(unsigned int& dead_resource, unsigned int& live_resource);
    /**
       @brief Release memory
       @param type Memory type to be released
       @param is_synchronous
       - != false Release memory GC immediately
       - == false Just request memory release
       @return None
       @details
       release memory.
    */
    static void releaseMemory(WKC::ReleaseMemoryType type, bool is_synchronous);
    /**
        @brief Count number of font
        @param None
        @return Number of font
        @details
        Count number of font.
    */
    static size_t fontCount();
    /**
        @brief Count number of inactive font
        @param None
        @return Number of inactive font
        @details
        Count number of inactive font.
    */
    static size_t inactiveFontCount();
    /**
       @brief Clears font cache
       @param clearsAll
       - != false clear all cache
       - == false do not clear all cache
       @return None
       @details
       Clears font cache.
    */
    static void clearFontCache(bool clearsAll);
    /**
       @brief Clears emoji cache
       @return None
       @details
       Clears emoji cache.
    */
    static void clearEmojiCache(void);
    /**
       @brief Sets number of pages to cache
       @param capacity  Number of pages to cache
       @return None
       @details
       Sets the number of pages to cache.
       To use page cache, set the argument in WKCSettings::setUsesPageCache() to true.
    */
    static void setPageCacheCapacity(int capacity);

    static unsigned int getCachedPageCount();

    // plugins
    /**
       @brief Sets setting location path of plug-in module
       @param in_folder  Setting location path of plug-in module
       @return None
       @details
       Sets the setting location path of plug-in module
       When plug-ins are enabled, the plug-in modules in the location specified by this API are loaded and used.
    */
    static void setPluginsFolder(const char* in_folder);

    /**
       @brief Gets pointer to WKC::Element object of element that has focus
       @return Pointer to WKC::Element object
       @details
       This function can get various information about an element, from the WKC::Element object.@n
       - Element name WKC::Element::elementName()@n
       Equivalent to the element name in W3C.
       - Attributes WKC::Element::attributes()@n
       Equivalent to attributes name in W3C.
       - Coordinates and size WKC::Element::renderer()->absoluteBoundingBoxRect()
       @attention
       If an event, etc. occurs during the period after WKCWebView::getFocusedElement is called and before information is obtained from WKC::Element, the WKC::Element information may become unusable.@n
       In this case, the information must be obtained from WKC::Node after calling WKCWebView::getFocusedElement again.
    */
    WKC::Element* getFocusedElement();
    /**
       @brief Gets pointer to WKC::Element object of element below the pointer using specified coordinates
       @param x X coordinate
       @param y Y coordinate
       @return Pointer to WKC::Element object
       @attention
       Every coordinate value must be specified using the same coordinate system as WKC::WKCWebView::notifyMouseDown (Move, Up).
       @attention
       If an event, etc. occurs during the period after WKCWebView::getElementFromPoint is called and before information is obtained from WKC::Element, the WKC::Element information may become unusable.@n
       In this case, the information must be obtained from WKC::Element after calling WKCWebView::getElementFromPoint again.
    */
    WKC::Element* getElementFromPoint(int x, int y);
    /**
       @brief Gets information about whether element below pointer is clickable or not using specified coordinates
       @param x X coordinate
       @param y Y coordinate
       @retval "!= false" Clickable
       @retval "== false" Not clickable
       @attention
       Every coordinate value must be specified using the same coordinate system as WKC::WKCWebView::notifyMouseDown (Move, Up).
    */
    bool clickableFromPoint(int x, int y);
    /**
       @brief Gets information about whether specified element is clickable or not
       @param element Pointer to element
       @retval "!= false" Clickable
       @retval "== false" Not clickable
    */
    bool isClickableElement(WKC::Element* element);
    /**
       @brief Gets information about whether element below pointer is draggable or not (whether there is an ondrag, dragstart, or dragend event listener) using specified coordinates
       @param x X coordinate
       @param y Y coordinate
       @retval "!= false" Draggable
       @retval "== false" Not draggable
       @attention
       Every coordinate value must be specified using the same coordinate system as WKC::WKCWebView::notifyMouseDown (Move, Up).
    */
    bool draggableFromPoint(int x, int y);

    /** @brief Scroll bar part types */
    enum ScrollbarPart {
        /** @brief Default value */
        NoPart,
        /** @brief Back button part */
        BackButtonPart,
        /** @brief Forward button part */
        ForwardButtonPart,
        /** @brief Part between Back button and Thumb button */
        BackTrackPart,
        /** @brief Thumb button part */
        ThumbPart,
        /** @brief Part between Forward button and Thumb button */
        ForwardTrackPart,
        /** @brief Entire scroll bar part */
        ScrollbarBGPart,
        /** @brief BackTrackPart+ThumbPart+ForwardTrackPart part */
        TrackBGPart,
        /** @brief Back button start part */
        BackButtonStartPart,
        /** @brief Back button end part */
        BackButtonEndPart,
        /** @brief Forward button start part */
        ForwardButtonStartPart,
        /** @brief Forward button end part */
        ForwardButtonEndPart,
    };
    /**
       @brief Determines whether there is a scroll bar for engine to draw below pointer using specified coordinates
       @param x X coordinate
       @param y Y coordinate
       @param part Part type scroll bar indicated by pointer ( WKC::WKCWebView::ScrollbarPart )
       @param rect Rectangle of part
       @retval "!= false" Scroll bar exists
       @retval "== false" No scroll bar
       @attention
       Every coordinate value must be specified using the same coordinate system as WKC::WKCWebView::notifyMouseDown (Move, Up).
    */
    bool isScrollbarFromPoint(int x, int y, ScrollbarPart& part, WKCRect& rect);

    // cookie setting to WebCore
    /**
       @brief Sets navigator.cookieEnabled to be enabled/disabled
       @param flag Enable/Disable navigator.cookieEnabled
       @retval None
       @details
       Sets the options that affect navigator.cookieEnabled only. Notification must be given separately for enabling/disabling actual cookies.
    */
    void setCookieEnabled(bool flag);
    /**
       @brief Gets current navigator.cookieEnabled value
       @retval "!= false" Enable navigator.cookieEnabled
       @retval "== false" Disable navigator.cookieEnabled
    */
    bool cookieEnabled();

    // permit to send request
    /**
       @brief Permits communications for void pointer of communication target
       @param handle void pointer of communication target
       @param permit Permission flag
       @return None
       @details
       Gives notification of determining whether to permit for sending that is requested by WKC::FrameLoaderClientIf::dispatchWillPermitSendRequest().
    */
    static void permitSendRequest(void *handle, bool permit);

    // History
    /**
       @brief Registers visited link information in browser engine
       @param uri URI of visited link
       @param title Title string of visited link (not supported)
       @param date Date visited (not supported)
       @retval "!= false" Succeeded in registering
       @retval "== false" Failed to register
       @details
       Registers visited link information in the browser engine, and uses it for visit history information, etc.@n
       The History-related APIs of the WKC::WKCWebView class must be used for operations relating to visit history.@n
       @attention
       Either ASCII-only or UTF-8 C strings must be used for uri.@n
       UTF-16 strings must be used for title.
    */
    bool addVisitedLink(const char* uri, const unsigned short* title, const struct tm* date);
    /**
       @brief Adds visited link hash
       @param hash Hash of visited link
       @retval "!=false" Succeeded
       @retval "==false" Failed
       @details
       Adds visited link hash.
    */
    bool addVisitedLinkHash(SharedStringHash hash);

    /**
       @brief Gets pointer to view-state data about specified item in browser engine transition history list
       @param index Index value of item to get (0 to WKC::WKCWebView::getHistoryLength()-1)
       @return Pointer to view-state data created by application.
    */
    void* getHistoryItemViewStateByIndex(unsigned int index);

    /**
       @brief Sets pointer to view-state data about specified item in browser engine transition history list
       @param index Index value of item to get (0 to WKC::WKCWebView::getHistoryLength()-1)
       @param view_state pinter to view-state data which is created by application
       @return None
    */
    void setHistoryItemViewStateByIndex(unsigned int index, void* view_state);

    // images
    enum {
        EInternalColorFormat8888,
        EInternalColorFormat8888or565,
        EInternalColorFormats
    };
    /**
       @brief Sets color format inside browser engine
       @param fmt Format
       @details
       For fmt, specify WKC::WKCWebView::EInternalColorFormat8888 or WKC::WKCWebView::EInternalColorFormat8888or565.
    */
    static void setInternalColorFormat(int fmt);
    /**
       @brief Sets bilinear complement while zooming in/out images to be enabled/disabled
       @param flag true: enabled, false: disabled
    */
    void setUseBilinearForScaledImages(bool flag);
    /**
       @brief Sets anti-aliasing while drawing graphics to be enabled/disabled
       @param flag true: enabled, false: disabled_
    */
    void setUseAntiAliasForDrawings(bool flag);
    /**
       @brief Sets bilinear complement while drawing canvas to be enabled/disabled 
       @param flag true: enabled, false: disabled_
    */
    static void setUseBilinearForCanvasImages(bool flag);
    /**
       @brief Sets anti-aliasing while drawing canvas to be enabled/disabled
       @param flag true: enabled, false: disabled_
    */
    static void setUseAntiAliasForCanvas(bool flag);

    /**
       @brief Sets scroll position to correct the Offscreen drawing position
       @param scrollPosition Scroll position
       @retval None
       @details
       Sets the value to use when utilizing Scroll position in the correction process when drawing Offscreen.@n
       scrollPosition assumes that the value returned by WKC::WKCWebView::scrollPosition() has been passed.@n
       It can only be called immediately before calling WKC::WKCWebView::notifyPaintOffscreen().@n
       Only works with the WKCOffscreen5650Tiny version. It must not be called for formats other than WKCOffscreen5650Tiny.
    */
    void setScrollPositionForOffscreen(const WKCPoint& scrollPosition);

    // extra draw
    static void setClipsRepaints(bool enable) { m_clipsRepaints = enable; }
    static bool clipsRepaints() { return m_clipsRepaints; }

    // scroll node
    static void scrollNodeByRecursively(WKC::Node* node, int dx, int dy);
    static void scrollNodeBy(WKC::Node* node, int dx, int dy);
    static bool isScrollableNode(const WKC::Node* node);
    static bool canScrollNodeInDirection(const WKC::Node* node, WKC::FocusDirection direction);
    // Gets the scrollable area inside the scroll bars
    static WKCRect getScrollableContentRect(const WKC::Node* scrollableNode);

    static WKCRect transformedRect(const WKC::Node* node, const WKCRect& rect);

    /**
       @brief Checks whether a point is inside a node.
       @param node Target node
       @param point Target point (absolute coordinates)
       @retval "!= false" Inside a node
       @retval "== false" Outside a node
       @details
       Checks whether a point is inside a node, taking transforms into account.
    */
    static bool containsPoint(const WKC::Node* node, const WKCPoint& point);

    // session storage and local storage
    unsigned sessionStorageMemoryConsumptionBytes();
    static unsigned localStorageMemoryConsumptionBytes(const char* pagegroupname);
    void clearSessionStorage();
    static void clearLocalStorage(const char* pagegroupname);

    /**
       @brief Sets page visibility
       @param isVisible Page visibility
       @retval None
       @details
       Sets page visibility.
    */
    void setIsVisible(bool isVisible);

    // webInspector
    /**
       @brief Enables / disables WebInspector
       @param enable flags for enable / disable WebInspector
    */
    void enableWebInspector(bool enable);
    /**
       @brief Returns whether WebInspector is enabled or not
       @retval "!= false" Enabled
       @retval "== false" Disabled
    */
    bool isWebInspectorEnabled();

    /**
       @brief Returns whether content is editable or not
       @retval "!= false" Enabled
       @retval "== false" Disabled
    */
    bool editable();
    /**
       @brief Enables / disables content editable
       @param enable flags for enable / disable content editable
    */
    void setEditable(bool enable);

    /**
       @brief Cancel fullscreen mode.
       @retval None
       @details
       Cancel fullscreen mode. Do nothing when the document is not in fullscreen mode.
     */
    void cancelFullScreen();

    // spatial navigation
    /**
       @brief Enables / disables spatial navigation
       @param enable flags for enable / disable spatial navigation
    */
    void setSpatialNavigationEnabled(bool enable);

    // recalc stylesheet
    /**
        @brief Recalc stylesheet for document.
        @details
        Recalc stylesheet if needed.
    */
    void recalcStyleSheet();

    /**
       @brief Allows / disallows layout updates
    */
    void allowLayout();
    void disallowLayout();

    // for debug
    /**
        @brief Dump render tree of a frame.
    */
    void dumpExternalRepresentation(Frame* frame);
    /**
        @brief Get the current focus point in content coordinate system.
    */
    void getCurrentFocusPoint(int& x, int& y);
    /**
        @brief Clear current focus point.
    */
    void clearCurrentFocusPoint();

    /**
       @brief Enables / disables auto play
       @param enable flags for enable / disable auto play
    */
    void enableAutoPlay(bool enable);
    /**
       @brief Returns whether auto play is enabled or not
       @retval "!= false" Enabled
       @retval "== false" Disabled
    */
    bool isAutoPlayEnabled();

    /**
       @brief Enables / disables dark appearance
       @param useDarkAppearance flags for enable / disable dark appearance
    */
    void setUseDarkAppearance(bool useDarkAppearance);
    /**
       @brief Returns whether dark appearance is enabled or not
       @retval "!= false" Enabled
       @retval "== false" Disabled
    */
    bool useDarkAppearance() const;

    // EPUB class
    class WKC_API EPUB {
    public:
        static bool initialize();
        static void finalize();
        static void forceTerminate();
    public:
        bool openEPUB(const char* path_to_epub, bool parseTOC=true);
        void closeEPUB();
        bool loadSpineItem(unsigned int index);
        const WKCEPUBData& metaData() const { return m_EPUBData; }

        static const WKC::FileSystemProcs* EPUBFileProcsWrapper(const WKC::FileSystemProcs* procs);

    private:
        friend class WKCWebView;
        EPUB(WKCWebView* parent);
        ~EPUB();

        bool parseMetaInfo(bool parseTOC);

    private:
        WKCWebView* m_parent;
        char* m_path;
        WKCEPUBData m_EPUBData;
        void* m_zip;
    };

    inline EPUB* epub() const { return m_EPUB; }

    bool addUserContentExtension(const char* name, const char* extension);
    bool compileUserContentExtension(const char* extension, unsigned char** out_filtersWithoutDomainsBytecode, unsigned int* out_filtersWithoutDomainsBytecodeLen, unsigned char** out_filtersWithDomainsBytecode, unsigned int* out_filtersWithDomainsBytecodeLen, unsigned char** out_domainFiltersBytecode, unsigned int* out_domainFiltersBytecodeLen, unsigned char** out_actions, unsigned int* out_actionsLen);
    bool addCompiledUserContentExtension(const char* name, const unsigned char* filtersWithoutDomainsBytecode, unsigned int filtersWithoutDomainsBytecodeLen, const unsigned char* filtersWithDomainsBytecode, unsigned int filtersWithDomainsBytecodeLen, const unsigned char* domainFiltersBytecode, unsigned int domainFiltersBytecodeLen, const unsigned char* actions, unsigned int actionsLen);

private:
    WKCWebView();
    ~WKCWebView();
    bool construct(WKCClientBuilders& builders);

private:
    WKCWebViewPrivate* m_private;
    EPUB* m_EPUB;
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(bool, m_clipsRepaints);
};

/*@{*/
/** @brief Namespace of API layer of NetFront Browser NX WKC IDN @n */
namespace IDN {
/**
@brief Decodes IDN encoded hostname to unicode
@param idn IDN encoded hostname
@param host Buffer for storing decoded hostname
@param maxhost Max length of host
@retval Length of decoded hostname.
@details
Decodes IDN encoded hostname to unicode
*/
WKC_API int fromUnicode(const unsigned short* idn, char* host, int maxhost);
/**
@brief Encodes unicode hostname to IDN
@param host Unicode hostname
@param idn buffer for storing IDN encoded hostname
@param maxidn Max length of idn
@retval Length of encoded hostname
@details
Encodes unicode hostname to IDN
*/
WKC_API int toUnicode(const char* host, unsigned short* idn, int maxidn);
} // namespace IDN
/*@}*/

/*@{*/
/** @brief Namespace of API layer of NetFront Browser NX WKC NetUtil @n */
namespace NetUtil {
/**
@brief Determines whether IP address format is correct
@param in_ipaddress Pointer to buffer that stores IP address string
@retval 0 Other than IP address
@retval 4 IPv4 Address
@retval 6 IPv6 Address
@details
Examines a string and checks whether it complies with IP address format.@n
@attention
An IPv6 address must be enclosed between "[" and "]".
*/
WKC_API int correctIPAddress(const char *in_ipaddress);
/*@}*/
} // namespace NetUtil

/*@{*/
/** @brief Namespace of API layer of NetFront Browser NX WKC Base64 @n */
namespace Base64 {
    /**
    @brief Encodes string to Base64-encode string
    @param in Pointer to string that stores original string
    @param buf Pointer to buffer for storing encoded string
    @param buflen Max length of buf
    @retval Length of encoded string
    @details
    if buf is set null, just return the size of encoded string. @n
    */
    WKC_API int base64Encode(const char* in, char* buf, int buflen);
    /*@}*/
} // namespace Base64

/*@}*/

} // namespace

#ifndef WKC_WEB_VIEW_COMPILE_ASSERT
#define WKC_WEB_VIEW_COMPILE_ASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1]
#endif

#endif  // WKCWebView_h
