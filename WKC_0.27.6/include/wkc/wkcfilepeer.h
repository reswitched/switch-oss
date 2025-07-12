/*
 *  wkcfilepeer.h
 *
 *  Copyright(c) 2014-2016 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_FILEPEER_H_
#define _WKC_FILEPEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>
#include <wkc/wkcfileprocs.h>

/**
   @file
   @brief WKC file peers.
 */
/*@{*/

WKC_BEGIN_C_LINKAGE

// file peer
/**
@brief Initializes file system
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the initialization process of file system. Implementing it is required only if needed by platform.
*/
WKC_PEER_API bool wkcFileInitializePeer(void);
/**
@brief Finalizes file system
@details
Performs the finalization process of the file system if needed. Implementing it is required only if needed by platform.
*/
WKC_PEER_API void wkcFileFinalizePeer(void);
/**
@brief Forcibly terminates file system
@details
Forcibly terminates process of the file system if needed. Implementing it is required only if needed by platform.
*/
WKC_PEER_API void wkcFileForceTerminatePeer(void);

enum {
    WKC_FILEOPEN_USAGE_WEBCORE = 0,
    WKC_FILEOPEN_USAGE_FILE_SCHEME,
    WKC_FILEOPEN_USAGE_FORMDATA,
    WKC_FILEOPEN_USAGE_DATABASE,
    WKC_FILEOPEN_USAGE_JAVASCRIPT,
    WKC_FILEOPEN_USAGE_OPENSSL,
    WKC_FILEOPEN_USAGE_PLUGIN,
    WKC_FILEOPEN_USAGE_MEDIA,
    WKC_FILEOPEN_USAGE_CACHE,
    WKC_FILEOPEN_USAGE_APP = 0x10000
};

enum {
    WKC_FILELOCK_NONE = 0, // file is unlocked
    WKC_FILELOCK_SHARED,
    WKC_FILELOCK_EXCLUSIVE
};

struct stat;

