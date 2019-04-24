/*
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "SharedBuffer.h"

#include "CString.h"
#include "FileSystem.h"

#include <wkc/wkcfilepeer.h>

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

namespace WebCore {

RefPtr<SharedBuffer>
SharedBuffer::createFromReadingFile(const String& filePath)
{
    if (filePath.isEmpty())
        return nullptr;

    CString filename = fileSystemRepresentation(filePath);
    void* fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, filename.data(), "rb");
    if (!fd)
        return nullptr;

    struct stat fileStat;
    if (wkcFileFStatPeer(fd, &fileStat) == -1) {
        wkcFileFClosePeer(fd);
        return nullptr;
    }

    size_t bytesToRead = fileStat.st_size;
    if (fileStat.st_size < 0 || bytesToRead != static_cast<unsigned long long>(fileStat.st_size)) {
        wkcFileFClosePeer(fd);
        return nullptr;
    }

    Vector<char> buffer(bytesToRead);

    size_t totalBytesRead = 0;
    ssize_t bytesRead;
    while ((bytesRead = wkcFileFReadPeer(buffer.data() + totalBytesRead, 1, bytesToRead - totalBytesRead, fd)) > 0)
        totalBytesRead += bytesRead;

    wkcFileFClosePeer(fd);

    return totalBytesRead == bytesToRead ? SharedBuffer::adoptVector(buffer) : nullptr;
}

} // namespace WebCore
