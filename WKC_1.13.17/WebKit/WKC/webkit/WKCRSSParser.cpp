/*
    WKCRSSParser.cpp

    Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
    Boston, MA  02110-1301, USA.
*/

#include "config.h"
#include "WKCRSSParser.h"
#include <wtf/ASCIICType.h>

#include <wkc/wkcpeer.h> // wkcI18NDecodePeer()
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h> // strcpy(), strlen()

namespace {

#define arraysizeof(array)      (sizeof(array) / sizeof((array)[0]))

unsigned int
lastDayOfMonth(unsigned int year, unsigned int month)
{
    ASSERT(0 < year && 1 <= month && month <= 12);
    if (12 < month) {
        return 0;   /* for coverity.  coverity does not understand ASSERT() above. */
    }
    static const unsigned int days[2][13] = {
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    };
    const unsigned int leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
    return days[leap][month];
}

/******************************************************************************
    RFC5234    Augmented BNF for Syntax Specifications: ABNF
*/

const char RFC5234_HTAB = 0x09;
const char RFC5234_LF   = 0x0a;
const char RFC5234_CR   = 0x0d;
const char RFC5234_SP   = 0x20;

/*
    RFC5234
    OCTET
*/
bool
parseRFC5234Octet(const char** inout_src, char in_ref, char** inout_dst)
{
    ASSERT(inout_src && *inout_src);
    if (**inout_src == in_ref) {
        (*inout_src)++;
        if (inout_dst) {
            *(*inout_dst)++ = in_ref;
        }
        return true;
    }
    return false;
}

/*
    RFC5234
    *OCTET    ; no case
*/
bool
parseRFC5234OctetsNocase(const char** inout_src, const char* in_ref, char** inout_dst)
{
    ASSERT(inout_src && *inout_src);
    ASSERT(in_ref);
    const char* s = *inout_src;
    unsigned int n = 0;
    while (*in_ref && toASCIILower(*s) == toASCIILower(*in_ref)) {
        n++;
        s++;
        in_ref++;
    }
    if (*in_ref) {
        return false;
    }
    if (inout_dst) {
        s = *inout_src;
        for (int i = 0; i < n; i++) {
            *(*inout_dst)++ = *s++;
        }
    }
    *inout_src += n;
    return true;
}

/*
    RFC5234
    DIGIT
*/
bool
parseRFC5234Digit(const char** inout_src, unsigned int* out_value)
{
    ASSERT(inout_src && *inout_src);
    const char c = **inout_src;
    if (isASCIIDigit(c)) {
        (*inout_src)++;
        if (out_value) {
            *out_value = c - '0';
        }
        return true;
    }
    return false;
}

/*
    RFC5234
    m*nDIGIT
*/
bool
parseRFC5234Digits(const char** inout_src, unsigned int in_m, unsigned int in_n, unsigned int* out_value)
{
    ASSERT(inout_src && *inout_src);
    ASSERT(in_m <= in_n && in_n <= 9);
    unsigned int i = 0;
    unsigned int value = 0;
    while (i < in_n && isASCIIDigit((*inout_src)[i])) {
        value = 10 * value + ((*inout_src)[i] - '0');
        i++;
    }
    if (in_m <= i) {
        (*inout_src) += i;
        if (out_value) {
            *out_value = value;
        }
        return true;
    }
    return false;
}

/*
    BWS             =   1*(HTAB / LF / CR / SP)
    Defined BWS (Broad White Space) to apply RFC822 date-time to RSS pubDate
*/
bool
parseBWS(const char** inout_src, char** inout_dst)
{
    ASSERT(inout_src && *inout_src);
    if (**inout_src == RFC5234_HTAB || **inout_src == RFC5234_LF || **inout_src == RFC5234_CR || **inout_src == RFC5234_SP) {
        if (inout_dst) {
            *(*inout_dst)++ = *(*inout_src);
        }
        (*inout_src)++;
        while (**inout_src == RFC5234_HTAB || **inout_src == RFC5234_LF || **inout_src == RFC5234_CR || **inout_src == RFC5234_SP) {
            if (inout_dst) {
                *(*inout_dst)++ = *(*inout_src);
            }
            (*inout_src)++;
        }
        return true;
    }
    return false;
}

/******************************************************************************
    RFC5322     Internet Message Format
*/

/*
    RFC5322 date-time parser
*/

/*
    RFC5322
    obs-zone        =       "UT" / "GMT" /          ; Universal Time
                                                    ; North American UT
                                                    ; offsets
                            "EST" / "EDT" /         ; Eastern:  - 5/ - 4
                            "CST" / "CDT" /         ; Central:  - 6/ - 5
                            "MST" / "MDT" /         ; Mountain: - 7/ - 6
                            "PST" / "PDT" /         ; Pacific:  - 8/ - 7
                            %d65-73 /               ; Military zones - "A"
                            %d75-90 /               ; through "I" and "K"
                            %d97-105 /              ; through "Z", both
                            %d107-122               ; upper and lower case
*/
bool
parseRFC5322ObsZone(const char** inout_src, int* out_zone)
{
    ASSERT(inout_src && *inout_src);
    static const struct {
        const char* symbol;
        int zone;
    } obs_zone[] = {
        { "UT",         0 },    { "GMT",        0 },
        { "EST",     -500 },    { "EDT",     -400 },
        { "CST",     -600 },    { "CDT",     -500 },
        { "MST",     -700 },    { "MDT",     -600 },
        { "PST",     -800 },    { "PDT",     -700 },
        { "JST",     +900 },
        { "A",       +100 },    { "B",       +200 },    { "C",       +300 },    { "D",       +400 },
        { "E",       +500 },    { "F",       +600 },    { "G",       +700 },    { "H",       +800 },
        { "I",       +900 },    { "K",      +1000 },    { "L",      +1100 },    { "M",      +1200 },
        { "N",       -100 },    { "O",       -200 },    { "P",       -300 },    { "Q",       -400 },
        { "R",       -500 },    { "S",       -600 },    { "T",       -700 },    { "U",       -800 },
        { "V",       -900 },    { "W",      -1000 },    { "X",      -1100 },    { "Y",      -1200 },
        { "Z",          0 }
    };
    unsigned int i;
    for (i = 0; i < arraysizeof(obs_zone); i++) {
        if (parseRFC5234OctetsNocase(inout_src, obs_zone[i].symbol, 0)) {
            if (out_zone) {
                *out_zone = 60 * (obs_zone[i].zone / 100) + obs_zone[i].zone % 100;
            }
            return true;
        }
    }
    return false;
}

/*
    RFC5322
    zone            =   (( "+" / "-" ) 4DIGIT) / obs-zone
*/
bool
parseRFC5322Zone(const char** inout_src, int* out_zone)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    int sign = 0;
    unsigned int zone;
    if (parseRFC5234Octet(&s, '+', 0)) {
        sign = 1;
    } else if (parseRFC5234Octet(&s, '-', 0)) {
        sign = -1;
    }
    if (sign != 0 && parseRFC5234Digits(&s, 4, 4, &zone) && zone <= 9959) {
        *inout_src = s;
        if (out_zone) {
            *out_zone = sign * (60 * (zone / 100) + zone % 100);
        }
        return true;
    }
    return parseRFC5322ObsZone(inout_src, out_zone);
}

