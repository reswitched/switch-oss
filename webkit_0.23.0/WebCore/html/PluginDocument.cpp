/*
 * Copyright (C) 2006, 2008, 2013 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PluginDocument.h"

#include "DocumentLoader.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLEmbedElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RawDataDocumentParser.h"
#include "RenderEmbeddedObject.h"
#include "Settings.h"

namespace WebCore {
    
using namespace HTMLNames;

// FIXME: Share more code with MediaDocumentParser.
class PluginDocumentParser final : public RawDataDocumentParser {
public:
    static Ref<PluginDocumentParser> create(PluginDocument& document)
    {
        return adoptRef(*new PluginDocumentParser(document));
    }

private:
    PluginDocumentParser(Document& document)
        : RawDataDocumentParser(document)
        , m_embedElement(0)
    {
    }

    virtual void appendBytes(DocumentWriter&, const char*, size_t) override;

    void createDocumentStructure();

    HTMLEmbedElement* m_embedElement;
};

void PluginDocumentParser::createDocumentStructure()
{
    Ref<Element> rootElement = document()->createElement(htmlTag, false);
    document()->appendChild(rootElement.copyRef(), IGNORE_EXCEPTION);
    downcast<HTMLHtmlElement>(rootElement.get()).insertedByParser();

    if (document()->frame())
        document()->frame()->injectUserScripts(InjectAtDocumentStart);

#if PLATFORM(IOS)
    // Should not be able to zoom into standalone plug-in documents.
    document()->processViewport(ASCIILiteral("user-scalable=no"), ViewportArguments::PluginDocument);
#endif

    Ref<Element> body = document()->createElement(bodyTag, false);
    body->setAttribute(marginwidthAttr, AtomicString("0", AtomicString::ConstructFromLiteral));
    body->setAttribute(marginheightAttr, AtomicString("0", AtomicString::ConstructFromLiteral));
#if PLATFORM(IOS)
    body->setAttribute(styleAttr, AtomicString("background-color: rgb(217,224,233)", AtomicString::ConstructFromLiteral));
#else
    body->setAttribute(styleAttr, AtomicString("background-color: rgb(38,38,38)", AtomicString::ConstructFromLiteral));
#endif

    rootElement->appendChild(body.copyRef(), IGNORE_EXCEPTION);
        
    Ref<Element> embedElement = document()->createElement(embedTag, false);
        
    m_embedElement = downcast<HTMLEmbedElement>(embedElement.ptr());
    m_embedElement->setAttribute(widthAttr, "100%");
    m_embedElement->setAttribute(heightAttr, "100%");
    
    m_embedElement->setAttribute(nameAttr, "plugin");
    m_embedElement->setAttribute(srcAttr, document()->url().string());
    
    DocumentLoader* loader = document()->loader();
    ASSERT(loader);
    if (loader)
        m_embedElement->setAttribute(typeAttr, loader->writer().mimeType());

    downcast<PluginDocument>(*document()).setPluginElement(m_embedElement);

    body->appendChild(WTF::move(embedElement), IGNORE_EXCEPTION);
}

void PluginDocumentParser::appendBytes(DocumentWriter&, const char*, size_t)
{
    if (m_embedElement)
        return;

    createDocumentStructure();

    Frame* frame = document()->frame();
    if (!frame)
        return;

    document()->updateLayout();

    // Below we assume that renderer->widget() to have been created by
    // document()->updateLayout(). However, in some cases, updateLayout() will 
    // recurse too many times and delay its post-layout tasks (such as creating
    // the widget). Here we kick off the pending post-layout tasks so that we
    // can synchronously redirect data to the plugin.
    frame->view()->flushAnyPendingPostLayoutTasks();

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (RenderWidget* renderer = m_embedElement->renderWidget()) {
        if (Widget* widget = renderer->widget()) {
            frame->loader().client().redirectDataToPlugin(widget);
            // In a plugin document, the main resource is the plugin. If we have a null widget, that means
            // the loading of the plugin was cancelled, which gives us a null mainResourceLoader(), so we
            // need to have this call in a null check of the widget or of mainResourceLoader().
            frame->loader().activeDocumentLoader()->setMainResourceDataBufferingPolicy(DoNotBufferData);
        }
    }
#endif
}

PluginDocument::PluginDocument(Frame* frame, const URL& url)
    : HTMLDocument(frame, url, PluginDocumentClass)
    , m_shouldLoadPluginManually(true)
{
    setCompatibilityMode(DocumentCompatibilityMode::QuirksMode);
    lockCompatibilityMode();
}

Ref<DocumentParser> PluginDocument::createParser()
{
    return PluginDocumentParser::create(*this);
}

Widget* PluginDocument::pluginWidget()
{
    if (m_pluginElement && m_pluginElement->renderer())
        return downcast<RenderEmbeddedObject>(*m_pluginElement->renderer()).widget();
    return nullptr;
}

void PluginDocument::setPluginElement(PassRefPtr<HTMLPlugInElement> element)
{
    m_pluginElement = element;
}

void PluginDocument::detachFromPluginElement()
{
    // Release the plugin Element so that we don't have a circular reference.
    m_pluginElement = nullptr;
#if ENABLE(NETSCAPE_PLUGIN_API)
    frame()->loader().client().redirectDataToPlugin(nullptr);
#endif
}

void PluginDocument::cancelManualPluginLoad()
{
    // PluginDocument::cancelManualPluginLoad should only be called once, but there are issues
    // with how many times we call beforeload on object elements. <rdar://problem/8441094>.
    if (!shouldLoadPluginManually())
        return;

    DocumentLoader* documentLoader = frame()->loader().activeDocumentLoader();
    documentLoader->cancelMainResourceLoad(frame()->loader().cancelledError(documentLoader->request()));
    setShouldLoadPluginManually(false);
}

}
