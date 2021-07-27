/*
 * Copyright (C) 2007, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
 *
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
 */

#include "config.h"
#include "FileSystem.h"
#include "FileMetadata.h"

#include "WTFString.h"
#include "CString.h"

#include "NotImplemented.h"

#include <wkc/wkcfilepeer.h>

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

namespace WebCore {

String filenameToString(const char* filename)
{
    notImplemented();
    return String::fromUTF8(filename);
}

char* filenameFromString(const String& string)
{
    notImplemented();
    return (char *)string.utf8().data();
}

// Converts a string to something suitable to be displayed to the user.
String filenameForDisplay(const String& string)
{
    notImplemented();
    return string;
}

bool fileExists(const String& path)
{
    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return false;
    wkcFileFClosePeer(fp);
    return true;
}

bool deleteFile(const String& path)
{
    wkcFileUnlinkPeer(path.utf8().data());
    return true;
}

bool deleteEmptyDirectory(const String& path)
{
    notImplemented();
    return false;
}

bool getFileSize(const String& path, long long& resultSize)
{
    if (path.isNull() || path.isEmpty()) {
        return false;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return false;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return false;
    }
    resultSize = st.st_size;

    return true;
}

bool getFileModificationTime(const String& path, time_t& modifiedTime)
{
    if (path.isNull() || path.isEmpty()) {
        return false;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return false;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return false;
    }

    modifiedTime = st.st_mtime;

    return true;
}

bool getFileMetadata(const String& path, FileMetadata& metadata)
{
    if (path.isNull() || path.isEmpty()) {
        return false;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return false;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return false;
    }

    metadata.modificationTime = st.st_mtime;
    metadata.length = st.st_size;

    // FIXME: metadata.type is need to set correct value.
    metadata.type = FileMetadata::Type::TypeFile;

    return true;
}

String pathByAppendingComponent(const String& path, const String& component)
{
    char buf[MAX_PATH] = {0};

    int ret = wkcFilePathByAppendingComponentPeer(path.utf8().data(), component.utf8().data(), buf, MAX_PATH);
    if (!ret) {
        return String();
    }

    return String::fromUTF8(buf);
}

bool makeAllDirectories(const String& path)
{
    return wkcFileMakeAllDirectoriesPeer(path.utf8().data());
}

String homeDirectoryPath()
{
    char buf[MAX_PATH] = {0};

    int ret = wkcFileHomeDirectoryPathPeer(buf, MAX_PATH);
    if (!ret) {
        return String();
    }

    return String::fromUTF8(buf);
}

String pathGetFileName(const String& pathName)
{
    char buf[MAX_PATH] = {0};

    if (pathName.isEmpty() || pathName.isNull())
        return String();

    int ret = wkcFilePathGetFileNamePeer(pathName.utf8().data(), buf, MAX_PATH);

    if (!ret) {
        return String();
    }

    return String::fromUTF8(buf);
}

String directoryName(const String& path)
{
    char buf[MAX_PATH] = {0};

    int ret = wkcFileDirectoryNamePeer(path.utf8().data(), buf, MAX_PATH);
    if (!ret) {
        return String();
    }

    return String::fromUTF8(buf);
}

Vector<String> listDirectory(const String& path, const String& filter)
{
    char** buf = nullptr;
    Vector<String> entries;

    buf = (char **)fastMalloc(sizeof(char*)*1024);
    for (int i = 0; i < 1024; i++) {
        buf[i] = (char *)fastMalloc(MAX_PATH);
        ::memset(buf[i], 0, MAX_PATH);
    }
    int ret = wkcFileListDirectoryPeer(path.utf8().data(), filter.utf8().data(), buf, MAX_PATH, 1024);
    if (ret<0)
        ret = 0;
    for (int i = 0; i < ret; i++) {
        entries.append(String::fromUTF8(buf[i]));
        fastFree(buf[i]);
    }
    for (int i = ret; i < 1024; i++) {
        fastFree(buf[i]);
    }
    fastFree(buf);

    return entries;
}

String openTemporaryFile(const String& prefix, PlatformFileHandle& handle)
{
    char name[1024] = {0};
    void* fd = wkcFileOpenTemporaryFilePeer(prefix.utf8().data(), name, sizeof(name));
    if (!fd)
        goto error_end;
    handle = (PlatformFileHandle)reinterpret_cast<uintptr_t>(fd);
    return String::fromUTF8(name);

error_end:
    handle = 0;
    return String();
}

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
{
    void* fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), mode==OpenForRead ? "rb" : "wb");
    if (!fd) {
        return invalidPlatformFileHandle;
    }
    return (PlatformFileHandle)reinterpret_cast<uintptr_t>(fd);
}

void closeFile(PlatformFileHandle& handle)
{
    if (handle==invalidPlatformFileHandle)
        return;
    void* fd = reinterpret_cast<void *>(handle);
    wkcFileFClosePeer(fd);
}

int writeToFile(PlatformFileHandle handle, const char* data, int length)
{
    if (handle==invalidPlatformFileHandle)
        return 0;
    void* fd = reinterpret_cast<void *>(handle);
    size_t ret = wkcFileFWritePeer(data, 1, length, fd);
    return ret;
}

int readFromFile(PlatformFileHandle handle, char* data, int length)
{
    if (handle==invalidPlatformFileHandle)
        return 0;
    void* fd = reinterpret_cast<void *>(handle);
    size_t ret = wkcFileFReadPeer(data, 1, length, fd);
    return ret;
}

long long seekFile(PlatformFileHandle handle, long long offset, FileSeekOrigin origin)
{
    if (handle==invalidPlatformFileHandle)
        return 0;
    void* fd = reinterpret_cast<void *>(handle);
    size_t pos = wkcFileFSeekPeer(fd, offset, origin);
    return pos;
}

bool getFileCreationTime(const String&, time_t& result)
{
    notImplemented();
    result = 0;
    return true;
}

CString fileSystemRepresentation(const String& str)
{
    return str.utf8();
}

}