/*
    RFC5322
    time-of-day     =   hour ":" minute [ ":" second ]
    hour            =   [BWS] 2DIGIT [BWS]
    minute          =   [BWS] 2DIGIT [BWS]
    second          =   [BWS] 2DIGIT [BWS]
*/
bool
parseRFC5322TimeOfDay(const char** inout_src, WKC::WKCRSSFeed::Date::Type* out_type, unsigned int* out_hour, unsigned int* out_minute, unsigned int* out_second)
{
    ASSERT(inout_src && *inout_src);
    WKC::WKCRSSFeed::Date::Type type;
    unsigned int hour, minute, second;
    const char* s = *inout_src;

    if (
        (parseBWS(&s, 0), parseRFC5234Digits(&s, 2, 2, &hour)) && hour <= 23 &&
        (parseBWS(&s, 0), parseRFC5234Octet(&s, ':', 0)) &&
        (parseBWS(&s, 0), parseRFC5234Digits(&s, 2, 2, &minute)) && minute <= 59
    ) {
        *inout_src = s;
        if (
            (parseBWS(&s, 0), parseRFC5234Octet(&s, ':', 0)) &&
            (parseBWS(&s, 0), parseRFC5234Digits(&s, 2, 2, &second)) && second <= 60
        ) {
            *inout_src = s;
            type = WKC::WKCRSSFeed::Date::EType_YMDHMSZ;
        } else {
            type = WKC::WKCRSSFeed::Date::EType_YMDHMZ;
            second = 0;
        }
        parseBWS(inout_src, 0);
        if (out_type) {
            *out_type = type;
        }
        if (out_hour) {
            *out_hour = hour;
        }
        if (out_minute) {
            *out_minute = minute;
        }
        if (out_second) {
            *out_second = second;
        }
        return true;
    }
    return false;
}

/*
    RFC5322
    time            =   time-of-day zone
*/
bool
parseRFC5322Time(const char** inout_src, WKC::WKCRSSFeed::Date::Type* out_type, unsigned int* out_hour, unsigned int* out_minute, unsigned int* out_second, int* out_zone)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    if (parseRFC5322TimeOfDay(&s, out_type, out_hour, out_minute, out_second) && parseRFC5322Zone(&s, out_zone)) {
        *inout_src = s;
        return true;
    }
    return false;
}

/*
    RFC5322
    year            =   BWS 2*9DIGIT BWS
*/
bool
parseRFC5322Year(const char** inout_src, unsigned int* out_year)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    if (parseBWS(&s, 0)) {
        unsigned year = 0;
        unsigned int count;
        for (count = 0; isASCIIDigit(*s); count++, s++) {
            year *= 10;
            year += *s - '0';
        }
        if (2 <= count && count <= 9) {
            if (count < 4) {
                if (year <= 49) {
                    year += 2000;
                } else {
                    year += 1900;
                }
            }
            if (0 < year && parseBWS(&s, 0)) {
                *inout_src = s;
                if (out_year) {
                    *out_year = year;
                }
                return true;
            }
        }
    }
    return false;
}

