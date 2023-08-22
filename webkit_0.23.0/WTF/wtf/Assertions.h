/*
 * Copyright (C) 2003, 2006, 2007, 2013 Apple Inc.  All rights reserved.
 * Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef WTF_Assertions_h
#define WTF_Assertions_h

#include <wtf/Platform.h>

/*
   no namespaces because this file has to be includable from C and Objective-C

   Note, this file uses many GCC extensions, but it should be compatible with
   C, Objective C, C++, and Objective C++.

   For non-debug builds, everything is disabled by default.
   Defining any of the symbols explicitly prevents this from having any effect.
*/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <wtf/ExportMacros.h>

#ifdef NDEBUG
/* Disable ASSERT* macros in release mode. */
#define ASSERTIONS_DISABLED_DEFAULT 1
#else
#define ASSERTIONS_DISABLED_DEFAULT 0
#endif

#ifndef BACKTRACE_DISABLED
#define BACKTRACE_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_DISABLED
#define ASSERT_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_MSG_DISABLED
#define ASSERT_MSG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_ARG_DISABLED
#define ASSERT_ARG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef FATAL_DISABLED
#define FATAL_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ERROR_DISABLED
#define ERROR_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef LOG_DISABLED
#define LOG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#if COMPILER(GCC)
#define WTF_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define WTF_PRETTY_FUNCTION __FUNCTION__
#endif

/* WTF logging functions can process %@ in the format string to log a NSObject* but the printf format attribute
   emits a warning when %@ is used in the format string.  Until <rdar://problem/5195437> is resolved we can't include
   the attribute when being used from Objective-C code in case it decides to use %@. */
#if COMPILER(GCC) && !defined(__OBJC__)
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments) __attribute__((__format__(printf, formatStringArgument, extraArguments)))
#else
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments)
#endif

#if PLATFORM(IOS)
/* For a project that uses WTF but has no config.h, we need to explicitly set the export defines here. */
#ifndef WTF_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE
#endif
#endif // PLATFORM(IOS)

/* These helper functions are always declared, but not necessarily always defined if the corresponding function is disabled. */

#ifdef __cplusplus
extern "C" {
#endif

/* CRASH() - Raises a fatal error resulting in program termination and triggering either the debugger or the crash reporter.

   Use CRASH() in response to known, unrecoverable errors like out-of-memory.
   Macro is enabled in both debug and release mode.
   To test for unknown errors and verify assumptions, use ASSERT instead, to avoid impacting performance in release builds.

   Signals are ignored by the crash reporter on OS X so we must do better.
*/
#if COMPILER(CLANG) || COMPILER(GCC) || COMPILER(MSVC)
#define NO_RETURN_DUE_TO_CRASH NO_RETURN
#else
#define NO_RETURN_DUE_TO_CRASH
#endif

typedef enum { WTFLogChannelOff, WTFLogChannelOn } WTFLogChannelState;

typedef struct {
    WTFLogChannelState state;
    const char* name;
} WTFLogChannel;

WTF_EXPORT_PRIVATE void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion);
WTF_EXPORT_PRIVATE void WTFReportAssertionFailureWithMessage(const char* file, int line, const char* function, const char* assertion, const char* format, ...) WTF_ATTRIBUTE_PRINTF(5, 6);
WTF_EXPORT_PRIVATE void WTFReportArgumentAssertionFailure(const char* file, int line, const char* function, const char* argName, const char* assertion);
WTF_EXPORT_PRIVATE void WTFReportFatalError(const char* file, int line, const char* function, const char* format, ...) WTF_ATTRIBUTE_PRINTF(4, 5);
WTF_EXPORT_PRIVATE void WTFReportError(const char* file, int line, const char* function, const char* format, ...) WTF_ATTRIBUTE_PRINTF(4, 5);
WTF_EXPORT_PRIVATE void WTFLog(WTFLogChannel*, const char* format, ...) WTF_ATTRIBUTE_PRINTF(2, 3);
WTF_EXPORT_PRIVATE void WTFLogVerbose(const char* file, int line, const char* function, WTFLogChannel*, const char* format, ...) WTF_ATTRIBUTE_PRINTF(5, 6);
WTF_EXPORT_PRIVATE void WTFLogAlwaysV(const char* format, va_list);
WTF_EXPORT_PRIVATE void WTFLogAlways(const char* format, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH void WTFLogAlwaysAndCrash(const char* format, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
WTF_EXPORT_PRIVATE WTFLogChannel* WTFLogChannelByName(WTFLogChannel*[], size_t count, const char*);
WTF_EXPORT_PRIVATE void WTFInitializeLogChannelStatesFromString(WTFLogChannel*[], size_t count, const char*);

WTF_EXPORT_PRIVATE void WTFGetBacktrace(void** stack, int* size);
WTF_EXPORT_PRIVATE void WTFReportBacktrace();
WTF_EXPORT_PRIVATE void WTFPrintBacktrace(void** stack, int size);

typedef void (*WTFCrashHookFunction)();
WTF_EXPORT_PRIVATE void WTFSetCrashHook(WTFCrashHookFunction);
WTF_EXPORT_PRIVATE void WTFInstallReportBacktraceOnCrashHook();

WTF_EXPORT_PRIVATE bool WTFIsDebuggerAttached();

#ifdef __cplusplus
}
#endif

#ifndef CRASH
#if PLATFORM(WKC)
#define CRASH() do { wkcMemorySetCrashReasonPeer(__FILE__,__LINE__,__FUNCTION__,"CRASH()"); wkcMemoryNotifyCrashPeer(); } while(false)
#define CRASH_WITH_NO_MEMORY() do { wkcMemorySetCrashReasonPeer(__FILE__,__LINE__,__FUNCTION__,"No Memory"); wkcMemoryNotifyCrashPeer(); } while(false)
#else
#define CRASH() WTFCrash()
#endif
#endif

#if PLATFORM(WKC)
extern "C" WKC_PEER_API void wkcMemoryNotifyStackOverflowPeer(bool needRestart, size_t margin, size_t consumption, void* currentStackTop, const char* file, int line, const char* function);
extern "C" WKC_PEER_API bool wkcMemoryIsStackOverflowPeer(size_t margin, size_t *consumption, void** currentStackTop);
#define CRASH_IF_STACK_OVERFLOW(margin) do { \
    size_t __consumption; \
    void* __stack_top; \
    if (wkcMemoryIsStackOverflowPeer((margin), &__consumption, &__stack_top)) { \
        wkcMemoryNotifyStackOverflowPeer(true, (margin), __consumption, __stack_top, __FILE__, __LINE__, WTF_PRETTY_FUNCTION); \
    } \
} while (false)
#endif

