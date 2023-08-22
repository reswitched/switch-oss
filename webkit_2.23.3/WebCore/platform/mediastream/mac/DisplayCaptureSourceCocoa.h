/*
 * Copyright (C) 2017-2018 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(MEDIA_STREAM)

#include "CaptureDevice.h"
#include "ImageTransferSessionVT.h"
#include "RealtimeMediaSource.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RunLoop.h>
#include <wtf/text/WTFString.h>

typedef struct CGImage *CGImageRef;
typedef struct __CVBuffer *CVPixelBufferRef;
typedef struct __IOSurface *IOSurfaceRef;

namespace WTF {
class MediaTime;
}

namespace WebCore {

class CaptureDeviceInfo;
class ImageTransferSessionVT;
class PixelBufferConformerCV;

class DisplayCaptureSourceCocoa : public RealtimeMediaSource {
public:

protected:
    DisplayCaptureSourceCocoa(String&& name);
    virtual ~DisplayCaptureSourceCocoa();

    using DisplayFrameType = WTF::Variant<RetainPtr<CGImageRef>, RetainPtr<IOSurfaceRef>>;
    virtual DisplayFrameType generateFrame() = 0;

    virtual RealtimeMediaSourceSettings::DisplaySurfaceType surfaceType() const = 0;

    void startProducingData() override;
    void stopProducingData() override;

    Seconds elapsedTime();

    void settingsDidChange(OptionSet<RealtimeMediaSourceSettings::Flag>) override;

    IntSize frameSize() const;

private:

    bool isCaptureSource() const final { return true; }

    const RealtimeMediaSourceCapabilities& capabilities() final;
    const RealtimeMediaSourceSettings& settings() final;

    void emitFrame();

    Optional<RealtimeMediaSourceCapabilities> m_capabilities;
    Optional<RealtimeMediaSourceSettings> m_currentSettings;
    RealtimeMediaSourceSupportedConstraints m_supportedConstraints;

    MonotonicTime m_startTime { MonotonicTime::nan() };
    Seconds m_elapsedTime { 0_s };

    RetainPtr<CFMutableDictionaryRef> m_bufferAttributes;
    RunLoop::Timer<DisplayCaptureSourceCocoa> m_timer;

    std::unique_ptr<ImageTransferSessionVT> m_imageTransferSession;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
