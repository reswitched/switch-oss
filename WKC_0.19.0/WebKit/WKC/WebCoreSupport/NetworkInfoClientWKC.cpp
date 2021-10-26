/*
 *  Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#if ENABLE(NETWORK_INFO)

#include "NetworkInfoClientWKC.h"

namespace WKC {

NetworkInfoClientWKC::NetworkInfoClientWKC(WKCWebViewPrivate* view)
     : m_view(view)
{
}

NetworkInfoClientWKC::~NetworkInfoClientWKC()
{
}

NetworkInfoClientWKC*
NetworkInfoClientWKC::create(WKCWebViewPrivate* view)
{
    NetworkInfoClientWKC* self = 0;
    self = new NetworkInfoClientWKC(view);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
NetworkInfoClientWKC::construct()
{
    return true;
}

double
NetworkInfoClientWKC::bandwidth() const
{
    // Ugh!: call client callback!
    // 140202 ACCESS Co.,Ltd.
    return 0;
}

bool
NetworkInfoClientWKC::metered() const
{
    // Ugh!: call client callback!
    // 140202 ACCESS Co.,Ltd.
    return false;
}

void
NetworkInfoClientWKC::startUpdating()
{
    // Ugh!: call client callback!
    // 140202 ACCESS Co.,Ltd.
}

void
NetworkInfoClientWKC::stopUpdating()
{
    // Ugh!: call client callback!
    // 140202 ACCESS Co.,Ltd.
}

void
NetworkInfoClientWKC::networkInfoControllerDestroyed()
{
    // Ugh!: call client callback!
    // 140202 ACCESS Co.,Ltd.
}

} // namespace

#endif // ENABLE(NETWORK_INFO)