/*
    RFC5322
    month           =   "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" / "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"
*/
bool
parseRFC5322Month(const char** inout_src, unsigned int* out_month)
{
    ASSERT(inout_src && *inout_src);
    static const char month_name[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    unsigned int i;
    for (i = 0; i < arraysizeof(month_name); i++) {
        if (parseRFC5234OctetsNocase(inout_src, month_name[i], 0)) {
            if (out_month) {
                *out_month = i + 1;
            }
            return true;
        }
    }
    return false;
}

/*
    RFC5322
    date            =   day month year
    day             =   [BWS] 1*2DIGIT BWS
*/
bool
parseRFC5322Date(const char** inout_src, unsigned int* out_year, unsigned int* out_month, unsigned int* out_day)
{
    ASSERT(inout_src && *inout_src);
    unsigned int year, month, day;
    const char* s = *inout_src;

    parseBWS(&s, 0);
    if (
        parseRFC5234Digits(&s, 1, 2, &day) &&
        parseBWS(&s, 0) &&
        parseRFC5322Month(&s, &month) &&
        parseRFC5322Year(&s, &year) &&
        1 <= day && day <= lastDayOfMonth(year, month)
    ) {
        *inout_src = s;
        if (out_year) {
            *out_year = year;
        }
        if (out_month) {
            *out_month = month;
        }
        if (out_day) {
            *out_day = day;
        }
        return true;
    }
    return false;
}

/*
    RFC5322
    day-of-week     =   [BWS] day-name [BWS]
    day-name        =   "Mon" / "Tue" / "Wed" / "Thu" / "Fri" / "Sat" / "Sun"
*/
bool
parseRFC5322DayOfWeek(const char** inout_src, unsigned int* out_day_of_week)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    static const char day_name[7][4] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    unsigned int i;
    parseBWS(&s, 0);
    for (i = 0; i < arraysizeof(day_name); i++) {
        if (parseRFC5234OctetsNocase(&s, day_name[i], 0)) {
            parseBWS(&s, 0);
            *inout_src = s;
            if (out_day_of_week) {
                *out_day_of_week = i;
            }
            return true;
        }
    }
    return false;
}

/*
    RFC5322
    date-time       =   [ day-of-week "," ] date time
*/
bool
parseRFC5322DateTime(const char** inout_src, WKC::WKCRSSFeed::Date* out_date)
{
    ASSERT(inout_src && *inout_src);
    WKC::WKCRSSFeed::Date::Type type;
    unsigned int year, month, day, hour, minute, second;
    int zone;
    const char* s = *inout_src;
    if (!(parseRFC5322DayOfWeek(&s, 0) && parseRFC5234Octet(&s, ',', 0))) {
        s = *inout_src;
    }
    if (parseRFC5322Date(&s, &year, &month, &day) && parseRFC5322Time(&s, &type, &hour, &minute, &second, &zone)) {
        *inout_src = s;
        if (out_date) {
            out_date->m_type = type;
            out_date->m_tm.tm_year = (int)year;
            out_date->m_tm.tm_mon = (int)(month - 1);
            out_date->m_tm.tm_mday = (int)day;
            out_date->m_tm.tm_hour = (int)hour;
            out_date->m_tm.tm_min = (int)minute;
            out_date->m_tm.tm_sec = (int)second;
            out_date->m_zone = zone;
        }
        return true;
    }
    return false;
}


/******************************************************************************
    RFC3339 date-time parser
*/

/*
    RFC3339
    partial-time    = time-hour ":" time-minute ":" time-second [time-secfrac]
    time-hour       = 2DIGIT  ; 00-23
    time-minute     = 2DIGIT  ; 00-59
    time-second     = 2DIGIT  ; 00-58, 00-59, 00-60 based on leap second
    time-secfrac    = "." 1*DIGIT
*/
bool
parseRFC3339PartialTime(const char** inout_src, unsigned int* out_hour, unsigned int* out_minute, unsigned int* out_second)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    unsigned int hour, minute, second;
    if (
        parseRFC5234Digits(&s, 2, 2, &hour) && hour <= 23 &&
        parseRFC5234Octet(&s, ':', 0) &&
        parseRFC5234Digits(&s, 2, 2, &minute) && minute <= 59 &&
        parseRFC5234Octet(&s, ':', 0) &&
        parseRFC5234Digits(&s, 2, 2, &second) && second <= 60
    ) {
        *inout_src = s;
        if (parseRFC5234Octet(&s, '.', 0) && parseRFC5234Digit(&s, 0)) {
            while (parseRFC5234Digit(&s, 0)) {
                // do nothing
            }
            *inout_src = s;
        }
        if (out_hour) {
            *out_hour = hour;
        }
        if (out_minute) {
            *out_minute = minute;
        }
        if (out_second) {
            *out_second = second;
        }
        return true;
    }
    return false;
}

/*
    RFC3339
    time-offset     = "Z" / time-numoffset
    time-numoffset  = ("+" / "-") time-hour ":" time-minute
*/
bool
parseRFC3339TimeOffset(const char** inout_src, int* out_offset)
{
    ASSERT(inout_src && *inout_src);
    if (parseRFC5234OctetsNocase(inout_src, "Z", 0)) {
        if (out_offset) {
            *out_offset = 0;
        }
        return true;
    }
    {
        const char* s = *inout_src;
        unsigned int hour, minute;
        int sign;
        if (parseRFC5234Octet(&s, '+', 0)) {
            sign = 1;
        } else if (parseRFC5234Octet(&s, '-', 0)) {
            sign = -1;
        } else {
            return false;
        }
        if (
            parseRFC5234Digits(&s, 2, 2, &hour) && hour <= 23 &&
            parseRFC5234Octet(&s, ':', 0) &&
            parseRFC5234Digits(&s, 2, 2, &minute) && minute <= 59
        ) {
            *inout_src = s;
            if (out_offset) {
                *out_offset = sign * (60 * hour + minute);
            }
            return true;
        }
    }
    return false;
}

/*
    RFC3339
    full-time       = partial-time time-offset
*/
bool
parseRFC3339FullTime(const char** inout_src, unsigned int* out_hour, unsigned int* out_minute, unsigned int* out_second, int* out_offset)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    if (
        parseRFC3339PartialTime(&s, out_hour, out_minute, out_second) &&
        parseRFC3339TimeOffset(&s, out_offset)
    ) {
        *inout_src = s;
        return true;
    }
    return false;
}

