/*
 * Copyright (c) 2016-2018 ACCESS CO., LTD. All rights reserved.
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
#include "EditorInsertAction.h"
#include "Element.h"
#include "FocusDirection.h"
#include "FrameLoaderTypes.h"
#include "GraphicsTypes.h"
#include "ProtectionSpaceBase.h"
#include "ResourceErrorBase.h"
#include "ScrollTypes.h"
#include "Settings.h"
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

WKC::EAffinity toWKCEAffinity(WebCore::EAffinity affinity);
WebCore::EAffinity toWebCoreEAffinity(WKC::EAffinity affinity);

WKC::PolicyAction toWKCPolicyAction(WebCore::PolicyAction action);
WebCore::PolicyAction toWebCorePolicyAction(WKC::PolicyAction action);

WKC::ObjectContentType toWKCObjectContentType(WebCore::ObjectContentType type);
WebCore::ObjectContentType toWebCoreObjectContentType(WKC::ObjectContentType type);

WKC::FrameLoadType toWKCFrameLoadType(WebCore::FrameLoadType type);
WebCore::FrameLoadType toWebCoreFrameLoadType(WKC::FrameLoadType type);

WKC::NavigationType toWKCNavigationType(WebCore::NavigationType type);
WebCore::NavigationType toWebCoreNavigationType(WKC::NavigationType type);

WKC::ProtectionSpaceServerType toWKCProtectionSpaceServerType(WebCore::ProtectionSpaceServerType type);
WebCore::ProtectionSpaceServerType toWebCoreProtectionSpaceServerType(WKC::ProtectionSpaceServerType type);

WKC::ProtectionSpaceAuthenticationScheme toWKCProtectionSpaceAuthenticationScheme(WebCore::ProtectionSpaceAuthenticationScheme scheme);
WebCore::ProtectionSpaceAuthenticationScheme toWebCoreProtectionSpaceAuthenticationScheme(WKC::ProtectionSpaceAuthenticationScheme scheme);

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

}

#endif // _WKC_HELPERS_PRIVATE_ENUMS_H_
