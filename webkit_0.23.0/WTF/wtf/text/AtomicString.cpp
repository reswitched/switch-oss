/*
 * Copyright (C) 2004-2008, 2013-2014 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "AtomicString.h"

#include "IntegerToStringConversion.h"
#include "dtoa.h"

#if USE(WEB_THREAD)
#include "SpinLock.h"
#endif

namespace WTF {

AtomicString AtomicString::lower() const
{
    // Note: This is a hot function in the Dromaeo benchmark.
    StringImpl* impl = this->impl();
    if (UNLIKELY(!impl))
        return AtomicString();

    RefPtr<StringImpl> lowercasedString = impl->lower();
    if (LIKELY(lowercasedString == impl))
        return *this;

    AtomicString result;
    result.m_string = AtomicStringImpl::add(lowercasedString.get());
    return result;
}

AtomicString AtomicString::convertToASCIILowercase() const
{
    StringImpl* impl = this->impl();
    if (UNLIKELY(!impl))
        return AtomicString();

    // Convert short strings without allocating a new StringImpl, since
    // there's a good chance these strings are already in the atomic
    // string table and so no memory allocation will be required.
    unsigned length;
    const unsigned localBufferSize = 100;
    if (impl->is8Bit() && (length = impl->length()) <= localBufferSize) {
        const LChar* characters = impl->characters8();
        unsigned failingIndex;
        for (unsigned i = 0; i < length; ++i) {
            if (UNLIKELY(isASCIIUpper(characters[i]))) {
                failingIndex = i;
                goto SlowPath;
            }
        }
        return *this;
SlowPath:
        LChar localBuffer[localBufferSize];
        for (unsigned i = 0; i < failingIndex; ++i)
            localBuffer[i] = characters[i];
        for (unsigned i = failingIndex; i < length; ++i)
            localBuffer[i] = toASCIILower(characters[i]);
        return AtomicString(localBuffer, length);
    }

    RefPtr<StringImpl> convertedString = impl->convertToASCIILowercase();
    if (LIKELY(convertedString == impl))
        return *this;

    AtomicString result;
    result.m_string = AtomicStringImpl::add(convertedString.get());
    return result;
}

AtomicString AtomicString::number(int number)
{
    return numberToStringSigned<AtomicString>(number);
}

AtomicString AtomicString::number(unsigned number)
{
    return numberToStringUnsigned<AtomicString>(number);
}

AtomicString AtomicString::number(double number)
{
    NumberToStringBuffer buffer;
    return String(numberToFixedPrecisionString(number, 6, buffer, true));
}

AtomicString AtomicString::fromUTF8Internal(const char* charactersStart, const char* charactersEnd)
{
    auto impl = AtomicStringImpl::addUTF8(charactersStart, charactersEnd);
    if (!impl)
        return nullAtom;
    return impl.get();
}

#ifndef NDEBUG
void AtomicString::show() const
{
    m_string.show();
}
#endif

} // namespace WTF
