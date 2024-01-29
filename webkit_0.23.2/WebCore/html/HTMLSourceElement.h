/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLSourceElement_h
#define HTMLSourceElement_h

#include "HTMLElement.h"
#include "MediaList.h"
#include "Timer.h"

namespace WebCore {

class HTMLSourceElement final : public HTMLElement, public ActiveDOMObject {
public:
    static Ref<HTMLSourceElement> create(const QualifiedName&, Document&);

    String media() const;
    String type() const;
    void setSrc(const String&);    
    void setMedia(const String&);
    void setType(const String&);
    
    void scheduleErrorEvent();
    void cancelPendingErrorEvent();

    MediaQuerySet* mediaQuerySet() const { return m_mediaQuerySet.get(); }

private:
    HTMLSourceElement(const QualifiedName&, Document&);
    
    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;
    virtual void removedFrom(ContainerNode&) override;
    virtual bool isURLAttribute(const Attribute&) const override;

    // ActiveDOMObject.
    const char* activeDOMObjectName() const override;
    bool canSuspendForPageCache() const override;
    void suspend(ReasonForSuspension) override;
    void resume() override;
    void stop() override;

    void parseAttribute(const QualifiedName&, const AtomicString&) override;

    void errorEventTimerFired();

    Timer m_errorEventTimer;
    bool m_shouldRescheduleErrorEventOnResume { false };
    RefPtr<MediaQuerySet> m_mediaQuerySet;
};

} //namespace

#endif