/*
    RFC3339
    full-date       = date-fullyear "-" date-month "-" date-mday
    date-fullyear   = 4DIGIT
    date-month      = 2DIGIT  ; 01-12
    date-mday       = 2DIGIT  ; 01-28, 01-29, 01-30, 01-31 based on month/year
*/
bool
parseRFC3339FullDate(const char** inout_src, unsigned int* out_year, unsigned int* out_month, unsigned int* out_mday)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    unsigned int year, month, mday;
    if (
        parseRFC5234Digits(&s, 4, 4, &year) && 0 < year &&
        parseRFC5234Octet(&s, '-', 0) &&
        parseRFC5234Digits(&s, 2, 2, &month) && 1 <= month && month <= 12 &&
        parseRFC5234Octet(&s, '-', 0) &&
        parseRFC5234Digits(&s, 2, 2, &mday) && 1 <= mday && mday <= lastDayOfMonth(year, month)
    ) {
        *inout_src = s;
        if (out_year) {
            *out_year = year;
        }
        if (out_month) {
            *out_month = month;
        }
        if (out_mday) {
            *out_mday = mday;
        }
        return true;
    }
    return false;
}

/*
    RFC3339
    date-time       = [BWS] full-date "T" full-time
*/
bool
parseRFC3339DateTime(const char** inout_src, WKC::WKCRSSFeed::Date* out_date)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    unsigned int year, month, mday, hour, minute, second;
    int offset;
    parseBWS(&s, 0);
    if (
        parseRFC3339FullDate(&s, &year, &month, &mday) &&
        parseRFC5234OctetsNocase(&s, "T", 0) &&
        parseRFC3339FullTime(&s, &hour, &minute, &second, &offset)
    ) {
        *inout_src = s;
        if (out_date) {
            out_date->m_type = WKC::WKCRSSFeed::Date::EType_YMDHMSZ;
            out_date->m_tm.tm_year = (int)year;
            out_date->m_tm.tm_mon = (int)(month - 1);
            out_date->m_tm.tm_mday = (int)mday;
            out_date->m_tm.tm_hour = (int)hour;
            out_date->m_tm.tm_min = (int)minute;
            out_date->m_tm.tm_sec = (int)second;
            out_date->m_zone = offset;
        }
        return true;
    }
    return false;
}


/******************************************************************************
    W3C NOTE-datetime parser    http://www.w3.org/TR/1998/NOTE-datetime-19980827
*/

/*
    W3C NOTE-datetime
    zone    = "Z" / (("+" / "-") 2DIGIT ":" 2DIGIT)
*/
bool
parseW3CZone(const char** inout_src, int* out_zone) {
    ASSERT(inout_src && *inout_src);
    // same as RFC3339 time-offset
    return parseRFC3339TimeOffset(inout_src, out_zone);
}

/*
    W3C NOTE-datetime
    time    = 2DIGIT ":" 2DIGIT [ ":" 2DIGIT ] [ "." 1*DIGIT ]
*/
bool
parseW3CTime(const char** inout_src, WKC::WKCRSSFeed::Date::Type* out_type, unsigned int* out_hour, unsigned int* out_minute, unsigned int* out_second) {
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    WKC::WKCRSSFeed::Date::Type type;
    unsigned int hour, minute, second;
    if (
        parseRFC5234Digits(&s, 2, 2, &hour) && hour <= 23 &&
        parseRFC5234Octet(&s, ':', 0) &&
        parseRFC5234Digits(&s, 2, 2, &minute) && minute <= 59
    ) {
        *inout_src = s;
        if (parseRFC5234Octet(&s, ':', 0) && parseRFC5234Digits(&s, 2, 2, &second) && second <= 60) {    // allow second = 60 though W3C NOTE-datetime does not allow it.
            *inout_src = s;
            type = WKC::WKCRSSFeed::Date::EType_YMDHMSZ;
        } else {
            second = 0;
            type = WKC::WKCRSSFeed::Date::EType_YMDHMZ;
        }
        s = *inout_src;
        if (parseRFC5234Octet(&s, '.', 0) && parseRFC5234Digit(&s, 0)) {
            while (parseRFC5234Digit(&s, 0)) {
                // do nothing
            }
            *inout_src = s;
        }
        if (out_type) {
            *out_type = type;
        }
        if (out_hour) {
            *out_hour = hour;
        }
        if (out_minute) {
            *out_minute = minute;
        }
        if (out_second) {
            *out_second = second;
        }
        return true;
    }
    return false;
}

