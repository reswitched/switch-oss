/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2015 Igalia S.L.
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

#include "config.h"
#include "Hyphenation.h"

#if USE(LIBHYPHEN)

#include "AtomicStringKeyedMRUCache.h"
#include "FileSystem.h"
#include <hyphen.h>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringView.h>

#if PLATFORM(GTK)
#include "GtkUtilities.h"
#include <wtf/glib/GUniquePtr.h>
#endif

namespace WebCore {

static const char* const gDictionaryDirectories[] = {
    "/usr/share/hyphen",
    "/usr/local/share/hyphen",
};

static String extractLocaleFromDictionaryFilePath(const String& filePath)
{
    // Dictionary files always have the form "hyph_<locale name>.dic"
    // so we strip everything except the locale.
    String fileName = pathGetFileName(filePath);
    static const int prefixLength = 5;
    static const int suffixLength = 4;
    return fileName.substring(prefixLength, fileName.length() - prefixLength - suffixLength);
}

static void scanDirectoryForDicionaries(const char* directoryPath, HashMap<AtomicString, String>& availableLocales)
{
    for (const auto& filePath : listDirectory(directoryPath, "hyph_*.dic"))
        availableLocales.set(AtomicString(extractLocaleFromDictionaryFilePath(filePath)), filePath);
}

#if ENABLE(DEVELOPER_MODE)
static void scanTestDictionariesDirectoryIfNecessary(HashMap<AtomicString, String>& availableLocales)
{
    // It's unfortunate that we need to look for the dictionaries this way, but
    // libhyphen doesn't have the concept of installed dictionaries. Instead,
    // we have this special case for WebKit tests.
#if PLATFORM(GTK)
    CString buildDirectory = webkitBuildDirectory();
    GUniquePtr<char> dictionariesPath(g_build_filename(buildDirectory.data(), "DependenciesGTK", "Root", "webkitgtk-test-dicts", nullptr));
    if (g_file_test(dictionariesPath.get(), static_cast<GFileTest>(G_FILE_TEST_IS_DIR))) {
        scanDirectoryForDicionaries(dictionariesPath.get(), availableLocales);
        return;
    }

    // Try alternative dictionaries path for people not using JHBuild.
    dictionariesPath.reset(g_build_filename(buildDirectory.data(), "webkitgtk-test-dicts", nullptr));
    scanDirectoryForDicionaries(dictionariesPath.get(), availableLocales);
#elif defined(TEST_HYPHENATAION_PATH)
    scanDirectoryForDicionaries(TEST_HYPHENATAION_PATH, availableLocales);
#endif
}
#endif

static HashMap<AtomicString, String>& availableLocales()
{
    static bool scannedLocales = false;
    static HashMap<AtomicString, String> availableLocales;

    if (!scannedLocales) {
        for (size_t i = 0; i < WTF_ARRAY_LENGTH(gDictionaryDirectories); i++)
            scanDirectoryForDicionaries(gDictionaryDirectories[i], availableLocales);

#if ENABLE(DEVELOPER_MODE)
        scanTestDictionariesDirectoryIfNecessary(availableLocales);
#endif

        scannedLocales = true;
    }

    return availableLocales;
}

bool canHyphenate(const AtomicString& localeIdentifier)
{
    if (localeIdentifier.isNull())
        return false;
    return availableLocales().contains(localeIdentifier);
}

class HyphenationDictionary : public RefCounted<HyphenationDictionary> {
    WTF_MAKE_NONCOPYABLE(HyphenationDictionary);
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef std::unique_ptr<HyphenDict, void(*)(HyphenDict*)> HyphenDictUniquePtr;

    virtual ~HyphenationDictionary() { }
    static RefPtr<HyphenationDictionary> createNull()
    {
        return adoptRef(new HyphenationDictionary());
    }

    static RefPtr<HyphenationDictionary> create(const CString& dictPath)
    {
        return adoptRef(new HyphenationDictionary(dictPath));
    }

    HyphenDict* libhyphenDictionary() const
    {
        return m_libhyphenDictionary.get();
    }

private:
    HyphenationDictionary(const CString& dictPath)
        : m_libhyphenDictionary(HyphenDictUniquePtr(hnj_hyphen_load(dictPath.data()), hnj_hyphen_free))
    {
    }

