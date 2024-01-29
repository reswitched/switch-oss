/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
#include <wtf/ProcessPrivilege.h>

#include <wtf/OptionSet.h>

namespace WTF {

OptionSet<ProcessPrivilege> allPrivileges()
{
    return {
        ProcessPrivilege::CanAccessRawCookies,
        ProcessPrivilege::CanAccessCredentials,
        ProcessPrivilege::CanCommunicateWithWindowServer,
    };
}

static OptionSet<ProcessPrivilege>& processPrivileges()
{
#if PLATFORM(WKC)
    WKC_DEFINE_STATIC_TYPE(OptionSet<ProcessPrivilege>, privileges, { });
#else
    static OptionSet<ProcessPrivilege> privileges = { };
#endif
    return privileges;
}

void setProcessPrivileges(OptionSet<ProcessPrivilege> privileges)
{
    processPrivileges() = privileges;
}

bool hasProcessPrivilege(ProcessPrivilege privilege)
{
    return processPrivileges().contains(privilege);
}

void addProcessPrivilege(ProcessPrivilege privilege)
{
    processPrivileges().add(privilege);
}

void removeProcessPrivilege(ProcessPrivilege privilege)
{
    processPrivileges().remove(privilege);
}

} // namespace WTF