/*
    W3C NOTE-datetime
    date-time    = [BWS] year [ "-" month [ "-" day [ "T" time zone ] ] ]
    year         = 4DIGIT
    month        = 2DIGIT
    day          = 2DIGIT
*/
bool
parseW3CDateTime(const char** inout_src, WKC::WKCRSSFeed::Date* out_date)
{
    ASSERT(inout_src && *inout_src);
    const char* s = *inout_src;
    WKC::WKCRSSFeed::Date::Type type;
    unsigned int year, month, mday;
    unsigned int hour, minute, second;
    int zone;

    parseBWS(&s, 0);
    if (!parseRFC5234Digits(&s, 4, 4, &year) || year == 0) {
        return false;
    }
    *inout_src = s;
    if (out_date) {
        out_date->m_type = WKC::WKCRSSFeed::Date::EType_Y;
        out_date->m_tm.tm_year = (int)year;
        out_date->m_tm.tm_mon = 0;
        out_date->m_tm.tm_mday = 0;
        out_date->m_tm.tm_hour = 0;
        out_date->m_tm.tm_min = 0;
        out_date->m_tm.tm_sec = 0;
        out_date->m_zone = 0;
    }

    if (
        !parseRFC5234Octet(&s, '-', 0) ||
        !parseRFC5234Digits(&s, 2, 2, &month) ||
        !(1 <= month && month <= 12)
    ) {
        return true;
    }
    *inout_src = s;
    if (out_date) {
        out_date->m_type = WKC::WKCRSSFeed::Date::EType_YM;
        out_date->m_tm.tm_mon = (int)(month - 1);
    }

    if (
        !parseRFC5234Octet(&s, '-', 0) ||
        !parseRFC5234Digits(&s, 2, 2, &mday) ||
        !(1 <= mday && mday <= lastDayOfMonth(year, month))
    ) {
        return true;
    }
    *inout_src = s;
    if (out_date) {
        out_date->m_type = WKC::WKCRSSFeed::Date::EType_YMD;
        out_date->m_tm.tm_mday = (int)mday;
    }
    if (
        !parseRFC5234OctetsNocase(&s, "T", 0) ||
        !parseW3CTime(&s, &type, &hour, &minute, &second) ||
        !parseW3CZone(&s, &zone)
    ) {
        return true;
    }
    if (out_date) {
        out_date->m_type = type;
        out_date->m_tm.tm_hour = hour;
        out_date->m_tm.tm_min = minute;
        out_date->m_tm.tm_sec = second;
        out_date->m_zone = zone;
    }
    return true;
}

/******************************************************************************
    Document Tree
*/

bool
isTargetElement(xmlNodePtr in_node, const char* in_ns, const char* in_name)
{
    ASSERT(in_node);
    ASSERT(in_name);
    if (in_node->type == XML_ELEMENT_NODE) {
        if (in_node->name && xmlStrcmp(in_node->name, (const xmlChar*)in_name) == 0) {
            if (!in_ns || (in_node->ns && in_node->ns->prefix && xmlStrcmp(in_node->ns->prefix, (const xmlChar*)in_ns) == 0)) {
                return true;
            }
        }
    }
    return false;
}

const xmlChar*
findText(xmlNodePtr in_node)
{
    for (xmlNodePtr t = in_node; t; t = t->next) {
        if (t->type == XML_TEXT_NODE && t->content) {
            return t->content;
        }
    }
    return 0;
}

const xmlChar*
findAttribute(xmlAttrPtr in_attr, const char* in_name)
{
    ASSERT(in_name);
    for (xmlAttrPtr t = in_attr; t; t = t->next) {
        if (t->type == XML_ATTRIBUTE_NODE) {
            if (t->name && xmlStrcmp(t->name, (const xmlChar*)in_name) == 0) {
                return findText(t->children);
            }
        }
    }
    return 0;
}

} // namespace

namespace WKC {

/******************************************************************************
    WKCRSSFeed
*/

WKCRSSFeed::WKCRSSFeed()
     : m_title(0)
     , m_description(0)
     , m_link(0)
     , m_item(0)
{
}

WKCRSSFeed::~WKCRSSFeed()
{
    if (m_title)
        WTF::fastFree((void *)m_title);
    if (m_description)
        WTF::fastFree((void *)m_description);
    if (m_link)
        WTF::fastFree((void *)m_link);
    Item* item = m_item;
    while (item) {
        Item* next_item = item->m_next;
        if (item->m_title)
            WTF::fastFree((void *)item->m_title);
        if (item->m_description)
            WTF::fastFree((void *)item->m_description);
        if (item->m_content)
            WTF::fastFree((void *)item->m_content);
        if (item->m_link)
            WTF::fastFree((void *)item->m_link);
        if (item->m_date)
            WTF::fastFree((void *)item->m_date);
        WTF::fastFree((void *)item);
        item = next_item;
    }
}

WKCRSSFeed::Item*
WKCRSSFeed::appendItem()
{
    void* p = WTF::fastMalloc(sizeof(Item));
    Item* new_item = new (p) Item;
    if (!new_item) {
        return 0;
    }
    new_item->m_next = 0;
    new_item->m_title = 0;
    new_item->m_description = 0;
    new_item->m_content = 0;
    new_item->m_link = 0;
    new_item->m_date = 0;
    if (!m_item) {
        m_item = new_item;
        return new_item;
    }
    Item* item = m_item;
    while (item->m_next) {
        item = item->m_next;
    }
    item->m_next = new_item;
    return new_item;
}

unsigned int
WKCRSSFeed::itemLength() const
{
    Item* item = m_item;
    unsigned int len = 0;
    while (item) {
        len++;
        item = item->m_next;
    }
    return len;
}

/******************************************************************************
    WKCRSSParserPrivate
*/

class WKCRSSParserPrivate {
    WTF_MAKE_FAST_ALLOCATED;
    friend class WKCRSSParser;

public:
    static WKCRSSParserPrivate* create();
    ~WKCRSSParserPrivate();
    void write(const char* in_str, unsigned int in_len, bool in_flush);
private:
    WKCRSSParserPrivate();
    bool construct();
    void setStatus(WKCRSSParser::Status in_status);
    const unsigned short* makeUTF16(const xmlChar* in_str);
    const char* makeUTF8(const xmlChar* in_str);
    const unsigned short* getContentInUTF16(xmlNodePtr in_node);
    const char* getContentInUTF8(xmlNodePtr in_node);

