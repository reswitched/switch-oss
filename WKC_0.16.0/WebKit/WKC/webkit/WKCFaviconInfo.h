/*
 *  WKCFaviconInfo.h
 *
 *  Copyright (c) 2010, 2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCFaviconInfo_h
#define WKCFaviconInfo_h

namespace WKC {
/*@{*/
    /**
       @brief Structure that holds favicon information
    */
    typedef struct WKCFaviconInfo_ {
        /**
           @brief Pointer to source data of Favicon file.@n
           Size of data must be obtained from WKC::WKCFaviconInfo::fIconSize.@n
           The application uses this information when saving the favicon file.
        */
        void* fIconData;
        /**
           @brief Pointer to bit values of bitmap from decoded favicon data.@n
           Size of data must be calculated from the following.@n
           WKC::WKCFaviconInfo::fIconBmpBpp * WKC::WKCFaviconInfo::fIconBmpWidth * WKC::WKCFaviconInfo::fIconBmpHeight @n
           The application uses this data when displaying the favicon.
        */
        void* fIconBmpData;
        /**
           @brief Pointer to bit values of bitmap mask from decoded favicon data.@n
           Size of data must be calculated from the following.@n
           WKC::WKCFaviconInfo::fIconBmpWidth * WKC::WKCFaviconInfo::fIconBmpHeight @n
           The application uses this data when displaying the favicon.@n
           And, when WKC::WKCFaviconInfo::fHasMask is false, there is no mask data.@n
           In that case, the application must not allocate the WKC::WKCFaviconInfo::fIconBmpMask memory range.
        */
        void* fIconBmpMask;
        /**
           @brief Flag that indicates whether there is favicon bitmap mask data.
        */
        bool fHasMask;
        /**
           @brief Size of source data of favicon file.
        */
        int fIconSize;
        /**
           @brief Number of bytes per pixel of favicon bitmap data.
        */
        int fIconBmpBpp;
        /**
           @brief Width of favicon bitmap.
        */
        int fIconBmpWidth;
        /**
           @brief Height of favicon bitmap.
        */
        int fIconBmpHeight;
    } WKCFaviconInfo;
    /**
       @typedef struct WKC::WKCFaviconInfo_ WKC::WKCFaviconInfo
       @brief Type definition of WKCFaviconInfo
    */
/*@}*/
} // namespace

#endif // WKCFaviconInfo_h
