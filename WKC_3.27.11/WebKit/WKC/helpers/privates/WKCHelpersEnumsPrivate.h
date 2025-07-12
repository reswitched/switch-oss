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

#ifndef _WKC_HELPERS_PRIVATE_ENUMS_H_
#define _WKC_HELPERS_PRIVATE_ENUMS_H_

#include "helpers/WKCHelpersEnums.h"

#include "DragActions.h"

#include "CredentialBase.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "EditorInsertAction.h"
#include "Element.h"
#include "EventTrackingRegions.h"
#include "FocusDirection.h"
#include "FrameLoaderTypes.h"
#include "GraphicsTypes.h"
#include "Length.h"
#include "ProtectionSpace.h"
#include "ResourceErrorBase.h"
#include "ScrollTypes.h"
#include "Settings.h"
#include "SpatialNavigationDirection.h"
#include "StorageArea.h"
#include "StorageType.h"
#include "TextAffinity.h"
#include "WritingMode.h"
#include <JavaScriptCore/runtime/ConsoleTypes.h>
#include <unicode/ubrk.h>

namespace WKC {

WKC::MessageSource toWKCMessageSource(JSC::MessageSource source);
JSC::MessageSource toJSCMessageSource(WKC::MessageSource source);

WKC::MessageType toWKCMessageType(JSC::MessageType type);
JSC::MessageType toJSCMessageType(WKC::MessageType type);

WKC::MessageLevel toWKCMessageLevel(JSC::MessageLevel level);
JSC::MessageLevel toJSCMessageLevel(WKC::MessageLevel level);

WKC::TextDirection toWKCTextDirection(WebCore::TextDirection dir);
WebCore::TextDirection toWebCoreTextDirection(WKC::TextDirection dir);

WKC::FocusDirection toWKCFocusDirection(WebCore::FocusDirection dir);
WebCore::FocusDirection toWebCoreFocusDirection(WKC::FocusDirection dir);
bool convertToSpatialNavigationDirection(WKC::FocusDirection from, WebCore::SpatialNavigationDirection& to);

WKC::DragDestinationAction toWKCDragDestinationAction(WebCore::DragDestinationAction action);
WebCore::DragDestinationAction toWebCoreDragDestinationAction(WKC::DragDestinationAction action);

WKC::DragSourceAction toWKCDragSourceAction(WebCore::DragSourceAction action);
WebCore::DragSourceAction toWebCoreDragSourceAction(WKC::DragSourceAction action);

WKC::DragOperation toWKCDragOperation(WebCore::DragOperation op);
WebCore::DragOperation toWebCoreDragOperation(WKC::DragOperation op);

WKC::ScrollbarOrientation toWKCScrollbarOrientation(WebCore::ScrollbarOrientation orientation);
WebCore::ScrollbarOrientation toWebCoreScrollbarOrientation(WKC::ScrollbarOrientation orientation);

WKC::ScrollbarControlSize toWKCScrollbarControlSize(WebCore::ScrollbarControlSize size);
WebCore::ScrollbarControlSize toWebCoreScrollbarControlSize(WKC::ScrollbarControlSize size);

WKC::ScrollDirection toWKCScrollDirection(WebCore::ScrollDirection dir);
WebCore::ScrollDirection toWebCoreScrollDirection(WKC::ScrollDirection dir);

WKC::EditorInsertAction toWKCEditorInsertAction(WebCore::EditorInsertAction action);
WebCore::EditorInsertAction toWebCoreEditorInsertAction(WKC::EditorInsertAction action);

WKC::EAffinity toWKCEAffinity(WebCore::Affinity affinity);
WebCore::Affinity toWebCoreEAffinity(WKC::EAffinity affinity);

WKC::PolicyAction toWKCPolicyAction(WebCore::PolicyAction action);
WebCore::PolicyAction toWebCorePolicyAction(WKC::PolicyAction action);

WKC::ObjectContentType toWKCObjectContentType(WebCore::ObjectContentType type);
WebCore::ObjectContentType toWebCoreObjectContentType(WKC::ObjectContentType type);

WKC::FrameLoadType toWKCFrameLoadType(WebCore::FrameLoadType type);
WebCore::FrameLoadType toWebCoreFrameLoadType(WKC::FrameLoadType type);

WKC::NavigationType toWKCNavigationType(WebCore::NavigationType type);
WebCore::NavigationType toWebCoreNavigationType(WKC::NavigationType type);

WKC::MediaSourcePolicy toWKCMediaSourcePolicy(WebCore::MediaSourcePolicy policy);
WebCore::MediaSourcePolicy toWebCoreMediaSourcePolicy(WKC::MediaSourcePolicy policy);

WKC::ProtectionSpaceServerType toWKCProtectionSpaceServerType(WebCore::ProtectionSpace::ServerType type);
WebCore::ProtectionSpace::ServerType toWebCoreProtectionSpaceServerType(WKC::ProtectionSpaceServerType type);

WKC::ProtectionSpaceAuthenticationScheme toWKCProtectionSpaceAuthenticationScheme(WebCore::ProtectionSpace::AuthenticationScheme scheme);
WebCore::ProtectionSpace::AuthenticationScheme toWebCoreProtectionSpaceAuthenticationScheme(WKC::ProtectionSpaceAuthenticationScheme scheme);

WKC::CredentialPersistence toWKCCredentialPersistence(WebCore::CredentialPersistence cp);
WebCore::CredentialPersistence toWebCoreCredentialPersistence(WKC::CredentialPersistence cp);

WKC::StorageType toWKCStorageType(WebCore::StorageType type);
WebCore::StorageType toWebCoreStorageType(WKC::StorageType type);

WKC::BlendMode toWKCBlendMode(WebCore::BlendMode mode);
WebCore::BlendMode toWebCoreBlendMode(WKC::BlendMode mode);

WKC::SelectionRestorationMode toWKCSelectionRestorationMode(WebCore::SelectionRestorationMode mode);
WebCore::SelectionRestorationMode toWebCoreSelectionRestorationMode(WKC::SelectionRestorationMode mode);

WKC::SelectionRevealMode toWKCSelectionRevealMode(WebCore::SelectionRevealMode mode);
WebCore::SelectionRevealMode toWebCoreSelectionRevealMode(WKC::SelectionRevealMode mode);

WKC::ResourceErrorType toWKCResourceErrorType(WebCore::ResourceErrorBase::Type type);
WebCore::ResourceErrorBase::Type toWebCoreResourceErrorType(WKC::ResourceErrorType type);

WKC::FrameFlattening toWKCFrameFlattening(WebCore::FrameFlattening flattening);
WebCore::FrameFlattening toWebCoreFrameFlattening(WKC::FrameFlattening flattening);

int toWKCTextBreakIteratorType(UBreakIteratorType type);
UBreakIteratorType toUBreakIteratorType(int type);

WKC::ForcedAccessibilityValue toWKCForcedAccessibilityValue(WebCore::ForcedAccessibilityValue value);
WebCore::ForcedAccessibilityValue toWebCoreForcedAccessibilityValue(WKC::ForcedAccessibilityValue value);

WKC::TrackingType toWKCTrackingType(WebCore::TrackingType type);

WKC::LengthType toWKCLengthType(WebCore::LengthType type);
WebCore::LengthType toWebCoreLengthType(WKC::LengthType type);

}

#endif // _WKC_HELPERS_PRIVATE_ENUMS_H_