#ifdef __cplusplus
extern "C" {
#endif
    WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH void WTFCrash();
#ifdef __cplusplus
}
#endif

#ifndef CRASH_WITH_SECURITY_IMPLICATION
#define CRASH_WITH_SECURITY_IMPLICATION() WTFCrashWithSecurityImplication()
#endif

#ifdef __cplusplus
extern "C" {
#endif
    WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH void WTFCrashWithSecurityImplication();
#ifdef __cplusplus
}
#endif

/* BACKTRACE

  Print a backtrace to the same location as ASSERT messages.
*/

#if BACKTRACE_DISABLED

#define BACKTRACE() ((void)0)

#else

#define BACKTRACE() do { \
    WTFReportBacktrace(); \
} while(false)

#endif

/* ASSERT, ASSERT_NOT_REACHED, ASSERT_UNUSED

  These macros are compiled out of release builds.
  Expressions inside them are evaluated in debug builds only.
*/

#if OS(WINDOWS)
/* FIXME: Change to use something other than ASSERT to avoid this conflict with the underlying platform */
#undef ASSERT
#endif

#if ASSERT_DISABLED

#define ASSERT(assertion) ((void)0)
#define ASSERT_AT(assertion, file, line, function) ((void)0)
#define ASSERT_NOT_REACHED() ((void)0)
#define ASSERT_IMPLIES(condition, assertion) ((void)0)
#define NO_RETURN_DUE_TO_ASSERT

#define ASSERT_UNUSED(variable, assertion) ((void)variable)

#if ENABLE(SECURITY_ASSERTIONS)
#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) \
    (!(assertion) ? \
        (WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion), \
         CRASH_WITH_SECURITY_IMPLICATION()) : \
        (void)0)

#define ASSERT_WITH_SECURITY_IMPLICATION_DISABLED 0
#else
#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) ((void)0)
#define ASSERT_WITH_SECURITY_IMPLICATION_DISABLED 1
#endif

#else

#if PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT(assertion) do {\
    if (!(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
        } \
} while(0)

#define ASSERT_AT(assertion, file, line, function) do {\
    if (!(assertion)) { \
        WTFReportAssertionFailure(file, line, function, #assertion); \
        }\
} while(0)

#define ASSERT_NOT_REACHED() do { \
    WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, 0); \
} while (0)

#define ASSERT_IMPLIES(condition, assertion) do { \
    if ((condition) && !(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #condition " => " #assertion); \
    } \
} while (0)

#else // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT(assertion) do {\
    if (!(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
        CRASH(); \
    } \
} while(0)

#define ASSERT_AT(assertion, file, line, function) do {\
    if (!(assertion)) { \
        WTFReportAssertionFailure(file, line, function, #assertion); \
        CRASH(); \
    }\
} while(0)

#define ASSERT_NOT_REACHED() do { \
    WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, 0); \
    CRASH(); \
} while (0)

#define ASSERT_IMPLIES(condition, assertion) do { \
    if ((condition) && !(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #condition " => " #assertion); \
        CRASH(); \
    } \
} while (0)

#endif // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_UNUSED(variable, assertion) ASSERT(assertion)

#define NO_RETURN_DUE_TO_ASSERT NO_RETURN_DUE_TO_CRASH

