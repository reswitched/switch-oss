/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKCHELPERSENUMSBSDS_H
#define WKCHELPERSENUMSBSDS_H

#include <limits.h>
#include <cstdint>

namespace WKC {

    // from JavaScriptCore/runtime/ConsoleTypes.h
    enum class MessageSource {
        XML,
        JS,
        Network,
        ConsoleAPI,
        Storage,
        AppCache,
        Rendering,
        CSS,
        Security,
        ContentBlocker,
        Other,
        Media,
        WebRTC,
    };

    enum class MessageType {
        Log,
        Dir,
        DirXML,
        Table,
        Trace,
        StartGroup,
        StartGroupCollapsed,
        EndGroup,
        Clear,
        Assert,
        Timing,
        Profile,
        ProfileEnd,
    };

    enum class MessageLevel {
        Log = 1,
        Warning = 2,
        Error = 3,
        Debug = 4,
        Info = 5,
    };

    // from platform/text/WritingMode.h
    enum TextDirection { LTR, RTL };

    // from page/FocusDirection.h
    enum class FocusDirection : uint8_t {
        None = 0,
        Forward,
        Backward,
        Up,
        Down,
        Left,
        Right,
        UpLeft,
        UpRight,
        DownLeft,
        DownRight
    };

    // from page/DragActions.h
    enum class DragDestinationAction : uint8_t {
        None    = 0,
        DHTML   = 1,
        Edit    = 2,
        Load    = 4,
    };

    enum class DragSourceAction : uint8_t {
        None         = 0,
        DHTML        = 1,
        Image        = 2,
        Link         = 4,
        Selection    = 8,
        Attachment   = 16,
        Color        = 32,
        Model        = 64,
        Any          = DHTML | Image | Link | Selection | Attachment | Color | Model,
    };

    enum class DragOperation : uint8_t {
        None    = 0,
        Copy    = 1,
        Link    = 2,
        Generic = 4,
        Private = 8,
        Move    = 16,
        Delete  = 32,
    };

    // from platform/ScrollTypes.h
    enum class ScrollbarOrientation : uint8_t { HorizontalScrollbar, VerticalScrollbar };
    enum class ScrollbarControlSize : uint8_t { Regular, Small };
    enum class ScrollDirection : uint8_t {
        ScrollUp,
        ScrollDown,
        ScrollLeft,
        ScrollRight
    };

    // from editing/EditorInsertAction.h
    enum class EditorInsertAction {
        Typed,
        Pasted,
        Dropped,
    };

    // from TextAffinity.h
    enum EAffinity { UPSTREAM = 0, DOWNSTREAM = 1 };

    // from loader/FrameLoaderTypes.h
    enum class PolicyAction : uint8_t {
        Use,
        Download,
        Ignore,
        StopAllLoads,
    };
    enum class ObjectContentType : uint8_t {
        None,
        Image,
        Frame,
        Plugin
    };
    enum class FrameLoadType {
        Standard,
        Back,
        Forward,
        IndexedBackForward, // a multi-item hop in the backforward list
        Reload,
        Same, // user loads same URL again (but not reload button)
        RedirectWithLockedBackForwardList, // FIXME: Merge "lockBackForwardList", "lockHistory", "quickRedirect" and "clientRedirect" into a single concept of redirect.
        Replace,
        ReloadFromOrigin,
        ReloadExpiredOnly,
    };

    enum class NavigationType {
        LinkClicked,
        FormSubmitted,
        BackForward,
        Reload,
        FormResubmitted,
        Other
    };

    // from DocumentLoader.h
    enum class MediaSourcePolicy : uint8_t {
        Default,
        Disable,
        Enable
    };

