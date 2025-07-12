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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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
