/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DataLog.h"
#include <stdarg.h>
#include <string.h>
#include <wtf/FilePrintStream.h>
#include <wtf/WTFThreadData.h>
#include <wtf/Threading.h>

#if OS(UNIX) || OS(DARWIN)
#include <unistd.h>
#endif

#define DATA_LOG_TO_FILE 0

// Set to 1 to use the temp directory from confstr instead of hardcoded directory.
// The last component of DATA_LOG_FILENAME will still be used.
#define DATA_LOG_TO_DARWIN_TEMP_DIR 0

// Uncomment to force logging to the given file regardless of what the environment variable says.
// Note that we will append ".<pid>.txt" where <pid> is the PID.
// This path won't work on Windows, make sure to change to something like C:\\Users\\<more path>\\log.txt.
#define DATA_LOG_FILENAME "/tmp/WTFLog"

namespace WTF {

#if USE(PTHREADS)
static pthread_once_t initializeLogFileOnceKey = PTHREAD_ONCE_INIT;
#endif

#if !PLATFORM(WKC)
static FilePrintStream* file;
#else
WKC_DEFINE_GLOBAL_PTR(FilePrintStream*, file, 0);
#endif

static uint64_t fileData[(sizeof(FilePrintStream) + 7) / 8];

static void initializeLogFileOnce()
{
#if DATA_LOG_TO_FILE
    const long maxPathLength = 1024;

    char filenameSuffix[maxPathLength + 1];

#if PLATFORM(WIN)
    _snprintf(filenameSuffix, sizeof(filenameSuffix), ".%d.txt", GetCurrentProcessId());
#else
    snprintf(filenameSuffix, sizeof(filenameSuffix), ".%d.txt", getpid());
#endif

#if DATA_LOG_TO_DARWIN_TEMP_DIR
    char filenameBuffer[maxPathLength + 1];
    unsigned suffixLength = strlen(filenameSuffix);

#if defined(DATA_LOG_FILENAME)
    char* logBasename = strrchr(DATA_LOG_FILENAME, '/');
    if (!logBasename)
        logBasename = (char*)DATA_LOG_FILENAME;
#else
    const char* logBasename = "WTFLog";
#endif

    const char* filename = 0;

    size_t lastComponentLength = strlen(logBasename) + suffixLength;
    size_t dirnameLength = confstr(_CS_DARWIN_USER_TEMP_DIR, filenameBuffer, 1024);
    if ((dirnameLength + lastComponentLength + suffixLength) < maxPathLength) {
        strncat(filenameBuffer, logBasename, maxPathLength - dirnameLength);
        filename = filenameBuffer;
    }
#elif defined(DATA_LOG_FILENAME)
    const char* filename = DATA_LOG_FILENAME;
#else
    const char* filename = getenv("WTF_DATA_LOG_FILENAME");
#endif
    char actualFilename[maxPathLength + 1];

    if (filename) {
#if PLATFORM(WIN)
        _snprintf(actualFilename, sizeof(actualFilename), "%s%s", filename, filenameSuffix);
#else
        snprintf(actualFilename, sizeof(actualFilename), "%s%s", filename, filenameSuffix);
#endif
        
        file = FilePrintStream::open(actualFilename, "w").release();
        if (file)
            WTFLogAlways("*** DataLog output to \"%s\" ***\n", actualFilename);
        else
            WTFLogAlways("Warning: Could not open DataLog file %s for writing.\n", actualFilename);
    }
#endif // DATA_LOG_TO_FILE
    if (!file) {
        // Use placement new; this makes it easier to use dataLog() to debug
        // fastMalloc.
        file = new (fileData) FilePrintStream(stderr, FilePrintStream::Borrow);
    }
    
    setvbuf(file->file(), 0, _IONBF, 0); // Prefer unbuffered output, so that we get a full log upon crash or deadlock.
}

static void initializeLogFile()
{
#if USE(PTHREADS)
    pthread_once(&initializeLogFileOnceKey, initializeLogFileOnce);
#else
    if (!file)
        initializeLogFileOnce();
#endif
}

FilePrintStream& dataFile()
{
    initializeLogFile();
    return *file;
}

void dataLogFV(const char* format, va_list argList)
{
    dataFile().vprintf(format, argList);
}

void dataLogF(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    dataLogFV(format, argList);
    va_end(argList);
}

void dataLogFString(const char* str)
{
    dataFile().printf("%s", str);
}

} // namespace WTF

