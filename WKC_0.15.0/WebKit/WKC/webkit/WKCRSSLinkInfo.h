/*
 *  WKCRSSLinkInfo.h
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

#ifndef WKCRSSLinkInfo_h
#define WKCRSSLinkInfo_h

namespace WKC {

/*@{*/

    /** @brief Limit value for title/URL. If the limit value is exceeded, it is truncated at the limit value. */
    enum RSSLinkMaxLength {
        /** @brief Limit value for title length */
        ERSSTitleLenMax = 128,
        /** @brief Limit value for URL length */
        ERSSUrlLenMax = 1024
    };
    /** @brief RSS Link element status flag, notified by WKC::WKCRSSLinkInfo_::m_flag */
    enum RSSLinkFlag {
        /** @brief Initial value */
        ERSSLinkFlagNone = 0x0,
        /** @brief Title length exceeds limit value */
        ERSSLinkFlagTitleTruncate = 0x1,
        /** @brief URL length exceeds limit value */
        ERSSLinkFlagUrlTruncate = 0x2
    };
    /** @brief Structure that holds RSS information in each Link element */
    struct WKCRSSLinkInfo_ {
        /** @brief Status flag@n
            Status value is WKC::RSSLinkFlag.
        */
        unsigned int m_flag;
        /** @brief Title (NULL terminated)@n
            WKC::ERSSTitleLenMax Title length limit value
        */
        unsigned short m_title[ERSSTitleLenMax + 1];
        /**
           @brief URL (NULL terminated)@n
           WKC::ERSSUrlLenMax URL length limit value
        */
        char m_url[ERSSUrlLenMax + 1];
    };
    /** @brief Type definition of WKCRSSLinkInfo */
    typedef struct WKCRSSLinkInfo_ WKCRSSLinkInfo;

/*@}*/

} // namespace

#endif // WKCRSSLinkInfo_h
