/*
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
    case WebCore::TextDirection::LTR:
        return WKC::LTR;
    case WebCore::TextDirection::RTL:
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
        return WebCore::TextDirection::LTR;
    case WKC::RTL:
        return WebCore::TextDirection::RTL;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::TextDirection::LTR;
    }
}

WKC::FocusDirection
toWKCFocusDirection(WebCore::FocusDirection dir)
{
    switch (dir) {
    case WebCore::FocusDirection::None:
        return WKC::FocusDirection::None;
    case WebCore::FocusDirection::Forward:
        return WKC::FocusDirection::Forward;
    case WebCore::FocusDirection::Backward:
        return WKC::FocusDirection::Backward;
    case WebCore::FocusDirection::Up:
        return WKC::FocusDirection::Up;
    case WebCore::FocusDirection::Down:
        return WKC::FocusDirection::Down;
    case WebCore::FocusDirection::Left:
        return WKC::FocusDirection::Left;
    case WebCore::FocusDirection::Right:
        return WKC::FocusDirection::Right;
    case WebCore::FocusDirection::UpLeft:
        return WKC::FocusDirection::UpLeft;
    case WebCore::FocusDirection::UpRight:
        return WKC::FocusDirection::UpRight;
    case WebCore::FocusDirection::DownLeft:
        return WKC::FocusDirection::DownLeft;
    case WebCore::FocusDirection::DownRight:
        return WKC::FocusDirection::DownRight;
    default:
        ASSERT_NOT_REACHED();
        return WKC::FocusDirection::None;
    }
}

WebCore::FocusDirection
toWebCoreFocusDirection(WKC::FocusDirection dir)
{
    switch (dir) {
    case WKC::FocusDirection::None:
        return WebCore::FocusDirection::None;
    case WKC::FocusDirection::Forward:
        return WebCore::FocusDirection::Forward;
    case WKC::FocusDirection::Backward:
        return WebCore::FocusDirection::Backward;
    case WKC::FocusDirection::Up:
        return WebCore::FocusDirection::Up;
    case WKC::FocusDirection::Down:
        return WebCore::FocusDirection::Down;
    case WKC::FocusDirection::Left:
        return WebCore::FocusDirection::Left;
    case WKC::FocusDirection::Right:
        return WebCore::FocusDirection::Right;
    case WKC::FocusDirection::UpLeft:
        return WebCore::FocusDirection::UpLeft;
    case WKC::FocusDirection::UpRight:
        return WebCore::FocusDirection::UpRight;
    case WKC::FocusDirection::DownLeft:
        return WebCore::FocusDirection::DownLeft;
    case WKC::FocusDirection::DownRight:
        return WebCore::FocusDirection::DownRight;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::FocusDirection::None;
    }
}

bool
convertToSpatialNavigationDirection(WKC::FocusDirection from, WebCore::SpatialNavigationDirection& to)
{
    switch (from) {
    case WKC::FocusDirection::Up:
        to = WebCore::SpatialNavigationDirection::Up;
        break;
    case WKC::FocusDirection::Down:
        to = WebCore::SpatialNavigationDirection::Down;
        break;
    case WKC::FocusDirection::Left:
        to = WebCore::SpatialNavigationDirection::Left;
        break;
    case WKC::FocusDirection::Right:
        to = WebCore::SpatialNavigationDirection::Right;
        break;
    case WKC::FocusDirection::UpLeft:
        to = WebCore::SpatialNavigationDirection::UpLeft;
        break;
    case WKC::FocusDirection::UpRight:
        to = WebCore::SpatialNavigationDirection::UpRight;
        break;
    case WKC::FocusDirection::DownLeft:
        to = WebCore::SpatialNavigationDirection::DownLeft;
        break;
    case WKC::FocusDirection::DownRight:
        to = WebCore::SpatialNavigationDirection::DownRight;
        break;
    default:
        return false;
    }
    return true;
}


WKC::DragDestinationAction
toWKCDragDestinationAction(WebCore::DragDestinationAction action)
{
    switch (action) {
    case WebCore::DragDestinationAction::DHTML:
        return WKC::DragDestinationAction::DHTML;
    case WebCore::DragDestinationAction::Edit:
        return WKC::DragDestinationAction::Edit;
    case WebCore::DragDestinationAction::Load:
        return WKC::DragDestinationAction::Load;
    default:
        ASSERT_NOT_REACHED();
        return WKC::DragDestinationAction::None;
    }
}

WebCore::DragDestinationAction
toWebCoreDragDestinationAction(WKC::DragDestinationAction action)
{
    switch (action) {
    case WKC::DragDestinationAction::DHTML:
        return WebCore::DragDestinationAction::DHTML;
    case WKC::DragDestinationAction::Edit:
        return WebCore::DragDestinationAction::Edit;
    case WKC::DragDestinationAction::Load:
        return WebCore::DragDestinationAction::Load;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::DragDestinationAction(0);
    }
}

WKC::DragSourceAction
toWKCDragSourceAction(WebCore::DragSourceAction action)
{
    switch (action) {
    case WebCore::DragSourceAction::DHTML:
        return WKC::DragSourceAction::DHTML;
    case WebCore::DragSourceAction::Image:
        return WKC::DragSourceAction::Image;
    case WebCore::DragSourceAction::Link:
        return WKC::DragSourceAction::Link;
    case WebCore::DragSourceAction::Selection:
        return WKC::DragSourceAction::Selection;
    default:
        ASSERT_NOT_REACHED();
        return WKC::DragSourceAction::None;
    }
}

WebCore::DragSourceAction
toWebCoreDragSourceAction(WKC::DragSourceAction action)
{
    switch (action) {
    case WKC::DragSourceAction::DHTML:
        return WebCore::DragSourceAction::DHTML;
    case WKC::DragSourceAction::Image:
        return WebCore::DragSourceAction::Image;
    case WKC::DragSourceAction::Link:
        return WebCore::DragSourceAction::Link;
    case WKC::DragSourceAction::Selection:
        return WebCore::DragSourceAction::Selection;
#if ENABLE(ATTACHMENT_ELEMENT)
    case WKC::DragSourceAction::Attachment:
        return WebCore::DragSourceAction::Attachment;
#endif
    default:
        ASSERT_NOT_REACHED();
        return WebCore::DragSourceAction(0);
    }
}

WKC::DragOperation
toWKCDragOperation(WebCore::DragOperation op)
{
    switch (op) {
    case WebCore::DragOperation::Copy:
        return WKC::DragOperation::Copy;
    case WebCore::DragOperation::Link:
        return WKC::DragOperation::Link;
    case WebCore::DragOperation::Generic:
        return WKC::DragOperation::Generic;
    case WebCore::DragOperation::Private:
        return WKC::DragOperation::Private;
    case WebCore::DragOperation::Move:
        return WKC::DragOperation::Move;
    case WebCore::DragOperation::Delete:
        return WKC::DragOperation::Delete;
    default:
        ASSERT_NOT_REACHED();
        return WKC::DragOperation::None;
    }
}

WebCore::DragOperation
toWebCoreDragOperation(WKC::DragOperation op)
{
    switch (op) {
    case WKC::DragOperation::Copy:
        return WebCore::DragOperation::Copy;
    case WKC::DragOperation::Link:
        return WebCore::DragOperation::Link;
    case WKC::DragOperation::Generic:
        return WebCore::DragOperation::Generic;
    case WKC::DragOperation::Private:
        return WebCore::DragOperation::Private;
    case WKC::DragOperation::Move:
        return WebCore::DragOperation::Move;
    case WKC::DragOperation::Delete:
        return WebCore::DragOperation::Delete;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::DragOperation(0);
    }
}

WKC::ScrollbarOrientation
toWKCScrollbarOrientation(WebCore::ScrollbarOrientation orientation)
{
    switch (orientation) {
    case WebCore::ScrollbarOrientation::Horizontal:
        return WKC::ScrollbarOrientation::HorizontalScrollbar;
    case WebCore::ScrollbarOrientation::Vertical:
        return WKC::ScrollbarOrientation::VerticalScrollbar;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ScrollbarOrientation::HorizontalScrollbar;
    }
}

WebCore::ScrollbarOrientation
toWebCoreScrollbarOrientation(WKC::ScrollbarOrientation orientation)
{
    switch (orientation) {
    case WKC::ScrollbarOrientation::HorizontalScrollbar:
        return WebCore::ScrollbarOrientation::Horizontal;
    case WKC::ScrollbarOrientation::VerticalScrollbar:
        return WebCore::ScrollbarOrientation::Vertical;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ScrollbarOrientation::Horizontal;
    }
}

WKC::ScrollbarControlSize
toWKCScrollbarControlSize(WebCore::ScrollbarControlSize size)
{
    switch (size) {
    case WebCore::ScrollbarControlSize::Regular:
        return WKC::ScrollbarControlSize::Regular;
    case WebCore::ScrollbarControlSize::Small:
        return WKC::ScrollbarControlSize::Small;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ScrollbarControlSize::Regular;
    }
}

WebCore::ScrollbarControlSize
toWebCoreScrollbarControlSize(WKC::ScrollbarControlSize size)
{
    switch (size) {
    case WKC::ScrollbarControlSize::Regular:
        return WebCore::ScrollbarControlSize::Regular;
    case WKC::ScrollbarControlSize::Small:
        return WebCore::ScrollbarControlSize::Small;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ScrollbarControlSize::Regular;
    }
}

WKC::ScrollDirection
toWKCScrollDirection(WebCore::ScrollDirection dir)
{
    switch (dir) {
    case WebCore::ScrollDirection::ScrollUp:
        return WKC::ScrollDirection::ScrollUp;
    case WebCore::ScrollDirection::ScrollDown:
        return WKC::ScrollDirection::ScrollDown;
    case WebCore::ScrollDirection::ScrollLeft:
        return WKC::ScrollDirection::ScrollLeft;
    case WebCore::ScrollDirection::ScrollRight:
        return WKC::ScrollDirection::ScrollRight;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ScrollDirection::ScrollUp;
    }
}

WebCore::ScrollDirection
toWebCoreScrollDirection(WKC::ScrollDirection dir)
{
    switch (dir) {
    case WKC::ScrollDirection::ScrollUp:
        return WebCore::ScrollDirection::ScrollUp;
    case WKC::ScrollDirection::ScrollDown:
        return WebCore::ScrollDown;
    case WKC::ScrollDirection::ScrollLeft:
        return WebCore::ScrollLeft;
    case WKC::ScrollDirection::ScrollRight:
        return WebCore::ScrollDirection::ScrollRight;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ScrollDirection::ScrollUp;
    }
}

WKC::EditorInsertAction
toWKCEditorInsertAction(WebCore::EditorInsertAction action)
{
    switch (action) {
    case WebCore::EditorInsertAction::Typed:
        return WKC::EditorInsertAction::Typed;
    case WebCore::EditorInsertAction::Pasted:
        return WKC::EditorInsertAction::Pasted;
    case WebCore::EditorInsertAction::Dropped:
        return WKC::EditorInsertAction::Dropped;
    default:
        ASSERT_NOT_REACHED();
        return WKC::EditorInsertAction::Typed;
    }
}

WebCore::EditorInsertAction
toWebCoreEditorInsertAction(WKC::EditorInsertAction action)
{
    switch (action) {
    case WKC::EditorInsertAction::Typed:
        return WebCore::EditorInsertAction::Typed;
    case WKC::EditorInsertAction::Pasted:
        return WebCore::EditorInsertAction::Pasted;
    case WKC::EditorInsertAction::Dropped:
        return WebCore::EditorInsertAction::Dropped;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::EditorInsertAction::Typed;
    }
}

WKC::EAffinity
toWKCEAffinity(WebCore::Affinity affinity)
{
    switch (affinity) {
    case WebCore::Affinity::Upstream:
        return WKC::UPSTREAM;
    case WebCore::Affinity::Downstream:
        return WKC::DOWNSTREAM;
    default:
        ASSERT_NOT_REACHED();
        return WKC::UPSTREAM;
    }
}

WebCore::Affinity
toWebCoreEAffinity(WKC::EAffinity affinity)
{
    switch (affinity) {
    case WKC::UPSTREAM:
        return WebCore::Affinity::Upstream;
    case WKC::DOWNSTREAM:
        return WebCore::Affinity::Downstream;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::Affinity::Upstream;
    }
}

WKC::PolicyAction
toWKCPolicyAction(WebCore::PolicyAction action)
{
    switch (action) {
    case WebCore::PolicyAction::Use:
        return WKC::PolicyAction::Use;
    case WebCore::PolicyAction::Download:
        return WKC::PolicyAction::Download;
    case WebCore::PolicyAction::Ignore:
        return WKC::PolicyAction::Ignore;
    case WebCore::PolicyAction::StopAllLoads:
        return WKC::PolicyAction::StopAllLoads;
    default:
        ASSERT_NOT_REACHED();
        return WKC::PolicyAction::Use;
    }
}

WebCore::PolicyAction
toWebCorePolicyAction(WKC::PolicyAction action)
{
    switch (action) {
    case WKC::PolicyAction::Use:
        return WebCore::PolicyAction::Use;
    case WKC::PolicyAction::Download:
        return WebCore::PolicyAction::Download;
    case WKC::PolicyAction::Ignore:
        return WebCore::PolicyAction::Ignore;
    case WKC::PolicyAction::StopAllLoads:
        return WebCore::PolicyAction::StopAllLoads;
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
        return WKC::ObjectContentType::None;
    case WebCore::ObjectContentType::Image:
        return WKC::ObjectContentType::Image;
    case WebCore::ObjectContentType::Frame:
        return WKC::ObjectContentType::Frame;
    case WebCore::ObjectContentType::PlugIn:
        return WKC::ObjectContentType::Plugin;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ObjectContentType::None;
    }
}

WebCore::ObjectContentType
toWebCoreObjectContentType(WKC::ObjectContentType type)
{
    switch (type) {
    case WKC::ObjectContentType::None:
        return WebCore::ObjectContentType::None;
    case WKC::ObjectContentType::Image:
        return WebCore::ObjectContentType::Image;
    case WKC::ObjectContentType::Frame:
        return WebCore::ObjectContentType::Frame;
    case WKC::ObjectContentType::Plugin:
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

WKC::MediaSourcePolicy
toWKCMediaSourcePolicy(WebCore::MediaSourcePolicy policy)
{
    switch (policy) {
    case WebCore::MediaSourcePolicy::Default:
        return WKC::MediaSourcePolicy::Default;
    case WebCore::MediaSourcePolicy::Disable:
        return WKC::MediaSourcePolicy::Disable;
    case WebCore::MediaSourcePolicy::Enable:
        return WKC::MediaSourcePolicy::Enable;
    default:
        ASSERT_NOT_REACHED();
        return WKC::MediaSourcePolicy::Default;
    }
}

WebCore::MediaSourcePolicy
toWebCoreMediaSourcePolicy(WKC::MediaSourcePolicy policy)
{
    switch (policy) {
    case WKC::MediaSourcePolicy::Default:
        return WebCore::MediaSourcePolicy::Default;
    case WKC::MediaSourcePolicy::Disable:
        return WebCore::MediaSourcePolicy::Disable;
    case WKC::MediaSourcePolicy::Enable:
        return WebCore::MediaSourcePolicy::Enable;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::MediaSourcePolicy::Default;
    }
}

WKC::ProtectionSpaceServerType
toWKCProtectionSpaceServerType(WebCore::ProtectionSpace::ServerType type)
{
    switch (type) {
    case WebCore::ProtectionSpace::ServerType::HTTP:
        return WKC::ProtectionSpaceServerHTTP;
    case WebCore::ProtectionSpace::ServerType::HTTPS:
        return WKC::ProtectionSpaceServerHTTPS;
    case WebCore::ProtectionSpace::ServerType::FTP:
        return WKC::ProtectionSpaceServerFTP;
    case WebCore::ProtectionSpace::ServerType::FTPS:
        return WKC::ProtectionSpaceServerFTPS;
    case WebCore::ProtectionSpace::ServerType::ProxyHTTP:
        return WKC::ProtectionSpaceProxyHTTP;
    case WebCore::ProtectionSpace::ServerType::ProxyHTTPS:
        return WKC::ProtectionSpaceProxyHTTPS;
    case WebCore::ProtectionSpace::ServerType::ProxyFTP:
        return WKC::ProtectionSpaceProxyFTP;
    case WebCore::ProtectionSpace::ServerType::ProxySOCKS:
        return WKC::ProtectionSpaceProxySOCKS;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ProtectionSpaceServerHTTP;
    }
}

WebCore::ProtectionSpace::ServerType
toWebCoreProtectionSpaceServerType(WKC::ProtectionSpaceServerType type)
{
    switch (type) {
    case WKC::ProtectionSpaceServerHTTP:
        return WebCore::ProtectionSpace::ServerType::HTTP;
    case WKC::ProtectionSpaceServerHTTPS:
        return WebCore::ProtectionSpace::ServerType::HTTPS;
    case WKC::ProtectionSpaceServerFTP:
        return WebCore::ProtectionSpace::ServerType::FTP;
    case WKC::ProtectionSpaceServerFTPS:
        return WebCore::ProtectionSpace::ServerType::FTPS;
    case WKC::ProtectionSpaceProxyHTTP:
        return WebCore::ProtectionSpace::ServerType::ProxyHTTP;
    case WKC::ProtectionSpaceProxyHTTPS:
        return WebCore::ProtectionSpace::ServerType::ProxyHTTPS;
    case WKC::ProtectionSpaceProxyFTP:
        return WebCore::ProtectionSpace::ServerType::ProxyFTP;
    case WKC::ProtectionSpaceProxySOCKS:
        return WebCore::ProtectionSpace::ServerType::ProxySOCKS;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ProtectionSpace::ServerType::HTTP;
    }
}

WKC::ProtectionSpaceAuthenticationScheme
toWKCProtectionSpaceAuthenticationScheme(WebCore::ProtectionSpace::AuthenticationScheme scheme)
{
    switch (scheme) {
    case WebCore::ProtectionSpace::AuthenticationScheme::Default:
        return WKC::ProtectionSpaceAuthenticationSchemeDefault;
    case WebCore::ProtectionSpace::AuthenticationScheme::HTTPBasic:
        return WKC::ProtectionSpaceAuthenticationSchemeHTTPBasic;
    case WebCore::ProtectionSpace::AuthenticationScheme::HTTPDigest:
        return WKC::ProtectionSpaceAuthenticationSchemeHTTPDigest;
    case WebCore::ProtectionSpace::AuthenticationScheme::HTMLForm:
        return WKC::ProtectionSpaceAuthenticationSchemeHTMLForm;
    case WebCore::ProtectionSpace::AuthenticationScheme::NTLM:
        return WKC::ProtectionSpaceAuthenticationSchemeNTLM;
    case WebCore::ProtectionSpace::AuthenticationScheme::Negotiate:
        return WKC::ProtectionSpaceAuthenticationSchemeNegotiate;
    case WebCore::ProtectionSpace::AuthenticationScheme::ClientCertificateRequested:
        return WKC::ProtectionSpaceAuthenticationSchemeClientCertificateRequested;
    case WebCore::ProtectionSpace::AuthenticationScheme::ServerTrustEvaluationRequested:
        return WKC::ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested;
    case WebCore::ProtectionSpace::AuthenticationScheme::Unknown:
        return WKC::ProtectionSpaceAuthenticationSchemeUnknown;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ProtectionSpaceAuthenticationSchemeDefault;
    }
}

WebCore::ProtectionSpace::AuthenticationScheme
toWebCoreProtectionSpaceAuthenticationScheme(WKC::ProtectionSpaceAuthenticationScheme scheme)
{
    switch (scheme) {
    case WKC::ProtectionSpaceAuthenticationSchemeDefault:
        return WebCore::ProtectionSpace::AuthenticationScheme::Default;
    case WKC::ProtectionSpaceAuthenticationSchemeHTTPBasic:
        return WebCore::ProtectionSpace::AuthenticationScheme::HTTPBasic;
    case WKC::ProtectionSpaceAuthenticationSchemeHTTPDigest:
        return WebCore::ProtectionSpace::AuthenticationScheme::HTTPDigest;
    case WKC::ProtectionSpaceAuthenticationSchemeHTMLForm:
        return WebCore::ProtectionSpace::AuthenticationScheme::HTMLForm;
    case WKC::ProtectionSpaceAuthenticationSchemeNTLM:
        return WebCore::ProtectionSpace::AuthenticationScheme::NTLM;
    case WKC::ProtectionSpaceAuthenticationSchemeNegotiate:
        return WebCore::ProtectionSpace::AuthenticationScheme::Negotiate;
    case WKC::ProtectionSpaceAuthenticationSchemeClientCertificateRequested:
        return WebCore::ProtectionSpace::AuthenticationScheme::ClientCertificateRequested;
    case WKC::ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested:
        return WebCore::ProtectionSpace::AuthenticationScheme::ServerTrustEvaluationRequested;
    case WKC::ProtectionSpaceAuthenticationSchemeUnknown:
        return WebCore::ProtectionSpace::AuthenticationScheme::Unknown;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ProtectionSpace::AuthenticationScheme::Default;
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
    case WebCore::BlendMode::Normal:
        return WKC::BlendModeNormal;
    case WebCore::BlendMode::Multiply:
        return WKC::BlendModeMultiply;
    case WebCore::BlendMode::Screen:
        return WKC::BlendModeScreen;
    case WebCore::BlendMode::Darken:
        return WKC::BlendModeDarken;
    case WebCore::BlendMode::Lighten:
        return WKC::BlendModeLighten;
    case WebCore::BlendMode::Overlay:
        return WKC::BlendModeOverlay;
    case WebCore::BlendMode::ColorDodge:
        return WKC::BlendModeColorDodge;
    case WebCore::BlendMode::ColorBurn:
        return WKC::BlendModeColorBurn;
    case WebCore::BlendMode::HardLight:
        return WKC::BlendModeHardLight;
    case WebCore::BlendMode::SoftLight:
        return WKC::BlendModeSoftLight;
    case WebCore::BlendMode::Difference:
        return WKC::BlendModeDifference;
    case WebCore::BlendMode::Exclusion:
        return WKC::BlendModeExclusion;
    case WebCore::BlendMode::Hue:
        return WKC::BlendModeHue;
    case WebCore::BlendMode::Saturation:
        return WKC::BlendModeSaturation;
    case WebCore::BlendMode::Color:
        return WKC::BlendModeColor;
    case WebCore::BlendMode::Luminosity:
        return WKC::BlendModeLuminosity;
    case WebCore::BlendMode::PlusDarker:
        return WKC::BlendModePlusDarker;
    case WebCore::BlendMode::PlusLighter:
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
        return WebCore::BlendMode::Normal;
    case WKC::BlendModeMultiply:
        return WebCore::BlendMode::Multiply;
    case WKC::BlendModeScreen:
        return WebCore::BlendMode::Screen;
    case WKC::BlendModeDarken:
        return WebCore::BlendMode::Darken;
    case WKC::BlendModeLighten:
        return WebCore::BlendMode::Lighten;
    case WKC::BlendModeOverlay:
        return WebCore::BlendMode::Overlay;
    case WKC::BlendModeColorDodge:
        return WebCore::BlendMode::ColorDodge;
    case WKC::BlendModeColorBurn:
        return WebCore::BlendMode::ColorBurn;
    case WKC::BlendModeHardLight:
        return WebCore::BlendMode::HardLight;
    case WKC::BlendModeSoftLight:
        return WebCore::BlendMode::SoftLight;
    case WKC::BlendModeDifference:
        return WebCore::BlendMode::Difference;
    case WKC::BlendModeExclusion:
        return WebCore::BlendMode::Exclusion;
    case WKC::BlendModeHue:
        return WebCore::BlendMode::Hue;
    case WKC::BlendModeSaturation:
        return WebCore::BlendMode::Saturation;
    case WKC::BlendModeColor:
        return WebCore::BlendMode::Color;
    case WKC::BlendModeLuminosity:
        return WebCore::BlendMode::Luminosity;
    case WKC::BlendModePlusDarker:
        return WebCore::BlendMode::PlusDarker;
    case WKC::BlendModePlusLighter:
        return WebCore::BlendMode::PlusLighter;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::BlendMode::Normal;
    }
}

WKC::SelectionRestorationMode
toWKCSelectionRestorationMode(WebCore::SelectionRestorationMode mode)
{
    switch (mode) {
    case WebCore::SelectionRestorationMode::RestoreOrSelectAll:
        return WKC::SelectionRestorationMode::RestoreOrSelectAll;
    case WebCore::SelectionRestorationMode::SelectAll:
        return WKC::SelectionRestorationMode::SelectAll;
    case WebCore::SelectionRestorationMode::PlaceCaretAtStart:
        return WKC::SelectionRestorationMode::PlaceCaretAtStart;
    default:
        ASSERT_NOT_REACHED();
        return WKC::SelectionRestorationMode::RestoreOrSelectAll;
    }
}

WebCore::SelectionRestorationMode
toWebCoreSelectionRestorationMode(WKC::SelectionRestorationMode mode)
{
    switch (mode) {
    case WKC::SelectionRestorationMode::RestoreOrSelectAll:
        return WebCore::SelectionRestorationMode::RestoreOrSelectAll;
    case WKC::SelectionRestorationMode::SelectAll:
        return WebCore::SelectionRestorationMode::SelectAll;
    case WKC::SelectionRestorationMode::PlaceCaretAtStart:
        return WebCore::SelectionRestorationMode::PlaceCaretAtStart;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::SelectionRestorationMode::RestoreOrSelectAll;
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

WKC::ForcedAccessibilityValue
toWKCForcedAccessibilityValue(WebCore::ForcedAccessibilityValue value)
{
    switch (value) {
    case WebCore::ForcedAccessibilityValue::System:
        return WKC::ForcedAccessibilityValue::System;
    case WebCore::ForcedAccessibilityValue::On:
        return WKC::ForcedAccessibilityValue::On;
    case WebCore::ForcedAccessibilityValue::Off:
        return WKC::ForcedAccessibilityValue::Off;
    default:
        ASSERT_NOT_REACHED();
        return WKC::ForcedAccessibilityValue::System;
    }
}

WebCore::ForcedAccessibilityValue
toWebCoreForcedAccessibilityValue(WKC::ForcedAccessibilityValue value)
{
    switch (value) {
    case WKC::ForcedAccessibilityValue::System:
        return WebCore::ForcedAccessibilityValue::System;
    case WKC::ForcedAccessibilityValue::On:
        return WebCore::ForcedAccessibilityValue::On;
    case WKC::ForcedAccessibilityValue::Off:
        return WebCore::ForcedAccessibilityValue::Off;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::ForcedAccessibilityValue::System;
    }
}

WKC::TrackingType
toWKCTrackingType(WebCore::TrackingType type)
{
    switch (type) {
    case WebCore::TrackingType::NotTracking:
        return WKC::TrackingType::NotTracking;
    case WebCore::TrackingType::Asynchronous:
        return WKC::TrackingType::Asynchronous;
    case WebCore::TrackingType::Synchronous:
        return WKC::TrackingType::Synchronous;
    default:
        ASSERT_NOT_REACHED();
        return WKC::TrackingType::NotTracking;
    }
}

WebCore::LengthType toWebCoreLengthType(WKC::LengthType type)
{
    switch (type) {
    case WKC::LengthType::Auto:
        return WebCore::LengthType::Auto;
    case WKC::LengthType::Relative:
        return WebCore::LengthType::Relative;
    case WKC::LengthType::Percent:
        return WebCore::LengthType::Percent;
    case WKC::LengthType::Fixed:
        return WebCore::LengthType::Fixed;
    case WKC::LengthType::Intrinsic:
        return WebCore::LengthType::Intrinsic;
    case WKC::LengthType::MinIntrinsic:
        return WebCore::LengthType::MinIntrinsic;
    case WKC::LengthType::MinContent:
        return WebCore::LengthType::MinContent;
    case WKC::LengthType::MaxContent:
        return WebCore::LengthType::MaxContent;
    case WKC::LengthType::FillAvailable:
        return WebCore::LengthType::FillAvailable;
    case WKC::LengthType::FitContent:
        return WebCore::LengthType::FitContent;
    case WKC::LengthType::Calculated:
        return WebCore::LengthType::Calculated;
    case WKC::LengthType::Content:
        return WebCore::LengthType::Content;
    case WKC::LengthType::Undefined:
        return WebCore::LengthType::Undefined;
    default:
        ASSERT_NOT_REACHED();
        return WebCore::LengthType::Undefined;
    }
}

WKC::LengthType toWKCLengthType(WebCore::LengthType type)
{
    switch (type) {
    case WebCore::LengthType::Auto:
        return WKC::LengthType::Auto;
    case WebCore::LengthType::Relative:
        return WKC::LengthType::Relative;
    case WebCore::LengthType::Percent:
        return WKC::LengthType::Percent;
    case WebCore::LengthType::Fixed:
        return WKC::LengthType::Fixed;
    case WebCore::LengthType::Intrinsic:
        return WKC::LengthType::Intrinsic;
    case WebCore::LengthType::MinIntrinsic:
        return WKC::LengthType::MinIntrinsic;
    case WebCore::LengthType::MinContent:
        return WKC::LengthType::MinContent;
    case WebCore::LengthType::MaxContent:
        return WKC::LengthType::MaxContent;
    case WebCore::LengthType::FillAvailable:
        return WKC::LengthType::FillAvailable;
    case WebCore::LengthType::FitContent:
        return WKC::LengthType::FitContent;
    case WebCore::LengthType::Calculated:
        return WKC::LengthType::Calculated;
    case WebCore::LengthType::Content:
        return WKC::LengthType::Content;
    case WebCore::LengthType::Undefined:
        return WKC::LengthType::Undefined;
    default:
        ASSERT_NOT_REACHED();
        return WKC::LengthType::Undefined;
    }
}


}
