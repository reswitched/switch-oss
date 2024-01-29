/*
 * Copyright (c) 2011-2021 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"
#include "MIMETypeRegistry.h"
#include "WTFString.h"
#include "helpers/WKCMIMETypeRegistry.h"
#include "helpers/WKCString.h"

namespace WKC {
String
MIMETypeRegistry::getMIMETypeForPath(const String& path)
{
    return WebCore::MIMETypeRegistry::getMIMETypeForPath(path);
}

String
MIMETypeRegistry::getMIMETypeForExtension(const String& ext)
{
#if ENABLE(WORKERS)
    return WebCore::MIMETypeRegistry::getMIMETypeForExtension(ext);
#else
    return String();
#endif
}

String
MIMETypeRegistry::getMediaMIMETypeForExtension(const String& ext)
{
    return WebCore::MIMETypeRegistry::getMediaMIMETypeForExtension(ext);
}

bool
MIMETypeRegistry::isSupportedImageMIMEType(const String& mimeType)
{
    return WebCore::MIMETypeRegistry::isSupportedImageMIMEType(mimeType);
}

bool
MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(const String& mimeType)
{
    return WebCore::MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType);
}

bool
MIMETypeRegistry::isSupportedJavaScriptMIMEType(const String& mimeType)
{
    return WebCore::MIMETypeRegistry::isSupportedJavaScriptMIMEType(mimeType);
}

bool
MIMETypeRegistry::isSupportedNonImageMIMEType(const String& mimeType)
{
    return WebCore::MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType);
}

bool
MIMETypeRegistry::isSupportedMediaMIMEType(const String& mimeType)
{
    return WebCore::MIMETypeRegistry::isSupportedMediaMIMEType(mimeType);
}

bool
MIMETypeRegistry::isJavaAppletMIMEType(const String& mimeType)
{
    return WebCore::MIMETypeRegistry::isJavaAppletMIMEType(mimeType);
}

} // namespace
