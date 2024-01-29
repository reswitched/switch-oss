/*
* Copyright (C) 2010 Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
*     * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/UUID.h>

#include <mutex>
#include <wtf/ASCIICType.h>
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/HexNumber.h>
#include <wtf/text/StringBuilder.h>

#if OS(DARWIN)
#include <sys/sysctl.h>
#endif

namespace WTF {

String createCanonicalUUIDString()
{
    unsigned randomData[4];
    cryptographicallyRandomValues(reinterpret_cast<unsigned char*>(randomData), sizeof(randomData));

    // Format as Version 4 UUID.
    StringBuilder builder;
    builder.reserveCapacity(36);
    appendUnsignedAsHexFixedSize(randomData[0], builder, 8, Lowercase);
    builder.append('-');
    appendUnsignedAsHexFixedSize(randomData[1] >> 16, builder, 4, Lowercase);
    builder.appendLiteral("-4");
    appendUnsignedAsHexFixedSize(randomData[1] & 0x00000fff, builder, 3, Lowercase);
    builder.append('-');
    appendUnsignedAsHexFixedSize((randomData[2] >> 30) | 0x8, builder, 1, Lowercase);
    appendUnsignedAsHexFixedSize((randomData[2] >> 16) & 0x00000fff, builder, 3, Lowercase);
    builder.append('-');
    appendUnsignedAsHexFixedSize(randomData[2] & 0x0000ffff, builder, 4, Lowercase);
    appendUnsignedAsHexFixedSize(randomData[3], builder, 8, Lowercase);
    return builder.toString();
}

String bootSessionUUIDString()
{
#if OS(DARWIN)
    static LazyNeverDestroyed<String> bootSessionUUID;
    static std::once_flag onceKey;
    std::call_once(onceKey, [] {
        size_t uuidLength = 37;
        char uuid[uuidLength];
        if (sysctlbyname("kern.bootsessionuuid", uuid, &uuidLength, nullptr, 0))
            return;
        bootSessionUUID.construct(static_cast<const char*>(uuid), uuidLength - 1);
    });
    return bootSessionUUID;
#else
    return String();
#endif
}

bool isVersion4UUID(StringView value)
{
    // Version 4 UUIDs have the form xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx with hexadecimal digits for x and one of 8, 9, A, or B for y.
    if (value.length() != 36)
        return false;

    for (auto cptr = 0; cptr < 36; ++cptr) {
        if (cptr == 8 || cptr == 13 || cptr == 18 || cptr == 23) {
            if (value[cptr] != '-')
                return false;
            continue;
        }
        if (cptr == 14) {
            if (value[cptr] != '4')
                return false;
            continue;
        }
        if (cptr == 19) {
            auto y = value[cptr];
            if (y != '8' && y != '9' && y != 'a' && y != 'A' && y != 'b' && y != 'B')
                return false;
            continue;
        }
        if (!isASCIIHexDigit(value[cptr]))
            return false;
    }
    return true;
}

} // namespace WTF