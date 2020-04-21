/*
 * WKCRSSParser.h
 *
 * Copyright (c) 2010, 2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCRSSParser_h
#define WKCRSSParser_h

#include <ctime>

namespace WKC {

/*@{*/

class WKCRSSParserPrivate;

/**
@brief Class that holds RSS feed information
*/
class WKC_API WKCRSSFeed {
public:
    /** @brief Channel title (0, if it does not exist) */
    const unsigned short* m_title;
    /** @brief Channel description (0, if it does not exist) */
    const unsigned short* m_description;
    /** @brief URL of Web site that represents channel (0, if it does not exist) */
    const char* m_link;
    /** @brief Structure that holds RSS feed date information */
    struct Date {
        /** @brief Date element held */
        enum Type {
            /** @brief Only Year is enabled. Month, Day, Hour, Minute, Second, and Time zone are 0. */
            EType_Y,
            /** @brief Only Year and Month are enabled. Day, Hour, Minute, Second, and Time zone are 0. */
            EType_YM,
            /** @brief Only Year, Month, and Day are enabled. Hour, Minute, Second, and Time zone are 0. */
            EType_YMD,
            /** @brief Only Year, Month, Day, Hour, Minute, and Time zone are enabled. Second is 0. */
            EType_YMDHMZ,
            /** @brief Year, Month, Day, Hour, Minute, Second, and Time zone are all enabled */
            EType_YMDHMSZ
        };
        /** @brief Date element that holds m_tm, m_zone */
        Type m_type;
        /** @brief Local time. tm_wday, tm_yday are not set, tm_isdst is 0, other members are defined by m_type. */
        struct std::tm m_tm;
        /** @brief Time zone. Expresses offset from UTC in minutes. Japan is +540. -5999 to +5999 are defined by m_type. */
        int m_zone;
    };
    /** @brief Structure that holds RSS feed item information */
    struct Item {
        /** @brief Next item (0, if it does not exist) */
        struct Item* m_next;
        /** @brief Item title (0, if it does not exist)  */
        const unsigned short* m_title;
        /** @brief Item outline (0, if it does not exist) */
        const unsigned short* m_description;
        /** @brief Item content (0, if it does not exist) */
        const unsigned short* m_content;
        /** @brief URL of Web site that represents item (0, if it does not exist) */
        const char* m_link;
        /** @brief Date item was published (0, if it does not exist) */
        const struct Date* m_date;
    };
    /** @brief Item (0, if it does not exist) */
    struct Item *m_item;

public:
    /**
       @brief Gets number of items
       @return Number of items
    */
    unsigned int itemLength() const;

private:
    WKCRSSFeed();
    ~WKCRSSFeed();
    WKCRSSFeed::Item* appendItem();

    friend class WKCRSSParserPrivate;
};

/** @brief Class that parses RSS feeds */
class WKC_API WKCRSSParser {
public:
    /**
       @brief Creates RSS parser
       @retval !=0 Pointer to RSS parser
       @retval ==0  Failed to generate
       @details
       Creates WKCRSSParser and returns its pointer as a return value.
    */
    static WKCRSSParser* create();
    /**
       @brief Destroys RSS parser
       @param self Pointer to RSS parser (0 is allowed, in which case nothing happens.)
       @return None
    */
    static void deleteWKCRSSParser(WKCRSSParser *self);
    /**
       @brief Enters feed content (chunk)
       @param in_str Content (0 is not allowed)
       @param in_len Length of in_str
       @param in_flush false = More exists, true = End of entry
       @return None
       @details
       Divides entire feed into multiple chunks and puts it into the parser. Call this function with false for in_flush for everything except the last chunk, for the last chunk call it with true for in_flush.
    */
    void write(const char* in_str, unsigned int in_len, bool in_flush);
    /**
       @brief Gets feed information
       @return Pointer to feed information. Never return 0
    */
    const WKCRSSFeed* feed() const;
    /** @brief RSS parser status */
    enum Status {
        /** @brief Normal */
        EStatus_OK,
        /** @brief Input is not complete yet */
        EStatus_NoInput,
        /** @brief Insufficient memory */
        EStatus_NoMemory,
        /** @brief Error occurred in XML */
        EStatus_XMLError
    };
    /**
       @brief Returns status
       @return Status
    */
    enum Status status() const;

private:
    WKCRSSParser();
    ~WKCRSSParser();
    bool construct();

private:
    WKCRSSParserPrivate* m_private;
};

/*@}*/

} // namespace

#endif  // WKCRSSParser_h
