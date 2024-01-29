/*
 * Copyright (C) 2008, 2009, 2014 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Threading.h"

#include <string.h>

#if HAVE(QOS_CLASSES)
#include <bmalloc/bmalloc.h>
#endif

namespace WTF {

struct NewThreadContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    const char* name;
    std::function<void()> entryPoint;
    Mutex creationMutex;
};

static void threadEntryPoint(void* contextData)
{
    NewThreadContext* context = static_cast<NewThreadContext*>(contextData);

    // Block until our creating thread has completed any extra setup work, including
    // establishing ThreadIdentifier.
    {
        MutexLocker locker(context->creationMutex);
    }

    initializeCurrentThreadInternal(context->name);

    auto entryPoint = WTF::move(context->entryPoint);

    // Delete the context before starting the thread.
    delete context;

    entryPoint();
}

ThreadIdentifier createThread(const char* name, std::function<void()> entryPoint)
{
    // Visual Studio has a 31-character limit on thread names. Longer names will
    // be truncated silently, but we'd like callers to know about the limit.
#if !LOG_DISABLED && PLATFORM(WIN)
    if (name && strlen(name) > 31)
        LOG_ERROR("Thread name \"%s\" is longer than 31 characters and will be truncated by Visual Studio", name);
#endif

    NewThreadContext* context = new NewThreadContext { name, WTF::move(entryPoint), { } };

    // Prevent the thread body from executing until we've established the thread identifier.
    MutexLocker locker(context->creationMutex);

    return createThreadInternal(threadEntryPoint, context, name);
}

ThreadIdentifier createThread(ThreadFunction entryPoint, void* data, const char* name)
{
#if !PLATFORM(WKC)
    return createThread(name, [entryPoint, data] {
        entryPoint(data);
    });
#else
    auto p = std::function<void()>(std::allocator_arg, WTF::voidFuncAllocator(), [entryPoint, data] {
        entryPoint(data);
    });
    return createThread(name, p);
#endif
}

void setCurrentThreadIsUserInteractive()
{
#if HAVE(QOS_CLASSES)
    pthread_set_qos_class_self_np(adjustedQOSClass(QOS_CLASS_USER_INTERACTIVE), relativePriority);
#endif
}

void setCurrentThreadIsUserInitiated()
{
#if HAVE(QOS_CLASSES)
    pthread_set_qos_class_self_np(adjustedQOSClass(QOS_CLASS_USER_INITIATED), relativePriority);
#endif
}

#if HAVE(QOS_CLASSES)
static qos_class_t globalMaxQOSclass { QOS_CLASS_UNSPECIFIED };

void setGlobalMaxQOSClass(qos_class_t maxClass)
{
    bmalloc::api::setScavengerThreadQOSClass(maxClass);
    globalMaxQOSclass = maxClass;
}

qos_class_t adjustedQOSClass(qos_class_t originalClass)
{
    if (globalMaxQOSclass != QOS_CLASS_UNSPECIFIED)
        return std::min(originalClass, globalMaxQOSclass);
    return originalClass;
}
#endif

} // namespace WTF
