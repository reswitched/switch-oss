/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (c) 2010-2018 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ResourceRequest_h
#define ResourceRequest_h

#include "ResourceRequestBase.h"

typedef const struct _CFURLRequest* CFURLRequestRef;

namespace WebCore {

    class ResourceRequest : public ResourceRequestBase {
    public:
        // The type of this ResourceRequest, based on how the resource will be used.
        enum TargetType {
            TargetIsMainFrame,
            TargetIsSubframe,
            TargetIsSubresource, // Resource is a generic subresource. (Generally a specific type should be specified)
            TargetIsStyleSheet,
            TargetIsScript,
            TargetIsFontResource,
            TargetIsImage,
            TargetIsObject,
            TargetIsMedia,
            TargetIsWorker,
            TargetIsSharedWorker,
            TargetIsPrefetch,
            TargetIsPrerender,
            TargetIsFavicon,
            TargetIsXHR,
            TargetIsTextTrack,
            TargetIsUnspecified,
        };

        ResourceRequest(const String& url)
            : ResourceRequestBase(URL(ParsedURLString, url), ResourceRequestCachePolicy::UseProtocolCachePolicy)
            , m_isMainResource(false)
            , m_targetType(TargetIsSubresource)
        {
            m_platformRequestUpdated = true;
            m_resourceRequestUpdated = true;
        }

        ResourceRequest(const URL& url)
            : ResourceRequestBase(url, ResourceRequestCachePolicy::UseProtocolCachePolicy)
            , m_isMainResource(false)
            , m_targetType(TargetIsSubresource)
        {
            m_platformRequestUpdated = true;
            m_resourceRequestUpdated = true;
        }

        ResourceRequest(const URL& url, const String& referrer, ResourceRequestCachePolicy policy = ResourceRequestCachePolicy::UseProtocolCachePolicy)
            : ResourceRequestBase(url, policy)
            , m_isMainResource(false)
            , m_targetType(TargetIsSubresource)
        {
            setHTTPReferrer(referrer);
            m_platformRequestUpdated = true;
            m_resourceRequestUpdated = true;
        }

        ResourceRequest()
            : ResourceRequestBase(URL(), ResourceRequestCachePolicy::UseProtocolCachePolicy)
            , m_isMainResource(false)
            , m_targetType(TargetIsSubresource)
        {
            m_platformRequestUpdated = true;
            m_resourceRequestUpdated = true;
        }

        // Needed for compatibility.
        CFURLRequestRef cfURLRequest() const { return 0; }

        void setMainResource() { m_isMainResource = true; }
        bool isMainResource() const { return (m_isMainResource)?true:false; }

        // What this request is for.
        TargetType targetType() const { return m_targetType; }
        void setTargetType(TargetType type) { m_targetType = type; } 

        void doUpdatePlatformHTTPBody();
        void doUpdateResourceHTTPBody();
        ResourceRequest& operator=(int);

    private:
        friend class ResourceRequestBase;

        void doUpdatePlatformRequest() { m_platformRequestUpdated = true; }
        void doUpdateResourceRequest() { m_resourceRequestUpdated = true; }

        bool m_isMainResource;
        TargetType m_targetType;
    };
} // namespace WebCore

#endif // ResourceRequest_h
