/*
 * WKCWebViewPrivate.h
 *
 * Copyright (c) 2010-2020 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKCWebViewPrivate_h
#define WKCWebViewPrivate_h

#include "WKCClientBuilders.h"
#include "WKCEnums.h"

#include "IntRect.h"
#include "FloatPoint.h"
#include "ActivityState.h"
#include "WTFString.h"

namespace WebCore {
    class Page;
    class GraphicsLayer;
    class Frame;
    class GraphicsContext;
    class Node;
}

namespace JSC {
    class JSGlobalObject;
}

namespace WKC {
class WKCWebView;
class InspectorClientWKC;
class DropDownListClientWKC;
class FrameLoaderClientWKC;
class WKCSettings;
class WKCWebViewPrefs;
#if ENABLE(REMOTE_INSPECTOR)
class InspectorServerClientWKC;
#endif
class DeviceMotionClientWKC;
class DeviceOrientationClientWKC;
class NotificationClientWKC;
class SpeechRecognitionClientWKC;
class ProgressTrackerClientWKC;
class WKCOverlayIf;
class WKCOverlayList;

class Element;
class Node;
class Page;
class ElementPrivate;
class NodePrivate;
class PagePrivate;

class ContentExtensionClientWKC;

bool shouldInterruptJavaScript(JSC::JSGlobalObject*, void*, void*);

// class definitions

class WKCWebViewPrivate
{
WTF_MAKE_FAST_ALLOCATED;
    friend class WKCWebView;
    friend class FrameLoaderClientWKC;

public:
    static WKCWebViewPrivate* create(WKCWebView* parent, WKCClientBuilders& builders);
    ~WKCWebViewPrivate();
    void notifyForceTerminate();

    WKCWebViewPrivate(const WKCWebViewPrivate&) = delete;
    WKCWebViewPrivate& operator=(const WKCWebViewPrivate&) = delete;

    static bool isValidView(WKCWebViewPrivate*);

    inline WebCore::Page* core() const { return m_corePage; };
    WKC::Page* wkcCore() const;
    inline WKCWebView* parent() const { return m_parent; };
    inline WKCClientBuilders& clientBuilders() const { return m_clientBuilders; };

    inline DropDownListClientWKC* dropdownlistclient() { return m_dropdownlist; };
    // settings
    WKCSettings* settings();

    // drawings
    bool setOffscreen(OffscreenFormat format, void* bitmap, int rowbytes, const WebCore::IntSize& offscreensize, const WebCore::IntSize& viewsize, bool fixedlayout, const WebCore::IntSize* desktopsize, bool needslayout);
    void notifyResizeDesktopSize(const WebCore::IntSize& size, bool in_resizeevent);
    void notifyResizeViewSize(const WebCore::IntSize& size);
    void notifyRelayout();
    void notifyPaintOffscreenFrom(const WebCore::IntRect& rect, const WKCPoint& p);
    void notifyPaintOffscreen(const WebCore::IntRect& rect);
#if USE(WKC_CAIRO)
    void notifyPaintToContext(const WebCore::IntRect& rect, void* context);
#endif
    void notifyScrollOffscreen(const WebCore::IntRect& rect, const WebCore::IntSize& diff);
    const WebCore::IntSize& desktopSize() const;
    const WebCore::IntSize& viewSize() const;
    const WebCore::IntSize& defaultDesktopSize() const;
    const WebCore::IntSize& defaultViewSize() const;
    void setUseAntiAliasForDrawings(bool flag);
    void setUseBilinearForScaledImages(bool flag);
    static void setUseBilinearForCanvasImages(bool flag);
    static void setUseAntiAliasForCanvas(bool flag);
    void setScrollPositionForOffscreen(const WebCore::IntPoint& scrollPosition);
    void notifyScrollPositionChanged();

    bool transparent() { return m_isTransparent; };
    void setTransparent(bool flag);

    inline float opticalZoomLevel() const { return m_opticalZoomLevel; };
    inline const WKCFloatPoint& opticalZoomOffset() const { return m_opticalZoomOffset; };
    void setOpticalZoom(float, const WKCFloatPoint&);

    void enterCompositingMode();

    WKC::Element* getFocusedElement();
    WKC::Element* getElementFromPoint(int x, int y);

    inline bool isZoomFullContent() const { return m_isZoomFullContent; };
    inline void setZoomFullContent(bool flag) { m_isZoomFullContent = flag; };

    // webInspector
    void enableWebInspector(bool enable);
    bool isWebInspectorEnabled();

    void setEditable(bool enable) { m_editable = enable; }
    bool editable() const { return m_editable; }

    void setSpatialNavigationEnabled(bool enable);

    const unsigned short* selectionText();

    WebCore::GraphicsLayer* rootGraphicsLayer() { return m_rootGraphicsLayer; }
    void setRootGraphicsLayer(WebCore::GraphicsLayer* layer) { m_rootGraphicsLayer = layer; }

    void addOverlay(WKCOverlayIf* overlay, int zOrder, int fixedDirectionFlag);
    void removeOverlay(WKCOverlayIf* overlay);
    void updateOverlay(const WebCore::IntRect& rect, bool immediate);
    void paintOverlay(WebCore::GraphicsContext& ctx);

    void setIsVisible(bool isVisible);
    void notifyFocusIn();
    void notifyFocusOut();

    bool addUserContentExtension(const char* name, const char* extension);
    bool compileUserContentExtension(const char* extension, unsigned char** out_filtersWithoutDomainsBytecode, unsigned int* out_filtersWithoutDomainsBytecodeLen, unsigned char** out_filtersWithDomainsBytecode, unsigned int* out_filtersWithDomainsBytecodeLen, unsigned char** out_domainFiltersBytecode, unsigned int* out_domainFiltersBytecodeLen, unsigned char** out_actions, unsigned int* out_actionsLen);
    bool addCompiledUserContentExtension(const char* name, const unsigned char* filtersWithoutDomainsBytecode, unsigned int filtersWithoutDomainsBytecodeLen, const unsigned char* filtersWithDomainsBytecode, unsigned int filtersWithDomainsBytecodeLen, const unsigned char* domainFiltersBytecode, unsigned int domainFiltersBytecodeLen, const unsigned char* actions, unsigned int actionsLen);

    void recalcStyleSheet();

    void allowLayout();
    void disallowLayout();

    void enableAutoPlay(bool enable);
    bool isAutoPlayEnabled();

    void setUseDarkAppearance(bool useDarkAppearance);
    bool useDarkAppearance() const;

private:
    WKCWebViewPrivate(WKCWebView* parent, WKCClientBuilders& builders);
    bool construct();

    bool prepareDrawings();
#if USE(WKC_CAIRO)
    bool recoverFromCairoError();
#endif

private:
    WKCWebView* m_parent;
    WKCClientBuilders& m_clientBuilders;

    // core
    WebCore::Page* m_corePage;
    WKC::PagePrivate* m_wkcCorePage;

    // instances
    WKCWebFrame* m_mainFrame;
    InspectorClientWKC* m_inspector;
    DropDownListClientWKC* m_dropdownlist;
    DeviceMotionClientWKC* m_devicemotion;
    DeviceOrientationClientWKC* m_deviceorientation;
    NotificationClientWKC* m_notification;
    ProgressTrackerClientWKC* m_progress;
    WKCSettings* m_settings;
#if ENABLE(REMOTE_INSPECTOR)
    InspectorServerClientWKC* m_inspectorServerClient;
    bool m_inspectorIsEnabled;
#endif

    // offscreen
    void* m_drawContext;
    void* m_offscreen;

#if USE(WKC_CAIRO)
    int m_offscreenFormat;
    void* m_offscreenBitmap;
    int m_offscreenRowBytes;
    WKCSize m_offscreenSize;
#endif

    WebCore::IntSize m_desktopSize;
    WebCore::IntSize m_viewSize;
    WebCore::IntSize m_defaultDesktopSize;
    WebCore::IntSize m_defaultViewSize;

    float m_opticalZoomLevel;
    WKCFloatPoint m_opticalZoomOffset;

    bool m_isZoomFullContent;
    bool m_isTransparent;
    WKC::LoadStatus m_loadStatus;

    // temporary string resources
    unsigned short* m_encoding;
    unsigned short* m_customEncoding;

    bool m_forceTerminated;

    WKC::ElementPrivate* m_focusedElement;
    WKC::ElementPrivate* m_elementFromPoint;

    WebCore::Node* m_lastNodeUnderMouse;

    WTF::String m_selectedText;

    bool m_editable;

    WebCore::GraphicsLayer* m_rootGraphicsLayer;
    RefPtr<WKCOverlayList> m_overlayList;

    OptionSet<WebCore::ActivityState::Flag> m_activityState;

    bool m_autoPlayIsEnabled;
};

} // namespace

#endif // WKCWebViewPrivate_h
