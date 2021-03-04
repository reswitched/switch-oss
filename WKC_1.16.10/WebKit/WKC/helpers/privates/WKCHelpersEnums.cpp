/*
 * Copyright (c) 2016-2019 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/privates/WKCHelpersEnumsPrivate.h"

namespace WKC {

WKC::MessageSource
toWKCMessageSource(JSC::MessageSource source)
{
    switch (source) {
    case JSC::MessageSource::XML:
        return WKC::MessageSource::XML;
    case JSC::MessageSource::JS:
        return WKC::MessageSource::JS;
    case JSC::MessageSource::Network:
        return WKC::MessageSource::Network;
    case JSC::MessageSource::ConsoleAPI:
        return WKC::MessageSource::ConsoleAPI;
    case JSC::MessageSource::Storage:
        return WKC::MessageSource::Storage;
    case JSC::MessageSource::AppCache:
        return WKC::MessageSource::AppCache;
    case JSC::MessageSource::Rendering:
        return WKC::MessageSource::Rendering;
    case JSC::MessageSource::CSS:
        return WKC::MessageSource::CSS;
    case JSC::MessageSource::Security:
        return WKC::MessageSource::Security;
    case JSC::MessageSource::ContentBlocker:
        return WKC::MessageSource::ContentBlocker;
    case JSC::MessageSource::Other:
        return WKC::MessageSource::Other;
    case JSC::MessageSource::Media:
        return WKC::MessageSource::Media;
    case JSC::MessageSource::WebRTC:
        return WKC::MessageSource::WebRTC;
    default:
        ASSERT_NOT_REACHED();
        return WKC::MessageSource::Other;
    }
}

JSC::MessageSource
toJSCMessageSource(WKC::MessageSource source)
{
    switch (source) {
    case WKC::MessageSource::XML:
        return JSC::MessageSource::XML;
    case WKC::MessageSource::JS:
        return JSC::MessageSource::JS;
    case WKC::MessageSource::Network:
        return JSC::MessageSource::Network;
    case WKC::MessageSource::ConsoleAPI:
        return JSC::MessageSource::ConsoleAPI;
    case WKC::MessageSource::Storage:
        return JSC::MessageSource::Storage;
    case WKC::MessageSource::AppCache:
        return JSC::MessageSource::AppCache;
    case WKC::MessageSource::Rendering:
        return JSC::MessageSource::Rendering;
    case WKC::MessageSource::CSS:
        return JSC::MessageSource::CSS;
    case WKC::MessageSource::Security:
        return JSC::MessageSource::Security;
    case WKC::MessageSource::ContentBlocker:
        return JSC::MessageSource::ContentBlocker;
    case WKC::MessageSource::Other:
        return JSC::MessageSource::Other;
    case WKC::MessageSource::Media:
        return JSC::MessageSource::Media;
    case WKC::MessageSource::WebRTC:
        return JSC::MessageSource::WebRTC;
    default:
        ASSERT_NOT_REACHED();
        return JSC::MessageSource::Other;
    }
}

WKC::MessageType
toWKCMessageType(JSC::MessageType type)
{
    switch (type) {
    case JSC::MessageType::Log:
        return WKC::MessageType::Log;
    case JSC::MessageType::Dir:
        return WKC::MessageType::Dir;
    case JSC::MessageType::DirXML:
        return WKC::MessageType::DirXML;
    case JSC::MessageType::Table:
        return WKC::MessageType::Table;
    case JSC::MessageType::Trace:
        return WKC::MessageType::Trace;
    case JSC::MessageType::StartGroup:
        return WKC::MessageType::StartGroup;
    case JSC::MessageType::StartGroupCollapsed:
        return WKC::MessageType::StartGroupCollapsed;
    case JSC::MessageType::EndGroup:
        return WKC::MessageType::EndGroup;
    case JSC::MessageType::Clear:
        return WKC::MessageType::Clear;
    case JSC::MessageType::Assert:
        return WKC::MessageType::Assert;
    case JSC::MessageType::Timing:
        return WKC::MessageType::Timing;
    case JSC::MessageType::Profile:
        return WKC::MessageType::Profile;
    case JSC::MessageType::ProfileEnd:
        return WKC::MessageType::ProfileEnd;
    default:
        ASSERT_NOT_REACHED();
        return WKC::MessageType::Log;
    }
}

JSC::MessageType
toJSCMessageType(WKC::MessageType type)
{
    switch (type) {
    case WKC::MessageType::Log:
        return JSC::MessageType::Log;
    case WKC::MessageType::Dir:
        return JSC::MessageType::Dir;
    case WKC::MessageType::DirXML:
        return JSC::MessageType::DirXML;
    case WKC::MessageType::Table:
        return JSC::MessageType::Table;
    case WKC::MessageType::Trace:
        return JSC::MessageType::Trace;
    case WKC::MessageType::StartGroup:
        return JSC::MessageType::StartGroup;
    case WKC::MessageType::StartGroupCollapsed:
        return JSC::MessageType::StartGroupCollapsed;
    case WKC::MessageType::EndGroup:
        return JSC::MessageType::EndGroup;
    case WKC::MessageType::Clear:
        return JSC::MessageType::Clear;
    case WKC::MessageType::Assert:
        return JSC::MessageType::Assert;
    case WKC::MessageType::Timing:
        return JSC::MessageType::Timing;
    case WKC::MessageType::Profile:
        return JSC::MessageType::Profile;
    case WKC::MessageType::ProfileEnd:
        return JSC::MessageType::ProfileEnd;
    default:
        ASSERT_NOT_REACHED();
        return JSC::MessageType::Log;
    }
}

WKC::MessageLevel
toWKCMessageLevel(JSC::MessageLevel level)
{
    switch (level) {
    case JSC::MessageLevel::Log:
        return WKC::MessageLevel::Log;
    case JSC::MessageLevel::Warning:
        return WKC::MessageLevel::Warning;
    case JSC::MessageLevel::Error:
        return WKC::MessageLevel::Error;
    case JSC::MessageLevel::Debug:
        return WKC::MessageLevel::Debug;
    case JSC::MessageLevel::Info:
        return WKC::MessageLevel::Info;
    default:
        ASSERT_NOT_REACHED();
        return WKC::MessageLevel::Log;
    }
}

JSC::MessageLevel
toJSCMessageLevel(WKC::MessageLevel level)
{
    switch (level) {
    case WKC::MessageLevel::Log:
        return JSC::MessageLevel::Log;
    case WKC::MessageLevel::Warning:
        return JSC::MessageLevel::Warning;
    case WKC::MessageLevel::Error:
        return JSC::MessageLevel::Error;
    case WKC::MessageLevel::Debug:
        return JSC::MessageLevel::Debug;
    case WKC::MessageLevel::Info:
        return JSC::MessageLevel::Info;
    default:
        ASSERT_NOT_REACHED();
        return JSC::MessageLevel::Log;
    }
}

WKC::TextDirection
toWKCTextDirection(WebCore::TextDirection dir)
{
    switch (dir) {
    case WebCore::LTR:
        return WKC::LTR;
    case WebCore::RTL:
        return WKC::RTL;
    default:
        ASSERT_NOT_REACHED();
        return WKC::LTR;
    }
}

WebCore::TextDirection
toWebCoreTextDirection(WKC::TextDirection dir)
{
    switch (dir) {
    case WKC::LTR:
        return WebCore::LTR;
    case WKC::RTL:
        return WebCore::RTL;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::LTR;
    }
}

WKC::FocusDirection
toWKCFocusDirection(WebCore::FocusDirection dir)
{
    switch (dir) {
    case WebCore::FocusDirectionNone:
        return WKC::FocusDirectionNone;
    case WebCore::FocusDirectionForward:
        return WKC::FocusDirectionForward;
    case WebCore::FocusDirectionBackward:
        return WKC::FocusDirectionBackward;
    case WebCore::FocusDirectionUp:
        return WKC::FocusDirectionUp;
    case WebCore::FocusDirectionDown:
        return WKC::FocusDirectionDown;
    case WebCore::FocusDirectionLeft:
        return WKC::FocusDirectionLeft;
    case WebCore::FocusDirectionRight:
        return WKC::FocusDirectionRight;
    case WebCore::FocusDirectionUpLeft:
        return WKC::FocusDirectionUpLeft;
    case WebCore::FocusDirectionUpRight:
        return WKC::FocusDirectionUpRight;
    case WebCore::FocusDirectionDownLeft:
        return WKC::FocusDirectionDownLeft;
    case WebCore::FocusDirectionDownRight:
        return WKC::FocusDirectionDownRight;
    default:
        ASSERT_NOT_REACHED();
        return WKC::FocusDirectionNone;
    }
}

WebCore::FocusDirection
toWebCoreFocusDirection(WKC::FocusDirection dir)
{
    switch (dir) {
    case WKC::FocusDirectionNone:
        return WebCore::FocusDirectionNone;
    case WKC::FocusDirectionForward:
        return WebCore::FocusDirectionForward;
    case WKC::FocusDirectionBackward:
        return WebCore::FocusDirectionBackward;
    case WKC::FocusDirectionUp:
        return WebCore::FocusDirectionUp;
    case WKC::FocusDirectionDown:
        return WebCore::FocusDirectionDown;
    case WKC::FocusDirectionLeft:
        return WebCore::FocusDirectionLeft;
    case WKC::FocusDirectionRight:
        return WebCore::FocusDirectionRight;
    case WKC::FocusDirectionUpLeft:
        return WebCore::FocusDirectionUpLeft;
    case WKC::FocusDirectionUpRight:
        return WebCore::FocusDirectionUpRight;
    case WKC::FocusDirectionDownLeft:
        return WebCore::FocusDirectionDownLeft;
    case WKC::FocusDirectionDownRight:
        return WebCore::FocusDirectionDownRight;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::FocusDirectionNone;
    }
}

WKC::DragDestinationAction
toWKCDragDestinationAction(WebCore::DragDestinationAction action)
{
    switch (action) {
    case WebCore::DragDestinationActionNone:
        return WKC::DragDestinationActionNone;
    case WebCore::DragDestinationActionDHTML:
        return WKC::DragDestinationActionDHTML;
    case WebCore::DragDestinationActionEdit:
        return WKC::DragDestinationActionEdit;
    case WebCore::DragDestinationActionLoad:
        return WKC::DragDestinationActionLoad;
    case WebCore::DragDestinationActionAny:
        return WKC::DragDestinationActionAny;
    default:
        ASSERT_NOT_REACHED();
        return WKC::DragDestinationActionNone;
    }
}

WebCore::DragDestinationAction
toWebCoreDragDestinationAction(WKC::DragDestinationAction action)
{
    switch (action) {
    case WKC::DragDestinationActionNone:
        return WebCore::DragDestinationActionNone;
    case WKC::DragDestinationActionDHTML:
        return WebCore::DragDestinationActionDHTML;
    case WKC::DragDestinationActionEdit:
        return WebCore::DragDestinationActionEdit;
    case WKC::DragDestinationActionLoad:
        return WebCore::DragDestinationActionLoad;
    case WKC::DragDestinationActionAny:
        return WebCore::DragDestinationActionAny;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::DragDestinationActionNone;
    }
}

WKC::DragSourceAction
toWKCDragSourceAction(WebCore::DragSourceAction action)
{
    switch (action) {
    case WebCore::DragSourceActionNone:
        return WKC::DragSourceActionNone;
    case WebCore::DragSourceActionDHTML:
        return WKC::DragSourceActionDHTML;
    case WebCore::DragSourceActionImage:
        return WKC::DragSourceActionImage;
    case WebCore::DragSourceActionLink:
        return WKC::DragSourceActionLink;
    case WebCore::DragSourceActionSelection:
        return WKC::DragSourceActionSelection;
    case WebCore::DragSourceActionAny:
        return WKC::DragSourceActionAny;
    default:
        ASSERT_NOT_REACHED();
        return WKC::DragSourceActionNone;
    }
}

WebCore::DragSourceAction
toWebCoreDragSourceAction(WKC::DragSourceAction action)
{
    switch (action) {
    case WKC::DragSourceActionNone:
        return WebCore::DragSourceActionNone;
    case WKC::DragSourceActionDHTML:
        return WebCore::DragSourceActionDHTML;
    case WKC::DragSourceActionImage:
        return WebCore::DragSourceActionImage;
    case WKC::DragSourceActionLink:
        return WebCore::DragSourceActionLink;
    case WKC::DragSourceActionSelection:
        return WebCore::DragSourceActionSelection;
#if ENABLE(ATTACHMENT_ELEMENT)
    case WKC::DragSourceActionAttachment:
        return WebCore::DragSourceActionAttachment;
#endif
    case WKC::DragSourceActionAny:
        return WebCore::DragSourceActionAny;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::DragSourceActionNone;
    }
}

WKC::DragOperation
toWKCDragOperation(WebCore::DragOperation op)
{
    switch (op) {
    case WebCore::DragOperationNone:
        return WKC::DragOperationNone;
    case WebCore::DragOperationCopy:
        return WKC::DragOperationCopy;
    case WebCore::DragOperationLink:
        return WKC::DragOperationLink;
    case WebCore::DragOperationGeneric:
        return WKC::DragOperationGeneric;
    case WebCore::DragOperationPrivate:
        return WKC::DragOperationPrivate;
    case WebCore::DragOperationMove:
        return WKC::DragOperationMove;
    case WebCore::DragOperationDelete:
        return WKC::DragOperationDelete;
    case WebCore::DragOperationEvery:
        return WKC::DragOperationEvery;
    default:
        ASSERT_NOT_REACHED();
        return WKC::DragOperationNone;
    }
}

WebCore::DragOperation
toWebCoreDragOperation(WKC::DragOperation op)
{
    switch (op) {
    case WKC::DragOperationNone:
        return WebCore::DragOperationNone;
    case WKC::DragOperationCopy:
        return WebCore::DragOperationCopy;
    case WKC::DragOperationLink:
        return WebCore::DragOperationLink;
    case WKC::DragOperationGeneric:
        return WebCore::DragOperationGeneric;
    case WKC::DragOperationPrivate:
        return WebCore::DragOperationPrivate;
    case WKC::DragOperationMove:
        return WebCore::DragOperationMove;
    case WKC::DragOperationDelete:
        return WebCore::DragOperationDelete;
    case WKC::DragOperationEvery:
        return WebCore::DragOperationEvery;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::DragOperationNone;
    }
}

WKC::ScrollbarOrientation
toWKCScrollbarOrientation(WebCore::ScrollbarOrientation orientation)
{
    switch (orientation) {
    case WebCore::HorizontalScrollbar:
        return WKC::HorizontalScrollbar;
    case WebCore::VerticalScrollbar:
        return WKC::VerticalScrollbar;
    default:
        ASSERT_NOT_REACHED();
        return WKC::HorizontalScrollbar;
    }
}

WebCore::ScrollbarOrientation
toWebCoreScrollbarOrientation(WKC::ScrollbarOrientation orientation)
{
    switch (orientation) {
    case WKC::HorizontalScrollbar:
        return WebCore::HorizontalScrollbar;
    case WKC::VerticalScrollbar:
        return WebCore::VerticalScrollbar;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::HorizontalScrollbar;
    }
}

WKC::ScrollbarControlSize
toWKCScrollbarControlSize(WebCore::ScrollbarControlSize size)
{
    switch (size) {
    case WebCore::RegularScrollbar:
        return WKC::RegularScrollbar;
    case WebCore::SmallScrollbar:
        return WKC::SmallScrollbar;
    default:
        ASSERT_NOT_REACHED();
        return WKC::RegularScrollbar;
    }
}

WebCore::ScrollbarControlSize
toWebCoreScrollbarControlSize(WKC::ScrollbarControlSize size)
{
    switch (size) {
    case WKC::RegularScrollbar:
        return WebCore::RegularScrollbar;
    case WKC::SmallScrollbar:
        return WebCore::SmallScrollbar;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::RegularScrollbar;
    }
}

WKC::ScrollDirection
toWKCScrollDirection(WebCore::ScrollDirection dir)
{
    switch (dir) {
    case WebCore::ScrollUp:
        return WKC::ScrollUp;
    case WebCore::ScrollDown:
        return WKC::ScrollDown;
    case WebCore::ScrollLeft:
        return WKC::ScrollLeft;
    case WebCore::ScrollRight:
        return WKC::ScrollRight;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ScrollUp;
    }
}

WebCore::ScrollDirection
toWebCoreScrollDirection(WKC::ScrollDirection dir)
{
    switch (dir) {
    case WKC::ScrollUp:
        return WebCore::ScrollUp;
    case WKC::ScrollDown:
        return WebCore::ScrollDown;
    case WKC::ScrollLeft:
        return WebCore::ScrollLeft;
    case WKC::ScrollRight:
        return WebCore::ScrollRight;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ScrollUp;
    }
}

WKC::EditorInsertAction
toWKCEditorInsertAction(WebCore::EditorInsertAction action)
{
    switch (action) {
    case WebCore::EditorInsertAction::Typed:
        return WKC::EditorInsertActionTyped;
    case WebCore::EditorInsertAction::Pasted:
        return WKC::EditorInsertActionPasted;
    case WebCore::EditorInsertAction::Dropped:
        return WKC::EditorInsertActionDropped;
    default:
        ASSERT_NOT_REACHED();
        return WKC::EditorInsertActionTyped;
    }
}

WebCore::EditorInsertAction
toWebCoreEditorInsertAction(WKC::EditorInsertAction action)
{
    switch (action) {
    case WKC::EditorInsertActionTyped:
        return WebCore::EditorInsertAction::Typed;
    case WKC::EditorInsertActionPasted:
        return WebCore::EditorInsertAction::Pasted;
    case WKC::EditorInsertActionDropped:
        return WebCore::EditorInsertAction::Dropped;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::EditorInsertAction::Typed;
    }
}

WKC::EAffinity
toWKCEAffinity(WebCore::EAffinity affinity)
{
    switch (affinity) {
    case WebCore::UPSTREAM:
        return WKC::UPSTREAM;
    case WebCore::DOWNSTREAM:
        return WKC::DOWNSTREAM;
    default:
        ASSERT_NOT_REACHED();
        return WKC::UPSTREAM;
    }
}

WebCore::EAffinity
toWebCoreEAffinity(WKC::EAffinity affinity)
{
    switch (affinity) {
    case WKC::UPSTREAM:
        return WebCore::UPSTREAM;
    case WKC::DOWNSTREAM:
        return WebCore::DOWNSTREAM;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::UPSTREAM;
    }
}

WKC::PolicyAction
toWKCPolicyAction(WebCore::PolicyAction action)
{
    switch (action) {
    case WebCore::PolicyAction::Use:
        return WKC::PolicyUse;
    case WebCore::PolicyAction::Download:
        return WKC::PolicyDownload;
    case WebCore::PolicyAction::Ignore:
        return WKC::PolicyIgnore;
    case WebCore::PolicyAction::Suspend:
        return WKC::PolicySuspend;
    default:
        ASSERT_NOT_REACHED();
        return WKC::PolicyUse;
    }
}

WebCore::PolicyAction
toWebCorePolicyAction(WKC::PolicyAction action)
{
    switch (action) {
    case WKC::PolicyUse:
        return WebCore::PolicyAction::Use;
    case WKC::PolicyDownload:
        return WebCore::PolicyAction::Download;
    case WKC::PolicyIgnore:
        return WebCore::PolicyAction::Ignore;
    case WKC::PolicySuspend:
        return WebCore::PolicyAction::Suspend;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::PolicyAction::Use;
    }
}

WKC::ObjectContentType
toWKCObjectContentType(WebCore::ObjectContentType type)
{
    switch (type) {
    case WebCore::ObjectContentType::None:
        return WKC::ObjectContentNone;
    case WebCore::ObjectContentType::Image:
        return WKC::ObjectContentImage;
    case WebCore::ObjectContentType::Frame:
        return WKC::ObjectContentFrame;
    case WebCore::ObjectContentType::PlugIn:
        return WKC::ObjectContentPlugin;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ObjectContentNone;
    }
}

WebCore::ObjectContentType
toWebCoreObjectContentType(WKC::ObjectContentType type)
{
    switch (type) {
    case WKC::ObjectContentNone:
        return WebCore::ObjectContentType::None;
    case WKC::ObjectContentImage:
        return WebCore::ObjectContentType::Image;
    case WKC::ObjectContentFrame:
        return WebCore::ObjectContentType::Frame;
    case WKC::ObjectContentPlugin:
        return WebCore::ObjectContentType::PlugIn;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ObjectContentType::None;
    }
}

WKC::FrameLoadType
toWKCFrameLoadType(WebCore::FrameLoadType type)
{
    switch (type) {
    case WebCore::FrameLoadType::Standard:
        return WKC::FrameLoadType::Standard;
    case WebCore::FrameLoadType::Back:
        return WKC::FrameLoadType::Back;
    case WebCore::FrameLoadType::Forward:
        return WKC::FrameLoadType::Forward;
    case WebCore::FrameLoadType::IndexedBackForward:
        return WKC::FrameLoadType::IndexedBackForward;
    case WebCore::FrameLoadType::Reload:
        return WKC::FrameLoadType::Reload;
    case WebCore::FrameLoadType::Same:
        return WKC::FrameLoadType::Same;
    case WebCore::FrameLoadType::RedirectWithLockedBackForwardList:
        return WKC::FrameLoadType::RedirectWithLockedBackForwardList;
    case WebCore::FrameLoadType::Replace:
        return WKC::FrameLoadType::Replace;
    case WebCore::FrameLoadType::ReloadFromOrigin:
        return WKC::FrameLoadType::ReloadFromOrigin;
    case WebCore::FrameLoadType::ReloadExpiredOnly:
        return WKC::FrameLoadType::ReloadExpiredOnly;
    default:
        ASSERT_NOT_REACHED();
        return WKC::FrameLoadType::Standard;
    }
}

WebCore::FrameLoadType
toWebCoreFrameLoadType(WKC::FrameLoadType type)
{
    switch (type) {
    case WKC::FrameLoadType::Standard:
        return WebCore::FrameLoadType::Standard;
    case WKC::FrameLoadType::Back:
        return WebCore::FrameLoadType::Back;
    case WKC::FrameLoadType::Forward:
        return WebCore::FrameLoadType::Forward;
    case WKC::FrameLoadType::IndexedBackForward:
        return WebCore::FrameLoadType::IndexedBackForward;
    case WKC::FrameLoadType::Reload:
        return WebCore::FrameLoadType::Reload;
    case WKC::FrameLoadType::Same:
        return WebCore::FrameLoadType::Same;
    case WKC::FrameLoadType::RedirectWithLockedBackForwardList:
        return WebCore::FrameLoadType::RedirectWithLockedBackForwardList;
    case WKC::FrameLoadType::Replace:
        return WebCore::FrameLoadType::Replace;
    case WKC::FrameLoadType::ReloadFromOrigin:
        return WebCore::FrameLoadType::ReloadFromOrigin;
    case WKC::FrameLoadType::ReloadExpiredOnly:
        return WebCore::FrameLoadType::ReloadExpiredOnly;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::FrameLoadType::Standard;
    }
}

WKC::NavigationType
toWKCNavigationType(WebCore::NavigationType type)
{
    switch (type) {
    case WebCore::NavigationType::LinkClicked:
        return WKC::NavigationType::LinkClicked;
    case WebCore::NavigationType::FormSubmitted:
        return WKC::NavigationType::FormSubmitted;
    case WebCore::NavigationType::BackForward:
        return WKC::NavigationType::BackForward;
    case WebCore::NavigationType::Reload:
        return WKC::NavigationType::Reload;
    case WebCore::NavigationType::FormResubmitted:
        return WKC::NavigationType::FormResubmitted;
    case WebCore::NavigationType::Other:
        return WKC::NavigationType::Other;
    default:
        ASSERT_NOT_REACHED();
        return WKC::NavigationType::Other;
    }
}

WebCore::NavigationType
toWebCoreNavigationType(WKC::NavigationType type)
{
    switch (type) {
    case WKC::NavigationType::LinkClicked:
        return WebCore::NavigationType::LinkClicked;
    case WKC::NavigationType::FormSubmitted:
        return WebCore::NavigationType::FormSubmitted;
    case WKC::NavigationType::BackForward:
        return WebCore::NavigationType::BackForward;
    case WKC::NavigationType::Reload:
        return WebCore::NavigationType::Reload;
    case WKC::NavigationType::FormResubmitted:
        return WebCore::NavigationType::FormResubmitted;
    case WKC::NavigationType::Other:
        return WebCore::NavigationType::Other;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::NavigationType::Other;
    }
}

WKC::ProtectionSpaceServerType
toWKCProtectionSpaceServerType(WebCore::ProtectionSpaceServerType type)
{
    switch (type) {
    case WebCore::ProtectionSpaceServerHTTP:
        return WKC::ProtectionSpaceServerHTTP;
    case WebCore::ProtectionSpaceServerHTTPS:
        return WKC::ProtectionSpaceServerHTTPS;
    case WebCore::ProtectionSpaceServerFTP:
        return WKC::ProtectionSpaceServerFTP;
    case WebCore::ProtectionSpaceServerFTPS:
        return WKC::ProtectionSpaceServerFTPS;
    case WebCore::ProtectionSpaceProxyHTTP:
        return WKC::ProtectionSpaceProxyHTTP;
    case WebCore::ProtectionSpaceProxyHTTPS:
        return WKC::ProtectionSpaceProxyHTTPS;
    case WebCore::ProtectionSpaceProxyFTP:
        return WKC::ProtectionSpaceProxyFTP;
    case WebCore::ProtectionSpaceProxySOCKS:
        return WKC::ProtectionSpaceProxySOCKS;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ProtectionSpaceServerHTTP;
    }
}

WebCore::ProtectionSpaceServerType
toWebCoreProtectionSpaceServerType(WKC::ProtectionSpaceServerType type)
{
    switch (type) {
    case WKC::ProtectionSpaceServerHTTP:
        return WebCore::ProtectionSpaceServerHTTP;
    case WKC::ProtectionSpaceServerHTTPS:
        return WebCore::ProtectionSpaceServerHTTPS;
    case WKC::ProtectionSpaceServerFTP:
        return WebCore::ProtectionSpaceServerFTP;
    case WKC::ProtectionSpaceServerFTPS:
        return WebCore::ProtectionSpaceServerFTPS;
    case WKC::ProtectionSpaceProxyHTTP:
        return WebCore::ProtectionSpaceProxyHTTP;
    case WKC::ProtectionSpaceProxyHTTPS:
        return WebCore::ProtectionSpaceProxyHTTPS;
    case WKC::ProtectionSpaceProxyFTP:
        return WebCore::ProtectionSpaceProxyFTP;
    case WKC::ProtectionSpaceProxySOCKS:
        return WebCore::ProtectionSpaceProxySOCKS;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ProtectionSpaceServerHTTP;
    }
}

WKC::ProtectionSpaceAuthenticationScheme
toWKCProtectionSpaceAuthenticationScheme(WebCore::ProtectionSpaceAuthenticationScheme scheme)
{
    switch (scheme) {
    case WebCore::ProtectionSpaceAuthenticationSchemeDefault:
        return WKC::ProtectionSpaceAuthenticationSchemeDefault;
    case WebCore::ProtectionSpaceAuthenticationSchemeHTTPBasic:
        return WKC::ProtectionSpaceAuthenticationSchemeHTTPBasic;
    case WebCore::ProtectionSpaceAuthenticationSchemeHTTPDigest:
        return WKC::ProtectionSpaceAuthenticationSchemeHTTPDigest;
    case WebCore::ProtectionSpaceAuthenticationSchemeHTMLForm:
        return WKC::ProtectionSpaceAuthenticationSchemeHTMLForm;
    case WebCore::ProtectionSpaceAuthenticationSchemeNTLM:
        return WKC::ProtectionSpaceAuthenticationSchemeNTLM;
    case WebCore::ProtectionSpaceAuthenticationSchemeNegotiate:
        return WKC::ProtectionSpaceAuthenticationSchemeNegotiate;
    case WebCore::ProtectionSpaceAuthenticationSchemeClientCertificateRequested:
        return WKC::ProtectionSpaceAuthenticationSchemeClientCertificateRequested;
    case WebCore::ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested:
        return WKC::ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested;
    case WebCore::ProtectionSpaceAuthenticationSchemeUnknown:
        return WKC::ProtectionSpaceAuthenticationSchemeUnknown;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ProtectionSpaceAuthenticationSchemeDefault;
    }
}

WebCore::ProtectionSpaceAuthenticationScheme
toWebCoreProtectionSpaceAuthenticationScheme(WKC::ProtectionSpaceAuthenticationScheme scheme)
{
    switch (scheme) {
    case WKC::ProtectionSpaceAuthenticationSchemeDefault:
        return WebCore::ProtectionSpaceAuthenticationSchemeDefault;
    case WKC::ProtectionSpaceAuthenticationSchemeHTTPBasic:
        return WebCore::ProtectionSpaceAuthenticationSchemeHTTPBasic;
    case WKC::ProtectionSpaceAuthenticationSchemeHTTPDigest:
        return WebCore::ProtectionSpaceAuthenticationSchemeHTTPDigest;
    case WKC::ProtectionSpaceAuthenticationSchemeHTMLForm:
        return WebCore::ProtectionSpaceAuthenticationSchemeHTMLForm;
    case WKC::ProtectionSpaceAuthenticationSchemeNTLM:
        return WebCore::ProtectionSpaceAuthenticationSchemeNTLM;
    case WKC::ProtectionSpaceAuthenticationSchemeNegotiate:
        return WebCore::ProtectionSpaceAuthenticationSchemeNegotiate;
    case WKC::ProtectionSpaceAuthenticationSchemeClientCertificateRequested:
        return WebCore::ProtectionSpaceAuthenticationSchemeClientCertificateRequested;
    case WKC::ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested:
        return WebCore::ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested;
    case WKC::ProtectionSpaceAuthenticationSchemeUnknown:
        return WebCore::ProtectionSpaceAuthenticationSchemeUnknown;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ProtectionSpaceAuthenticationSchemeDefault;
    }
}

WKC::CredentialPersistence
toWKCCredentialPersistence(WebCore::CredentialPersistence cp)
{
    switch (cp) {
    case WebCore::CredentialPersistenceNone:
        return WKC::CredentialPersistenceNone;
    case WebCore::CredentialPersistenceForSession:
        return WKC::CredentialPersistenceForSession;
    case WebCore::CredentialPersistencePermanent:
        return WKC::CredentialPersistencePermanent;
    default:
        ASSERT_NOT_REACHED();
        return WKC::CredentialPersistenceNone;
    }
}

WebCore::CredentialPersistence
toWebCoreCredentialPersistence(WKC::CredentialPersistence cp)
{
    switch (cp) {
    case WKC::CredentialPersistenceNone:
        return WebCore::CredentialPersistenceNone;
    case WKC::CredentialPersistenceForSession:
        return WebCore::CredentialPersistenceForSession;
    case WKC::CredentialPersistencePermanent:
        return WebCore::CredentialPersistencePermanent;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::CredentialPersistenceNone;
    }
}

WKC::StorageType
toWKCStorageType(WebCore::StorageType type)
{
    switch (type) {
    case WebCore::StorageType::Local:
        return WKC::LocalStorage;
    case WebCore::StorageType::Session:
        return WKC::SessionStorage;
    case WebCore::StorageType::EphemeralLocal:
        return WKC::EphemeralLocalStorage;
    case WebCore::StorageType::TransientLocal:
        return WKC::TransientLocalStorage;
    default:
        ASSERT_NOT_REACHED();
        return WKC::LocalStorage;
    }
}

WebCore::StorageType
toWebCoreStorageType(WKC::StorageType type)
{
    switch (type) {
    case WKC::LocalStorage:
        return WebCore::StorageType::Local;
    case WKC::SessionStorage:
        return WebCore::StorageType::Session;
    case WKC::EphemeralLocalStorage:
        return WebCore::StorageType::EphemeralLocal;
    case WKC::TransientLocalStorage:
        return WebCore::StorageType::TransientLocal;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::StorageType::Session;
    }
}

WKC::BlendMode
toWKCBlendMode(WebCore::BlendMode mode)
{
    switch (mode) {
    case WebCore::BlendModeNormal:
        return WKC::BlendModeNormal;
    case WebCore::BlendModeMultiply:
        return WKC::BlendModeMultiply;
    case WebCore::BlendModeScreen:
        return WKC::BlendModeScreen;
    case WebCore::BlendModeDarken:
        return WKC::BlendModeDarken;
    case WebCore::BlendModeLighten:
        return WKC::BlendModeLighten;
    case WebCore::BlendModeOverlay:
        return WKC::BlendModeOverlay;
    case WebCore::BlendModeColorDodge:
        return WKC::BlendModeColorDodge;
    case WebCore::BlendModeColorBurn:
        return WKC::BlendModeColorBurn;
    case WebCore::BlendModeHardLight:
        return WKC::BlendModeHardLight;
    case WebCore::BlendModeSoftLight:
        return WKC::BlendModeSoftLight;
    case WebCore::BlendModeDifference:
        return WKC::BlendModeDifference;
    case WebCore::BlendModeExclusion:
        return WKC::BlendModeExclusion;
    case WebCore::BlendModeHue:
        return WKC::BlendModeHue;
    case WebCore::BlendModeSaturation:
        return WKC::BlendModeSaturation;
    case WebCore::BlendModeColor:
        return WKC::BlendModeColor;
    case WebCore::BlendModeLuminosity:
        return WKC::BlendModeLuminosity;
    case WebCore::BlendModePlusDarker:
        return WKC::BlendModePlusDarker;
    case WebCore::BlendModePlusLighter:
        return WKC::BlendModePlusLighter;
    default:
        ASSERT_NOT_REACHED();
        return WKC::BlendModeNormal;
    }
}

WebCore::BlendMode
toWebCoreBlendMode(WKC::BlendMode mode)
{
    switch (mode) {
    case WKC::BlendModeNormal:
        return WebCore::BlendModeNormal;
    case WKC::BlendModeMultiply:
        return WebCore::BlendModeMultiply;
    case WKC::BlendModeScreen:
        return WebCore::BlendModeScreen;
    case WKC::BlendModeDarken:
        return WebCore::BlendModeDarken;
    case WKC::BlendModeLighten:
        return WebCore::BlendModeLighten;
    case WKC::BlendModeOverlay:
        return WebCore::BlendModeOverlay;
    case WKC::BlendModeColorDodge:
        return WebCore::BlendModeColorDodge;
    case WKC::BlendModeColorBurn:
        return WebCore::BlendModeColorBurn;
    case WKC::BlendModeHardLight:
        return WebCore::BlendModeHardLight;
    case WKC::BlendModeSoftLight:
        return WebCore::BlendModeSoftLight;
    case WKC::BlendModeDifference:
        return WebCore::BlendModeDifference;
    case WKC::BlendModeExclusion:
        return WebCore::BlendModeExclusion;
    case WKC::BlendModeHue:
        return WebCore::BlendModeHue;
    case WKC::BlendModeSaturation:
        return WebCore::BlendModeSaturation;
    case WKC::BlendModeColor:
        return WebCore::BlendModeColor;
    case WKC::BlendModeLuminosity:
        return WebCore::BlendModeLuminosity;
    case WKC::BlendModePlusDarker:
        return WebCore::BlendModePlusDarker;
    case WKC::BlendModePlusLighter:
        return WebCore::BlendModePlusLighter;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::BlendModeNormal;
    }
}

WKC::SelectionRestorationMode
toWKCSelectionRestorationMode(WebCore::SelectionRestorationMode mode)
{
    switch (mode) {
    case WebCore::SelectionRestorationMode::Restore:
        return WKC::SelectionRestorationMode::Restore;
    case WebCore::SelectionRestorationMode::SetDefault:
        return WKC::SelectionRestorationMode::SetDefault;
    default:
        ASSERT_NOT_REACHED();
        return WKC::SelectionRestorationMode::Restore;
    }
}

WebCore::SelectionRestorationMode
toWebCoreSelectionRestorationMode(WKC::SelectionRestorationMode mode)
{
    switch (mode) {
    case WKC::SelectionRestorationMode::Restore:
        return WebCore::SelectionRestorationMode::Restore;
    case WKC::SelectionRestorationMode::SetDefault:
        return WebCore::SelectionRestorationMode::SetDefault;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::SelectionRestorationMode::Restore;
    }
}

WKC::SelectionRevealMode 
toWKCSelectionRevealMode(WebCore::SelectionRevealMode mode)
{
    switch (mode) {
    case WebCore::SelectionRevealMode::Reveal:
        return WKC::SelectionRevealMode::Reveal;
    case WebCore::SelectionRevealMode::RevealUpToMainFrame:
        return WKC::SelectionRevealMode::RevealUpToMainFrame;
    case WebCore::SelectionRevealMode::DoNotReveal:
        return WKC::SelectionRevealMode::DoNotReveal;
    default:
        ASSERT_NOT_REACHED();
        return WKC::SelectionRevealMode::Reveal;
    }
}

WebCore::SelectionRevealMode
toWebCoreSelectionRevealMode(WKC::SelectionRevealMode mode)
{
    switch (mode) {
    case WKC::SelectionRevealMode::Reveal:
        return WebCore::SelectionRevealMode::Reveal;
    case WKC::SelectionRevealMode::RevealUpToMainFrame:
        return WebCore::SelectionRevealMode::RevealUpToMainFrame;
    case WKC::SelectionRevealMode::DoNotReveal:
        return WebCore::SelectionRevealMode::DoNotReveal;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::SelectionRevealMode::Reveal;
    }
}

WKC::ResourceErrorType
toWKCResourceErrorType(WebCore::ResourceErrorBase::Type type)
{
    switch (type) {
    case WebCore::ResourceErrorBase::Type::Null:
        return WKC::ResourceErrorType::ResourceErrorNull;
    case WebCore::ResourceErrorBase::Type::General:
        return WKC::ResourceErrorType::ResourceErrorGeneral;
    case WebCore::ResourceErrorBase::Type::AccessControl:
        return WKC::ResourceErrorType::ResourceErrorAccessControl;
    case WebCore::ResourceErrorBase::Type::Cancellation:
        return WKC::ResourceErrorType::ResourceErrorCancellation;
    case WebCore::ResourceErrorBase::Type::Timeout:
        return WKC::ResourceErrorType::ResourceErrorTimeout;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ResourceErrorType::ResourceErrorNull;
    }
}

WebCore::ResourceErrorBase::Type
toWebCoreResourceErrorType(WKC::ResourceErrorType type)
{
    switch (type) {
    case WKC::ResourceErrorType::ResourceErrorNull:
        return WebCore::ResourceErrorBase::Type::Null;
    case WKC::ResourceErrorType::ResourceErrorGeneral:
        return WebCore::ResourceErrorBase::Type::General;
    case WKC::ResourceErrorType::ResourceErrorAccessControl:
        return WebCore::ResourceErrorBase::Type::AccessControl;
    case WKC::ResourceErrorType::ResourceErrorCancellation:
        return WebCore::ResourceErrorBase::Type::Cancellation;
    case WKC::ResourceErrorType::ResourceErrorTimeout:
        return WebCore::ResourceErrorBase::Type::Timeout;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ResourceErrorBase::Type::Null;
    }
}

WKC::FrameFlattening
toWKCFrameFlattening(WebCore::FrameFlattening flattening)
{
    switch (flattening) {
    case WebCore::FrameFlattening::Disabled:
        return WKC::FrameFlattening::FrameFlatteningDisabled;
    case WebCore::FrameFlattening::EnabledForNonFullScreenIFrames:
        return WKC::FrameFlattening::FrameFlatteningEnabledForNonFullScreenIFrames;
    case WebCore::FrameFlattening::FullyEnabled:
        return WKC::FrameFlattening::FrameFlatteningFullyEnabled;
    default:
        ASSERT_NOT_REACHED();
        return WKC::FrameFlattening::FrameFlatteningDisabled;
    }
}

WebCore::FrameFlattening 
toWebCoreFrameFlattening(WKC::FrameFlattening flattening)
{
    switch (flattening) {
    case WKC::FrameFlattening::FrameFlatteningDisabled:
        return WebCore::FrameFlattening::Disabled;
    case WKC::FrameFlattening::FrameFlatteningEnabledForNonFullScreenIFrames:
        return WebCore::FrameFlattening::EnabledForNonFullScreenIFrames;
    case WKC::FrameFlattening::FrameFlatteningFullyEnabled:
        return WebCore::FrameFlattening::FullyEnabled;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::FrameFlattening::Disabled;
    }
}

int
toWKCTextBreakIteratorType(UBreakIteratorType type)
{
    switch (type) {
    case UBRK_CHARACTER:
        return WKC_TEXTBREAKITERATOR_TYPE_CHARACTER;
    case UBRK_WORD:
        return WKC_TEXTBREAKITERATOR_TYPE_WORD;
    case UBRK_LINE:
        return WKC_TEXTBREAKITERATOR_TYPE_LINE;
    case UBRK_SENTENCE:
        return WKC_TEXTBREAKITERATOR_TYPE_SENTENCE;
    case UBRK_COUNT:
    default:
        ASSERT_NOT_REACHED();
        return WKC_TEXTBREAKITERATOR_TYPE_CHARACTER;
    }
}

UBreakIteratorType
toUBreakIteratorType(int type)
{
    switch (type) {
    case WKC_TEXTBREAKITERATOR_TYPE_CHARACTER:
    case WKC_TEXTBREAKITERATOR_TYPE_CHARACTER_NONSHARED:
        return UBRK_CHARACTER;
    case WKC_TEXTBREAKITERATOR_TYPE_WORD:
        return UBRK_WORD;
    case WKC_TEXTBREAKITERATOR_TYPE_LINE:
        return UBRK_LINE;
    case WKC_TEXTBREAKITERATOR_TYPE_SENTENCE:
        return UBRK_SENTENCE;
    case WKC_TEXTBREAKITERATOR_TYPE_CURSORMOVEMENT:
    default:
        ASSERT_NOT_REACHED();
        return UBRK_CHARACTER;
    }
}

}
