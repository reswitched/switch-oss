/*
 * Copyright (C) 2006 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "config.h"
#include "EmptyClients.h"

#include "ColorChooser.h"
#include "DatabaseProvider.h"
#include "DocumentLoader.h"
#include "FileChooser.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameNetworkingContext.h"
#include "HTMLFormElement.h"
#include "IDBFactoryBackendInterface.h"
#include "PageConfiguration.h"
#include "StorageArea.h"
#include "StorageNamespace.h"
#include "StorageNamespaceProvider.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class EmptyDatabaseProvider final : public DatabaseProvider {
#if ENABLE(INDEXED_DATABASE)
    virtual RefPtr<IDBFactoryBackendInterface> createIDBFactoryBackend() { return nullptr; }
#endif
};

class EmptyStorageNamespaceProvider final : public StorageNamespaceProvider {
    struct EmptyStorageArea : public StorageArea {
        virtual unsigned length() override { return 0; }
        virtual String key(unsigned) override { return String(); }
        virtual String item(const String&) override { return String(); }
        virtual void setItem(Frame*, const String&, const String&, bool&) override { }
        virtual void removeItem(Frame*, const String&) override { }
        virtual void clear(Frame*) override { }
        virtual bool contains(const String&) override { return false; }
        virtual bool canAccessStorage(Frame*) override { return false; }
        virtual StorageType storageType() const override { return LocalStorage; }
        virtual size_t memoryBytesUsedByCache() override { return 0; }
        SecurityOrigin& securityOrigin() override { return SecurityOrigin::createUnique(); }
    };

    struct EmptyStorageNamespace final : public StorageNamespace {
        virtual PassRefPtr<StorageArea> storageArea(PassRefPtr<SecurityOrigin>) override { return adoptRef(new EmptyStorageArea); }
        virtual PassRefPtr<StorageNamespace> copy(Page*) override { return adoptRef(new EmptyStorageNamespace); }
    };

    virtual RefPtr<StorageNamespace> createSessionStorageNamespace(Page&, unsigned) override
    {
        return adoptRef(new EmptyStorageNamespace);
    }

    virtual RefPtr<StorageNamespace> createLocalStorageNamespace(unsigned) override
    {
        return adoptRef(new EmptyStorageNamespace);
    }

    virtual RefPtr<StorageNamespace> createTransientLocalStorageNamespace(SecurityOrigin&, unsigned) override
    {
        return adoptRef(new EmptyStorageNamespace);
    }
};

class EmptyVisitedLinkStore : public VisitedLinkStore {
    virtual bool isLinkVisited(Page&, LinkHash, const URL&, const AtomicString&) override { return false; }
    virtual void addVisitedLink(Page&, LinkHash) override { }
};

void fillWithEmptyClients(PageConfiguration& pageConfiguration)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<EmptyChromeClient> dummyChromeClient;
    pageConfiguration.chromeClient = &dummyChromeClient.get();

#if ENABLE(CONTEXT_MENUS)
    static NeverDestroyed<EmptyContextMenuClient> dummyContextMenuClient;
    pageConfiguration.contextMenuClient = &dummyContextMenuClient.get();
#endif

#if ENABLE(DRAG_SUPPORT)
    static NeverDestroyed<EmptyDragClient> dummyDragClient;
    pageConfiguration.dragClient = &dummyDragClient.get();
#endif

    static NeverDestroyed<EmptyEditorClient> dummyEditorClient;
    pageConfiguration.editorClient = &dummyEditorClient.get();

    static NeverDestroyed<EmptyInspectorClient> dummyInspectorClient;
    pageConfiguration.inspectorClient = &dummyInspectorClient.get();

    static NeverDestroyed<EmptyFrameLoaderClient> dummyFrameLoaderClient;
    pageConfiguration.loaderClientForMainFrame = &dummyFrameLoaderClient.get();

    static NeverDestroyed<EmptyProgressTrackerClient> dummyProgressTrackerClient;
    pageConfiguration.progressTrackerClient = &dummyProgressTrackerClient.get();

    static NeverDestroyed<EmptyDiagnosticLoggingClient> dummyDiagnosticLoggingClient;
    pageConfiguration.diagnosticLoggingClient = &dummyDiagnosticLoggingClient.get();
#else
    WKC_DEFINE_STATIC_PTR(EmptyChromeClient*, dummyChromeClient, 0);
    if (!dummyChromeClient)
        dummyChromeClient = new EmptyChromeClient();
    pageConfiguration.chromeClient = dummyChromeClient;

#if ENABLE(CONTEXT_MENUS)
    WKC_DEFINE_STATIC_PTR(EmptyContextMenuClient*, dummyContextMenuClient, 0);
    if (!dummyContextMenuClient)
        dummyContextMenuClient = new EmptyContextMenuClient();
    pageConfiguration.contextMenuClient = dummyContextMenuClient;
#endif

#if ENABLE(DRAG_SUPPORT)
    WKC_DEFINE_STATIC_PTR(EmptyDragClient*, dummyDragClient, 0);
    if (!dummyDragClient)
        dummyDragClient = new EmptyDragClient();
    pageConfiguration.dragClient = dummyDragClient;
#endif

    WKC_DEFINE_STATIC_PTR(EmptyEditorClient*, dummyEditorClient, 0);
    if (!dummyEditorClient)
        dummyEditorClient = new EmptyEditorClient();
    pageConfiguration.editorClient = dummyEditorClient;

    WKC_DEFINE_STATIC_PTR(EmptyInspectorClient*, dummyInspectorClient, 0);
    if (!dummyInspectorClient)
        dummyInspectorClient = new EmptyInspectorClient();
    pageConfiguration.inspectorClient = dummyInspectorClient;

    WKC_DEFINE_STATIC_PTR(EmptyFrameLoaderClient*, dummyFrameLoaderClient, 0);
    if (!dummyFrameLoaderClient)
        dummyFrameLoaderClient = new EmptyFrameLoaderClient();
    pageConfiguration.loaderClientForMainFrame = dummyFrameLoaderClient;

    WKC_DEFINE_STATIC_PTR(EmptyProgressTrackerClient*, dummyProgressTrackerClient, 0);
    if (!dummyProgressTrackerClient)
        dummyProgressTrackerClient = new EmptyProgressTrackerClient();
    pageConfiguration.progressTrackerClient = dummyProgressTrackerClient;

    WKC_DEFINE_STATIC_PTR(EmptyDiagnosticLoggingClient*, dummyDiagnosticLoggingClient, 0);
    if (!dummyDiagnosticLoggingClient)
        dummyDiagnosticLoggingClient = new EmptyDiagnosticLoggingClient();
    pageConfiguration.diagnosticLoggingClient = dummyDiagnosticLoggingClient;
#endif

    pageConfiguration.databaseProvider = adoptRef(new EmptyDatabaseProvider);
    pageConfiguration.storageNamespaceProvider = adoptRef(new EmptyStorageNamespaceProvider);
    pageConfiguration.visitedLinkStore = adoptRef(new EmptyVisitedLinkStore);
}

class EmptyPopupMenu : public PopupMenu {
public:
    virtual void show(const IntRect&, FrameView*, int) { }
    virtual void hide() { }
    virtual void updateFromElement() { }
    virtual void disconnectClient() { }
};

class EmptySearchPopupMenu : public SearchPopupMenu {
public:
    virtual PopupMenu* popupMenu() { return m_popup.get(); }
    virtual void saveRecentSearches(const AtomicString&, const Vector<RecentSearch>&) { }
    virtual void loadRecentSearches(const AtomicString&, Vector<RecentSearch>&) { }
    virtual bool enabled() { return false; }

private:
    RefPtr<EmptyPopupMenu> m_popup;
};

PassRefPtr<PopupMenu> EmptyChromeClient::createPopupMenu(PopupMenuClient*) const
{
    return adoptRef(new EmptyPopupMenu());
}

PassRefPtr<SearchPopupMenu> EmptyChromeClient::createSearchPopupMenu(PopupMenuClient*) const
{
    return adoptRef(new EmptySearchPopupMenu());
}

#if ENABLE(INPUT_TYPE_COLOR)
std::unique_ptr<ColorChooser> EmptyChromeClient::createColorChooser(ColorChooserClient*, const Color&)
{
    return nullptr;
}
#endif

void EmptyChromeClient::runOpenPanel(Frame*, PassRefPtr<FileChooser>)
{
}

void EmptyFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String&, FramePolicyFunction)
{
}

void EmptyFrameLoaderClient::dispatchDecidePolicyForNavigationAction(const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, FramePolicyFunction)
{
}

void EmptyFrameLoaderClient::dispatchWillSendSubmitEvent(PassRefPtr<FormState>)
{
}

void EmptyFrameLoaderClient::dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction)
{
}

PassRefPtr<DocumentLoader> EmptyFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    return DocumentLoader::create(request, substituteData);
}

PassRefPtr<Frame> EmptyFrameLoaderClient::createFrame(const URL&, const String&, HTMLFrameOwnerElement*, const String&, bool, int, int)
{
    return 0;
}

#if ENABLE(NETSCAPE_PLUGIN_API)
PassRefPtr<Widget> EmptyFrameLoaderClient::createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&, const Vector<String>&, const String&, bool)
{
    return 0;
}

void EmptyFrameLoaderClient::recreatePlugin(Widget*)
{
}
#endif

PassRefPtr<Widget> EmptyFrameLoaderClient::createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL&, const Vector<String>&, const Vector<String>&)
{
    return 0;
}

PassRefPtr<FrameNetworkingContext> EmptyFrameLoaderClient::createNetworkingContext()
{
    return PassRefPtr<FrameNetworkingContext>();
}

void EmptyTextCheckerClient::requestCheckingOfString(PassRefPtr<TextCheckingRequest>)
{
}

void EmptyEditorClient::registerUndoStep(PassRefPtr<UndoStep>)
{
}

void EmptyEditorClient::registerRedoStep(PassRefPtr<UndoStep>)
{
}

#if ENABLE(CONTEXT_MENUS)
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
std::unique_ptr<ContextMenu> EmptyContextMenuClient::customizeMenu(std::unique_ptr<ContextMenu>)
{
    return nullptr;
}
#endif
#endif

}
