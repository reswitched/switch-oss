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

#ifndef WKCWebNfcClientIf_h
#define WKCWebNfcClientIf_h

#include <wkc/wkcbase.h>

namespace WKC {

class WebNfcController;
class WebNfcMessage;
enum WebNfcMessageDataType;

class WKC_API WebNfcClientIf {
public:
    virtual void setController(WebNfcController*) = 0;
    virtual void webNfcControllerDestroyed() = 0;

    virtual bool requestPermission() = 0;
    virtual void requestAdapter(int id) = 0;
    virtual void startRequestMessage() = 0;
    virtual void stopRequestMessage() = 0;
    virtual void send(int id, WebNfcMessageDataType dataType,
                      const unsigned short* scope, int scopeLength,
                      const unsigned short** datas, const int* dataLengths,
                      int length,
                      const unsigned short* target, int targetLength) = 0;
    virtual void send(int id,
                      const unsigned short* scope, int scopeLength,
                      const char** datas, const unsigned long long* dataLengths,
                      const unsigned short** contentTypes, int* contentTypeLengths,
                      int length,
                      const unsigned short* target, int targetLength) = 0;

    virtual void setAdapter(int id, int result) = 0;
    virtual void setMessage(WebNfcMessageDataType dataType, const unsigned short* scope, const unsigned short** datas, int len) = 0;
    virtual void setMessage(const unsigned short* scope, const char** datas, const unsigned long long* dataLengths, const unsigned short** contentTypes, int len) = 0;
    virtual void notifySendResult(int id, int result) = 0;
};

} // namespace WKC

#endif // WKCWebNfcClientIf_h
