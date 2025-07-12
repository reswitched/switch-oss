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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WKCStorageAreaIf_h
#define WKCStorageAreaIf_h

#include <wkc/wkcbase.h>
#include "WKCHelpersEnums.h"

namespace WKC {

    class String;
    class Frame;

    class WKC_API StorageAreaIf {
    public:
        virtual ~StorageAreaIf() {}
        virtual unsigned length() = 0;
        virtual String key(unsigned index) = 0;
        virtual String item(const String& key) = 0;
        virtual void setItem(Frame* sourceFrame, const String& key, const String& value, bool& quotaException) = 0;
        virtual void removeItem(Frame* sourceFrame, const String& key) = 0;
        virtual void clear(Frame* sourceFrame) = 0;
        virtual bool contains(const String& key) = 0;
        virtual size_t memoryBytesUsedByCache() = 0;

        virtual void incrementAccessCount() = 0;
        virtual void decrementAccessCount() = 0;
        virtual void closeDatabaseIfIdle() = 0;
    };

} // namespace

#endif // WKCStorageAreaIf_h