    HyphenationDictionary()
        : m_libhyphenDictionary(HyphenDictUniquePtr(nullptr, hnj_hyphen_free))
    {
    }

    HyphenDictUniquePtr m_libhyphenDictionary;
};

template<>
RefPtr<HyphenationDictionary> AtomicStringKeyedMRUCache<RefPtr<HyphenationDictionary>>::createValueForNullKey()
{
    return HyphenationDictionary::createNull();
}

template<>
RefPtr<HyphenationDictionary> AtomicStringKeyedMRUCache<RefPtr<HyphenationDictionary>>::createValueForKey(const AtomicString& localeIdentifier)
{
    ASSERT(availableLocales().get(localeIdentifier));
    return HyphenationDictionary::create(fileSystemRepresentation(availableLocales().get(localeIdentifier)));
}

static AtomicStringKeyedMRUCache<RefPtr<HyphenationDictionary>>& hyphenDictionaryCache()
{
    static NeverDestroyed<AtomicStringKeyedMRUCache<RefPtr<HyphenationDictionary>>> cache;
    return cache;
}

static void countLeadingSpaces(const CString& utf8String, int32_t& pointerOffset, int32_t& characterOffset)
{
    pointerOffset = 0;
    characterOffset = 0;
    const char* stringData = utf8String.data();
    UChar32 character = 0;
    while (static_cast<unsigned>(pointerOffset) < utf8String.length()) {
        int32_t nextPointerOffset = pointerOffset;
        U8_NEXT(stringData, nextPointerOffset, static_cast<int32_t>(utf8String.length()), character);

        if (character < 0 || !u_isUWhiteSpace(character))
            return;

        pointerOffset = nextPointerOffset;
        characterOffset++;
    }
}

size_t lastHyphenLocation(StringView string, size_t beforeIndex, const AtomicString& localeIdentifier)
{
    ASSERT(availableLocales().contains(localeIdentifier));
    RefPtr<HyphenationDictionary> dictionary = hyphenDictionaryCache().get(localeIdentifier);

    // libhyphen accepts strings in UTF-8 format, but WebCore can only provide StringView
    // which stores either UTF-16 or Latin1 data. This is unfortunate for performance
    // reasons and we should consider switching to a more flexible hyphenation library
    // if it is available.
    CString utf8StringCopy = string.toStringWithoutCopying().utf8();

    // WebCore often passes strings like " wordtohyphenate" to the platform layer. Since
    // libhyphen isn't advanced enough to deal with leading spaces (presumably CoreFoundation
    // can), we should find the appropriate indexes into the string to skip them.
    int32_t leadingSpaceBytes;
    int32_t leadingSpaceCharacters;
    countLeadingSpaces(utf8StringCopy, leadingSpaceBytes, leadingSpaceCharacters);

    // The libhyphen documentation specifies that this array should be 5 bytes longer than
    // the byte length of the input string.
    Vector<char> hyphenArray(utf8StringCopy.length() - leadingSpaceBytes + 5);
    char* hyphenArrayData = hyphenArray.data();

    char** replacements = nullptr;
    int* positions = nullptr;
    int* removedCharacterCounts = nullptr;
    hnj_hyphen_hyphenate2(dictionary->libhyphenDictionary(),
        utf8StringCopy.data() + leadingSpaceBytes,
        utf8StringCopy.length() - leadingSpaceBytes,
        hyphenArrayData,
        nullptr, /* output parameter for hyphenated word */
        &replacements,
        &positions,
        &removedCharacterCounts);

    if (replacements) {
        for (unsigned i = 0; i < utf8StringCopy.length() - leadingSpaceBytes - 1; i++)
            free(replacements[i]);
        free(replacements);
    }

    free(positions);
    free(removedCharacterCounts);

    for (int i = beforeIndex - leadingSpaceCharacters - 1; i >= 0; i--) {
        // libhyphen will put an odd number in hyphenArrayData at all
        // hyphenation points. A number & 1 will be true for odd numbers.
        if (hyphenArrayData[i] & 1)
            return i + leadingSpaceCharacters;
    }

    return 0;
}

} // namespace WebCore

#endif // USE(LIBHYPHEN)
