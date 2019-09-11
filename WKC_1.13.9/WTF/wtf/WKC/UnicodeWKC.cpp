/*
 *  Copyright (C) 2008 Jurg Billeter <j@bitron.ch>
 *  Copyright (C) 2008 Dominik Rottsches <dominik.roettsches@access-company.com>
 *  Copyright (c) 2010, 2011 ACCESS CO., LTD. All rights reserved.
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include <stdint.h>
#include "UnicodeWKC.h"

namespace WTF {
namespace Unicode {

int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
{
    int i, j;
    UChar32 chars[4];
    int ret;
    int len;

    len = 0;
    *error = false;
    for (i=0; i<srcLength; i++) {
        ret = wkcUnicodeFoldCaseFullPeer(src[i], chars);
        for (j=0; j<ret; j++) {
            if (result && resultLength>=0) {
                result[len] = chars[j];
                resultLength--;
            } else {
                *error = true;
            }
            len++;
        }
    }
    return len;
}

int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
{
    int i=0;
    *error = false;
    for (i=0; i<srcLength; i++) {
        if (result && resultLength>=0) {
            result[i] = wkcUnicodeToLowerPeer(src[i]);
            resultLength--;
        } else {
            *error = true;
        }
    }
    return srcLength;
}

int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
{
    int i=0;
    *error = false;
    for (i=0; i<srcLength; i++) {
        if (result && resultLength>=0) {
            result[i] = wkcUnicodeToUpperPeer(src[i]);
            resultLength--;
        } else {
            *error = true;
        }
    }
    return srcLength;
}

}
}
