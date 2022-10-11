/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#ifndef DOMApplicationCache_h
#define DOMApplicationCache_h

#include "ApplicationCacheHost.h"
#include "DOMWindowProperty.h"
#include "EventTarget.h"
#include "ScriptWrappable.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class Frame;
class URL;

class DOMApplicationCache final : public ScriptWrappable, public RefCounted<DOMApplicationCache>, public EventTargetWithInlineData, public DOMWindowProperty {
public:
    static Ref<DOMApplicationCache> create(Frame* frame) { return adoptRef(*new DOMApplicationCache(frame)); }
    virtual ~DOMApplicationCache() { ASSERT(!m_frame); }

    virtual void disconnectFrameForPageCache() override;
    virtual void reconnectFrameFromPageCache(Frame*) override;
    virtual void willDestroyGlobalObjectInFrame() override;

    unsigned short status() const;
    void update(ExceptionCode&);
    void swapCache(ExceptionCode&);
    void abort();

    using RefCounted<DOMApplicationCache>::ref;
    using RefCounted<DOMApplicationCache>::deref;

    virtual EventTargetInterface eventTargetInterface() const override { return DOMApplicationCacheEventTargetInterfaceType; }
    virtual ScriptExecutionContext* scriptExecutionContext() const override;

    static const AtomicString& toEventType(ApplicationCacheHost::EventID id);

private:
    explicit DOMApplicationCache(Frame*);

    virtual void refEventTarget() override { ref(); }
    virtual void derefEventTarget() override { deref(); }

    ApplicationCacheHost* applicationCacheHost() const;
};

} // namespace WebCore

#endif // DOMApplicationCache_h
