/*
 * Copyright (C) 2006, 2008, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FontCache.h"

#include "FontCascade.h"
#include "FontPlatformData.h"
#include "FontSelector.h"
#include "MemoryPressureHandler.h"
#include "WebKitFontFamilyNames.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/StdLibExtras.h>
#include <wtf/TemporaryChange.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringHash.h>

#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif

#if PLATFORM(IOS)
#include <wtf/Noncopyable.h>

// FIXME: We may be able to simplify this code using C++11 threading primitives, including std::call_once().
static pthread_mutex_t fontLock;

static void initFontCacheLockOnce()
{
    pthread_mutexattr_t mutexAttribute;
    pthread_mutexattr_init(&mutexAttribute);
    pthread_mutexattr_settype(&mutexAttribute, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&fontLock, &mutexAttribute);
    pthread_mutexattr_destroy(&mutexAttribute);
}

static pthread_once_t initFontLockControl = PTHREAD_ONCE_INIT;

class FontLocker {
    WTF_MAKE_NONCOPYABLE(FontLocker);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    FontLocker()
    {
        pthread_once(&initFontLockControl, initFontCacheLockOnce);
        int lockcode = pthread_mutex_lock(&fontLock);
        ASSERT_WITH_MESSAGE_UNUSED(lockcode, !lockcode, "fontLock lock failed with code:%d", lockcode);    
    }
    ~FontLocker()
    {
        int lockcode = pthread_mutex_unlock(&fontLock);
        ASSERT_WITH_MESSAGE_UNUSED(lockcode, !lockcode, "fontLock unlock failed with code:%d", lockcode);
    }
};
#endif // PLATFORM(IOS)

using namespace WTF;

namespace WebCore {

FontCache& FontCache::singleton()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<FontCache> globalFontCache;
    return globalFontCache;
#else
    WKC_DEFINE_STATIC_PTR(FontCache*, globalFontCache, 0);
    if (!globalFontCache)
        globalFontCache = new FontCache();
    return *globalFontCache;
#endif
}

FontCache::FontCache()
    : m_purgeTimer(*this, &FontCache::purgeTimerFired)
{
}

struct FontPlatformDataCacheKey {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformDataCacheKey() { }
    FontPlatformDataCacheKey(const AtomicString& family, const FontDescription& description)
        : m_fontDescriptionKey(description)
        , m_family(family)
    { }

    explicit FontPlatformDataCacheKey(HashTableDeletedValueType t)
        : m_fontDescriptionKey(t)
    { }

    bool isHashTableDeletedValue() const { return m_fontDescriptionKey.isHashTableDeletedValue(); }

    bool operator==(const FontPlatformDataCacheKey& other) const
    {
        return equalIgnoringCase(m_family, other.m_family) && m_fontDescriptionKey == other.m_fontDescriptionKey;
    }

    FontDescriptionFontDataCacheKey m_fontDescriptionKey;
    AtomicString m_family;
};

inline unsigned computeHash(const FontPlatformDataCacheKey& fontKey)
{
    return pairIntHash(CaseFoldingHash::hash(fontKey.m_family), fontKey.m_fontDescriptionKey.computeHash());
}

struct FontPlatformDataCacheKeyHash {
    static unsigned hash(const FontPlatformDataCacheKey& font)
    {
        return computeHash(font);
    }
         
    static bool equal(const FontPlatformDataCacheKey& a, const FontPlatformDataCacheKey& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FontPlatformDataCacheKeyTraits : WTF::SimpleClassHashTraits<FontPlatformDataCacheKey> { };

typedef HashMap<FontPlatformDataCacheKey, std::unique_ptr<FontPlatformData>, FontPlatformDataCacheKeyHash, FontPlatformDataCacheKeyTraits> FontPlatformDataCache;

static FontPlatformDataCache& fontPlatformDataCache()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<FontPlatformDataCache> cache;
    return cache;
#else
    WKC_DEFINE_STATIC_PTR(FontPlatformDataCache*, cache, 0);
    if (!cache)
        cache = new FontPlatformDataCache();
    return *cache;
#endif
}

static bool familyNameEqualIgnoringCase(const AtomicString& familyName, const char* reference, unsigned length)
{
    ASSERT(length > 0);
    ASSERT(familyName.length() == length);
    ASSERT(strlen(reference) == length);
    const AtomicStringImpl* familyNameImpl = familyName.impl();
    if (familyNameImpl->is8Bit())
        return equalIgnoringCase(familyNameImpl->characters8(), reinterpret_cast<const LChar*>(reference), length);
    return equalIgnoringCase(familyNameImpl->characters16(), reinterpret_cast<const LChar*>(reference), length);
}

template<size_t length>
static inline bool familyNameEqualIgnoringCase(const AtomicString& familyName, const char (&reference)[length])
{
    return familyNameEqualIgnoringCase(familyName, reference, length - 1);
}

static const AtomicString alternateFamilyName(const AtomicString& familyName)
{
    // Alias Courier and Courier New.
    // Alias Times and Times New Roman.
    // Alias Arial and Helvetica.
    switch (familyName.length()) {
    case 5:
        if (familyNameEqualIgnoringCase(familyName, "Arial"))
            return AtomicString("Helvetica", AtomicString::ConstructFromLiteral);
        if (familyNameEqualIgnoringCase(familyName, "Times"))
            return AtomicString("Times New Roman", AtomicString::ConstructFromLiteral);
        break;
    case 7:
        if (familyNameEqualIgnoringCase(familyName, "Courier"))
            return AtomicString("Courier New", AtomicString::ConstructFromLiteral);
        break;
    case 9:
        if (familyNameEqualIgnoringCase(familyName, "Helvetica"))
            return AtomicString("Arial", AtomicString::ConstructFromLiteral);
        break;
#if !OS(WINDOWS)
    // On Windows, Courier New (truetype font) is always present and
    // Courier is a bitmap font. So, we don't want to map Courier New to
    // Courier.
    case 11:
        if (familyNameEqualIgnoringCase(familyName, "Courier New"))
            return AtomicString("Courier", AtomicString::ConstructFromLiteral);
        break;
#endif // !OS(WINDOWS)
    case 15:
        if (familyNameEqualIgnoringCase(familyName, "Times New Roman"))
            return AtomicString("Times", AtomicString::ConstructFromLiteral);
        break;
#if OS(WINDOWS)
    // On Windows, bitmap fonts are blocked altogether so that we have to 
    // alias MS Sans Serif (bitmap font) -> Microsoft Sans Serif (truetype font)
    case 13:
        if (familyNameEqualIgnoringCase(familyName, "MS Sans Serif"))
            return AtomicString("Microsoft Sans Serif", AtomicString::ConstructFromLiteral);
        break;

    // Alias MS Serif (bitmap) -> Times New Roman (truetype font). There's no 
    // 'Microsoft Sans Serif-equivalent' for Serif.
    case 8:
        if (familyNameEqualIgnoringCase(familyName, "MS Serif"))
            return AtomicString("Times New Roman", AtomicString::ConstructFromLiteral);
        break;
#endif // OS(WINDOWS)

    }

    return nullAtom;
}

FontPlatformData* FontCache::getCachedFontPlatformData(const FontDescription& fontDescription,
                                                       const AtomicString& passedFamilyName,
                                                       bool checkingAlternateName)
{
#if PLATFORM(IOS)
    FontLocker fontLocker;
#endif
    
#if OS(WINDOWS) && ENABLE(OPENTYPE_VERTICAL)
    // Leading "@" in the font name enables Windows vertical flow flag for the font.
    // Because we do vertical flow by ourselves, we don't want to use the Windows feature.
    // IE disregards "@" regardless of the orientatoin, so we follow the behavior.
    const AtomicString& familyName = (passedFamilyName.isEmpty() || passedFamilyName[0] != '@') ?
        passedFamilyName : AtomicString(passedFamilyName.impl()->substring(1));
#else
    const AtomicString& familyName = passedFamilyName;
#endif

#if !PLATFORM(WKC)
    static bool initialized;
#else
    WKC_DEFINE_STATIC_BOOL(initialized, false);
#endif
    if (!initialized) {
        platformInit();
        initialized = true;
    }

    FontPlatformDataCacheKey key(familyName, fontDescription);

    auto addResult = fontPlatformDataCache().add(key, nullptr);
    FontPlatformDataCache::iterator it = addResult.iterator;
    if (addResult.isNewEntry) {
        it->value = createFontPlatformData(fontDescription, familyName);

        if (!it->value && !checkingAlternateName) {
            // We were unable to find a font.  We have a small set of fonts that we alias to other names,
            // e.g., Arial/Helvetica, Courier/Courier New, etc.  Try looking up the font under the aliased name.
            const AtomicString alternateName = alternateFamilyName(familyName);
            if (!alternateName.isNull()) {
                FontPlatformData* fontPlatformDataForAlternateName = getCachedFontPlatformData(fontDescription, alternateName, true);
                // Lookup the key in the hash table again as the previous iterator may have
                // been invalidated by the recursive call to getCachedFontPlatformData().
                it = fontPlatformDataCache().find(key);
                ASSERT(it != fontPlatformDataCache().end());
                if (fontPlatformDataForAlternateName)
                    it->value = std::make_unique<FontPlatformData>(*fontPlatformDataForAlternateName);
            }
        }
    }

    return it->value.get();
}

#if ENABLE(OPENTYPE_VERTICAL)
struct FontVerticalDataCacheKeyHash {
    static unsigned hash(const FontCache::FontFileKey& fontFileKey)
    {
        return PtrHash<const FontCache::FontFileKey*>::hash(&fontFileKey);
    }

    static bool equal(const FontCache::FontFileKey& a, const FontCache::FontFileKey& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FontVerticalDataCacheKeyTraits : WTF::GenericHashTraits<FontCache::FontFileKey> {
    static const bool emptyValueIsZero = true;
    static const bool needsDestruction = true;
    static const FontCache::FontFileKey& emptyValue()
    {
        static NeverDestroyed<FontCache::FontFileKey> key = nullAtom;
        return key;
    }
    static void constructDeletedValue(FontCache::FontFileKey& slot)
    {
        new (NotNull, &slot) FontCache::FontFileKey(HashTableDeletedValue);
    }
    static bool isDeletedValue(const FontCache::FontFileKey& value)
    {
        return value.isHashTableDeletedValue();
    }
};

typedef HashMap<FontCache::FontFileKey, RefPtr<OpenTypeVerticalData>, FontVerticalDataCacheKeyHash, FontVerticalDataCacheKeyTraits> FontVerticalDataCache;

FontVerticalDataCache& fontVerticalDataCacheInstance()
{
    static NeverDestroyed<FontVerticalDataCache> fontVerticalDataCache;
    return fontVerticalDataCache;
}

PassRefPtr<OpenTypeVerticalData> FontCache::getVerticalData(const FontFileKey& key, const FontPlatformData& platformData)
{
    FontVerticalDataCache& fontVerticalDataCache = fontVerticalDataCacheInstance();
    FontVerticalDataCache::iterator result = fontVerticalDataCache.find(key);
    if (result != fontVerticalDataCache.end())
        return result.get()->value;

    RefPtr<OpenTypeVerticalData> verticalData = OpenTypeVerticalData::create(platformData);
    if (!verticalData->isOpenType())
        verticalData = nullptr;
    fontVerticalDataCache.set(key, verticalData);
    return verticalData;
}
#endif

struct FontDataCacheKeyHash {
    static unsigned hash(const FontPlatformData& platformData)
    {
        return platformData.hash();
    }
         
    static bool equal(const FontPlatformData& a, const FontPlatformData& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FontDataCacheKeyTraits : WTF::GenericHashTraits<FontPlatformData> {
    static const bool emptyValueIsZero = true;
    static const FontPlatformData& emptyValue()
    {
#if !PLATFORM(WKC)
        static NeverDestroyed<FontPlatformData> key(0.f, false, false);
        return key;
#else
        WKC_DEFINE_STATIC_PTR(FontPlatformData*, key, 0);
        if (!key)
            key = new FontPlatformData(0.f, false, false);
        return *key;
#endif
    }
    static void constructDeletedValue(FontPlatformData& slot)
    {
        new (NotNull, &slot) FontPlatformData(HashTableDeletedValue);
    }
    static bool isDeletedValue(const FontPlatformData& value)
    {
        return value.isHashTableDeletedValue();
    }
};

typedef HashMap<FontPlatformData, RefPtr<Font>, FontDataCacheKeyHash, FontDataCacheKeyTraits> FontDataCache;

static FontDataCache& cachedFonts()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<FontDataCache> cache;
    return cache;
#else
    WKC_DEFINE_STATIC_PTR(FontDataCache*, cache, 0);
    if (!cache)
        cache = new FontDataCache();
    return *cache;
#endif
}


#if PLATFORM(IOS)
const unsigned cMaxInactiveFontData = 120;
const unsigned cTargetInactiveFontData = 100;
#else
const unsigned cMaxInactiveFontData = 225;
const unsigned cTargetInactiveFontData = 200;
#endif

const unsigned cMaxUnderMemoryPressureInactiveFontData = 50;
const unsigned cTargetUnderMemoryPressureInactiveFontData = 30;

RefPtr<Font> FontCache::fontForFamily(const FontDescription& fontDescription, const AtomicString& family, bool checkingAlternateName)
{
    if (!m_purgeTimer.isActive())
        m_purgeTimer.startOneShot(std::chrono::milliseconds::zero());

    if (auto* platformData = getCachedFontPlatformData(fontDescription, family, checkingAlternateName))
        return fontForPlatformData(*platformData);

    return nullptr;
}

Ref<Font> FontCache::fontForPlatformData(const FontPlatformData& platformData)
{
#if PLATFORM(IOS)
    FontLocker fontLocker;
#endif
    
    auto addResult = cachedFonts().add(platformData, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = Font::create(platformData);

    return *addResult.iterator->value;
}

void FontCache::purgeTimerFired()
{
    purgeInactiveFontDataIfNeeded();
}

void FontCache::purgeInactiveFontDataIfNeeded()
{
    bool underMemoryPressure = MemoryPressureHandler::singleton().isUnderMemoryPressure();
    unsigned inactiveFontDataLimit = underMemoryPressure ? cMaxUnderMemoryPressureInactiveFontData : cMaxInactiveFontData;

    if (cachedFonts().size() < inactiveFontDataLimit)
        return;
    unsigned inactiveCount = inactiveFontCount();
    if (inactiveCount <= inactiveFontDataLimit)
        return;

    unsigned targetFontDataLimit = underMemoryPressure ? cTargetUnderMemoryPressureInactiveFontData : cTargetInactiveFontData;
    purgeInactiveFontData(inactiveCount - targetFontDataLimit);
}

void FontCache::purgeInactiveFontData(unsigned purgeCount)
{
    pruneUnreferencedEntriesFromFontCascadeCache();
    pruneSystemFallbackFonts();

#if PLATFORM(IOS)
    FontLocker fontLocker;
#endif

    while (purgeCount) {
        Vector<RefPtr<Font>, 20> fontsToDelete;
        for (auto& font : cachedFonts().values()) {
#if !PLATFORM(WKC)
            if (!font->hasOneRef())
#else
            if (!font || !font->hasOneRef())
#endif
                continue;
            fontsToDelete.append(WTF::move(font));
            if (!--purgeCount)
                break;
        }
        // Fonts may ref other fonts so we loop until there are no changes.
        if (fontsToDelete.isEmpty())
            break;
        for (auto& font : fontsToDelete)
            cachedFonts().remove(font->platformData());
    };

    Vector<FontPlatformDataCacheKey> keysToRemove;
    keysToRemove.reserveInitialCapacity(fontPlatformDataCache().size());
    for (auto& entry : fontPlatformDataCache()) {
        if (entry.value && !cachedFonts().contains(*entry.value))
            keysToRemove.append(entry.key);
    }
    for (auto key : keysToRemove)
        fontPlatformDataCache().remove(key);

#if ENABLE(OPENTYPE_VERTICAL)
    FontVerticalDataCache& fontVerticalDataCache = fontVerticalDataCacheInstance();
    if (!fontVerticalDataCache.isEmpty()) {
        // Mark & sweep unused verticalData
        for (auto& verticalData : fontVerticalDataCache.values()) {
            if (verticalData)
                verticalData->m_inFontCache = false;
        }
        for (auto& font : cachedFonts().values()) {
            auto* verticalData = const_cast<OpenTypeVerticalData*>(font->verticalData());
            if (verticalData)
                verticalData->m_inFontCache = true;
        }
        Vector<FontFileKey> keysToRemove;
        keysToRemove.reserveInitialCapacity(fontVerticalDataCache.size());
        for (auto& it : fontVerticalDataCache) {
            if (!it.value || !it.value->m_inFontCache)
                keysToRemove.append(it.key);
        }
        for (auto& key : keysToRemove)
            fontVerticalDataCache.remove(key);
    }
#endif

    platformPurgeInactiveFontData();
}

size_t FontCache::fontCount()
{
    return cachedFonts().size();
}

size_t FontCache::inactiveFontCount()
{
#if PLATFORM(IOS)
    FontLocker fontLocker;
#endif
    unsigned count = 0;
    for (auto& font : cachedFonts().values()) {
#if !PLATFORM(WKC)
        if (font->hasOneRef())
#else
        if (font && font->hasOneRef())
#endif
            ++count;
    }
    return count;
}

#if !PLATFORM(WKC)
static HashSet<FontSelector*>* gClients;
#else
WKC_DEFINE_GLOBAL_PTR(HashSet<FontSelector*>*, gClients, 0);
#endif

void FontCache::addClient(FontSelector* client)
{
    if (!gClients)
        gClients = new HashSet<FontSelector*>;

    ASSERT(!gClients->contains(client));
    gClients->add(client);
}

void FontCache::removeClient(FontSelector* client)
{
    ASSERT(gClients);
    ASSERT(gClients->contains(client));

    gClients->remove(client);
}

#if !PLATFORM(WKC)
static unsigned short gGeneration = 0;
#else
WKC_DEFINE_GLOBAL_SHORT(gGeneration, 0);
#endif

unsigned short FontCache::generation()
{
    return gGeneration;
}

void FontCache::invalidate()
{
    if (!gClients) {
        ASSERT(fontPlatformDataCache().isEmpty());
        return;
    }

    fontPlatformDataCache().clear();
    invalidateFontCascadeCache();

    gGeneration++;

    Vector<Ref<FontSelector>> clients;
    clients.reserveInitialCapacity(gClients->size());
    for (auto it = gClients->begin(), end = gClients->end(); it != end; ++it)
        clients.uncheckedAppend(**it);

    for (unsigned i = 0; i < clients.size(); ++i)
        clients[i]->fontCacheInvalidated();

    purgeInactiveFontData();
}

#if !PLATFORM(COCOA)
RefPtr<Font> FontCache::similarFont(const FontDescription&)
{
    return nullptr;
}
#endif

} // namespace WebCore
