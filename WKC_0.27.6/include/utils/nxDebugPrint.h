/*
 * nxLogPrint.h
 *
 * Copyright(c) 2014, 2015 ACCESS CO., LTD. All rights reserved.
 */

#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

#ifndef _NX_LOG_PRINT_H_
#define _NX_LOG_PRINT_H_

enum nxLogLevel {
    nxLogNo = 0,
    nxLogError,
    nxLogWarn,
    nxLogInfo,
    nxLogDebug,
    nxLogFunc,
    nxLogFunc2,
    nxLogMax
};

#define FUNC_INOUT   0x10000000  /**< display function input/output log in the same time for nxLogFunc/nxLogFunc2 */
#define FUNC_IN      0x20000000  /**< display function input log for nxLogFunc/nxLogFunc2 */
#define FUNC_OUT     0x40000000  /**< display function output log for nxLogFunc/nxLogFunc2 */
#define FUNC_ERROUT  0x80000000  /**< error output flag */

#ifndef DP_STATIC
#ifdef _MSC_VER
  #ifdef _USRDLL
    #define DP_EXPORT   __declspec(dllexport)
  #else
    #define DP_EXPORT   __declspec(dllimport)
  #endif
#else
  #define DP_EXPORT
#endif
#else
  #define DP_EXPORT
#endif

#define NX_LOG_SIZE  8192

#ifndef EXTERN_C
  #ifdef __cplusplus
    #define EXTERN_C  extern "C"
  #else
    #define EXTERN_C  extern
  #endif
#endif

#ifdef __cplusplus

class DP_EXPORT nxLogPrint
{
public:
    nxLogPrint();
    ~nxLogPrint();

    bool setOutFile(const char* outFile);
    void unsetOutFile(void);
    void setLogLevel(int logLevel);
    void nxPrintf(int level, const char* in_file, const char* in_func, int in_line, const char* in_format, ...);
    void nxConsoleMessage(const char* in_message);
    void nxPutsMessage(const char* in_message);
    void nxPutsMessage(const char* in_format, va_list in_args);

private:
    void nxTimeString(char* outBuff, int buffLen);
    void nxOutputString(const char* buff);

private:
    int m_LogLevel;

    void* m_LogFp;

#if defined(_MSC_VER)
    HANDLE m_LogMutex;
#elif defined(__ARMCC_VERSION) || defined(__clang__)
    void* m_LogMutex;
#else
    pthread_mutex_t m_LogMutex;
#endif

    char m_DebugBuf[NX_LOG_SIZE];
};

extern DP_EXPORT nxLogPrint gNXLog;

/**
 * @def nxLogSetOutFile(file)
 * Set logging out file name. (Re)Open \a file.
 * If Could open \a file, return true;
 * If Could not open \a file, return false;
 */
#define nxLogSetOutFile(file)    gNXLog.setOutFile((file))

/**
 * @def nxLogUnsetOutFile()
 * Set logging out file name and close file.
 */
#define nxLogUnsetOutFile()      gNXLog.unsetOutFile()

/**
 * @def nxLogChangeLogLevel(level)
 * Set logging level.
 * @see enum nxLogLevel
 */
#define nxLogChangeLogLevel(level)  gNXLog.setLogLevel((level))

/**
 * @def nxLogConsoleMessage(message)
 * Display console message
 */
#define nxLogConsoleMessage(message)  gNXLog.nxConsoleMessage((message))

/**
 * @def nxLogConsoleMessage(message)
 * Display console message
 */
#define nxLogPutsMessage(message)  gNXLog.nxPutsMessage((message))

/**
 * @def nxLogConsoleMessage(format,args)
 * Display console message
 */
#define nxLogPutsMessage2(format,args)  gNXLog.nxPutsMessage((format),(args))

/**
 * @def nxLog_boolstr(t)
 * convert boolean to string
 */
#define nxLog_boolstr(t)  (((t))?"true":"false")

/**
 * @def nxLog_e(...)
 * Output ERROR level debug print with file name, function name and line number
 */
#define nxLog_e(...)  gNXLog.nxPrintf(nxLogError, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

