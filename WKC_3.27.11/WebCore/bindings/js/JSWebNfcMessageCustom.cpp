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

#include "config.h"

#if ENABLE(WKC_WEB_NFC)

#include "JSWebNfcMessage.h"

#include "WebNfcMessage.h"
#include "Blob.h"

#include <runtime/JSArray.h>

namespace WebCore {
using namespace JSC;

JSValue JSWebNfcMessage::data(ExecState* exec) const
{
    WebNfcMessage& imp = impl();
    switch (imp.dataType()) {
    case WebNfcMessageDataTypeString:
        return jsArray(exec, globalObject(), imp.stringDatas());
    case WebNfcMessageDataTypeURL:
        return jsArray(exec, globalObject(), imp.urlDatas());
    case WebNfcMessageDataTypeBlob:
        {
            WTF::Vector<RefPtr<Blob> > vector;
            for (int i = 0; i < imp.blobDatas().size(); ++i) {
                RefPtr<Blob> blob = Blob::create(imp.blobDatas().at(i), imp.blobContentTypes().at(i));
                vector.append(blob);
            }
            return jsArray(exec, globalObject(), vector);
        }
    case WebNfcMessageDataTypeJSON:
        {
            WTF::Vector<JSValue> vector;
            for (int i = 0; i < imp.stringDatas().size(); ++i) {
                vector.append(JSONParse(exec, imp.stringDatas().at(i)));
            }

            JSC::MarkedArgumentBuffer list;
            for (auto& element : vector)
                list.append(element);
            return JSC::constructArray(exec, 0, globalObject(), list);
        }
    default:
        break;
    }
    return jsNull();
}

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
