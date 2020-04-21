/*
 *  WKCClientBuilders.h
 *
 *  Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCClientBuilders_h
#define WKCClientBuilders_h

#include <wkc/wkcbase.h>
#include <helpers/WKCHelpersEnums.h>

// class definition

namespace WKC {

class ChromeClientIf;
class ContextMenuClientIf;
class DragClientIf;
class EditorClientIf;
class FrameLoaderClientIf;
class InspectorClientIf;
class DropDownListClientIf;
class GeolocationClientIf;
class DeviceMotionClientIf;
class DeviceOrientationClientIf;
class BackForwardClientIf;
class BatteryClientIf;
class NavigatorContentUtilsClientIf;
class ProgressTrackerClientIf;
class WebNfcClientIf;
class StorageAreaIf;
class VibrationClientIf;

class WKCWebView;
class WKCWebFrame;
class String;

/*@{*/
/**
@class WKC::WKCClientBuilders 
@brief Class that holds client
*/
class WKC_API WKCClientBuilders
{
public:
    /**
       @brief Requests to generate chrome client
       @param view Pointer to WebView
       @retval WKC::ChromeClientIf* Pointer to chrome client
       @details
       (TBD) implement description 
    */
    virtual WKC::ChromeClientIf* createChromeClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard chrome client
       @param client Pointer to chrome client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteChromeClient(WKC::ChromeClientIf* client) = 0;

    /**
       @brief Requests to generate context menu client
       @param view Pointer to WebView
       @retval WKC::ContextMenuClientIf* Pointer to context menu client
       @details
       (TBD) implement description 
    */
    virtual WKC::ContextMenuClientIf* createContextMenuClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard context menu client
       @param client Pointer to context menu client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteContextMenuClient(WKC::ContextMenuClientIf* client) = 0;

    /**
       @brief Requests to generate drag client
       @param view Pointer to WebView
       @retval WKC::DragClientIf* Pointer to drag client
       @details
       (TBD) implement description 
    */
    virtual WKC::DragClientIf* createDragClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard drag client
       @param client Pointer to drag client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteDragClient(WKC::DragClientIf* client) = 0;

    /**
       @brief Requests to generate editor client
       @param view Pointer to WebView
       @retval WKC::EditorClient Pointer to editor client
       @details
       (TBD) implement description 
    */
    virtual WKC::EditorClientIf* createEditorClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard editor client
       @param client Pointer to editor client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteEditorClient(WKC::EditorClientIf* client) = 0;

    /**
       @brief Requests to generate frame loader client
       @param frame Pointer to web frame
       @retval WKC::FrameLoaderClientIf* Pointer to frame loader client
       @details
       (TBD) implement description 
    */
    virtual WKC::FrameLoaderClientIf* createFrameLoaderClient(WKCWebFrame* frame) = 0;
    /**
       @brief Request to discard frame loader client
       @param client Pointer to frame loader client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteFrameLoaderClient(WKC::FrameLoaderClientIf* client) = 0;

    /**
       @brief Requests to generate inspector client
       @param view Pointer to WebView
       @retval WKC::InspectorClientIf* Pointer to inspector client
       @details
       (TBD) implement description 
    */
    virtual WKC::InspectorClientIf* createInspectorClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard inspector client
       @param client Pointer to inspector client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteInspectorClient(WKC::InspectorClientIf* client) = 0;

    /**
       @brief Requests to generate drop-down list client
       @param view Pointer to WebView
       @retval "WKC::DropDownListClientIf*" Pointer to drop-down list client
       @details
       (TBD) implement description 
    */
    virtual WKC::DropDownListClientIf* createDropDownListClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard drop-down list client
       @param client Pointer to drop-down list client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteDropDownListClient(WKC::DropDownListClientIf* client) = 0;

    /**
       @brief Requests to generate geolocation client
       @param view Pointer to WebView
       @retval "WKC::GeolocationClientIf*" Pointer to geolocation client
       @details
       (TBD) implement description 
    */
    virtual WKC::GeolocationClientIf* createGeolocationClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard geolocation client
       @param client Pointer to geolocation client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteGeolocationClient(WKC::GeolocationClientIf* client) = 0;

