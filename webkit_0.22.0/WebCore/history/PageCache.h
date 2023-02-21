/*
 * Copyright (C) 2007, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PageCache_h
#define PageCache_h

#include "HistoryItem.h"
#include "Timer.h"
#include <wtf/Forward.h>
#include <wtf/ListHashSet.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class CachedPage;
class Frame;
class Page;

enum class PruningReason { None, ProcessSuspended, MemoryPressure, ReachedMaxSize };

class PageCache {
    WTF_MAKE_NONCOPYABLE(PageCache); WTF_MAKE_FAST_ALLOCATED;
public:
    // Function to obtain the global page cache.
    WEBCORE_EXPORT static PageCache& singleton();

    bool canCache(Page&) const;

    // Used when memory is low to prune some cached pages.
    WEBCORE_EXPORT void pruneToSizeNow(unsigned maxSize, PruningReason);
    WEBCORE_EXPORT void setMaxSize(unsigned); // number of pages to cache.
    unsigned maxSize() const { return m_maxSize; }

    void addIfCacheable(HistoryItem&, Page*); // Prunes if maxSize() is exceeded.
    WEBCORE_EXPORT void remove(HistoryItem&);
    CachedPage* get(HistoryItem&, Page*);
    std::unique_ptr<CachedPage> take(HistoryItem&, Page*);

    void removeAllItemsForPage(Page&);

    unsigned pageCount() const { return m_items.size(); }
    WEBCORE_EXPORT unsigned frameCount() const;

    WEBCORE_EXPORT void markPagesForVisitedLinkStyleRecalc();
    // Will mark all cached pages associated with the given page as needing style recalc.
    void markPagesForFullStyleRecalc(Page&);
    void markPagesForDeviceOrPageScaleChanged(Page&);
    void markPagesForContentsSizeChanged(Page&);
#if ENABLE(VIDEO_TRACK)
    void markPagesForCaptionPreferencesChanged();
#endif

    bool shouldClearBackingStores() const { return m_shouldClearBackingStores; }
    void setShouldClearBackingStores(bool flag) { m_shouldClearBackingStores = flag; }

private:
    PageCache() = default; // Use singleton() instead.
    ~PageCache()  = delete; // Make sure nobody accidentally calls delete -- WebCore does not delete singletons.

    static bool canCachePageContainingThisFrame(Frame&);

    void prune(PruningReason);

    ListHashSet<RefPtr<HistoryItem>> m_items;
    unsigned m_maxSize {0};
    bool m_shouldClearBackingStores {false};

    friend class WTF::NeverDestroyed<PageCache>;
};

} // namespace WebCore

#endif // PageCache_h