/**
 * @def nxLog_eout(...)
 * Output Function error exit log with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_eout(...)  gNXLog.nxPrintf(nxLogError,  __FILE__, __FUNCTION__, (__LINE__|FUNC_ERROUT), __VA_ARGS__)

/**
 * @def nxLog_eout2(...)
 * Output Function error exit log with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_eout2(...)  gNXLog.nxPrintf(nxLogError,  __FILE__, __FUNCTION__, (__LINE__|FUNC_ERROUT), __VA_ARGS__)

#ifndef NO_NXLOG

/**
 * @def nxLog_w(...)
 * Output WARNING level debug print with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_w(...)  gNXLog.nxPrintf(nxLogWarn,  __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

/**
 * @def nxLog_i(...)
 * Output INFO level debug print with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_i(...)  gNXLog.nxPrintf(nxLogInfo,  __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

/**
 * @def nxLog_d(...)
 * Output DEBUG level debug print with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_d(...)  gNXLog.nxPrintf(nxLogDebug, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

/**
 * @def nxLog_in(...)
 * Output Function enter log with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_in(...)  gNXLog.nxPrintf(nxLogFunc,  __FILE__, __FUNCTION__, (__LINE__|FUNC_IN),  __VA_ARGS__)

/**
 * @def nxLog_out(...)
 * Output Function exit log with file name, function name and line number
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_out(...)  gNXLog.nxPrintf(nxLogFunc,  __FILE__, __FUNCTION__, (__LINE__|FUNC_OUT), __VA_ARGS__)

/**
 * @def nxLog_inout(...)
 * Output Function log enter and exit with file name, function name and line number
 * if function does nothing, use this log.
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_inout(...)  gNXLog.nxPrintf(nxLogFunc,  __FILE__, __FUNCTION__, (__LINE__|FUNC_INOUT),  __VA_ARGS__)

/**
 * @def nxLog_noimp()
 * Input/Output Function log with "Not Implemented Yet".
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_noimp()  gNXLog.nxPrintf(nxLogFunc2,  __FILE__, __FUNCTION__, (__LINE__|FUNC_INOUT), "Not Implemented Yet.")

/**
 * @def nxLog_in2(...)
 * Output Function enter log with file name, function name and line number.
 * This define is used for a function which is called a lot like a function which related mouse move.
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_in2(...)  gNXLog.nxPrintf(nxLogFunc2,  __FILE__, __FUNCTION__, (__LINE__|FUNC_IN),  __VA_ARGS__)

/**
 * @def nxLog_out2(...)
 * Output Function exit log with file name, function name and line number.
 * This define is used for a function which is called a lot like a function which related mouse move.
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_out2(...)  gNXLog.nxPrintf(nxLogFunc2,  __FILE__, __FUNCTION__, (__LINE__|FUNC_OUT), __VA_ARGS__)

/**
 * @def nxLog_inout2(...)
 * INput/Output Function log enter and exit with file name, function name and line number.
 * This define is used for a function which is called a lot like a function which related mouse move.
 * if function does nothing, use this log.
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_inout2(...)  gNXLog.nxPrintf(nxLogFunc2,  __FILE__, __FUNCTION__, (__LINE__|FUNC_INOUT),  __VA_ARGS__)

/**
 * @def nxLog_noimp2()
 * Input/Output Function log with "Not Implemented Yet".
 * @note this define is not used if defined NO_NXLOG.
 */
#define nxLog_noimp2()  gNXLog.nxPrintf(nxLogFunc2,  __FILE__, __FUNCTION__, (__LINE__|FUNC_INOUT), "Not Implemented Yet.")

#else // NO_NXLOG

#define nxLog_w(...)       ((void)0)
#define nxLog_i(...)       ((void)0)
#define nxLog_d(...)       ((void)0)
#define nxLog_in(...)      ((void)0)
#define nxLog_out(...)     ((void)0)
#define nxLog_inout(...)   ((void)0)
#define nxLog_noimp()      ((void)0)
#define nxLog_in2(...)     ((void)0)
#define nxLog_out2(...)    ((void)0)
#define nxLog_inout2(...)  ((void)0)
#define nxLog_noimp2()     ((void)0)

#endif // NO_NXLOG

#endif /* __cplusplus */

#if defined(__GNUC__) && !defined(__clang__)
EXTERN_C DP_EXPORT void __cyg_profile_func_enter(void* func_address, void* call_site);
EXTERN_C DP_EXPORT void __cyg_profile_func_exit(void* func_address, void* call_site);
#endif /* __GNUC__ */

EXTERN_C DP_EXPORT void nxLogPrintf(int level, const char* in_file, const char* in_func, int in_line, const char* in_format, ...);

#endif /* _NX_LOG_PRINT_H_ */
