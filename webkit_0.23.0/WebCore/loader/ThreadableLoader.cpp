/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThreadableLoader.h"

#include "Document.h"
#include "DocumentThreadableLoader.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "WorkerGlobalScope.h"
#include "WorkerRunLoop.h"
#include "WorkerThreadableLoader.h"

namespace WebCore {

ThreadableLoaderOptions::ThreadableLoaderOptions()
{
}

ThreadableLoaderOptions::~ThreadableLoaderOptions()
{
}

ThreadableLoaderOptions::ThreadableLoaderOptions(const ResourceLoaderOptions& baseOptions, PreflightPolicy preflightPolicy, CrossOriginRequestPolicy crossOriginRequestPolicy, RefPtr<SecurityOrigin>&& securityOrigin, String&& initiator)
    : ResourceLoaderOptions(baseOptions)
    , preflightPolicy(preflightPolicy)
    , crossOriginRequestPolicy(crossOriginRequestPolicy)
    , securityOrigin(WTF::move(securityOrigin))
    , initiator(WTF::move(initiator))
{
}

std::unique_ptr<ThreadableLoaderOptions> ThreadableLoaderOptions::isolatedCopy() const
{
    RefPtr<SecurityOrigin> securityOriginCopy;
    if (securityOrigin)
        securityOriginCopy = securityOrigin->isolatedCopy();
    return std::make_unique<ThreadableLoaderOptions>(*this, preflightPolicy, crossOriginRequestPolicy,
        WTF::move(securityOriginCopy), initiator.isolatedCopy());
}

PassRefPtr<ThreadableLoader> ThreadableLoader::create(ScriptExecutionContext* context, ThreadableLoaderClient* client, const ResourceRequest& request, const ThreadableLoaderOptions& options)
{
    ASSERT(client);
    ASSERT(context);

    if (is<WorkerGlobalScope>(*context))
        return WorkerThreadableLoader::create(downcast<WorkerGlobalScope>(context), client, WorkerRunLoop::defaultMode(), request, options);

    return DocumentThreadableLoader::create(downcast<Document>(*context), *client, request, options);
}

void ThreadableLoader::loadResourceSynchronously(ScriptExecutionContext* context, const ResourceRequest& request, ThreadableLoaderClient& client, const ThreadableLoaderOptions& options)
{
    ASSERT(context);

    if (is<WorkerGlobalScope>(*context))
        WorkerThreadableLoader::loadResourceSynchronously(downcast<WorkerGlobalScope>(context), request, client, options);
    else
        DocumentThreadableLoader::loadResourceSynchronously(downcast<Document>(*context), request, client, options);
    context->didLoadResourceSynchronously(request);
}

} // namespace WebCore
