/*
    WKCPlatformStrategies.h
    Copyright (c) 2013, 2015 ACCESS CO., LTD. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _WKCPLATFORMSTRATEGIES_H_
#define _WKCPLATFORMSTRATEGIES_H_

#include "PlatformStrategies.h"

namespace WKC {

class PlatformStrategiesWKC : public WebCore::PlatformStrategies
{
WTF_MAKE_FAST_ALLOCATED;
public:
    PlatformStrategiesWKC();
    ~PlatformStrategiesWKC();

    virtual WebCore::CookiesStrategy* createCookiesStrategy();
    virtual WebCore::LoaderStrategy* createLoaderStrategy();
    virtual WebCore::PasteboardStrategy* createPasteboardStrategy();
    virtual WebCore::PluginStrategy* createPluginStrategy();
};

} // namespace

#endif // _WKCPLATFORMSTRATEGIES_H_
