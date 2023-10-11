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

#ifndef MediaDeviceInfo_h
#define MediaDeviceInfo_h

#if ENABLE(MEDIA_STREAM)

#include "ContextDestructionObserver.h"
#include "ScriptWrappable.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaDeviceInfo : public RefCounted<MediaDeviceInfo>, public ScriptWrappable, public ContextDestructionObserver {
public:
    static Ref<MediaDeviceInfo> create(ScriptExecutionContext*, const String&, const String&, const String&, const String&);
    
    
    virtual ~MediaDeviceInfo() { }
    
    const String& label() const { return m_label; }
    const String& deviceId() const { return m_deviceId; }
    const String& groupId() const { return m_groupId; }
    const String& kind() const { return m_kind; }

    static const AtomicString& audioInputType();
    static const AtomicString& audioOutputType();
    static const AtomicString& videoInputType();

private:
    MediaDeviceInfo(ScriptExecutionContext*, const String&, const String&, const String&, const String&);

    const String m_label;
    const String m_deviceId;
    const String m_groupId;
    const String m_kind;
};

}

#endif

#endif /* MediaDeviceInfo_h */
