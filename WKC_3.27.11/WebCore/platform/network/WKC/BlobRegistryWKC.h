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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#pragma once

#include "BlobRegistry.h"
#include <memory>

namespace WKC {
    
class BlobRegistryWKC : public WebCore::BlobRegistry
{
public:
    BlobRegistryWKC();
    BlobRegistryWKC(const BlobRegistryWKC&) = delete;
    BlobRegistryWKC(BlobRegistryWKC&&) = default;

    virtual void registerFileBlobURL(const WTF::URL&, WTF::Ref<WebCore::BlobDataFileReference>&&, const WTF::String& path, const WTF::String& contentType) override final;

    virtual void registerBlobURL(const WTF::URL&, Vector<WebCore::BlobPart>&&, const WTF::String& contentType) override final;

    virtual void registerBlobURL(const WTF::URL&, const WTF::URL& srcURL, const WebCore::PolicyContainer&) override final;

    virtual void registerBlobURLOptionallyFileBacked(const WTF::URL&, const WTF::URL& srcURL, RefPtr<WebCore::BlobDataFileReference>&&, const WTF::String& contentType) override final;

    virtual void registerBlobURLForSlice(const WTF::URL&, const WTF::URL& srcURL, long long start, long long end, const WTF::String& contentType) override final;

    virtual void unregisterBlobURL(const WTF::URL&) override final;

    virtual void registerBlobURLHandle(const WTF::URL&) override final;

    virtual void unregisterBlobURLHandle(const WTF::URL&) override final;

    virtual unsigned long long blobSize(const WTF::URL&) override final;

    virtual void writeBlobsToTemporaryFilesForIndexedDB(const Vector<WTF::String>& blobURLs, CompletionHandler<void(Vector<WTF::String>&& filePaths)>&&) override final;

    virtual WebCore::BlobRegistryImpl* blobRegistryImpl() override final { return mImpl.get(); }
private:
    std::unique_ptr<WebCore::BlobRegistryImpl> mImpl;
};

}
