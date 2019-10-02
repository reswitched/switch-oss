/*
 * Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_MIMETYPEREGISTRY_H_
#define _WKC_HELPERS_WKC_MIMETYPEREGISTRY_H_

#include <wkc/wkcbase.h>

#include "WKCString.h"

namespace WKC {

class WKC_API MIMETypeRegistry {
public:
    static String getMIMETypeForPath(const String& path);
    static String getMIMETypeForExtension(const String& ext);
    static String getMediaMIMETypeForExtension(const String& ext);

    static bool isSupportedImageMIMEType(const String& mimeType);
    static bool isSupportedImageResourceMIMEType(const String& mimeType);
    static bool isSupportedImageMIMETypeForEncoding(const String& mimeType);
    static bool isSupportedJavaScriptMIMEType(const String& mimeType);    
    static bool isSupportedNonImageMIMEType(const String& mimeType);
    static bool isSupportedMediaMIMEType(const String& mimeType); 
    static bool isJavaAppletMIMEType(const String& mimeType);

private:
    MIMETypeRegistry();
    ~MIMETypeRegistry();
    MIMETypeRegistry(const MIMETypeRegistry&);
    MIMETypeRegistry& operator=(const MIMETypeRegistry&);
};
}

#endif // _WKC_HELPERS_WKC_MIMETYPEREGISTRY_H_
