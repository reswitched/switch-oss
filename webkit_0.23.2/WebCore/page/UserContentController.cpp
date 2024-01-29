/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "UserContentController.h"

#include "DOMWrapperWorld.h"
#include "Document.h"
#include "MainFrame.h"
#include "Page.h"
#include "ResourceLoadInfo.h"
#include "UserScript.h"
#include "UserStyleSheet.h"

#if ENABLE(USER_MESSAGE_HANDLERS)
#include "UserMessageHandlerDescriptor.h"
#endif

#if ENABLE(CONTENT_EXTENSIONS)
#include "ContentExtensionCompiler.h"
#include "ContentExtensionsBackend.h"
#endif

namespace WebCore {

Ref<UserContentController> UserContentController::create()
{
    return adoptRef(*new UserContentController);
}

UserContentController::UserContentController()
{
}

UserContentController::~UserContentController()
{
}

void UserContentController::addPage(Page& page)
{
    ASSERT(!m_pages.contains(&page));
    m_pages.add(&page);
}

void UserContentController::removePage(Page& page)
{
    ASSERT(m_pages.contains(&page));
    m_pages.remove(&page);
}

void UserContentController::addUserScript(DOMWrapperWorld& world, std::unique_ptr<UserScript> userScript)
{
    if (!m_userScripts)
        m_userScripts = std::make_unique<UserScriptMap>();

    auto& scriptsInWorld = m_userScripts->add(&world, nullptr).iterator->value;
    if (!scriptsInWorld)
        scriptsInWorld = std::make_unique<UserScriptVector>();
    scriptsInWorld->append(WTF::move(userScript));
}

void UserContentController::removeUserScript(DOMWrapperWorld& world, const URL& url)
{
    if (!m_userScripts)
        return;

    auto it = m_userScripts->find(&world);
    if (it == m_userScripts->end())
        return;

    auto scripts = it->value.get();
    for (int i = scripts->size() - 1; i >= 0; --i) {
        if (scripts->at(i)->url() == url)
            scripts->remove(i);
    }

    if (scripts->isEmpty())
        m_userScripts->remove(it);
}

void UserContentController::removeUserScripts(DOMWrapperWorld& world)
{
    if (!m_userScripts)
        return;

    m_userScripts->remove(&world);
}

void UserContentController::addUserStyleSheet(DOMWrapperWorld& world, std::unique_ptr<UserStyleSheet> userStyleSheet, UserStyleInjectionTime injectionTime)
{
    if (!m_userStyleSheets)
        m_userStyleSheets = std::make_unique<UserStyleSheetMap>();

    auto& styleSheetsInWorld = m_userStyleSheets->add(&world, nullptr).iterator->value;
    if (!styleSheetsInWorld)
        styleSheetsInWorld = std::make_unique<UserStyleSheetVector>();
    styleSheetsInWorld->append(WTF::move(userStyleSheet));

    if (injectionTime == InjectInExistingDocuments)
        invalidateInjectedStyleSheetCacheInAllFrames();
}

void UserContentController::removeUserStyleSheet(DOMWrapperWorld& world, const URL& url)
{
    if (!m_userStyleSheets)
        return;

    auto it = m_userStyleSheets->find(&world);
    if (it == m_userStyleSheets->end())
        return;

    auto& stylesheets = *it->value;

    bool sheetsChanged = false;
    for (int i = stylesheets.size() - 1; i >= 0; --i) {
        if (stylesheets[i]->url() == url) {
            stylesheets.remove(i);
            sheetsChanged = true;
        }
    }

    if (!sheetsChanged)
        return;

    if (stylesheets.isEmpty())
        m_userStyleSheets->remove(it);

    invalidateInjectedStyleSheetCacheInAllFrames();
}

void UserContentController::removeUserStyleSheets(DOMWrapperWorld& world)
{
    if (!m_userStyleSheets)
        return;

    if (!m_userStyleSheets->remove(&world))
        return;

    invalidateInjectedStyleSheetCacheInAllFrames();
}

#if ENABLE(USER_MESSAGE_HANDLERS)
void UserContentController::addUserMessageHandlerDescriptor(UserMessageHandlerDescriptor& descriptor)
{
    if (!m_userMessageHandlerDescriptors)
        m_userMessageHandlerDescriptors = std::make_unique<UserMessageHandlerDescriptorMap>();

    m_userMessageHandlerDescriptors->add(std::make_pair(descriptor.name(), &descriptor.world()), &descriptor);
}

void UserContentController::removeUserMessageHandlerDescriptor(UserMessageHandlerDescriptor& descriptor)
{
    if (!m_userMessageHandlerDescriptors)
        return;

    m_userMessageHandlerDescriptors->remove(std::make_pair(descriptor.name(), &descriptor.world()));
}
#endif

#if ENABLE(CONTENT_EXTENSIONS)
void UserContentController::addUserContentExtension(const String& name, RefPtr<ContentExtensions::CompiledContentExtension> contentExtension)
{
    if (!m_contentExtensionBackend)
        m_contentExtensionBackend = std::make_unique<ContentExtensions::ContentExtensionsBackend>();
    
    m_contentExtensionBackend->addContentExtension(name, contentExtension);
}

void UserContentController::removeUserContentExtension(const String& name)
{
    if (!m_contentExtensionBackend)
        return;

    m_contentExtensionBackend->removeContentExtension(name);
}

void UserContentController::removeAllUserContentExtensions()
{
    if (!m_contentExtensionBackend)
        return;

    m_contentExtensionBackend->removeAllContentExtensions();
}

void UserContentController::processContentExtensionRulesForLoad(Page& page, ResourceRequest& request, ResourceType resourceType, DocumentLoader& initiatingDocumentLoader)
{
    if (!m_contentExtensionBackend)
        return;

    if (!page.userContentExtensionsEnabled())
        return;

    m_contentExtensionBackend->processContentExtensionRulesForLoad(request, resourceType, initiatingDocumentLoader);
}

Vector<ContentExtensions::Action> UserContentController::actionsForResourceLoad(Page& page, const ResourceLoadInfo& resourceLoadInfo)
{
    if (!m_contentExtensionBackend)
        return Vector<ContentExtensions::Action>();
    
    if (!page.userContentExtensionsEnabled())
        return Vector<ContentExtensions::Action>();

    return m_contentExtensionBackend->actionsForResourceLoad(resourceLoadInfo);
}
#endif

void UserContentController::removeAllUserContent()
{
    m_userScripts = nullptr;

    if (m_userStyleSheets) {
        m_userStyleSheets = nullptr;
        invalidateInjectedStyleSheetCacheInAllFrames();
    }
}

void UserContentController::invalidateInjectedStyleSheetCacheInAllFrames()
{
    for (auto& page : m_pages) {
        for (Frame* frame = &page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
            frame->document()->styleSheetCollection().invalidateInjectedStyleSheetCache();
            frame->document()->styleResolverChanged(DeferRecalcStyle);
        }
    }
}

} // namespace WebCore
