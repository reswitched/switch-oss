/*
 * Copyright (C) 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora Ltd.
 * All rights reserved.
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FormDataStreamWKC.h"

#include "BlobRegistryImpl.h"
#include "CString.h"
#include "FormData.h"
#include "LoggingWKC.h"
#include "ResourceRequest.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternalWKC.h"
#include "ResourceHandleClient.h"
#include <wkc/wkcfilepeer.h>

namespace WebCore {

FormDataStream::~FormDataStream()
{
}

size_t FormDataStream::read(void* ptr, size_t blockSize, size_t numberOfBlocks)
{
    ResourceHandleInternal* d = m_resourceHandle->getInternal();
    if (!d->client())
        return 0;

    // Check for overflow.
    if (!numberOfBlocks || blockSize > std::numeric_limits<size_t>::max() / numberOfBlocks)
        return 0;

    Vector<FormDataElement> elements;
    if (m_formData)
        elements = m_formData->elements();

    if (m_formDataElementIndex >= elements.size())
        return 0;

    FormDataElement element = elements[m_formDataElementIndex];

    size_t toSend = blockSize * numberOfBlocks;
    size_t sent;
    size_t remainingDataSize;

    if (auto encodedFile = WTF::get_if<FormDataElement::EncodedFileData>(element.data)) {
        size_t fileSize = element.lengthInBytes();
        if (m_fileData.isEmpty()) {
            bool ret = readFromFile(encodedFile->filename.utf8().data(), encodedFile->fileStart, fileSize, m_fileData);
            if (!ret) {
                LOG(FormData, "(d:%p) Failed to read the file. (type: EncodedFileData, index: %d, length: %" PRIi64 ", offset: %" PRIi64 ", name: %s)", d, m_formDataElementIndex, fileSize, encodedFile->fileStart, encodedFile->filename.utf8().data());
                m_formDataElementIndex++;
                return 0;
            }
        }
        remainingDataSize = fileSize - m_formDataElementDataOffset;
        sent = remainingDataSize > toSend ? toSend : remainingDataSize;
        memcpy(ptr, m_fileData.data() + m_formDataElementDataOffset, sent);
        LOG(FormData, "(d:%p) Sent form stream data bytes: %zu (type: EncodedFileData, index: %d, remaining: %zu)", d, sent, m_formDataElementIndex, remainingDataSize - sent);
        if (remainingDataSize > sent)
            m_formDataElementDataOffset += sent;
        else {
            m_fileData.clear();
            m_formDataElementDataOffset = 0;
            m_formDataElementIndex++;
        }
    } else if (auto encodedBlob = WTF::get_if<FormDataElement::EncodedBlobData>(element.data)) {
        // FIXME: Do not call blobRegistry() from threads other than the main thread.
        BlobData* blobData = static_cast<BlobRegistryImpl&>(*blobRegistry().blobRegistryImpl()).getBlobDataFromURL(encodedBlob->url);
        if (!blobData)
            blobData = d->m_sendingBlobDataList[m_formDataElementIndex].get();
        if (!blobData || blobData->items().isEmpty()) {
            LOG(FormData, "(d:%p) No blob data. (type: EncodedBlobData, index: %d, url: %s)", d, m_formDataElementIndex, encodedBlob->url.string().utf8().data());
            m_formDataElementIndex++;
            return 0;
        }
        const BlobDataItem& item = blobData->items()[m_formDataBlobItemIndex];
        if (item.type() == BlobDataItem::Type::File) {
            if (m_fileData.isEmpty()) {
                if (!item.file() || item.length() == 0) {
                    LOG(FormData, "(d:%p) No item file. (type: EncodedFileData(%d), index: %d(%d))", d, item.type(), m_formDataElementIndex, m_formDataBlobItemIndex);
                    ASSERT_NOT_REACHED();
                    m_formDataElementIndex++;
                    return 0;
                }
                bool ret = readFromFile(item.file()->path().utf8().data(), item.offset(), item.length(), m_fileData);
                if (!ret) {
                    LOG(FormData, "(d:%p) Failed to read the file. (type: EncodedFileData(%d), index: %d(%d), length: %" PRIi64 ", offset: %" PRIi64 ", name: %s)", d, item.type(), m_formDataElementIndex, m_formDataBlobItemIndex, item.length(), item.offset(), item.file()->path().utf8().data());
                    m_formDataElementIndex++;
                    return 0;
                }
            }
        } else {
            if (!item.data().data()) {
                LOG(FormData, "(d:%p) No item data. (type: EncodedFileData(%d), index: %d(%d), length: %" PRIi64 ", offset: %" PRIi64 ")", d, item.type(), m_formDataElementIndex, m_formDataBlobItemIndex, item.length(), item.offset());
                m_formDataElementIndex++;
                return 0;
            }
        }
        const uint8_t* data = (item.type() == BlobDataItem::Type::File) ? m_fileData.data() : item.data().data()->data() + item.offset();
        remainingDataSize = item.length() - m_formDataElementDataOffset;
        sent = remainingDataSize > toSend ? toSend : remainingDataSize;
        memcpy(ptr, data + m_formDataElementDataOffset, sent);
        LOG(FormData, "(d:%p) Sent form stream data bytes: %zu (type: EncodedBlobData, index: %d(%d), remaining: %zu)", d, sent, m_formDataElementIndex, m_formDataBlobItemIndex, remainingDataSize - sent);
        if (remainingDataSize > sent)
            m_formDataElementDataOffset += sent;
        else {
            m_fileData.clear();
            m_formDataElementDataOffset = 0;
            m_formDataBlobItemIndex++;
            if (m_formDataBlobItemIndex >= blobData->items().size()) {
                m_formDataBlobItemIndex = 0;
                m_formDataElementIndex++;
            }
        }
    } else {
        auto data = WTF::get_if<Vector<char>>(element.data);
        if (!data) {
            ASSERT_NOT_REACHED();
            m_formDataElementIndex++;
            return 0;
        }
        remainingDataSize = data->size() - m_formDataElementDataOffset;
        sent = remainingDataSize > toSend ? toSend : remainingDataSize;
        memcpy(ptr, data->data() + m_formDataElementDataOffset, sent);
        LOG(FormData, "(d:%p) Sent form stream data bytes: %zu (type: Data, index: %d, remaining: %zu)", d, sent, m_formDataElementIndex, remainingDataSize - sent);
        if (remainingDataSize > sent)
            m_formDataElementDataOffset += sent;
        else {
            m_formDataElementDataOffset = 0;
            m_formDataElementIndex++;
        }
    }

    return sent;
}

bool FormDataStream::readFromFile(const char* filename, int64_t offset, int64_t length, Vector<uint8_t>& data)
{
    void* file = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_FORMDATA, filename, "rb");
    if (!file) {
        return false;
    }

    if (offset > 0) {
        int err = wkcFileFSeekPeer(file, offset, SEEK_SET);
        if (err) {
            wkcFileFClosePeer(file);
            return false;
        }
    }

    data.resize(length);

    size_t totalBytesRead = 0;
    ssize_t bytesRead;

    while ((bytesRead = wkcFileFReadPeer(data.data() + totalBytesRead, 1, length - totalBytesRead, file)) > 0)
        totalBytesRead += bytesRead;

    if (bytesRead < 0) {
        wkcFileFClosePeer(file);
        return false;
    }

    wkcFileFClosePeer(file);

    return true;
}

bool FormDataStream::hasMoreElements() const
{
    if (!m_formData)
        return false;

    return m_formDataElementIndex < m_formData->elements().size();
}

void FormDataStream::refresh()
{
    m_formDataElementIndex = 0;
    m_formDataElementDataOffset = 0;
    m_formDataBlobItemIndex = 0;
    m_fileData.clear();
}

} // namespace WebCore
