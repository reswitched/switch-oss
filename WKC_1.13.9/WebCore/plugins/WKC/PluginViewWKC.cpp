/*
 *  Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "BitmapImage.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HostWindow.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "ImageWKC.h"
#include "JSLock.h"
#include "JSDOMWindow.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "PlatformMouseEvent.h"
#include "PluginView.h"
#include "PluginPackage.h"
#include "ScriptController.h"

#include "WKCWebViewPrivate.h"

#include "npruntime_impl.h"

#include <wkc/wkcpluginpeer.h>
#include <wkc/wkcgpeer.h>
#include <wkc/wkcfilepeer.h>

#include <sys/stat.h>

#include "NotImplemented.h"

namespace WebCore {

bool
PluginView::dispatchNPEvent(NPEvent& ev)
{
    if (!m_plugin->pluginFuncs()->event)
        return false;

    PluginView::setCurrentPluginView(this);
    JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindow::commonVM());
    setCallingPlugin(true);

    bool accepted = m_plugin->pluginFuncs()->event(m_instance, &ev);

    setCallingPlugin(false);
    PluginView::setCurrentPluginView(0);
    return accepted;
}

void
PluginView::setFocus(bool flag)
{
    Widget::setFocus(flag);
}

void
PluginView::show()
{
    setSelfVisible(true);
    Widget::show();
    bool ret = wkcPluginWindowSetVisiblePeer(platformPluginWidget(), true);
    if (!ret) {
        // Ugh!: handle error state!
        // 110704 ACCESS Co.,Ltd.
    }
}

void
PluginView::hide()
{
    setSelfVisible(false);
    Widget::hide();
    wkcPluginWindowSetVisiblePeer(platformPluginWidget(), false);
}

void
PluginView::paint(GraphicsContext* ctx, const IntRect& rect)
{
    if (!m_isStarted) {
        paintMissingPluginIcon(ctx, rect);
        return;
    }

    const IntRect framerect = frameRect();

    if (ctx->paintingDisabled())
        return;

    setNPWindowRect(framerect);

    if (m_isWindowed) {
        return;
    }

    const IntRect fr = static_cast<FrameView*>(parent())->contentsToWindow(framerect);
    const WKCRect wfr = { fr.x(), fr.y(), fr.width(), fr.height() };
    const WKCRect r = { rect.x(), rect.y(), rect.width(), rect.height() };

    wkcPluginWindowNotifyPaintPeer(platformPluginWidget(), &wfr, &r);

    int fmt = 0;
    int rowbytes = 0;
    WKCSize wsize = {0};
    void* bitmap = wkcPluginWindowLockOffscreenPeer(platformPluginWidget(), &fmt, &rowbytes, &wsize);

    if (!bitmap) return;

    const IntSize size(wsize.fWidth, wsize.fHeight);

    int type = ImageWKC::EColorNone;
    switch (fmt&WKC_IMAGETYPE_TYPEMASK) {
    case WKC_IMAGETYPE_ARGB8888:
        type = ImageWKC::EColorARGB8888;
        break;
    default:
        break;
    }

    if (type!=ImageWKC::EColorNone) {
        Ref<ImageWKC> wimg = ImageWKC::create(ImageWKC::EColorARGB8888, bitmap, rowbytes, size, false);
        wimg->setHasAlpha(false);
        if (m_isTransparent) {
            if (fmt&WKC_IMAGETYPE_FLAG_HASALPHA)
                wimg->setHasAlpha(true);
        }
        Ref<BitmapImage> img = BitmapImage::create(WTFMove(wimg));
        ctx->drawImage(img.get(), framerect.location());
    }

    wkcPluginWindowUnlockOffscreenPeer(platformPluginWidget(), bitmap);
}

void
PluginView::handleKeyboardEvent(KeyboardEvent* ev)
{
    int type = WKC_PLUGINWINDOW_HIDEVENT_KEYDOWN;
    if (ev->type() == eventNames().keydownEvent) {
        type = WKC_PLUGINWINDOW_HIDEVENT_KEYDOWN;
    } else if (ev->type() == eventNames().keyupEvent) {
        type = WKC_PLUGINWINDOW_HIDEVENT_KEYUP;
    } else {
        ev->setDefaultHandled();
        return;
    }

    int key = ev->keyCode();
    int mod = WKC_PLUGINWINDOW_MODIFIER_NONE;
    if (ev->getModifierState("Control"))
        mod |= WKC_PLUGINWINDOW_MODIFIER_CONTROL;
    if (ev->getModifierState("Alt"))
        mod |= WKC_PLUGINWINDOW_MODIFIER_ALT;
    if (ev->getModifierState("Shift"))
        mod |= WKC_PLUGINWINDOW_MODIFIER_SHIFT;
    if (ev->getModifierState("Meta"))
        mod |= WKC_PLUGINWINDOW_MODIFIER_META;

    bool ret = wkcPluginWindowNotifyKeyboardEventPeer(platformPluginWidget(), type, key, mod);
    if (!ret) {
        ev->setDefaultHandled();
    }
}

void
PluginView::handleMouseEvent(MouseEvent* ev)
{
    IntPoint p = static_cast<FrameView*>(parent())->contentsToWindow(IntPoint(ev->pageX(), ev->pageY()));
    if (!m_isWindowed) {
        const IntPoint fp = static_cast<FrameView*>(parent())->contentsToWindow(frameRect().location());
        p.move(IntSize(-fp.x(), -fp.y()));
    }

    int type = WKC_PLUGINWINDOW_HIDEVENT_MOUSEDOWN;
    bool checkbutton = true;
    if (ev->type() == eventNames().mousedownEvent) {
        focusPluginElement();
        type = WKC_PLUGINWINDOW_HIDEVENT_MOUSEDOWN;
    } else if (ev->type() == eventNames().mouseupEvent) {
        type = WKC_PLUGINWINDOW_HIDEVENT_MOUSEUP;
    } else if (ev->type() == eventNames().mousemoveEvent ||
        ev->type() == eventNames().mouseoutEvent ||
        ev->type() == eventNames().mouseoverEvent) {
        type = WKC_PLUGINWINDOW_HIDEVENT_MOUSEMOVE;
        if (!ev->buttonDown())
            checkbutton = false;
    } else {
        ev->setDefaultHandled();
        return;
    }

    int button = WKC_PLUGINWINDOW_BUTTON_NONE;
    if (checkbutton) {
        switch (ev->button()) {
        case LeftButton:
            button = WKC_PLUGINWINDOW_BUTTON_LEFT;
            break;
        case MiddleButton:
            button = WKC_PLUGINWINDOW_BUTTON_MIDDLE;
            break;
        case RightButton:
            button = WKC_PLUGINWINDOW_BUTTON_RIGHT;
            break;
        default:
            break;
        }
    }

    int mod = WKC_PLUGINWINDOW_MODIFIER_NONE;
    if (ev->ctrlKey())
        mod |= WKC_PLUGINWINDOW_MODIFIER_CONTROL;
    if (ev->shiftKey())
        mod |= WKC_PLUGINWINDOW_MODIFIER_SHIFT;
    if (ev->altKey())
        mod |= WKC_PLUGINWINDOW_MODIFIER_ALT;
    if (ev->metaKey())
        mod |= WKC_PLUGINWINDOW_MODIFIER_META;

    bool ret = wkcPluginWindowNotifyMouseEventPeer(platformPluginWidget(), type, p.x(), p.y(), button, mod);
    if (!ret) {
        ev->setDefaultHandled();
    }
}

void
PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

    if (parent)
        init();
}

void
PluginView::setNPWindowRect(const IntRect& orect)
{
    if (!m_isStarted)
        return;

    int type = 0;
    void* win = 0;
    win = wkcPluginWindowGetNPWindowPeer(platformPluginWidget(), &type);
    m_npWindow.type = (NPWindowType)type;
    m_npWindow.window = win;

    IntRect rect = orect;
    IntRect wcr = windowClipRect();
    if (m_isWindowed) {
        HostWindow* hostwindow = static_cast<FrameView*>(parent())->hostWindow();
        if (hostwindow) {
            PlatformPageClient pageclient = hostwindow->platformPageClient();
            if (pageclient) {
                WKC::WKCWebViewPrivate* webview = (WKC::WKCWebViewPrivate*)pageclient;
                const float zoom = webview->opticalZoomLevel();
                const WKCFloatPoint ofs = webview->opticalZoomOffset();
                rect.setX(rect.x() * zoom + ofs.fX);
                rect.setY(rect.y() * zoom + ofs.fY);
                rect.setWidth(rect.width() * zoom);
                rect.setHeight(rect.height() * zoom);

                wcr.setX(wcr.x() * zoom + ofs.fX);
                wcr.setY(wcr.y() * zoom + ofs.fY);
                wcr.setWidth(wcr.width() * zoom);
                wcr.setHeight(wcr.height() * zoom);
            }
        }
    }


    if (m_isWindowed) {
        const IntPoint p = static_cast<FrameView*>(parent())->contentsToWindow(rect.location());
        m_npWindow.x = p.x();
        m_npWindow.y = p.y();
    } else {
        m_npWindow.x = 0;
        m_npWindow.y = 0;
    }

    m_npWindow.width = rect.width();
    m_npWindow.height = rect.height();

    m_npWindow.clipRect.left = 0;
    m_npWindow.clipRect.top = 0;
    m_npWindow.clipRect.right = rect.width();
    m_npWindow.clipRect.bottom = rect.height();

    if (m_plugin->pluginFuncs()->setwindow) {
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindow::commonVM());
        setCallingPlugin(true);
        m_plugin->pluginFuncs()->setwindow(m_instance, &m_npWindow);
        setCallingPlugin(false);
    }

    const WKCRect wr = { m_npWindow.x, m_npWindow.y, m_npWindow.width, m_npWindow.height };
    WKCRect cr = {0};
    if (m_isWindowed) {
        wcr.intersect(rect);
        WKCRect_SetXYWH(&cr, wcr.x()-m_npWindow.x, wcr.y()-m_npWindow.y, wcr.width(), wcr.height());
    } else {
        WKCRect_SetXYWH(&cr, 0, 0, m_npWindow.clipRect.right, m_npWindow.clipRect.bottom);
    }
    bool ret = wkcPluginWindowSetRectPeer(platformPluginWidget(), &wr, &cr);
    if (!ret) {
        // Ugh!: handle error state!
        // 110704 ACCESS Co.,Ltd.
    }
}

NPError
PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t pathlen, const char* path)
{
    String filename(path, pathlen);
    if (filename.startsWith("file:///"))
        filename = filename.substring(8);

    void* fd = 0;
    struct stat st = {0};
    int size = 0;

    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_PLUGIN, (filename.utf8()).data(), "rb");
    if (!fd)
        goto error_end;
    if (wkcFileFStatPeer(fd, &st)<0)
        goto error_end;

    buffer.resize(st.st_size);

    size = wkcFileFReadPeer(buffer.data(), 1, st.st_size, fd);
    if (size!=st.st_size)
        goto error_end;

    wkcFileFClosePeer(fd);
    return NPERR_NO_ERROR;

error_end:
    if (fd) {
        wkcFileFClosePeer(fd);
    }
    return NPERR_FILE_NOT_FOUND;
}

bool
PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
#if ENABLE(NETSCAPE_PLUGIN_API)
    case NPNVWindowNPObject: {
        if (m_isJavaScriptPaused) {
            *result = NPERR_GENERIC_ERROR;
            return true;
        }

        NPObject* windowScriptObject = m_parentFrame->script().windowScriptNPObject();

        // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugin/npruntime.html>
        if (windowScriptObject)
            _NPN_RetainObject(windowScriptObject);

        void** v = (void**)value;
        *v = windowScriptObject;

        *result = NPERR_NO_ERROR;
        return true;
    }

    case NPNVPluginElementNPObject: {
        if (m_isJavaScriptPaused) {
            *result = NPERR_GENERIC_ERROR;
            return true;
        }

        NPObject* pluginScriptObject = 0;

        if (m_element->hasTagName(HTMLNames::appletTag) || m_element->hasTagName(HTMLNames::embedTag) || m_element->hasTagName(HTMLNames::objectTag))
            pluginScriptObject = (reinterpret_cast<HTMLPlugInElement*>(m_element))->getNPObject();

        // Return value is expected to be retained, as described here: <http://www.mozilla.org/projects/plugin/npruntime.html>
        if (pluginScriptObject)
            _NPN_RetainObject(pluginScriptObject);

        void** v = (void**)value;
        *v = pluginScriptObject;

        *result = NPERR_NO_ERROR;
        return true;
    }
#endif

    default:
        if (wkcPluginWindowGetValuePeer(platformPluginWidget(), (int)variable, value)) {
            *result = NPERR_NO_ERROR;
            return true;
        }
#if ENABLE(NETSCAPE_PLUGIN_API)
        return platformGetValueStatic(variable, value, result);
#else
        *result = NPERR_GENERIC_ERROR;
        return false;
#endif
    }
}

#if ENABLE(NETSCAPE_PLUGIN_API)
bool
PluginView::platformGetValueStatic(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVjavascriptEnabledBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    default:
        if (wkcPluginGetStaticValuePeer((int)variable, value)) {
            *result = NPERR_NO_ERROR;
            return true;
        } else {
            *result = NPERR_GENERIC_ERROR;
            return false;
        }
    }
}
#endif

static void
requestInvalidateOnMainThread(void* ctx)
{
    PluginView* self = (PluginView* )ctx;
    self->invalidateRect(0);
}

void
PluginView::invalidateRect(NPRect* rect)
{
    if (!isMainThread()) {
        callOnMainThread(requestInvalidateOnMainThread, (void *)this);
        return;
    }

    if (!rect) {
        invalidate();
    } else {
        const IntRect r(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
        if (m_isWindowed) {
            const WKCRect wrect = { r.x(), r.y(), r.width(), r.height() };
            wkcPluginWindowNotifyInvalRectPeer(platformPluginWidget(), &wrect);
        } else {
            if (m_plugin->quirks().contains(PluginQuirkThrottleInvalidate)) {
                m_invalidRects.append(r);
                if (!m_invalidateTimer.isActive())
                    m_invalidateTimer.startOneShot(0.001);
            } else {
                invalidateRect(r);
            }
        }
    }
}

void
PluginView::invalidateRect(const IntRect& rect)
{
    if (m_isWindowed) {
        const WKCRect r = { rect.x(), rect.y(), rect.width(), rect.height() };
        wkcPluginWindowNotifyInvalRectPeer(platformPluginWidget(), &r);
    } else {
        invalidateWindowlessPluginRect(rect);
    }
}

void
PluginView::invalidateRegion(NPRegion)
{
    invalidate();
}

void
PluginView::forceRedraw()
{
    wkcPluginWindowNotifyForceRedrawPeer(platformPluginWidget());
}

bool
PluginView::notifyNPEvent(void* opaque, void* npevent)
{
    PluginView* self = (PluginView *)opaque;
    NPEvent* ev = (NPEvent *)npevent;
    return self->dispatchNPEvent(*ev);
}

void
PluginView::notifyKeyEvent(void* opaque, int type, int key, int modifiers)
{
#if 0
    PluginView* self = (PluginView *)opaque;
    KeyboardEvent ev;
    // Ugh!: fill data!
    // 110520 nagano
    self->handleKeyboardEvent(&ev);
#endif
}

void
PluginView::notifyMouseEvent(void* opaque, int type, int in_x, int in_y, int button, int modifiers)
{
#if 0
    PluginView* self = (PluginView *)opaque;
    MouseEvent ev;
    // Ugh!: fill data!
    // 110520 nagano
    self->handleMouseEvent(&ev);
#endif
}

void
PluginView::notifyDestroy(void* opaque)
{
    PluginView* self = reinterpret_cast<PluginView*>(opaque);
    if (!self)
        return;

    if (self->m_plugin->pluginFuncs()->destroy) {
        self->m_plugin->pluginFuncs()->destroy(self->m_instance, 0);
    }
}

bool
PluginView::platformStart()
{
    static const WKCPluginWindowEventHandlers cHandlers = {
        notifyMouseEvent,
        notifyKeyEvent,
        notifyNPEvent,
        notifyDestroy,
    };

    bool xembed = false;
#ifdef XP_UNIX
    if (m_plugin->pluginFuncs()->getvalue) {
        PluginView::setCurrentPluginView(this);
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        setCallingPlugin(true);
        m_plugin->pluginFuncs()->getvalue(m_instance, NPPVpluginNeedsXEmbed, &m_needsXEmbed);
        setCallingPlugin(false);
        PluginView::setCurrentPluginView(0);
    }
    xembed = m_needsXEmbed;
#endif

    void* win = wkcPluginWindowNewPeer(m_isWindowed, xembed, &cHandlers, this);
    if (!win)
        return false;
    setPlatformPluginWidget(win);

    void* npwindow = 0;
    int type = 0;
    npwindow = wkcPluginWindowGetNPWindowPeer(platformPluginWidget(), &type);

    m_npWindow.type = (NPWindowType)type;
    m_npWindow.window = npwindow;

#ifdef XP_UNIX
    NPSetWindowCallbackStruct* wsi = new NPSetWindowCallbackStruct();
    ::memset(wsi, 0, sizeof(NPSetWindowCallbackStruct));
    wsi->type = 0;
    void* display =0;
    void* visual = 0;
    int depth = 0;
    void* colormap = 0;
    if (!wkcPluginWindowX11GetWSIInfoPeer(platformPluginWidget(), &display, &visual, &depth, &colormap))
      return false;
    wsi->display = (Display *)display;
    wsi->visual = (Visual *)visual;
    wsi->depth = depth;
    wsi->colormap = (Colormap)colormap;
    m_npWindow.ws_info = wsi;
#endif

    updatePluginWidget();

    if (!m_plugin->quirks().contains(PluginQuirkDeferFirstSetWindowCall))
        setNPWindowRect(frameRect());

    return true;
}

void
PluginView::platformDestroy()
{
    void* win = platformPluginWidget();
    wkcPluginWindowDeletePeer(win);
    setPlatformPluginWidget(0);
#ifdef XP_UNIX
    if (m_status != PluginStatusCanNotFindPlugin) {
        if (m_npWindow.ws_info)
            delete ((NPSetWindowCallbackStruct *)m_npWindow.ws_info);
        m_npWindow.ws_info = 0;
    }
#endif
    cancelCallOnMainThread(requestInvalidateOnMainThread, (void *)this);
}

void
PluginView::setParentVisible(bool visible)
{
    if (isParentVisible()==visible)
        return;
    Widget::setParentVisible(visible);
}

void
PluginView::updatePluginWidget()
{
    if (!parent())
        return;

    FrameView* view = 0;

    view = (FrameView *)parent();
    m_windowRect = IntRect(view->contentsToWindow(frameRect().location()), frameRect().size());
    m_clipRect = windowClipRect();
    m_clipRect.move(-m_windowRect.x(), -m_windowRect.y());

    const int type = wkcPluginGetTypePeer(plugin()->module());
    if (type==WKC_PLUGIN_TYPE_WIN)
        setNPWindowRect(frameRect());
}

#if 0
void
PluginView::halt()
{
    wkcPluginWindowNotifySuspendPeer(platformPluginWidget());
    platformDestroy();
    stop();
}

void
PluginView::restart()
{
    wkcPluginWindowNotifyResumePeer(platformPluginWidget());

    start();
}
#endif

#if defined(XP_UNIX) && ENABLE(NETSCAPE_PLUGIN_API)
void
PluginView::handleFocusInEvent()
{
    notImplemented();
}

void
PluginView::handleFocusOutEvent()
{
    notImplemented();
}
#endif

} // namespace WebCore

#endif // ENABLE(NETSCAPE_PLUGIN_API)

// dummy code for QuickTimePlugin

#include "JSQuickTimePluginReplacement.h"
#include "QuickTimePluginReplacement.h"
#include "JSCJSValue.h"
#include "JSCJSValueInlines.h"

namespace WebCore {

JSC::JSValue
JSQuickTimePluginReplacement::timedMetaData(JSC::ExecState&) const
{
    return JSC::JSValue(JSC::JSValue::JSUndefined);
}

JSC::JSValue
JSQuickTimePluginReplacement::accessLog(JSC::ExecState&) const
{
    return JSC::JSValue(JSC::JSValue::JSUndefined);
}

JSC::JSValue
JSQuickTimePluginReplacement::errorLog(JSC::ExecState&) const
{
    return JSC::JSValue(JSC::JSValue::JSUndefined);
}

unsigned long long
QuickTimePluginReplacement::movieSize() const
{
    return 0;
}

void
QuickTimePluginReplacement::postEvent(const String&)
{
}

} // namespace WebCore