    /**
       @brief Requests to generate device motion client
       @param view Pointer to WebView
       @retval "WKC::GeolocationClientIf*" Pointer to device motion client
       @details
       (TBD) implement description 
    */
    virtual WKC::DeviceMotionClientIf* createDeviceMotionClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard device motion client
       @param client Pointer to device motion client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteDeviceMotionClient(WKC::DeviceMotionClientIf* client) = 0;
    /**
       @brief Notifies to device motion client is deleted
       @param client Pointer to device orientation client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void notifyDeviceMotionClientDestroyed(WKC::DeviceMotionClientIf* client) = 0;

    /**
       @brief Requests to generate device orientation client
       @param view Pointer to WebView
       @retval "WKC::GeolocationClientIf*" Pointer to device orientation client
       @details
       (TBD) implement description 
    */
    virtual WKC::DeviceOrientationClientIf* createDeviceOrientationClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard device orientation client
       @param client Pointer to device orientation client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteDeviceOrientationClient(WKC::DeviceOrientationClientIf* client) = 0;
    /**
       @brief Notifies to device orientation client is deleted
       @param client Pointer to device orientation client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void notifyDeviceOrientationClientDestroyed(WKC::DeviceOrientationClientIf* client) = 0;

    /**
    @brief Requests to generate back-forward client
    @param view Pointer to WebView
    @retval "WKC::BackForwardClientIf*" Pointer to back-forward client
    @details
    (TBD) implement description
    */
    virtual WKC::BackForwardClientIf* createBackForwardClient(WKCWebView* view) = 0;
    /**
    @brief Requests to discard back-forward client
    @param client Pointer to back-forward client
    @return None
    @details
    (TBD) implement description
    */
    virtual void deleteBackForwardClient(WKC::BackForwardClientIf* client) = 0;
    
    /**
       @brief Requests to generate battery client
       @param view Pointer to WebView
       @retval "WKC::BatteryClientIf*" Pointer to battery client
       @details
       (TBD) implement description 
    */
    virtual WKC::BatteryClientIf* createBatteryClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard battery client
       @param client Pointer to battery client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteBatteryClient(WKC::BatteryClientIf* client) = 0;

    /**
       @brief Requests to generate navigator content utils client
       @param view Pointer to WebView
       @retval "WKC::BatteryClientIf*" Pointer to navigator content utils client
       @details
       (TBD) implement description 
    */
    virtual WKC::NavigatorContentUtilsClientIf* createNavigatorContentUtilsClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard navigator content utils client
       @param client Pointer to navigator content utils client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteNavigatorContentUtilsClient(NavigatorContentUtilsClientIf* client) = 0;

    /**
       @brief Requests to generate progress tracker client
       @param view Pointer to WebView
       @retval "WKC::BatteryClientIf*" Pointer to progress tracker client
       @details
       (TBD) implement description 
    */
    virtual WKC::ProgressTrackerClientIf* createProgressTrackerClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard progress tracker client
       @param client Pointer to progress tracker client
       @return None
       @details
       (TBD) implement description 
    */
    virtual void deleteProgressTrackerClient(WKC::ProgressTrackerClientIf* client) = 0;

    /**
       @brief Request to generate web nfc client
       @param view Pointer to WebView
       @retval "WKC::WebNfcClientIf*" Pointer to web nfc client
       @details
       (TBD) implement description
    */
    virtual WKC::WebNfcClientIf* createWebNfcClient(WKCWebView* view) = 0;
    /**
       @brief Requests to discard web nfc client
       @param client Pointer to web nfc client
       @return None
       @details
       (TBD) implement description
    */
    virtual void deleteWebNfcClient(WKC::WebNfcClientIf* client) = 0;

    /**
    @brief Request to generate Local / Session Storage client
    @param view Pointer to WebView
    @param origin String of security origin identifier
    @type type of storage
    @param quota Quota
    @retval "WKC::StorageAreaIf*" Pointer to Local / Session Storage client
    @details
    (TBD) implement description
    */
    virtual WKC::StorageAreaIf* createStorageAreaClient(WKCWebView* view, const WKC::String& origin, WKC::StorageType type, unsigned int quota) = 0;

    /**
    @brief Requests to discard Local / Session Storage client
    @param client Pointer to Local / Session Storage client
    @return None
    @details
    (TBD) implement description
    */
    virtual void deleteStorageAreaClient(WKC::StorageAreaIf* client) = 0;

    /**
    @brief Request to generate Vibration client
    @param view Pointer to WebView
    @retval "WKC::VibrationClientIf*" Pointer to Vibration client
    @details
    (TBD) implement description
    */
    virtual WKC::VibrationClientIf* createVibrationClient(WKCWebView* view) = 0;

    /**
    @brief Requests to discard Vibration client
    @param client Pointer to Vibration client
    @return None
    @details
    (TBD) implement description
    */
    virtual void deleteVibrationClient(WKC::VibrationClientIf* client) = 0;
};

/*@}*/
} // namespace

#endif // WKCClientBuilders_h