/**
@brief Sets callback related to files
@param "in_procs" Structure of callback related to files
@details
Sets the callback related to files. This function is called from WKCWebKitSetFileSystemProcs().
*/
WKC_PEER_API void       wkcFileCallbackSetPeer(const WKCFileProcs* in_procs);
/**
@brief Gets callback related to files
@details
Gets the callback related to files.
*/
WKC_PEER_API const WKCFileProcs* wkcFileCallbackGetPeer();
/**
@brief Opens file
@param "in_usage" Reference to wkcFileFOpenProc()
@param "in_filename" Reference to wkcFileFOpenProc()
@param "in_mode" Reference to wkcFileFOpenProc()
@retval Reference to wkcFileFOpenProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API void*      wkcFileFOpenPeer(int in_usage, const char* in_filename, const char* in_mode);
/**
@brief Closes file
@param "in_fd" Reference to wkcFileFCloseProc()
@retval Reference to wkcFileFCloseProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFClosePeer(void* in_fd);
/**
@brief Reads file
@param "out_buffer" Reference to wkcFileFReadProc()
@param "in_size" Reference to wkcFileFReadProc()
@param "in_count" Reference to wkcFileFReadProc()
@param "in_fd" Reference to wkcFileFReadProc()
@retval Reference to wkcFileFReadProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API wkc_uint64 wkcFileFReadPeer(void *out_buffer, wkc_uint64 in_size, wkc_uint64 in_count, void *in_fd);
/**
@brief Writes file
@param "in_buffer" Reference to wkcFileFWriteProc()
@param "in_size" Reference to wkcFileFWriteProc()
@param "in_count" Reference to wkcFileFWriteProc()
@param "in_fd" Reference to wkcFileFWriteProc()
@retval Reference to wkcFileFWriteProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API wkc_uint64 wkcFileFWritePeer(const void *in_buffer, wkc_uint64 in_size, wkc_uint64 in_count, void *in_fd);
/**
@brief Checks end of file
@param "in_fd" Reference to wkcFileFeofProc()
@retval Reference to wkcFileFeofProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFeofPeer(void *in_fd);
/**
@brief Gets file information
@param "in_fd" Reference to wkcFileFStatProc()
@param "buf" Reference to wkcFileFStatProc()
@retval Reference to wkcFileFStatProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFStatPeer(void *in_fd, struct stat *buf);
/**
@brief Seeks file
@param "in_fd" Reference to wkcFileFSeekProc()
@param "in_offset" Reference to wkcFileFSeekProc()
@param "in_whence" Reference to wkcFileFSeekProc()
@retval Reference to wkcFileFSeekProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFSeekPeer(void *in_fd, wkc_int64 in_offset, int in_whence);
/**
@brief Gets current file location
@param "in_fd" Reference to wkcFileFTellProc()
@retval Reference to wkcFileFTellProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API wkc_int64  wkcFileFTellPeer(void *in_fd);
/**
@brief Changes file size
@param "in_fd" Reference to wkcFileFTruncateProc()
@param "length" Reference to wkcFileFTruncateProc()
@retval Reference to wkcFileFTruncateProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFTruncatePeer(void *in_fd, wkc_uint64 length);
/**
@brief Locks file
@param "in_fd" Reference to wkcFileLockProc()
@param "in_type" Reference to wkcFileLockProc()
@param "in_offset" Reference to wkcFileLockProc()
@param "in_length" Reference to wkcFileLockProc()
@retval Reference to wkcFileLockProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileLockPeer(void *in_fd, int in_type, wkc_uint64 in_offset, wkc_uint64 in_length);
/**
@brief Unlocks file
@param "in_fd" Reference to wkcFileUnlockProc()
@param "in_type" Reference to wkcFileUnlockProc()
@param "in_offset" Reference to wkcFileUnlockProc()
@param "in_length" Reference to wkcFileUnlockProc()
@retval Reference to wkcFileUnlockProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileUnlockPeer(void *in_fd, int in_type, wkc_uint64 in_offset, wkc_uint64 in_length);
/**
@brief Syncs file
@param "in_fd" Reference to wkcFileFSyncProc()
@retval Reference to wkcFileFSyncProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFSyncPeer(int in_fd);
/**
@brief Gets one character from file
@param "in_fd" Reference to wkcFileFGetcProc()
@retval Reference to wkcFileFGetcProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFGetcPeer(void* in_fd);
/**
@brief Writes one character in file
@param "in_c" Reference to wkcFileFPutcProc()
@param "in_fd" Reference to wkcFileFPutcProc()
@retval Reference to wkcFileFPutcProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFPutcPeer(int in_c, void* in_fd);
/**
@brief Gets one line from file
@param "out_s" Reference to wkcFileFGetsProc()
@param "in_size" Reference to wkcFileFGetsProc()
@param "in_fd" Reference to wkcFileFGetsProc()
@retval Reference to wkcFileFGetsProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API char*      wkcFileFGetsPeer(char *out_s, int in_size, void* in_fd);
/**
@brief Outputs string to file
@param "in_s" Reference to wkcFileFPutsProc()
@param "in_fd" Reference to wkcFileFPutsProc()
@retval Reference to wkcFileFPutsProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFPutsPeer(const char *in_s, void* in_fd);
/**
@brief Flushes file
@param "in_fd" Reference to wkcFileFFlushProc()
@retval Reference to wkcFileFFlushProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFFlushPeer(void* in_fd);
/**
@brief Gets file error
@param "in_fd" Reference to wkcFileFErrorProc()
@retval Reference to wkcFileFErrorProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileFErrorPeer(void* in_fd);
/**
@brief Checks file access permission
@param "in_filename" Reference to wkcFileAccessProc()
@param "in_mode" Reference to wkcFileAccessProc()
@retval Reference to wkcFileAccessProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileAccessPeer(const char* in_filename, int in_mode);
/**
@brief Deletes file
@param "in_path" Reference to wkcFileUnlinkProc()
@retval Reference to wkcFileUnlinkProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileUnlinkPeer(const char* in_path);
/**
@brief Gets file information
@param "in_filename" Reference to wkcFileStatProc()
@param "out_buf" Reference to wkcFileStatProc()
@retval Reference to wkcFileStatProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileStatPeer(const char* in_filename, struct stat* out_buf);
/**
@brief Gets file error information
@retval Reference to wkcFileErrnoProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileErrnoPeer(void);
/**
@brief Creates directory
@param "in_path" Reference to wkcFileMakeAllDirectoriesProc()
@retval Reference to wkcFileMakeAllDirectoriesProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API bool       wkcFileMakeAllDirectoriesPeer(const char* in_path);
/**
@brief Gets file name from file path
@param "in_path" Reference to wkcFilePathGetFileNameProc()
@param "out_name" Reference to wkcFilePathGetFileNameProc()
@param "in_maxnamelen" Reference to wkcFilePathGetFileNameProc()
@retval Reference to wkcFilePathGetFileNameProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFilePathGetFileNamePeer(const char* in_path, char *out_name, int in_maxnamelen);
/**
@brief Combines component with file path
@param "in_path" Reference to wkcFilePathByAppendingComponentProc()
@param "in_component" Reference to wkcFilePathByAppendingComponentProc()
@param "out_path" Reference to wkcFilePathByAppendingComponentProc()
@param "in_maxpathlen" Reference to wkcFilePathByAppendingComponentProc()
@retval Reference to wkcFilePathByAppendingComponentProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFilePathByAppendingComponentPeer(const char *in_path, const char *in_component, char *out_path, int in_maxpathlen);
/**
@brief Gets home directory
@param "out_path" Reference to wkcFileHomeDirectoryPathProc()
@param "in_maxpathlen" Reference to wkcFileHomeDirectoryPathProc()
@retval Reference to wkcFileHomeDirectoryPathProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileHomeDirectoryPathPeer(char *out_path, int in_maxpathlen);
/**
@brief Gets a list of directory entries
@param "in_path" Reference to wkcFileListDirectoryProc()
@param "in_filter" Reference to wkcFileListDirectoryProc()
@param "out_entries" Reference to wkcFileListDirectoryProc()
@param "in_maxentrystrlen" Reference to wkcFileListDirectoryProc()
@param "in_maxentrylen" Reference to wkcFileListDirectoryProc()
@retval Reference to wkcFileListDirectoryProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileListDirectoryPeer(const char* in_path, const char* in_filter, char** out_entries, int in_maxentrystrlen, int in_maxentrylen);
/**
@brief Gets the directory name of the path
@param "in_path" Reference to wkcFileDirectoryNameProc()
@param "out_name" Reference to wkcFileDirectoryNameProc()
@param "in_maxnamelen" Reference to wkcFileDirectoryNameProc()
@retval Reference to wkcFileDirectoryNameProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API int        wkcFileDirectoryNamePeer(const char* in_path, char* out_name, int in_maxnamelen);
/**
@brief Creates temporary file
@param "in_prefix" Reference to wkcFileOpenTemporaryFileProc()
@param "out_name" Reference to wkcFileOpenTemporaryFileProc()
@param "in_maxnamelen" Reference to wkcFileOpenTemporaryFileProc()
@retval Reference to wkcFileOpenTemporaryFileProc()
@details
Calls the callback function, WKCFileProcs::fFOpenProc, set by wkcFileCallbackSetPeer().
*/
WKC_PEER_API void*      wkcFileOpenTemporaryFilePeer(const char* in_prefix, char* out_name, int in_maxnamelen);

WKC_END_C_LINKAGE
/*@}*/

#endif // _WKC_FILEPEER_H_
