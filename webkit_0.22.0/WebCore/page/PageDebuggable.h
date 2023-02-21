/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef PageDebuggable_h
#define PageDebuggable_h

#if ENABLE(REMOTE_INSPECTOR)

#include <JavaScriptCore/inspector/remote/RemoteInspectorDebuggable.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class Page;

class PageDebuggable final : public Inspector::RemoteInspectorDebuggable {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(PageDebuggable);
public:
    PageDebuggable(Page&);
    ~PageDebuggable() { }

    virtual Inspector::RemoteInspectorDebuggable::DebuggableType type() const override { return Inspector::RemoteInspectorDebuggable::Web; }

    virtual String name() const override;
    virtual String url() const override;
    virtual bool hasLocalDebugger() const override;

    virtual void connect(Inspector::FrontendChannel*, bool isAutomaticInspection) override;
    virtual void disconnect(Inspector::FrontendChannel*) override;
    virtual void dispatchMessageFromRemoteFrontend(const String& message) override;
    virtual void setIndicating(bool) override;

private:
    Page& m_page;
    bool m_forcedDeveloperExtrasEnabled;
};

} // namespace WebCore

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // !defined(PageDebuggable_h)
