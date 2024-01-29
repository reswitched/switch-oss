/*
 * Copyright (C) 2013 Apple Inc. All Rights Reserved.
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

#if ENABLE(REMOTE_INSPECTOR)

#ifndef RemoteInspector_h
#define RemoteInspector_h

#if OS(IOS)

#import "RemoteInspectorXPCConnection.h"
#import <wtf/Forward.h>
#import <wtf/HashMap.h>
#import <wtf/RetainPtr.h>

OBJC_CLASS NSDictionary;
OBJC_CLASS NSString;

namespace Inspector {

class RemoteInspectorClient;
class RemoteInspectorDebuggable;
class RemoteInspectorDebuggableConnection;
struct RemoteInspectorDebuggableInfo;

class JS_EXPORT_PRIVATE RemoteInspector final : public RemoteInspectorXPCConnection::Client {
public:
    static void startDisabled();
    static RemoteInspector& singleton();
    friend class NeverDestroyed<RemoteInspector>;

    void registerDebuggable(RemoteInspectorDebuggable*);
    void unregisterDebuggable(RemoteInspectorDebuggable*);
    void updateDebuggable(RemoteInspectorDebuggable*);
    void updateDebuggableAutomaticInspectCandidate(RemoteInspectorDebuggable*);
    void sendMessageToRemoteFrontend(unsigned identifier, const String& message);
    void setupFailed(unsigned identifier);
    void setupCompleted(unsigned identifier);
    bool waitingForAutomaticInspection(unsigned identifier);

    bool enabled() const { return m_enabled; }
    bool hasActiveDebugSession() const { return m_hasActiveDebugSession; }

    void start();
    void stop();

    bool hasParentProcessInformation() const { return m_parentProcessIdentifier != 0; }
    pid_t parentProcessIdentifier() const { return m_parentProcessIdentifier; }
    RetainPtr<CFDataRef> parentProcessAuditData() const { return m_parentProcessAuditData; }
    void setParentProcessInformation(pid_t, RetainPtr<CFDataRef> auditData);
    void setParentProcessInfomationIsDelayed();

private:
    RemoteInspector();

    unsigned nextAvailableIdentifier();

    enum class StopSource { API, XPCMessage };
    void stopInternal(StopSource);

    void setupXPCConnectionIfNeeded();

    NSDictionary *listingForDebuggable(const RemoteInspectorDebuggableInfo&) const;
    void pushListingNow();
    void pushListingSoon();

    void updateHasActiveDebugSession();

    void sendAutomaticInspectionCandidateMessage();

    virtual void xpcConnectionReceivedMessage(RemoteInspectorXPCConnection*, NSString *messageName, NSDictionary *userInfo) override;
    virtual void xpcConnectionFailed(RemoteInspectorXPCConnection*) override;
    virtual void xpcConnectionUnhandledMessage(RemoteInspectorXPCConnection*, xpc_object_t) override;

    void receivedSetupMessage(NSDictionary *userInfo);
    void receivedDataMessage(NSDictionary *userInfo);
    void receivedDidCloseMessage(NSDictionary *userInfo);
    void receivedGetListingMessage(NSDictionary *userInfo);
    void receivedIndicateMessage(NSDictionary *userInfo);
    void receivedProxyApplicationSetupMessage(NSDictionary *userInfo);
    void receivedConnectionDiedMessage(NSDictionary *userInfo);
    void receivedAutomaticInspectionConfigurationMessage(NSDictionary *userInfo);
    void receivedAutomaticInspectionRejectMessage(NSDictionary *userInfo);

    static bool startEnabled;

    // Debuggables can be registered from any thread at any time.
    // Any debuggable can send messages over the XPC connection.
    // So lock access to all maps and state as they can change
    // from any thread.
    std::mutex m_mutex;

    HashMap<unsigned, std::pair<RemoteInspectorDebuggable*, RemoteInspectorDebuggableInfo>> m_debuggableMap;
    HashMap<unsigned, RefPtr<RemoteInspectorDebuggableConnection>> m_connectionMap;
    RefPtr<RemoteInspectorXPCConnection> m_xpcConnection;

    dispatch_queue_t m_xpcQueue;
    unsigned m_nextAvailableIdentifier;
    int m_notifyToken;
    bool m_enabled;
    bool m_hasActiveDebugSession;
    bool m_pushScheduled;

    pid_t m_parentProcessIdentifier;
    RetainPtr<CFDataRef> m_parentProcessAuditData;
    bool m_shouldSendParentProcessInformation;
    bool m_automaticInspectionEnabled;
    bool m_automaticInspectionPaused;
    unsigned m_automaticInspectionCandidateIdentifier;
};

} // namespace Inspector

#endif // OS(IOS)

#endif // RemoteInspector_h

#endif // ENABLE(REMOTE_INSPECTOR)
