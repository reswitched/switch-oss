/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBCursorBackendOperations.h"

#include "IDBServerConnection.h"

#if ENABLE(INDEXED_DATABASE)

#include "Logging.h"

namespace WebCore {

void CursorAdvanceOperation::perform(std::function<void()> completionCallback)
{
    LOG(StorageAPI, "CursorAdvanceOperation");

    RefPtr<CursorAdvanceOperation> operation(this);
    auto callback = [this, operation, completionCallback](PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer> valueBuffer, PassRefPtr<IDBDatabaseError> error) {
        if (error) {
            m_cursor->clear();
            m_callbacks->onError(error);
        } else if (!key) {
            // If there's no error but also no key, then the cursor reached the end.
            m_cursor->clear();
            m_callbacks->onSuccess(static_cast<SharedBuffer*>(nullptr));
        } else {
            m_cursor->updateCursorData(key.get(), primaryKey.get(), valueBuffer.get());
            m_callbacks->onSuccess(key, primaryKey, valueBuffer);
        }

        // FIXME: Cursor operations should be able to pass along an error instead of success
        completionCallback();
    };

    m_cursor->transaction().database().serverConnection().cursorAdvance(*m_cursor, *this, callback);
}

void CursorIterationOperation::perform(std::function<void()> completionCallback)
{
    LOG(StorageAPI, "CursorIterationOperation");

    RefPtr<CursorIterationOperation> operation(this);
    auto callback = [this, operation, completionCallback](PassRefPtr<IDBKey> key, PassRefPtr<IDBKey> primaryKey, PassRefPtr<SharedBuffer> valueBuffer, PassRefPtr<IDBDatabaseError> error) {
        if (error) {
            m_cursor->clear();
            m_callbacks->onError(error);
        } else if (!key) {
            // If there's no error but also no key, then the cursor reached the end.
            m_cursor->clear();
            m_callbacks->onSuccess(static_cast<SharedBuffer*>(nullptr));
        } else {
            m_cursor->updateCursorData(key.get(), primaryKey.get(), valueBuffer.get());
            m_callbacks->onSuccess(key, primaryKey, valueBuffer);
        }

        // FIXME: Cursor operations should be able to pass along an error instead of success
        completionCallback();
    };

    m_cursor->transaction().database().serverConnection().cursorIterate(*m_cursor, *this, callback);
}

} // namespace WebCore

#endif // ENABLED(INDEXED_DATABASE)
