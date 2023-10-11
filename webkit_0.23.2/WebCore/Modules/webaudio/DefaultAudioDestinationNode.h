/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DefaultAudioDestinationNode_h
#define DefaultAudioDestinationNode_h

#include "AudioDestination.h"
#include "AudioDestinationNode.h"
#include <memory>

namespace WebCore {

class AudioContext;
    
class DefaultAudioDestinationNode : public AudioDestinationNode {
public:
    static Ref<DefaultAudioDestinationNode> create(AudioContext* context)
    {
        return adoptRef(*new DefaultAudioDestinationNode(context));     
    }

    virtual ~DefaultAudioDestinationNode();
    
    // AudioNode   
    virtual void initialize() override;
    virtual void uninitialize() override;
    virtual void setChannelCount(unsigned long, ExceptionCode&) override;

    // AudioDestinationNode
    virtual void enableInput(const String& inputDeviceId) override;
    virtual void startRendering() override;
    virtual void resume(std::function<void()>) override;
    virtual void suspend(std::function<void()>) override;
    virtual void close(std::function<void()>) override;
    virtual unsigned long maxChannelCount() const override;
    virtual bool isPlaying() override;

private:
    explicit DefaultAudioDestinationNode(AudioContext*);
    void createDestination();

    std::unique_ptr<AudioDestination> m_destination;
    String m_inputDeviceId;
    unsigned m_numberOfInputChannels;
};

} // namespace WebCore

#endif // DefaultAudioDestinationNode_h
