/*
 * Copyright (c) 2010-2013 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "Clipboard.h"

#include "Blob.h"
#include "DataTransferItem.h"
#include "DataTransferItemList.h"
#include "File.h"
#include "StringCallback.h"
#include "NotImplemented.h"

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)
void
Clipboard::declareAndWriteDragImage(Element*, const URL&, const String& title, Frame*)
{
    notImplemented();
}

#endif

DragImageRef
Clipboard::createDragImage(IntPoint& dragLocation) const
{
    notImplemented();
    return (DragImageRef)0;
}

#if ENABLE(DATA_TRANSFER_ITEMS)
class DataTransferItemWKC : public DataTransferItem
{
public:
    DataTransferItemWKC() {}
    virtual ~DataTransferItemWKC() {}

    virtual String kind() const { return String(); }
    virtual String type() const { return String(); }

    virtual void getAsString(PassRefPtr<StringCallback>) const {}
    virtual PassRefPtr<Blob> getAsFile() const
    {
        return Blob::create();
    }
};

class DataTransferItemListWKC : public DataTransferItemList
{
public:
    DataTransferItemListWKC() {}
    virtual ~DataTransferItemListWKC() {}
    virtual size_t length() const { return 0; }
    virtual PassRefPtr<DataTransferItem> item(unsigned long index)
    {
        DataTransferItemWKC* item = 0;
        return adoptRef(item);
    } 
    virtual void deleteItem(unsigned long index, ExceptionCode&) {}
    virtual void clear() {}
    virtual void add(const String& data, const String& type, ExceptionCode&) {}
    virtual void add(PassRefPtr<File>) {}
};

PassRefPtr<DataTransferItemList>
Clipboard::items()
{
    DataTransferItemListWKC* items = new DataTransferItemListWKC();
    return adoptRef(items);
}
#endif

}
