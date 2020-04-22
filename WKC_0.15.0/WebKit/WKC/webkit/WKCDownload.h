/*
 * WKCDownload.h
 *
 * Copyright (c) 2010-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCDownload_h
#define WKCDownload_h

// class definition

namespace WKC {

class WKCDownload;

class WKCWebView;
class WKCDownloadPrivate;

class ResourceHandle;
class ResourceRequest;
class ResourceResponse;

/*@{*/
/**
@class WKC::WKCDownloadClient
@brief Class for download process that needs to be implemented on application side
*/
class WKC_API WKCDownloadClient {
public:
    /**
       @brief Notifies of received data
       @param self Pointer to WKCDownload
       @param data Pointer to received data
       @param len Received data length (bytes)
       @param sum Total length of current received data (bytes)
       @return None
    */
    virtual void didReceiveData(WKCDownload* self, const char* data, long long len, long long sum) = 0;
    /**
       @brief Notifies of download finished
       @param self Pointer to WKCDownload
       @return None
       @details
       Notification of the finish of download.
    */
    virtual void didFinishLoading(WKCDownload* self) = 0;
    /**
       @brief Notifies of download failure
       @param self Pointer to WKCDownload
       @param err enum value defined in WKC::WKCDownload::Error
       @return None
       @details
       Notification of the cause of download failure is given by the second argument.
    */
    virtual void didFail(WKCDownload* self, int err) = 0;
    /**
       @brief This is scheduled to be used for future extension
       @param self Pointer to WKCDownload
       @return None
       @details
       Calling of this API is scheduled to be implemented in the future. Implementing empty function is required for classes that inherit WKC::WKCDownloadClient.
    */
    virtual void wasBlocked(WKCDownload* self) = 0;
    /**
       @brief This is scheduled to be used for future extension
       @param self Pointer to WKCDownload
       @return None
       @details
       Calling of this API is scheduled to be implemented in the future. Implementing empty function is required for classes that inherit WKC::WKCDownloadClient.
    */
    virtual void cannotShowURL(WKCDownload* self) = 0;
    /**
       @brief Notifies of receiving data
       @param self Pointer to WKCDownload
       @param length Data length
       @retval "!= false" Succeeded
       @retval "== false" Failed
       @details
       Notification is given when data is received.@n
       Types of data include HTTP header, various raw resource data (HTML document, image, CSS, JS file), and Data:-specified data.@n
       In this notification processing, the reception of the corresponding data can be interrupted by calling WKC::WKCDownload::cancel().
    */
    virtual bool willReceiveData(WKCDownload* self, int length) = 0;
};
/*@}*/

/*@{*/
/**
@class WKC::WKCDownload;
@brief Class for download processing
*/
class WKC_API WKCDownload {
public:
    /**
       @brief Requests to generate WKCDownload
       @param view Pointer to WKCWebView
       @param client Reference to WKCDownloadClient
       @param request References to WKC::ResourceRequest
       @retval !=0 Pointer to WKCDownload
       @retval ==0 Failed to generate
    */
    static WKCDownload* create(WKCWebView* view, WKCDownloadClient& client, const WKC::ResourceRequest& request);
    /**
       @brief Requests to discard WKCDownload
       @param self Pointer to WKCDownload
       @return None
    */
    static void deleteWKCDownload(WKCDownload* self);

    /**
       @brief Requests to forcibly terminate download processing
       @return None
       @details
       Since this function is intended for restarting, call this when restarting the browser engine. For information about restarting, see @ref forceterminate.@n
       Unlike WKC::WKCDownload::cancel(), download processing will be interrupted only for WKC::WKCDownload::EStarted. Do not call WKC::WKCDownloadClient::didFail() in that case.
    */
    void notifyForceTerminate();

