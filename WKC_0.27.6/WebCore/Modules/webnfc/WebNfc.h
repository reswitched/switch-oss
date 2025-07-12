/*
 *  WebNfc.h
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef WebNfc_h
#define WebNfc_h

#if ENABLE(WKC_WEB_NFC)

#include <wtf/RefCounted.h>
#include "Ref.h"

namespace WebCore {

class Navigator;
class WebNfcController;
class DeferredWrapper;

class WebNfc : public RefCounted<WebNfc> {
public:
    virtual ~WebNfc();
    static Ref<WebNfc> create(Navigator*);

    void requestAdapter(DeferredWrapper*);

private:
    explicit WebNfc(Navigator*);

    WebNfcController* m_webNfcController;
};

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfc_h
