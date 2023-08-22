/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef CanvasRenderingContext_h
#define CanvasRenderingContext_h

#include "GraphicsLayer.h"
#include "HTMLCanvasElement.h"
#include "ScriptWrappable.h"
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CanvasPattern;
class HTMLCanvasElement;
class HTMLImageElement;
class HTMLVideoElement;
class URL;
class WebGLObject;

class CanvasRenderingContext : public ScriptWrappable {
    WTF_MAKE_NONCOPYABLE(CanvasRenderingContext); WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~CanvasRenderingContext() { }

    void ref() { m_canvas->ref(); }
    void deref() { m_canvas->deref(); }
    HTMLCanvasElement* canvas() const { return m_canvas; }

    virtual bool is2d() const { return false; }
    virtual bool isWebGL1() const { return false; }
    virtual bool isWebGL2() const { return false; }
    bool is3d() const { return isWebGL1() || isWebGL1(); }
    virtual bool isAccelerated() const { return false; }

    virtual void paintRenderingResultsToCanvas() {}
    virtual PlatformLayer* platformLayer() const { return 0; }

protected:
    CanvasRenderingContext(HTMLCanvasElement*);
    bool wouldTaintOrigin(const CanvasPattern*);
    bool wouldTaintOrigin(const HTMLCanvasElement*);
    bool wouldTaintOrigin(const HTMLImageElement*);
    bool wouldTaintOrigin(const HTMLVideoElement*);
    bool wouldTaintOrigin(const URL&);

    template<class T> void checkOrigin(const T* arg)
    {
        if (wouldTaintOrigin(arg))
            canvas()->setOriginTainted();
    }
    void checkOrigin(const URL&);

private:
    HTMLCanvasElement* m_canvas;
};

} // namespace WebCore

#endif
