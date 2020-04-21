/*
* Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef WKCStorageAreaIf_h
#define WKCStorageAreaIf_h

#include <wkc/wkcbase.h>
#include "WKCHelpersEnums.h"

namespace WKC {

    class String;
    class Frame;

    class WKC_API StorageAreaIf {
    public:
        virtual ~StorageAreaIf() {}
        virtual unsigned length() = 0;
        virtual String key(unsigned index) = 0;
        virtual String item(const String& key) = 0;
        virtual void setItem(Frame* sourceFrame, const String& key, const String& value, bool& quotaException) = 0;
        virtual void removeItem(Frame* sourceFrame, const String& key) = 0;
        virtual void clear(Frame* sourceFrame) = 0;
        virtual bool contains(const String& key) = 0;
        virtual bool canAccessStorage(Frame* frame) = 0;
        virtual size_t memoryBytesUsedByCache() = 0;

        virtual void incrementAccessCount() = 0;
        virtual void decrementAccessCount() = 0;
        virtual void closeDatabaseIfIdle() = 0;
    };

} // namespace

#endif // WKCStorageAreaIf_h
