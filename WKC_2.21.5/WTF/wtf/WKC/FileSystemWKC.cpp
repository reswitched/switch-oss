/*
 * Copyright (C) 2007, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (c) 2010-2022 ACCESS CO., LTD. All rights reserved.
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
#include "DateMath.h"
#include "FileSystem.h"
#include "FileMetadata.h"

#include "WTFString.h"
#include "CString.h"

#include "NotImplemented.h"

#include <wkc/wkcfilepeer.h>

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

namespace WTF {

namespace FileSystemImpl {

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

bool moveFile(const String& oldPath, const String& newPath)
{
    notImplemented();
    return false;
}

bool getFileSize(const String& path, long long& result)
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
    result = st.st_size;

    return true;
}

bool getFileSize(PlatformFileHandle handle, long long& result)
{
    struct stat st;
    int err;

    void* fp = reinterpret_cast<void*>(handle);
    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return false;
    }
    result = st.st_size;

    return true;
}

Optional<WTF::WallTime> getFileModificationTime(const String& path)
{
    if (path.isNull() || path.isEmpty()) {
        return WTF::nullopt;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return WTF::nullopt;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return WTF::nullopt;
    }

    WTF::WallTime wt = WTF::WallTime::fromRawSeconds(st.st_mtime);
    return { wt };
}

Optional<WallTime> getFileCreationTime(const String&)
{
    notImplemented();
    return {};
}

Optional<FileMetadata> fileMetadata(const String& path)
{
    if (path.isNull() || path.isEmpty()) {
        return WTF::nullopt;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return WTF::nullopt;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return WTF::nullopt;
    }

    FileMetadata metadata;
    metadata.modificationTime = WallTime::fromRawSeconds(st.st_mtime);
    metadata.length = st.st_size;

    // FIXME: metadata.type is need to set correct value.
    metadata.type = FileMetadata::Type::File;

    return metadata;
}

Optional<FileMetadata> fileMetadataFollowingSymlinks(const String& path)
{
    return fileMetadata(path);
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

String pathByAppendingComponents(StringView path, const Vector<StringView>& components)
{
    notImplemented();
    return String();
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

bool getVolumeFreeSpace(const String&, uint64_t&)
{
    notImplemented();
    return false;
}

Optional<int32_t> getFileDeviceId(const CString& fsFile)
{
    if (fsFile.isNull() || fsFile.length() == 0) {
        return WTF::nullopt;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, fsFile.data(), "rb");
    if (!fp) {
        return WTF::nullopt;
    }

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return false;
    }

    return st.st_dev;
}

bool createSymbolicLink(const String& targetPath, const String& symbolicLinkPath)
{
    notImplemented();
    return false;
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

CString fileSystemRepresentation(const String& str)
{
    return str.utf8();
}

String stringFromFileSystemRepresentation(const char* str)
{
    return String(str);
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
    void* fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), mode==FileOpenMode::Read ? "rb" : "wb");
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

long long seekFile(PlatformFileHandle handle, long long offset, FileSeekOrigin origin)
{
    if (handle==invalidPlatformFileHandle)
        return 0;
    void* fd = reinterpret_cast<void *>(handle);
    size_t pos = wkcFileFSeekPeer(fd, offset, (int)origin);
    return pos;
}

bool truncateFile(PlatformFileHandle, long long offset)
{
    notImplemented();
    return false;
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

bool hardLinkOrCopyFile(const String& source, const String& destination)
{
    notImplemented();
    return false;
}

MappedFileData::MappedFileData(const String& filePath, MappedFileMode, bool& success)
{
    notImplemented();
    success = false;
}

bool MappedFileData::mapFileHandle(PlatformFileHandle, FileSystemImpl::MappedFileMode)
{
    notImplemented();
    return false;
}

bool unmapViewOfFile(void*, size_t)
{
    notImplemented();
    return false;
}

// bool lockFile(PlatformFileHandle, OptionSet<FileLockMode>);
// bool unlockFile(PlatformFileHandle);

}

}
