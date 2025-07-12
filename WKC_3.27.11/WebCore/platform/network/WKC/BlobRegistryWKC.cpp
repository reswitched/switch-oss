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

#include "config.h"
#include "BlobRegistryWKC.h"

#include "Assertions.h"
#include "BlobRegistryImpl.h"

#include <utility>

namespace WKC {

BlobRegistryWKC::BlobRegistryWKC()
    : mImpl(new WebCore::BlobRegistryImpl)
{
    if (!mImpl)
        CRASH_WITH_NO_MEMORY();
}

void BlobRegistryWKC::registerFileBlobURL(const WTF::URL& url, WTF::Ref<WebCore::BlobDataFileReference>&& blobData, const WTF::String& path, const WTF::String& contentType)
{
    mImpl->registerFileBlobURL(url, std::forward<WTF::Ref<WebCore::BlobDataFileReference>>(blobData), contentType);
}

void BlobRegistryWKC::registerBlobURL(const WTF::URL& url, Vector<WebCore::BlobPart>&& blobParts, const WTF::String & contentType)
{
    mImpl->registerBlobURL(url, std::forward<Vector<WebCore::BlobPart>>(blobParts), contentType);
}

void BlobRegistryWKC::registerBlobURL(const WTF::URL& blobURL, const WTF::URL& srcURL, const WebCore::PolicyContainer& policy)
{
    mImpl->registerBlobURL(blobURL, srcURL, policy);
}

void BlobRegistryWKC::registerBlobURLOptionallyFileBacked(const WTF::URL& blobURL, const WTF::URL& srcURL, RefPtr<WebCore::BlobDataFileReference>&& blobData, const WTF::String& contentType)
{
    mImpl->registerBlobURLOptionallyFileBacked(blobURL, srcURL, std::forward<RefPtr<WebCore::BlobDataFileReference>>(blobData), contentType, { });
}

void BlobRegistryWKC::registerBlobURLForSlice(const WTF::URL& blobURL, const WTF::URL& srcURL, long long start, long long end, const WTF::String& contentType)
{
    mImpl->registerBlobURLForSlice(blobURL, srcURL, start, end, contentType);
}

void BlobRegistryWKC::unregisterBlobURL(const WTF::URL& url)
{
    mImpl->unregisterBlobURL(url);
}

void BlobRegistryWKC::registerBlobURLHandle(const WTF::URL& url)
{
    mImpl->registerBlobURLHandle(url);
}

void BlobRegistryWKC::unregisterBlobURLHandle(const WTF::URL& url)
{
    mImpl->unregisterBlobURLHandle(url);
}

unsigned long long BlobRegistryWKC::blobSize(const WTF::URL& url)
{
    return mImpl->blobSize(url);
}

void BlobRegistryWKC::writeBlobsToTemporaryFilesForIndexedDB(const Vector<WTF::String>& blobURLs, CompletionHandler<void(Vector<WTF::String>&&filePaths)>&& callback)
{
    mImpl->writeBlobsToTemporaryFilesForIndexedDB(blobURLs, std::forward<CompletionHandler<void(Vector<WTF::String>&&filePaths)>>(callback));
}

}