    // Atom 1.0
    void nodeFeedEntry(xmlNodePtr in_entry_node);
    void nodeFeed(xmlNodePtr in_feed_node);
    // RSS 0.9, 1.0
    void nodeRdfItem(xmlNodePtr in_item_node);
    void nodeRdfChannel(xmlNodePtr in_channel_node);
    void nodeRdf(xmlNodePtr in_rdf_node);
    // RSS 0.91, 2.0
    void nodeRssChannelItem(xmlNodePtr in_item_node);
    void nodeRssChannel(xmlNodePtr in_channel_node);
    void nodeRss(xmlNodePtr in_rss_node);
    void docRoot(xmlNodePtr in_doc_child);

private:
    bool m_didFlush;
    WKCRSSParser::Status m_status;
    xmlParserCtxtPtr m_xmlParser;
    WKCRSSFeed* m_feed;
//  RefPtr<TextResourceDecoder> m_decoder;
};

const char* const Name_channel      = "channel";
const char* const Name_date         = "date";
const char* const Name_dc           = "dc";
const char* const Name_description  = "description";
const char* const Name_entry        = "entry";
const char* const Name_feed         = "feed";
const char* const Name_href         = "href";
const char* const Name_item         = "item";
const char* const Name_link         = "link";
const char* const Name_pubDate      = "pubDate";
const char* const Name_published    = "published";
const char* const Name_issued       = "issued";
const char* const Name_RDF          = "RDF";
const char* const Name_rdf          = "rdf";
const char* const Name_rss          = "rss";
const char* const Name_subtitle     = "subtitle";
const char* const Name_summary      = "summary";
const char* const Name_title        = "title";
const char* const Name_updated      = "updated";
const char* const Name_modified     = "modified";
const char* const Name_content      = "content";
const char* const Name_encoded      = "encoded";


WKCRSSParserPrivate::WKCRSSParserPrivate()
    : m_didFlush(false)
    , m_status(WKCRSSParser::EStatus_NoInput)
    , m_xmlParser(0)
    , m_feed(0)
{
}

WKCRSSParserPrivate::~WKCRSSParserPrivate()
{
    if (m_xmlParser) {
        if (m_xmlParser->myDoc) {
            xmlFreeDoc(m_xmlParser->myDoc);
        }
        xmlFreeParserCtxt(m_xmlParser);
//      xmlCleanupParser();
    }
    m_feed->~WKCRSSFeed();
    WTF::fastFree(m_feed);
}

WKCRSSParserPrivate*
WKCRSSParserPrivate::create()
{
    WKCRSSParserPrivate* self = new WKCRSSParserPrivate();
    if (!self) {
        return 0;
    }
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
WKCRSSParserPrivate::construct()
{
    m_xmlParser = xmlCreatePushParserCtxt((xmlSAXHandlerPtr)0, (void*)0, (const char*)0, 0, (const char*)0);
    void* p = WTF::fastMalloc(sizeof(WKCRSSFeed));
    m_feed = new (p) WKCRSSFeed;
    if (!m_xmlParser || !m_feed) {
        return false;
    }
    return true;
}

void
WKCRSSParserPrivate::write(const char* in_str, unsigned int in_len, bool in_flush)
{
    ASSERT(in_str);
    if (m_didFlush) {
        return;
    }
    int err = xmlParseChunk(m_xmlParser, in_str, in_len, in_flush);
    if (err) {
        if (err == XML_ERR_NO_MEMORY) {
            setStatus(WKCRSSParser::EStatus_NoMemory);
        } else {
            /* @todo need handling more?  err's real type is enum xmlParserErrors. */
            setStatus(WKCRSSParser::EStatus_XMLError);
        }
    }
    if (in_flush) {
        m_didFlush = true;
        setStatus(WKCRSSParser::EStatus_OK);
//      printf("well formed : %s\n", m_xmlParser->wellFormed ? "yes" : "no");
        if (m_xmlParser->myDoc) {
            docRoot(m_xmlParser->myDoc->children);
            xmlFreeDoc(m_xmlParser->myDoc);
            xmlFreeParserCtxt(m_xmlParser);
//          xmlCleanupParser();
            m_xmlParser = 0;
        } else {
            setStatus(WKCRSSParser::EStatus_XMLError);
        }
    }
}

void
WKCRSSParserPrivate::setStatus(WKCRSSParser::Status in_status)
{
    if (m_status == WKCRSSParser::EStatus_OK || m_status == WKCRSSParser::EStatus_NoInput) {
        m_status = in_status;
    }
}

const unsigned short*
WKCRSSParserPrivate::makeUTF16(const xmlChar* in_str)
{
    void* decoder = 0;
    decoder = wkcI18NBeginDecodePeer(WKC_I18N_CODEC_UTF8);
    if (!decoder) return 0;

    if (in_str) {
        const int srcLen = strlen((const char*)in_str);
        int len = wkcI18NDecodePeer(decoder, (const char*)in_str, srcLen, (unsigned short*)0, 0, 0, 0, 0);
        if (len < 0) {
            len = 0;
        }
        unsigned short* buf = (unsigned short *)WTF::fastMalloc(sizeof(unsigned short)*(len + 1));
        if (buf) {
            if (len > 0) {
                wkcI18NFlushDecodeStatePeer(decoder);
                wkcI18NDecodePeer(decoder, (const char*)in_str, srcLen, buf, len + 1, 0, 0, 0);
            }
            buf[len] = 0;
            wkcI18NEndDecodePeer(decoder);
            return buf;
        }
        setStatus(WKCRSSParser::EStatus_NoMemory);
    }
    wkcI18NEndDecodePeer(decoder);
    return 0;
}

const char*
WKCRSSParserPrivate::makeUTF8(const xmlChar* in_str)
{
    if (in_str) {
        const int len = strlen((const char*)in_str);
        char* buf = (char *)WTF::fastMalloc(len + 1);
        if (buf) {
            strncpy(buf, (const char*)in_str, len + 1);
            return buf;
        }
        setStatus(WKCRSSParser::EStatus_NoMemory);
    }
    return 0;
}

const unsigned short*
WKCRSSParserPrivate::getContentInUTF16(xmlNodePtr in_node)
{
    const char* content = getContentInUTF8(in_node);
    if (content == 0) {
        return 0;
    }
    const unsigned short* buf = makeUTF16((const xmlChar*)content);
    WTF::fastFree((void *)content);
    return buf;
}

const char*
WKCRSSParserPrivate::getContentInUTF8(xmlNodePtr in_node)
{
    unsigned int len = 0;
    for (xmlNodePtr t = in_node; t; t = t->next) {
        if ((t->type == XML_TEXT_NODE || t->type == XML_CDATA_SECTION_NODE) && t->content) {
            len += xmlStrlen(t->content);
        }
    }
    char* buf = (char *)WTF::fastMalloc(len + 1);
    if (buf == 0) {
        setStatus(WKCRSSParser::EStatus_NoMemory);
        return 0;
    }
    char* bp = buf;
    int bp_len = len;
    for (xmlNodePtr t = in_node; t; t = t->next) {
        if ((t->type == XML_TEXT_NODE || t->type == XML_CDATA_SECTION_NODE) && t->content) {
            strncpy(bp, (const char*)t->content, bp_len);
            bp += xmlStrlen(t->content);
            bp_len -= xmlStrlen(t->content);
        }
    }
    buf[len] = 0;
    return buf;

}

void
WKCRSSParserPrivate::nodeFeedEntry(xmlNodePtr in_entry_child)
{
    WKCRSSFeed::Item* item = m_feed->appendItem();
    if (!item) {
        setStatus(WKCRSSParser::EStatus_NoMemory);
        return;
    }
    for (xmlNodePtr node = in_entry_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_title)) {
            if (!item->m_title) {
                item->m_title = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_summary)) {
            if (!item->m_description) {
                item->m_description = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_content)) {
            if (!item->m_content) {
                item->m_content = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_link)) {
            if (!item->m_link) {
                item->m_link = makeUTF8(findAttribute(node->properties, Name_href));
            }
        } else if (isTargetElement(node, 0, Name_published) || isTargetElement(node, 0, Name_issued)) {
            if (item->m_date) {
                WTF::fastFree((void *)item->m_date);
                item->m_date = 0;
            }
            const char* src = getContentInUTF8(node->children);
            if (src) {
                WKCRSSFeed::Date* date = (WKCRSSFeed::Date*)WTF::fastMalloc(sizeof(WKCRSSFeed::Date));
                if (date) {
                    const char* s = src;
                    if (parseRFC3339DateTime(&s, date)) {
                        item->m_date = date;
                    } else {
                        WTF::fastFree(date);
                    }
                } else {
                    setStatus(WKCRSSParser::EStatus_NoMemory);
                }
                WTF::fastFree((void *)src);
            }
        } else if (isTargetElement(node, 0, Name_updated) || isTargetElement(node, 0, Name_modified)) {
            if (!item->m_date) {
                const char* src = getContentInUTF8(node->children);
                if (src) {
                    WKCRSSFeed::Date* date = (WKCRSSFeed::Date*)WTF::fastMalloc(sizeof(WKCRSSFeed::Date));
                    if (date) {
                        const char* s = src;
                        if (parseRFC3339DateTime(&s, date)) {
                            item->m_date = date;
                        } else {
                            WTF::fastFree(date);
                        }
                    } else {
                        setStatus(WKCRSSParser::EStatus_NoMemory);
                    }
                    WTF::fastFree((void *)src);
                }
            }
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeFeed(xmlNodePtr in_feed_child)
{
    for (xmlNodePtr node = in_feed_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_title)) {
            if (!m_feed->m_title) {
                m_feed->m_title = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_subtitle)) {
            if (!m_feed->m_description) {
                m_feed->m_description = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_link)) {
            if (!m_feed->m_link) {
                m_feed->m_link = makeUTF8(findAttribute(node->properties, Name_href));
            }
        } else if (isTargetElement(node, 0, Name_entry)) {
            nodeFeedEntry(node->children);
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeRdfItem(xmlNodePtr in_item_child)
{
    WKCRSSFeed::Item* item = m_feed->appendItem();
    if (!item) {
        setStatus(WKCRSSParser::EStatus_NoMemory);
        return;
    }
    for (xmlNodePtr node = in_item_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_title)) {
            if (!item->m_title) {
                item->m_title = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_description)) {
            if (!item->m_description) {
                item->m_description = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, Name_content, Name_encoded)) {
            if (!item->m_content) {
                item->m_content = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_link)) {
            if (!item->m_link) {
                item->m_link = getContentInUTF8(node->children);
            }
        } else if (isTargetElement(node, Name_dc, Name_date)) {
            if (!item->m_date) {
                const char* src = getContentInUTF8(node->children);
                if (src) {
                    WKCRSSFeed::Date* date = (WKCRSSFeed::Date*)WTF::fastMalloc(sizeof(WKCRSSFeed::Date));
                    if (date) {
                        const char* s = src;
                        if (parseW3CDateTime(&s, date)) {
                            item->m_date = date;
                        } else {
                            WTF::fastFree(date);
                        }
                    } else {
                        setStatus(WKCRSSParser::EStatus_NoMemory);
                    }
                    WTF::fastFree((void *)src);
                }
            }
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeRdfChannel(xmlNodePtr in_channel_child)
{
    for (xmlNodePtr node = in_channel_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_title)) {
            if (!m_feed->m_title) {
                m_feed->m_title = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_description)) {
            if (!m_feed->m_description) {
                m_feed->m_description = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_link)) {
            if (!m_feed->m_link) {
                m_feed->m_link = getContentInUTF8(node->children);
            }
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeRdf(xmlNodePtr in_rdf_child)
{
    for (xmlNodePtr node = in_rdf_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_channel)) {
            nodeRdfChannel(node->children);
        } else if (isTargetElement(node, 0, Name_item)) {
            nodeRdfItem(node->children);
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeRssChannelItem(xmlNodePtr in_item_child)
{
    WKCRSSFeed::Item* item = m_feed->appendItem();
    if (!item) {
        setStatus(WKCRSSParser::EStatus_NoMemory);
        return;
    }
    for (xmlNodePtr node = in_item_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_title)) {
            if (!item->m_title) {
                item->m_title = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_description)) {
            if (!item->m_description) {
                item->m_description = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, Name_content, Name_encoded)) {
            if (!item->m_content) {
                item->m_content = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_link)) {
            if (!item->m_link) {
                item->m_link = getContentInUTF8(node->children);
            }
        } else if (isTargetElement(node, 0, Name_pubDate)) {
            if (item->m_date) {
                WTF::fastFree((void *)item->m_date);
                item->m_date = 0;
            }
            const char* src = getContentInUTF8(node->children);
            if (src) {
                WKCRSSFeed::Date* date = (WKCRSSFeed::Date*)WTF::fastMalloc(sizeof(WKCRSSFeed::Date));
                if (date) {
                    const char* s = src;
                    if (parseRFC5322DateTime(&s, date)) {
                        item->m_date = date;
                    } else {
                        WTF::fastFree(date);
                    }
                } else {
                    setStatus(WKCRSSParser::EStatus_NoMemory);
                }
                WTF::fastFree((void *)src);
            }
        } else if (isTargetElement(node, Name_dc, Name_date)) {
            if (!item->m_date) {
                const char* src = getContentInUTF8(node->children);
                if (src) {
                    WKCRSSFeed::Date* date = (WKCRSSFeed::Date*)WTF::fastMalloc(sizeof(WKCRSSFeed::Date));
                    if (date) {
                        const char* s = src;
                        if (parseW3CDateTime(&s, date)) {
                            item->m_date = date;
                        } else {
                            WTF::fastFree(date);
                        }
                    } else {
                        setStatus(WKCRSSParser::EStatus_NoMemory);
                    }
                    WTF::fastFree((void *)src);
                }
            }
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeRssChannel(xmlNodePtr in_channel_child)
{
    for (xmlNodePtr node = in_channel_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_title)) {
            if (!m_feed->m_title) {
                m_feed->m_title = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_description)) {
            if (!m_feed->m_description) {
                m_feed->m_description = getContentInUTF16(node->children);
            }
        } else if (isTargetElement(node, 0, Name_link)) {
            if (!m_feed->m_link) {
                m_feed->m_link = getContentInUTF8(node->children);
            }
        } else if (isTargetElement(node, 0, Name_item)) {
            nodeRssChannelItem(node->children);
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::nodeRss(xmlNodePtr in_rss_child)
{
    for (xmlNodePtr node = in_rss_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_channel)) {
            nodeRssChannel(node->children);
        } else {
            // abandan
        }
    }
}

void
WKCRSSParserPrivate::docRoot(xmlNodePtr in_doc_child)
{
    for (xmlNodePtr node = in_doc_child; node; node = node->next) {
        if (isTargetElement(node, 0, Name_rss)) {
            nodeRss(node->children);
        } else if (isTargetElement(node, Name_rdf, Name_RDF)) {
            nodeRdf(node->children);
        } else if (isTargetElement(node, 0, Name_feed)) {
            nodeFeed(node->children);
        } else {
            // abandan
        }
    }
}

/******************************************************************************
    WKCRSSParser
*/

WKCRSSParser::WKCRSSParser()
    : m_private(0)
{
}

WKCRSSParser::~WKCRSSParser()
{
    delete m_private;
}

WKCRSSParser*
WKCRSSParser::create()
{
    void* p = WTF::fastMalloc(sizeof(WKCRSSParser));
    WKCRSSParser* self = new (p) WKCRSSParser();
    if (!self) {
        return 0;
    }
    if (!self->construct()) {
        self->~WKCRSSParser();
        WTF::fastFree(self);
        return 0;
    }
    return self;
}

bool
WKCRSSParser::construct()
{
    m_private = WKCRSSParserPrivate::create();
    if (!m_private) {
        return false;
    }
    return true;
}

void
WKCRSSParser::deleteWKCRSSParser(WKCRSSParser* self)
{
    self->~WKCRSSParser();
    WTF::fastFree(self);
}


void
WKCRSSParser::write(const char* in_str, unsigned int in_len, bool in_flush)
{
    ASSERT(in_str);
    m_private->write(in_str, in_len, in_flush);
}

const WKCRSSFeed*
WKCRSSParser::feed() const
{
    return m_private->m_feed;
}

WKCRSSParser::Status
WKCRSSParser::status() const
{
    return m_private->m_status;
}

} // namespace WKC
