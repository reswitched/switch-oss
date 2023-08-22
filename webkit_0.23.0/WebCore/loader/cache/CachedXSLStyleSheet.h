/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef CachedXSLStyleSheet_h
#define CachedXSLStyleSheet_h

#if ENABLE(XSLT)

#include "CachedResource.h"

namespace WebCore {

class TextResourceDecoder;

class CachedXSLStyleSheet final : public CachedResource {
public:
    CachedXSLStyleSheet(const ResourceRequest&, SessionID);
    virtual ~CachedXSLStyleSheet();

    const String& sheet() const { return m_sheet; }

private:
    virtual void checkNotify() override;
    virtual bool mayTryReplaceEncodedData() const override { return true; }
    virtual void didAddClient(CachedResourceClient*) override;
    virtual void setEncoding(const String&) override;
    virtual String encoding() const override;
    const TextResourceDecoder* textResourceDecoder() const override { return m_decoder.get(); }
    virtual void finishLoading(SharedBuffer*) override;

    String m_sheet;
    RefPtr<TextResourceDecoder> m_decoder;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CACHED_RESOURCE(CachedXSLStyleSheet, CachedResource::XSLStyleSheet)

#endif // ENABLE(XSLT)

#endif // CachedXSLStyleSheet_h