    // from ProtectionSpaceBase.h
    enum ProtectionSpaceServerType {
        ProtectionSpaceServerHTTP = 1,
        ProtectionSpaceServerHTTPS = 2,
        ProtectionSpaceServerFTP = 3,
        ProtectionSpaceServerFTPS = 4,
        ProtectionSpaceProxyHTTP = 5,
        ProtectionSpaceProxyHTTPS = 6, // not supported
        ProtectionSpaceProxyFTP = 7,
        ProtectionSpaceProxySOCKS = 8
    };
    enum ProtectionSpaceAuthenticationScheme {
        ProtectionSpaceAuthenticationSchemeDefault = 1,
        ProtectionSpaceAuthenticationSchemeHTTPBasic = 2,
        ProtectionSpaceAuthenticationSchemeHTTPDigest = 3,
        ProtectionSpaceAuthenticationSchemeHTMLForm = 4,
        ProtectionSpaceAuthenticationSchemeNTLM = 5,
        ProtectionSpaceAuthenticationSchemeNegotiate = 6,
        ProtectionSpaceAuthenticationSchemeClientCertificateRequested = 7,
        ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested = 8,
        ProtectionSpaceAuthenticationSchemeUnknown = 100
    };

    // from CredentialBase.h
    enum CredentialPersistence {
        CredentialPersistenceNone,
        CredentialPersistenceForSession,
        CredentialPersistencePermanent
    };

    // from WebCoreKeyboardUIMode.h
    enum KeyboardUIMode {
        KeyboardAccessDefault     = 0x00000000,
        KeyboardAccessFull        = 0x00000001,
        KeyboardAccessTabsToLinks = 0x10000000
    };

    // from IconURL.h
    enum IconType {
        InvalidIcon = 0,
        Favicon = 1,
        TouchIcon = 1<<1,
        TouchPrecomposedIcon = 1<<2
    };

    // from page/LayoutMilestones.h
    enum LayoutMilestoneFlag {
        DidFirstLayout = 1 << 0,
        DidFirstVisuallyNonEmptyLayout = 1 << 1,
        DidHitRelevantRepaintedObjectsAreaThreshold = 1 << 2,
        DidFirstFlushForHeaderLayer = 1 << 3,
        DidFirstLayoutAfterSuppressedIncrementalRendering = 1 << 4,
        DidFirstPaintAfterSuppressedIncrementalRendering = 1 << 5,
        ReachedSessionRestorationRenderTreeSizeThreshold = 1 << 6, // FIXME: only implemented by WK2 currently.
        DidRenderSignificantAmountOfText = 1 << 7,
    };
    typedef unsigned LayoutMilestones;

    // from storage/StorageArea.h
    enum StorageType {
        SessionStorage,
        LocalStorage,
        TransientLocalStorage
    };

    // from platform/graphics/GraphicsTypes.h
    enum BlendMode {
        BlendModeNormal = 1, // Start with 1 to match SVG's blendmode enumeration.
        BlendModeMultiply,
        BlendModeScreen,
        BlendModeDarken,
        BlendModeLighten,
        BlendModeOverlay,
        BlendModeColorDodge,
        BlendModeColorBurn,
        BlendModeHardLight,
        BlendModeSoftLight,
        BlendModeDifference,
        BlendModeExclusion,
        BlendModeHue,
        BlendModeSaturation,
        BlendModeColor,
        BlendModeLuminosity,
        BlendModePlusDarker,
        BlendModePlusLighter
    };

    // from platform/network/ResourceErrorBase.h
    enum class ResourceErrorType { // base of WebCore::ResourceErrorBase::Type
        ResourceErrorNull,          // Null
        ResourceErrorGeneral,       // General
        ResourceErrorAccessControl, // AccessControl
        ResourceErrorCancellation,  // Cancellation
        ResourceErrorTimeout        // Timeout
    };

    // from page/Settings.h
    enum FrameFlattening {
        FrameFlatteningDisabled,
        FrameFlatteningEnabledForNonFullScreenIFrames,
        FrameFlatteningFullyEnabled
    };

    enum class ForcedAccessibilityValue : uint8_t { System, On, Off };

    enum class TrackingType : uint8_t {
        NotTracking = 0,
        Asynchronous = 1,
        Synchronous = 2
    };

    enum class LengthType : uint8_t {
        Auto,
        Relative,
        Percent,
        Fixed,
        Intrinsic,
        MinIntrinsic,
        MinContent,
        MaxContent,
        FillAvailable,
        FitContent,
        Calculated,
        Content,
        Undefined
    };

} // namespace

#endif // WKCHELPERSENUMSBSDS_H
