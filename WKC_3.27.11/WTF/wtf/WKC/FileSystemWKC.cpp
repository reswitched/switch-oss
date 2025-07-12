/*
 * Copyright (C) 2007, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#include "WTFString.h"
#include "CString.h"
#include <wtf/text/StringBuilder.h>

#include "NotImplemented.h"

#include <wkc/wkcfilepeer.h>

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

namespace WTF {

namespace FileSystemImpl {

MappedFileData::~MappedFileData()
{
    notImplemented();
}

bool MappedFileData::mapFileHandle(PlatformFileHandle, FileOpenMode, MappedFileMode)
{
    notImplemented();
    return false;
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

std::optional<uint64_t> fileSize(const String& path)
{
    if (path.isNull() || path.isEmpty()) {
        return std::nullopt;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return std::nullopt;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return std::nullopt;
    }
    return st.st_size;
}

std::optional<uint64_t> fileSize(PlatformFileHandle handle)
{
    if (handle == invalidPlatformFileHandle)
        return std::nullopt;

    struct stat st;
    int err;

    void* fp = reinterpret_cast<void*>(handle);
    err = wkcFileFStatPeer(fp, &st);
    if (err == -1) {
        return std::nullopt;
    }
    return st.st_size;
}

std::optional<WTF::WallTime> fileModificationTime(const String& path)
{
    if (path.isNull() || path.isEmpty()) {
        return std::nullopt;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, path.utf8().data(), "rb");
    if (!fp)
        return std::nullopt;

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return std::nullopt;
    }

    WTF::WallTime wt = WTF::WallTime::fromRawSeconds(st.st_mtime);
    return { wt };
}

std::optional<WallTime> getFileCreationTime(const String&)
{
    notImplemented();
    return {};
}

String pathByAppendingComponent(const String& path, const String& component)
{
    if (path.endsWith('/'))
        return path + component;
    return path + "/" + component;
}

String pathByAppendingComponents(StringView path, const Vector<StringView>& components)
{
    StringBuilder builder;
    builder.append(path);
    bool isFirstComponent = true;
    for (auto& component : components) {
        if (isFirstComponent) {
            isFirstComponent = false;
            if (path.endsWith('/')) {
                builder.append(component);
                continue;
            }
        }
        builder.append('/', component);
    }
    return builder.toString();
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

String pathFileName(const String& pathName)
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

String parentPath(const String& path)
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

std::optional<int32_t> getFileDeviceId(const CString& fsFile)
{
    if (fsFile.isNull() || fsFile.length() == 0) {
        return std::nullopt;
    }

    struct stat st;
    int err;

    void* fp = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, fsFile.data(), "rb");
    if (!fp) {
        return std::nullopt;
    }

    err = wkcFileFStatPeer(fp, &st);
    wkcFileFClosePeer(fp);
    if (err == -1) {
        return false;
    }

    return st.st_dev;
}

Vector<String> listDirectory(const String& path)
{
    static const int cMaxEntryLen = 4096;

    char* strBuf = nullptr;
    char** strList = nullptr;
    Vector<String> entries;

    strBuf = static_cast<char*>(fastMalloc(MAX_PATH * cMaxEntryLen));
    strList = static_cast<char**>(fastMalloc(sizeof(char*) * cMaxEntryLen));

    ::memset(strBuf, 0, MAX_PATH * cMaxEntryLen);

    for (int i = 0; i < cMaxEntryLen; i++)
        strList[i] = &strBuf[i * MAX_PATH];

    int num = wkcFileListDirectoryPeer(path.utf8().data(), "", strList, MAX_PATH, cMaxEntryLen);

    num = std::clamp(num, 0, cMaxEntryLen);

    for (int i = 0; i < num; i++)
        entries.append(String::fromUTF8(strList[i]));

    fastFree(strList);
    fastFree(strBuf);

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

String openTemporaryFile(const String& prefix, PlatformFileHandle& handle, const String& suffix)
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

PlatformFileHandle openFile(const String& path, FileOpenMode mode, FileAccessPermission, bool failIfFileExists)
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

int writeToFile(PlatformFileHandle handle, const void* data, int length)
{
    if (handle==invalidPlatformFileHandle)
        return 0;
    void* fd = reinterpret_cast<void *>(handle);
    size_t ret = wkcFileFWritePeer(data, 1, length, fd);
    return ret;
}

int readFromFile(PlatformFileHandle handle, void* data, int length)
{
    if (handle==invalidPlatformFileHandle)
        return 0;
    void* fd = reinterpret_cast<void *>(handle);
    size_t ret = wkcFileFReadPeer(data, 1, length, fd);
    return ret;
}

bool unmapViewOfFile(void*, size_t)
{
    notImplemented();
    return false;
}

std::optional<FileType> fileTypeFollowingSymlinks(const String& path)
{
    notImplemented();
    return std::nullopt;
}

std::optional<FileType> fileType(const String& path)
{
    notImplemented();
    return std::nullopt;
}

bool isHiddenFile(const String& path)
{
#if OS(UNIX)
    auto fsPath = toStdFileSystemPath(path);
    std::filesystem::path::string_type filename = fsPath.filename();
    return !filename.empty() && filename[0] == '.';
#else
    UNUSED_PARAM(path);
    return false;
#endif
}

bool moveFile(const String& oldPath, const String& newPath)
{
    notImplemented();
    return false;
}

bool deleteEmptyDirectory(const String&)
{
    notImplemented();
    return false;
}

bool hardLinkOrCopyFile(const String&, const String&)
{
    notImplemented();
    return false;
}

std::optional<WallTime> fileCreationTime(const String&)
{
    notImplemented();
    return std::nullopt;
}

bool flushFile(PlatformFileHandle)
{
    notImplemented();
    return false;
}

bool deleteNonEmptyDirectory(const String&)
{
    notImplemented();
    return false;
}

// bool lockFile(PlatformFileHandle, OptionSet<FileLockMode>);
// bool unlockFile(PlatformFileHandle);

}

}