    /**
       @brief Requests to set reference to WKC::ResourceResponse object
       @param resourceHandle Pointer to WKC::ResourceHandle
       @param response References to WKC::ResourceResponse
       @retval "!= false" Succeeded
       @retval "== false" Failed
    */
    bool setResponse(WKC::ResourceHandle* resourceHandle, const WKC::ResourceResponse& response);
    /**
       @brief Requests to start download processing
       @retval "!= false" Succeeded
       @retval "== false" Failed
       @details
       Calling this API starts download processing. To check with the user whether to continue before starting download processing, perform the confirmation before calling this API.
    */
    bool start();
    /**
       @brief Notifies of canceling download processing
       @return None
       @details
       Stops downloading and calls WKC::WKCDownloadClient::didFail().
    */
    void cancel();
    /**
       @brief Gets URL string (C string) to be downloaded
       @retval !=0 Pointer to URL string
       @retval ==0 URL string does not exist
       @details
       Do not release the pointer returned by this API.@n
       The character code of the string returned by this API is UTF-8.
    */
    const char* getUri() const;
    /**
       @brief Gets name of file to save to
       @retval !=0 Pointer to string (C string) of name of file to save to
       @retval ==0 File name string does not exist
       @details
       Do not release the pointer returned by this API.@n
       Before WKC::WKCDownload::setResponse() is called, the file name obtained from the URL based on the extension is returned. If the file name is not included in the URL, string (C string) "download.dat" will be returned. After WKC::WKCDownload::setResponse() is called, if filename is specified in Content-Disposition of the HTTPHeader, the file name will be returned.@n
       This API returns a string in the UTF-8 character code.
    */
    const char* getSuggestedFilename() const;

    /**
       @brief Gets WKC::ResourceRequest object
       @return "const WKC::ResourceRequest&" Reference to ResourceRequest
       @details
       Do not release the pointer returned by this API.
    */
    const WKC::ResourceRequest& request() const;

    /**
       @brief Gets download state in percentage
       @return Integer value representing percentage
       @details
       The number is an integer value between 0 and 100, cutting off all the numbers after the decimal points.
    */
    int getProgressInPercent() const;
    /**
       @brief Gets time elapsed since downloading started
       @return Elapsed time since calling start()
       @details
       This function returns the elapsed time since calling start() in an integer value.@n
       If this API is called before calling WKC::WKCDownload::start(), the value will be invalid.@n
       The unit of the value returned by this API is milliseconds.
    */
    unsigned int getElapsedTimeInMilliSeconds() const;
    /**
       @brief Gets total length of file being downloaded
       @return Length of file to be downloaded (bytes)
       @details
       This API returns the value specified as the value of Content-Length in the HTTPHeader. Note that if the value of Content-Length in the HTTPHeader is invalid, or if the definition itself does not exist, then it returns the same value as the length of data already downloaded.
    */
    long long getTotalSize() const;
    /**
       @brief Gets length of data loaded from file being downloaded
       @return Length of data that has already been downloaded (bytes)
       @details
       Always return a value that is less than or equal to the value that can be obtained using WKC::WKCDownload::getTotalSize().
    */
    long long getCurrentSize() const;

    /**
       @brief WKCDownload state types
    */
    enum Status {
        /** @brief State when error occurs */
        EError = -1,
        /** @brief State right after WKCDownload is generated */
        ECreated = 0,
        /** @brief State when download processing starts */
        EStarted,
        /** @brief State when download processing is interrupted */
        ECancelled,
        /** @brief State when download processing is completed */
        EFinished,
        /** @brief State numbers */
        EStatus
    };
    /**
       @brief Gets WKCDownload state
       @return enum value defined as WKC::WKCDownload::Status
    */
    int getStatus() const;

    /**
       @brief Error code types
    */
    enum Error {
        /** @brief Error code when error is not detected */
        EErrorNone = 0,
        /** @brief Error code when download processing is interrupted */
        EErrorCancelled,
        /** @brief Error code when download processing failed due to network problem */
        EErrorNetwork,
        /** @brief Error numbers */
        EErrors
    };
    /**
       @brief Gets type of error that occurred during download
       @return enum value defined as WKC::WKCDownload::Error
    */
    int getError() const;

private:
    WKCDownload();
    ~WKCDownload();

    bool construct(WKCWebView*, WKCDownloadClient&, const ResourceRequest&);

private:
    WKCDownloadPrivate* m_private;
};
/*@}*/

} // namespace

#endif // WKCDownload_h
