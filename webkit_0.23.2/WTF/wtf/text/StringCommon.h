/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StringCommon_h
#define StringCommon_h

#include <unicode/uchar.h>
#include <wtf/ASCIICType.h>

namespace WTF {

template<typename T>
inline T loadUnaligned(const char* s)
{
#if COMPILER(CLANG)
    T tmp;
    memcpy(&tmp, s, sizeof(T));
    return tmp;
#else
    // This may result in undefined behavior due to unaligned access.
    return *reinterpret_cast<const T*>(s);
#endif
}

// Do comparisons 8 or 4 bytes-at-a-time on architectures where it's safe.
#if (CPU(X86_64) || CPU(ARM64)) && !ASAN_ENABLED
ALWAYS_INLINE bool equal(const LChar* aLChar, const LChar* bLChar, unsigned length)
{
    unsigned dwordLength = length >> 3;

    const char* a = reinterpret_cast<const char*>(aLChar);
    const char* b = reinterpret_cast<const char*>(bLChar);

    if (dwordLength) {
        for (unsigned i = 0; i != dwordLength; ++i) {
            if (loadUnaligned<uint64_t>(a) != loadUnaligned<uint64_t>(b))
                return false;

            a += sizeof(uint64_t);
            b += sizeof(uint64_t);
        }
    }

    if (length & 4) {
        if (loadUnaligned<uint32_t>(a) != loadUnaligned<uint32_t>(b))
            return false;

        a += sizeof(uint32_t);
        b += sizeof(uint32_t);
    }

    if (length & 2) {
        if (loadUnaligned<uint16_t>(a) != loadUnaligned<uint16_t>(b))
            return false;

        a += sizeof(uint16_t);
        b += sizeof(uint16_t);
    }

    if (length & 1 && (*reinterpret_cast<const LChar*>(a) != *reinterpret_cast<const LChar*>(b)))
        return false;

    return true;
}

ALWAYS_INLINE bool equal(const UChar* aUChar, const UChar* bUChar, unsigned length)
{
    unsigned dwordLength = length >> 2;

    const char* a = reinterpret_cast<const char*>(aUChar);
    const char* b = reinterpret_cast<const char*>(bUChar);

    if (dwordLength) {
        for (unsigned i = 0; i != dwordLength; ++i) {
            if (loadUnaligned<uint64_t>(a) != loadUnaligned<uint64_t>(b))
                return false;

            a += sizeof(uint64_t);
            b += sizeof(uint64_t);
        }
    }

    if (length & 2) {
        if (loadUnaligned<uint32_t>(a) != loadUnaligned<uint32_t>(b))
            return false;

        a += sizeof(uint32_t);
        b += sizeof(uint32_t);
    }

    if (length & 1 && (*reinterpret_cast<const UChar*>(a) != *reinterpret_cast<const UChar*>(b)))
        return false;

    return true;
}
#elif CPU(X86) && !ASAN_ENABLED
ALWAYS_INLINE bool equal(const LChar* aLChar, const LChar* bLChar, unsigned length)
{
    const char* a = reinterpret_cast<const char*>(aLChar);
    const char* b = reinterpret_cast<const char*>(bLChar);

    unsigned wordLength = length >> 2;
    for (unsigned i = 0; i != wordLength; ++i) {
        if (loadUnaligned<uint32_t>(a) != loadUnaligned<uint32_t>(b))
            return false;
        a += sizeof(uint32_t);
        b += sizeof(uint32_t);
    }

    length &= 3;

    if (length) {
        const LChar* aRemainder = reinterpret_cast<const LChar*>(a);
        const LChar* bRemainder = reinterpret_cast<const LChar*>(b);

        for (unsigned i = 0; i <  length; ++i) {
            if (aRemainder[i] != bRemainder[i])
                return false;
        }
    }

    return true;
}

ALWAYS_INLINE bool equal(const UChar* aUChar, const UChar* bUChar, unsigned length)
{
    const char* a = reinterpret_cast<const char*>(aUChar);
    const char* b = reinterpret_cast<const char*>(bUChar);

    unsigned wordLength = length >> 1;
    for (unsigned i = 0; i != wordLength; ++i) {
        if (loadUnaligned<uint32_t>(a) != loadUnaligned<uint32_t>(b))
            return false;
        a += sizeof(uint32_t);
        b += sizeof(uint32_t);
    }

    if (length & 1 && *reinterpret_cast<const UChar*>(a) != *reinterpret_cast<const UChar*>(b))
        return false;

    return true;
}
#elif PLATFORM(IOS) && WTF_ARM_ARCH_AT_LEAST(7) && !ASAN_ENABLED
ALWAYS_INLINE bool equal(const LChar* a, const LChar* b, unsigned length)
{
    bool isEqual = false;
    uint32_t aValue;
    uint32_t bValue;
    asm("subs   %[length], #4\n"
        "blo    2f\n"

        "0:\n" // Label 0 = Start of loop over 32 bits.
        "ldr    %[aValue], [%[a]], #4\n"
        "ldr    %[bValue], [%[b]], #4\n"
        "cmp    %[aValue], %[bValue]\n"
        "bne    66f\n"
        "subs   %[length], #4\n"
        "bhs    0b\n"

        // At this point, length can be:
        // -0: 00000000000000000000000000000000 (0 bytes left)
        // -1: 11111111111111111111111111111111 (3 bytes left)
        // -2: 11111111111111111111111111111110 (2 bytes left)
        // -3: 11111111111111111111111111111101 (1 byte left)
        // -4: 11111111111111111111111111111100 (length was 0)
        // The pointers are at the correct position.
        "2:\n" // Label 2 = End of loop over 32 bits, check for pair of characters.
        "tst    %[length], #2\n"
        "beq    1f\n"
        "ldrh   %[aValue], [%[a]], #2\n"
        "ldrh   %[bValue], [%[b]], #2\n"
        "cmp    %[aValue], %[bValue]\n"
        "bne    66f\n"

        "1:\n" // Label 1 = Check for a single character left.
        "tst    %[length], #1\n"
        "beq    42f\n"
        "ldrb   %[aValue], [%[a]]\n"
        "ldrb   %[bValue], [%[b]]\n"
        "cmp    %[aValue], %[bValue]\n"
        "bne    66f\n"

        "42:\n" // Label 42 = Success.
        "mov    %[isEqual], #1\n"
        "66:\n" // Label 66 = End without changing isEqual to 1.
        : [length]"+r"(length), [isEqual]"+r"(isEqual), [a]"+r"(a), [b]"+r"(b), [aValue]"+r"(aValue), [bValue]"+r"(bValue)
        :
        :
        );
    return isEqual;
}

ALWAYS_INLINE bool equal(const UChar* a, const UChar* b, unsigned length)
{
    bool isEqual = false;
    uint32_t aValue;
    uint32_t bValue;
    asm("subs   %[length], #2\n"
        "blo    1f\n"

        "0:\n" // Label 0 = Start of loop over 32 bits.
        "ldr    %[aValue], [%[a]], #4\n"
        "ldr    %[bValue], [%[b]], #4\n"
        "cmp    %[aValue], %[bValue]\n"
        "bne    66f\n"
        "subs   %[length], #2\n"
        "bhs    0b\n"

        // At this point, length can be:
        // -0: 00000000000000000000000000000000 (0 bytes left)
        // -1: 11111111111111111111111111111111 (1 character left, 2 bytes)
        // -2: 11111111111111111111111111111110 (length was zero)
        // The pointers are at the correct position.
        "1:\n" // Label 1 = Check for a single character left.
        "tst    %[length], #1\n"
        "beq    42f\n"
        "ldrh   %[aValue], [%[a]]\n"
        "ldrh   %[bValue], [%[b]]\n"
        "cmp    %[aValue], %[bValue]\n"
        "bne    66f\n"

        "42:\n" // Label 42 = Success.
        "mov    %[isEqual], #1\n"
        "66:\n" // Label 66 = End without changing isEqual to 1.
        : [length]"+r"(length), [isEqual]"+r"(isEqual), [a]"+r"(a), [b]"+r"(b), [aValue]"+r"(aValue), [bValue]"+r"(bValue)
        :
        :
        );
    return isEqual;
}
#elif !ASAN_ENABLED
ALWAYS_INLINE bool equal(const LChar* a, const LChar* b, unsigned length) { return !memcmp(a, b, length); }
ALWAYS_INLINE bool equal(const UChar* a, const UChar* b, unsigned length) { return !memcmp(a, b, length * sizeof(UChar)); }
#else
ALWAYS_INLINE bool equal(const LChar* a, const LChar* b, unsigned length)
{
    for (unsigned i = 0; i < length; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}
ALWAYS_INLINE bool equal(const UChar* a, const UChar* b, unsigned length)
{
    for (unsigned i = 0; i < length; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}
#endif

ALWAYS_INLINE bool equal(const LChar* a, const UChar* b, unsigned length)
{
    for (unsigned i = 0; i < length; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

ALWAYS_INLINE bool equal(const UChar* a, const LChar* b, unsigned length) { return equal(b, a, length); }

template<typename StringClassA, typename StringClassB>
ALWAYS_INLINE bool equalCommon(const StringClassA& a, const StringClassB& b)
{
    unsigned length = a.length();
    if (length != b.length())
        return false;

    if (a.is8Bit()) {
        if (b.is8Bit())
            return equal(a.characters8(), b.characters8(), length);

        return equal(a.characters8(), b.characters16(), length);
    }

    if (b.is8Bit())
        return equal(a.characters16(), b.characters8(), length);

    return equal(a.characters16(), b.characters16(), length);
}

template<typename StringClassA, typename StringClassB>
ALWAYS_INLINE bool equalCommon(const StringClassA* a, const StringClassB* b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    return equal(*a, *b);
}

template<typename CharacterTypeA, typename CharacterTypeB>
inline bool equalIgnoringASCIICase(const CharacterTypeA* a, const CharacterTypeB* b, unsigned length)
{
    for (unsigned i = 0; i < length; ++i) {
        if (toASCIILower(a[i]) != toASCIILower(b[i]))
            return false;
    }
    return true;
}

template<typename StringClassA, typename StringClassB>
bool equalIgnoringASCIICaseCommon(const StringClassA& a, const StringClassB& b)
{
    unsigned length = a.length();
    if (length != b.length())
        return false;

    if (a.is8Bit()) {
        if (b.is8Bit())
            return equalIgnoringASCIICase(a.characters8(), b.characters8(), length);

        return equalIgnoringASCIICase(a.characters8(), b.characters16(), length);
    }

    if (b.is8Bit())
        return equalIgnoringASCIICase(a.characters16(), b.characters8(), length);

    return equalIgnoringASCIICase(a.characters16(), b.characters16(), length);
}

template<typename StringClassA, typename StringClassB>
bool startsWith(const StringClassA& reference, const StringClassB& prefix)
{
    unsigned prefixLength = prefix.length();
    if (prefixLength > reference.length())
        return false;

    if (reference.is8Bit()) {
        if (prefix.is8Bit())
            return equal(reference.characters8(), prefix.characters8(), prefixLength);
        return equal(reference.characters8(), prefix.characters16(), prefixLength);
    }
    if (prefix.is8Bit())
        return equal(reference.characters16(), prefix.characters8(), prefixLength);
    return equal(reference.characters16(), prefix.characters16(), prefixLength);
}

template<typename StringClassA, typename StringClassB>
bool startsWithIgnoringASCIICase(const StringClassA& reference, const StringClassB& prefix)
{
    unsigned prefixLength = prefix.length();
    if (prefixLength > reference.length())
        return false;

    if (reference.is8Bit()) {
        if (prefix.is8Bit())
            return equalIgnoringASCIICase(reference.characters8(), prefix.characters8(), prefixLength);
        return equalIgnoringASCIICase(reference.characters8(), prefix.characters16(), prefixLength);
    }
    if (prefix.is8Bit())
        return equalIgnoringASCIICase(reference.characters16(), prefix.characters8(), prefixLength);
    return equalIgnoringASCIICase(reference.characters16(), prefix.characters16(), prefixLength);
}

template<typename StringClassA, typename StringClassB>
bool endsWith(const StringClassA& reference, const StringClassB& suffix)
{
    unsigned suffixLength = suffix.length();
    unsigned referenceLength = reference.length();
    if (suffixLength > referenceLength)
        return false;

    unsigned startOffset = referenceLength - suffixLength;

    if (reference.is8Bit()) {
        if (suffix.is8Bit())
            return equal(reference.characters8() + startOffset, suffix.characters8(), suffixLength);
        return equal(reference.characters8() + startOffset, suffix.characters16(), suffixLength);
    }
    if (suffix.is8Bit())
        return equal(reference.characters16() + startOffset, suffix.characters8(), suffixLength);
    return equal(reference.characters16() + startOffset, suffix.characters16(), suffixLength);
}

template<typename StringClassA, typename StringClassB>
bool endsWithIgnoringASCIICase(const StringClassA& reference, const StringClassB& suffix)
{
    unsigned suffixLength = suffix.length();
    unsigned referenceLength = reference.length();
    if (suffixLength > referenceLength)
        return false;

    unsigned startOffset = referenceLength - suffixLength;

    if (reference.is8Bit()) {
        if (suffix.is8Bit())
            return equalIgnoringASCIICase(reference.characters8() + startOffset, suffix.characters8(), suffixLength);
        return equalIgnoringASCIICase(reference.characters8() + startOffset, suffix.characters16(), suffixLength);
    }
    if (suffix.is8Bit())
        return equalIgnoringASCIICase(reference.characters16() + startOffset, suffix.characters8(), suffixLength);
    return equalIgnoringASCIICase(reference.characters16() + startOffset, suffix.characters16(), suffixLength);
}

template <typename SearchCharacterType, typename MatchCharacterType>
size_t findIgnoringASCIICase(const SearchCharacterType* source, const MatchCharacterType* matchCharacters, unsigned startOffset, unsigned searchLength, unsigned matchLength)
{
    ASSERT(searchLength >= matchLength);

    const SearchCharacterType* startSearchedCharacters = source + startOffset;

    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    for (unsigned i = 0; i <= delta; ++i) {
        if (equalIgnoringASCIICase(startSearchedCharacters + i, matchCharacters, matchLength))
            return startOffset + i;
    }
    return notFound;
}

template<typename StringClassA, typename StringClassB>
size_t findIgnoringASCIICase(const StringClassA& source, const StringClassB& stringToFind, unsigned startOffset)
{
    unsigned sourceStringLength = source.length();
    unsigned matchLength = stringToFind.length();
    if (!matchLength)
        return std::min(startOffset, sourceStringLength);

    // Check startOffset & matchLength are in range.
    if (startOffset > sourceStringLength)
        return notFound;
    unsigned searchLength = sourceStringLength - startOffset;
    if (matchLength > searchLength)
        return notFound;

    if (source.is8Bit()) {
        if (stringToFind.is8Bit())
            return findIgnoringASCIICase(source.characters8(), stringToFind.characters8(), startOffset, searchLength, matchLength);
        return findIgnoringASCIICase(source.characters8(), stringToFind.characters16(), startOffset, searchLength, matchLength);
    }

    if (stringToFind.is8Bit())
        return findIgnoringASCIICase(source.characters16(), stringToFind.characters8(), startOffset, searchLength, matchLength);

    return findIgnoringASCIICase(source.characters16(), stringToFind.characters16(), startOffset, searchLength, matchLength);
}

template <typename SearchCharacterType, typename MatchCharacterType>
ALWAYS_INLINE static size_t findInner(const SearchCharacterType* searchCharacters, const MatchCharacterType* matchCharacters, unsigned index, unsigned searchLength, unsigned matchLength)
{
    // Optimization: keep a running hash of the strings,
    // only call equal() if the hashes match.

    // delta is the number of additional times to test; delta == 0 means test only once.
    unsigned delta = searchLength - matchLength;

    unsigned searchHash = 0;
    unsigned matchHash = 0;

    for (unsigned i = 0; i < matchLength; ++i) {
        searchHash += searchCharacters[i];
        matchHash += matchCharacters[i];
    }

    unsigned i = 0;
    // keep looping until we match
    while (searchHash != matchHash || !equal(searchCharacters + i, matchCharacters, matchLength)) {
        if (i == delta)
            return notFound;
        searchHash += searchCharacters[i + matchLength];
        searchHash -= searchCharacters[i];
        ++i;
    }
    return index + i;
}

template<typename CharacterType>
inline size_t find(const CharacterType* characters, unsigned length, CharacterType matchCharacter, unsigned index = 0)
{
    while (index < length) {
        if (characters[index] == matchCharacter)
            return index;
        ++index;
    }
    return notFound;
}

ALWAYS_INLINE size_t find(const UChar* characters, unsigned length, LChar matchCharacter, unsigned index = 0)
{
    return find(characters, length, static_cast<UChar>(matchCharacter), index);
}

inline size_t find(const LChar* characters, unsigned length, UChar matchCharacter, unsigned index = 0)
{
    if (matchCharacter & ~0xFF)
        return notFound;
    return find(characters, length, static_cast<LChar>(matchCharacter), index);
}

template<typename StringClass>
size_t findCommon(const StringClass& haystack, const StringClass& needle, unsigned start)
{
    unsigned needleLength = needle.length();

    if (needleLength == 1) {
        if (haystack.is8Bit())
            return WTF::find(haystack.characters8(), haystack.length(), needle[0], start);
        return WTF::find(haystack.characters16(), haystack.length(), needle[0], start);
    }

    if (!needleLength)
        return std::min(start, haystack.length());

    if (start > haystack.length())
        return notFound;
    unsigned searchLength = haystack.length() - start;
    if (needleLength > searchLength)
        return notFound;

    if (haystack.is8Bit()) {
        if (needle.is8Bit())
            return findInner(haystack.characters8() + start, needle.characters8(), start, searchLength, needleLength);
        return findInner(haystack.characters8() + start, needle.characters16(), start, searchLength, needleLength);
    }

    if (needle.is8Bit())
        return findInner(haystack.characters16() + start, needle.characters8(), start, searchLength, needleLength);

    return findInner(haystack.characters16() + start, needle.characters16(), start, searchLength, needleLength);
}

}

#endif // StringCommon_h
