/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2014 Igalia S.L.
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

#ifndef BitmapTexturePool_h
#define BitmapTexturePool_h

#if USE(OPENGL_ES_2)
#define TEXMAP_OPENGL_ES_2
#endif

#include "BitmapTexture.h"
#include "IntRect.h"
#include "IntSize.h"
#include "Timer.h"
#include <wtf/CurrentTime.h>

#if USE(TEXTURE_MAPPER_GL)
#include "GraphicsContext3D.h"
#endif

namespace WebCore {

class TextureMapper;

struct BitmapTexturePoolEntry {
    explicit BitmapTexturePoolEntry(PassRefPtr<BitmapTexture> texture)
        : m_texture(texture)
    { }
    inline void markUsed() { m_timeLastUsed = monotonicallyIncreasingTime(); }
    static bool compareTimeLastUsed(const BitmapTexturePoolEntry& a, const BitmapTexturePoolEntry& b)
    {
        return a.m_timeLastUsed - b.m_timeLastUsed > 0;
    }

    RefPtr<BitmapTexture> m_texture;
    double m_timeLastUsed;
};

class BitmapTexturePool {
    WTF_MAKE_NONCOPYABLE(BitmapTexturePool);
    WTF_MAKE_FAST_ALLOCATED;
public:
    BitmapTexturePool();
#if USE(TEXTURE_MAPPER_GL)
    explicit BitmapTexturePool(PassRefPtr<GraphicsContext3D>);
#endif

    PassRefPtr<BitmapTexture> acquireTexture(const IntSize&);

private:
    void scheduleReleaseUnusedTextures();
    void releaseUnusedTexturesTimerFired();
    PassRefPtr<BitmapTexture> createTexture();

#if USE(TEXTURE_MAPPER_GL)
    RefPtr<GraphicsContext3D> m_context3D;
#endif

    Vector<BitmapTexturePoolEntry> m_textures;
    Timer m_releaseUnusedTexturesTimer;
};

}

#endif // BitmapTexturePool_h
