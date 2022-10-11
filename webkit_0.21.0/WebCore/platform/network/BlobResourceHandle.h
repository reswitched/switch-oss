/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BlobResourceHandle_h
#define BlobResourceHandle_h

#include "FileStreamClient.h"
#include "ResourceHandle.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AsyncFileStream;
class BlobData;
class FileStream;
class ResourceHandleClient;
class ResourceRequest;
struct BlobDataItem;

class BlobResourceHandle final : public FileStreamClient, public ResourceHandle  {
public:
    static PassRefPtr<BlobResourceHandle> createAsync(BlobData*, const ResourceRequest&, ResourceHandleClient*);

    static void loadResourceSynchronously(BlobData*, const ResourceRequest&, ResourceError&, ResourceResponse&, Vector<char>& data);

    void start();
    int readSync(char*, int);

    bool aborted() const { return m_aborted; }

    enum class Error {
        NoError = 0,
        NotFoundError = 1,
        SecurityError = 2,
        RangeError = 3,
        NotReadableError = 4,
        MethodNotAllowed = 5
    };

private:
    BlobResourceHandle(BlobData*, const ResourceRequest&, ResourceHandleClient*, bool async);
    virtual ~BlobResourceHandle();

    // FileStreamClient methods.
    virtual void didGetSize(long long) override;
    virtual void didOpen(bool) override;
    virtual void didRead(int) override;

    // ResourceHandle methods.
    virtual void cancel() override;
    virtual void continueDidReceiveResponse() override;

    void doStart();
    void getSizeForNext();
    void seek();
    void consumeData(const char* data, int bytesRead);
    void failed(Error);

    void readAsync();
    void readDataAsync(const BlobDataItem&);
    void readFileAsync(const BlobDataItem&);

    int readDataSync(const BlobDataItem&, char*, int);
    int readFileSync(const BlobDataItem&, char*, int);

    void notifyResponse();
    void notifyResponseOnSuccess();
    void notifyResponseOnError();
    void notifyReceiveData(const char*, int);
    void notifyFail(Error);
    void notifyFinish();

    bool erroredOrAborted() const { return m_aborted || m_errorCode != Error::NoError; }

    RefPtr<BlobData> m_blobData;
    bool m_async;
    std::unique_ptr<AsyncFileStream> m_asyncStream; // For asynchronous loading.
    std::unique_ptr<FileStream> m_stream; // For synchronous loading.
    Vector<char> m_buffer;
    Vector<long long> m_itemLengthList;
    Error m_errorCode;
    bool m_aborted;
    long long m_rangeOffset;
    long long m_rangeEnd;
    long long m_rangeSuffixLength;
    long long m_totalRemainingSize;
    long long m_currentItemReadSize;
    unsigned m_sizeItemCount;
    unsigned m_readItemCount;
    bool m_fileOpened;
};

} // namespace WebCore

#endif // BlobResourceHandle_h