/* ASSERT_WITH_SECURITY_IMPLICATION
 
   Failure of this assertion indicates a possible security vulnerability.
   Class of vulnerabilities that it tests include bad casts, out of bounds
   accesses, use-after-frees, etc. Please file a bug using the security
   template - https://bugs.webkit.org/enter_bug.cgi?product=Security.
 
*/
#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) \
    (!(assertion) ? \
        (WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion), \
         CRASH_WITH_SECURITY_IMPLICATION()) : \
        (void)0)
#define ASSERT_WITH_SECURITY_IMPLICATION_DISABLED 0
#endif

/* ASSERT_WITH_MESSAGE */

#if ASSERT_MSG_DISABLED
#define ASSERT_WITH_MESSAGE(assertion, ...) ((void)0)
#else

#if PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_WITH_MESSAGE(assertion, ...) do \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        } \
        while (0)

#else // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_WITH_MESSAGE(assertion, ...) do \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        CRASH(); \
    } \
while (0)

#endif // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#endif

/* ASSERT_WITH_MESSAGE_UNUSED */

#if ASSERT_MSG_DISABLED
#define ASSERT_WITH_MESSAGE_UNUSED(variable, assertion, ...) ((void)variable)
#else

#if PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_WITH_MESSAGE_UNUSED(variable, assertion, ...) do \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        } \
        while (0)

#else // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_WITH_MESSAGE_UNUSED(variable, assertion, ...) do \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        CRASH(); \
    } \
while (0)

#endif // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#endif
                        
                        
/* ASSERT_ARG */

#if ASSERT_ARG_DISABLED

#define ASSERT_ARG(argName, assertion) ((void)0)

#else

#if PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_ARG(argName, assertion) do \
    if (!(assertion)) { \
        WTFReportArgumentAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #argName, #assertion); \
        } \
        while (0)

#else // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#define ASSERT_ARG(argName, assertion) do \
    if (!(assertion)) { \
        WTFReportArgumentAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #argName, #assertion); \
        CRASH(); \
    } \
while (0)

#endif // PLATFORM(WKC) && CRASH_DEBUG_ASSERT_DISABLED

#endif

/* COMPILE_ASSERT */
#ifndef COMPILE_ASSERT
#if COMPILER_SUPPORTS(C_STATIC_ASSERT)
/* Unlike static_assert below, this also works in plain C code. */
#define COMPILE_ASSERT(exp, name) _Static_assert((exp), #name)
#else
#define COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif
#endif

/* FATAL */

#if FATAL_DISABLED
#define FATAL(...) ((void)0)
#else
#define FATAL(...) do { \
    WTFReportFatalError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, __VA_ARGS__); \
    CRASH(); \
} while (0)
#endif

/* LOG_ERROR */

#if ERROR_DISABLED
#define LOG_ERROR(...) ((void)0)
#else
#define LOG_ERROR(...) WTFReportError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, __VA_ARGS__)
#endif

/* LOG */

#if LOG_DISABLED
#define LOG(channel, ...) ((void)0)
#else
#define LOG(channel, ...) WTFLog(&JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, channel), __VA_ARGS__)
#define JOIN_LOG_CHANNEL_WITH_PREFIX(prefix, channel) JOIN_LOG_CHANNEL_WITH_PREFIX_LEVEL_2(prefix, channel)
#define JOIN_LOG_CHANNEL_WITH_PREFIX_LEVEL_2(prefix, channel) prefix ## channel
#endif

/* LOG_VERBOSE */

#if LOG_DISABLED
#define LOG_VERBOSE(channel, ...) ((void)0)
#else
#define LOG_VERBOSE(channel, ...) WTFLogVerbose(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, &JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, channel), __VA_ARGS__)
#endif

/* RELEASE_ASSERT */

#if ASSERT_DISABLED
#if PLATFORM(WKC)
# define RELEASE_ASSERT(assertion) do \
        if (UNLIKELY(!(assertion))) { \
            CRASH();                  \
        }                             \
    while (0)
#else
# define RELEASE_ASSERT(assertion) (UNLIKELY(!(assertion)) ? (CRASH()) : (void)0)
#endif
#define RELEASE_ASSERT_WITH_MESSAGE(assertion, ...) RELEASE_ASSERT(assertion)
#define RELEASE_ASSERT_NOT_REACHED() CRASH()
#else
#define RELEASE_ASSERT(assertion) ASSERT(assertion)
#define RELEASE_ASSERT_WITH_MESSAGE(assertion, ...) ASSERT_WITH_MESSAGE(assertion, __VA_ARGS__)
#define RELEASE_ASSERT_NOT_REACHED() ASSERT_NOT_REACHED()
#endif

/* UNREACHABLE_FOR_PLATFORM */

#if COMPILER(CLANG)
// This would be a macro except that its use of #pragma works best around
// a function. Hence it uses macro naming convention.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
static inline void UNREACHABLE_FOR_PLATFORM()
{
    RELEASE_ASSERT_NOT_REACHED();
}
#pragma clang diagnostic pop
#else
#define UNREACHABLE_FOR_PLATFORM() RELEASE_ASSERT_NOT_REACHED()
#endif


#endif /* WTF_Assertions_h */
