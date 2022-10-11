/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#define __STDC_FORMAT_MACROS
#include "config.h"
#include "SQLiteFileSystem.h"

#include "FileSystem.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"
#include <inttypes.h>
#include <sqlite3.h>

#if PLATFORM(IOS)
#include <sqlite3_private.h>
#endif

namespace WebCore {

SQLiteFileSystem::SQLiteFileSystem()
{
}

int SQLiteFileSystem::openDatabase(const String& filename, sqlite3** database, bool)
{
    return sqlite3_open(fileSystemRepresentation(filename).data(), database);
}

String SQLiteFileSystem::appendDatabaseFileNameToPath(const String& path, const String& fileName)
{
    return pathByAppendingComponent(path, fileName);
}

bool SQLiteFileSystem::ensureDatabaseDirectoryExists(const String& path)
{
    if (path.isEmpty())
        return false;
    return makeAllDirectories(path);
}

bool SQLiteFileSystem::ensureDatabaseFileExists(const String& fileName, bool checkPathOnly)
{
    if (fileName.isEmpty())
        return false;

    if (checkPathOnly) {
        String dir = directoryName(fileName);
        return ensureDatabaseDirectoryExists(dir);
    }

    return fileExists(fileName);
}

bool SQLiteFileSystem::deleteEmptyDatabaseDirectory(const String& path)
{
    return deleteEmptyDirectory(path);
}

bool SQLiteFileSystem::deleteDatabaseFile(const String& fileName)
{
    return deleteFile(fileName);
}

#if PLATFORM(IOS)
bool SQLiteFileSystem::truncateDatabaseFile(sqlite3* database)
{
    return sqlite3_file_control(database, 0, SQLITE_TRUNCATE_DATABASE, 0) == SQLITE_OK;
}
#endif
    
long long SQLiteFileSystem::getDatabaseFileSize(const String& fileName)
{        
    long long size;
    return getFileSize(fileName, size) ? size : 0;
}

double SQLiteFileSystem::databaseCreationTime(const String& fileName)
{
    time_t time;
    return getFileCreationTime(fileName, time) ? time : 0;
}

double SQLiteFileSystem::databaseModificationTime(const String& fileName)
{
    time_t time;
    return getFileModificationTime(fileName, time) ? time : 0;
}

} // namespace WebCore
