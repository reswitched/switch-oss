/*
 * Copyright (C) 2003, 2006, 2013 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Logging.h"

#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if !LOG_DISABLED

namespace WebCore {

#define DEFINE_LOG_CHANNEL(name) \
    WTFLogChannel JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, name) = { WTFLogChannelOff, #name };
WEBCORE_LOG_CHANNELS(DEFINE_LOG_CHANNEL)

#define LOG_CHANNEL_ADDRESS(name)  &JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, name),
WTFLogChannel* logChannels[] = {
    WEBCORE_LOG_CHANNELS(LOG_CHANNEL_ADDRESS)
};

size_t logChannelCount = WTF_ARRAY_LENGTH(logChannels);

bool isLogChannelEnabled(const String& name)
{
    WTFLogChannel* channel = WTFLogChannelByName(logChannels, logChannelCount, name.utf8().data());
    if (!channel)
        return false;
    return channel->state != WTFLogChannelOff;
}

void initializeLoggingChannelsIfNecessary()
{
#if !PLATFORM(WKC)
    static bool haveInitializedLoggingChannels = false;
#else
    WKC_DEFINE_STATIC_BOOL(haveInitializedLoggingChannels, false);
#endif
    if (haveInitializedLoggingChannels)
        return;
    haveInitializedLoggingChannels = true;

    WTFInitializeLogChannelStatesFromString(logChannels, logChannelCount, logLevelString().utf8().data());
}

}

#endif // !LOG_DISABLED
