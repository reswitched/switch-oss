/*
 *  wkcfileprocs.h
 *
 *  Copyright(c) 2014-2016 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCFILEPROCS_H_
#define _WKCFILEPROCS_H_

#include <wkc/wkcbase.h>

/**
   @file
   @brief WKC file callbacks.
 */
/*@{*/

WKC_BEGIN_C_LINKAGE

// file system callbacks

/**
@brief File open callback function, set by wkcFileCallbackSetPeer()
@param "in_usage" File usage
@param "in_filename" File name
@param "in_mode" File mode. Complies with standard C fopen function.
@retval "!= 0" File descriptor
@retval "== 0" Failed
@details
Opens a file specified by in_filename.@n
The following values are specified for in_usage.
@li WKC_FILEOPEN_USAGE_WEBCORE Use inside WebCore
@li WKC_FILEOPEN_USAGE_FILE_SCHEME Read file with file scheme
@li WKC_FILEOPEN_USAGE_FORMDATA Upload form file
@li WKC_FILEOPEN_USAGE_DATABASE Database
@li WKC_FILEOPEN_USAGE_JAVASCRIPT Javascript
@li WKC_FILEOPEN_USAGE_OPENSSL OpenSSL
@li WKC_FILEOPEN_USAGE_PLUGIN Plug-in
@li WKC_FILEOPEN_USAGE_MEDIA Media player

A file descriptor can be defined on the callback implementation side, however, it must be cast to void* and returned.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fopen() function.
*/
typedef void*  (*wkcFileFOpenProc)(int in_usage, const char* in_filename, const char* in_mode);
/**
@brief File close callback function, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "0" Succeeded
@retval "-1" Failed
@details
Closes a file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fclose() function.
*/
typedef int    (*wkcFileFCloseProc)(void* in_fd);
/**
@brief File read callback function, set by wkcFileCallbackSetPeer()
@param "out_buffer" Buffer that stores read data
@param "in_size" Size of data. Unit is bytes.
@param "in_count" Number of data
@param "in_fd" File descriptor
@return Number of read data
@details
Reads in_count number of data items from the file specified by in_fd, and copies them to the area out_buffer. Each individual data item has a length of in_size.@n
The return value is the number of items that were read successfully.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fread() function.
*/
typedef wkc_uint64 (*wkcFileFReadProc)(void *out_buffer, wkc_uint64 in_size, wkc_uint64 in_count, void *in_fd);
/**
@brief File write callback function, set by wkcFileCallbackSetPeer()
@param "in_buffer" Buffer for data to write
@param "in_size" Size of data. Unit is bytes.
@param "in_count" Number of data
@param "in_fd" File descriptor
@return Number of data written
@details
Writes in_count number of data items in the area in_buffer to the file specified by in_fd.@n
Each individual data item has a length of in_size. The return value is the number of items that were written successfully.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fwrite() function.
*/
typedef wkc_uint64 (*wkcFileFWriteProc)(const void *in_buffer, wkc_uint64 in_size, wkc_uint64 in_count, void *in_fd);
/**
@brief Callback function for checking end of file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "== 0" File position indicator reached end of file
@retval "!= 0" File position indicator does not reach end of file
@details
Checks EOF of the file specified by in_fd, and returns a number other than 0 when EOF is reached.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library feof() function.
*/
typedef int    (*wkcFileFeofProc)(void *in_fd);
/**
@brief Callback function for getting file information, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@param "buf" Buffer that stores file information
@retval "0" Succeeded
@retval "-1" Failed
@details
Gets information of the file specified by in_fd, and stores in buf.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the POSIX fstat() function.
*/
typedef int    (*wkcFileFStatProc)(void *in_fd, struct stat *buf);
/**
@brief Callback function for seeking file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@param "in_offset" Offset from position set by in_whence. Unit is bytes.
@param "in_whence" Standard offset position
- SEEK_SET Beginning of file
- SEEK_CUR Value indicated by file position indicator
- SEEK_END End of file
@retval "0" Succeeded
@retval "-1" Failed
@details
Changes the value of a file position indicator of the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fseek() function.
*/
typedef int    (*wkcFileFSeekProc)(void *in_fd, wkc_int64 in_offset, int in_whence);
/**
@brief Callback function for getting file position, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "!= -1" Number of bytes from beginning of file of file position indicator
@retval "== -1" Failed
@details
Gets the value of a file position indicator of the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library ftell() function.
*/
typedef wkc_int64 (*wkcFileFTellProc)(void *in_fd);
/**
@brief Callback function for setting file size, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@param "length" File size. Unit is bytes.
@retval "0" Succeeded
@retval "-1" Failed
@details
Expands file size of the file specified by in_fd, or truncates it.@n
The expanded part is filled with "\0", and the truncated part becomes lost.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the POSIX ftruncate() function.
*/
typedef int    (*wkcFileFTruncateProc)(void *in_fd, wkc_uint64 length);
/**
@brief Callback function for locking file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@param "in_type" Type of lock
- WKC_FILELOCK_SHARED Shared lock
- WKC_FILELOCK_EXCLUSIVE Excluded lock
@param "in_offset" Number of bytes from beginning of area to lock
@param "in_length" Length of area to lock. Size is bytes.
@retval "!= 0" Succeeded
@retval "== 0" Failed
@details
Locks the area of a file of the file specified by in_fd.@n
Getting the error value is not required, even if an error occurs.
*/
typedef int    (*wkcFileLockProc)(void *in_fd, int in_type, wkc_uint64 in_offset, wkc_uint64 in_length);
/**
@brief Callback function for unlocking file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@param "in_type" Type of lock to unlock
- WKC_FILELOCK_SHARED Shared lock
- WKC_FILELOCK_EXCLUSIVE Excluded lock
@param "in_offset" Number of bytes from beginning of area to unlock
@param "in_length" Length of area to unlock. Size is bytes.
@retval "!= 0" Succeeded
@retval "== 0" Failed
@details
Unlocks the area of a file of the file specified by in_fd.@n
The type of locks to be unlocked is only the same as those locked by wkcFileLockProc.@n
Unlocking the area without locking can be assumed to be successful.@n
Getting the error value is not required, even if an error occurs.
*/
typedef int    (*wkcFileUnlockProc)(void *in_fd, int in_type, wkc_uint64 in_offset, wkc_uint64 in_length);
/**
@brief Callback function for syncing file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "0" Succeeded
@retval "-1" Failed
@details
Syncs content of the file specified by in_fd with that on a storage device.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the POSIX fsync() function.
*/
typedef int    (*wkcFileFSyncProc)(int in_fd);
/**
@brief Callback function for reading one character from file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "!= -1" Character, read from file
@retval "== -1" Failed, or file position indicator already reached end of file
@details
Reads one character from the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fgetc() function.
*/
typedef int    (*wkcFileFGetcProc)(void* in_fd);
/**
@brief Callback function for writing one character from file, set by wkcFileCallbackSetPeer()
@param "in_c" Character to write
@param "in_fd" File descriptor
@retval "0" Succeeded
@retval "-1" Failed
@details
Reads one character from the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fputc() function.
*/
typedef int    (*wkcFileFPutcProc)(int in_c, void* in_fd);
/**
@brief Callback function for reading one line from file, set by wkcFileCallbackSetPeer()
@param "out_s" Buffer that stores read data
@param "in_size" Buffer size. Unit is bytes.
@param "in_fd" File descriptor
@retval "== NULL" Failed, or file position indicator already reached end of file
@retval "!= NULL" out_s, buffer for output
@details
Reads a maximum of in_size - 1 characters up to a line feeder or end of the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fgets() function.
*/
typedef char*  (*wkcFileFGetsProc)(char *out_s, int in_size, void* in_fd);
/**
@brief Callback function for writing string to file, set by wkcFileCallbackSetPeer()
@param "in_s" C string to write
@param "in_fd" File descriptor
@retval ">= 0" Succeeded
@retval "== -1" Failed
@details
Outputs a string to the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fputs() function.
*/
typedef int    (*wkcFileFPutsProc)(const char *in_s, void* in_fd);
/**
@brief Callback function for flushing file, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "0" Succeeded
@retval "-1" Failed
@details
Writes data that is not yet applied to the file specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library fflush() function.
*/
typedef int    (*wkcFileFFlushProc)(void* in_fd);
/**
@brief Callback function for getting file error, set by wkcFileCallbackSetPeer()
@param "in_fd" File descriptor
@retval "== 0" No error
@retval "!= 0" Error
@details
Gets the result of whether there is an error within file access specified by in_fd.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library ferror() function.
*/
typedef int    (*wkcFileFErrorProc)(void* in_fd);
/**
@brief Callback function for getting file access permission, set by wkcFileCallbackSetPeer()
@param "in_filename" File name
@param "in_mode" Access permission to check
@retval "== 0" Succeeded
@retval "== -1" Error occurrence
@details
Checks whether in_mode access permission exists in the file specified by in_filename.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the POSIX access() function.
*/
typedef int    (*wkcFileAccessProc)(const char* in_filename, int in_mode);
/**
@brief Callback function for deleting file, set by wkcFileCallbackSetPeer()
@param "in_path" Path of file to delete
@retval "== 0" Succeeded
@retval "== -1" Failed
@details
Deletes the file specified by in_path.
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the POSIX unlink() function.
*/
typedef int    (*wkcFileUnlinkProc)(const char* in_path);
/**
@brief Callback function for getting file information, set by wkcFileCallbackSetPeer()
@param "in_filename" File name
@param "buf" Buffer that stores file information
@retval "0" Succeeded
@retval "-1" Failed
@details
Gets information of the file specified by in_filename, and stores in buf.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the POSIX stat() function.
*/
typedef int    (*wkcFileStatProc)(const char* in_filename, struct stat* out_buf);
/**
@brief Callback function for getting error information, set by wkcFileCallbackSetPeer()
@return Number of error that recently occurred 
@details
Returns the code of error that last occurred.@n
Error codes obtained by standard C library or POSIX errno must be returned.
*/
typedef int    (*wkcFileErrnoProc)(void);
/**
@brief Callback function for creating directory, set by wkcFileCallbackSetPeer()
@param "in_path" Path of directory to create
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Creates the directory specified by in_path.@n
If no upper directory exists, creates it.@n
If there is already a specified directory, it is regarded as success.@n
Getting the error value is not required, even if an error occurs.
*/
typedef bool   (*wkcFileMakeAllDirectoriesProc)(const char* in_path);
/**
@brief Callback function for getting file name, set by wkcFileCallbackSetPeer()
@param "in_path" File path
@param "out_name" Buffer that stores file name obtained from in_path
@param "in_maxnamelen" Buffer size of out_name. Unit is bytes.
@return Length of file name "\0" is not included. Unit is bytes.
@details
Gets the file name from the file path specified by in_path, and stores it in out_name.@n
in_maxnamelen represents the size including "\0".@n
If NULL is specified for out_name, it returns only the length of file name.@n
Getting the error value is not required, even if an error occurs.
*/
typedef int    (*wkcFilePathGetFileNameProc)(const char* in_path, char *out_name, int in_maxnamelen);
/**
@brief Callback function for combining file path, set by wkcFileCallbackSetPeer()
@param "in_path" File path
@param "in_component" Component name to combine
@param "out_path" Buffer that stores combined file path
@param "in_maxpathlen" Buffer size of out_path. Unit is bytes.
@return Length of combined path. "\0" is not included. Unit is bytes.
@details
Stores file path that combined in_component of the file path specified by in_path.@n
in_maxpathlen represents the size including "\0".@n
If NULL is specified for out_path, it returns only the length of combined path.@n
Getting the error value is not required, even if an error occurs.
*/
typedef int    (*wkcFilePathByAppendingComponentProc)(const char *in_path, const char *in_component, char *out_path, int in_maxpathlen);
/**
@brief Callback function for getting home directory, set by wkcFileCallbackSetPeer()
@param "out_path" Buffer that stores home directory path
@param "in_maxpathlen" Buffer size of out_path. Unit is bytes.
@return Length of home directory path. "\0" is not included. Unit is bytes.
@details
Gets a home directory path and stores it in out_path.@n
in_maxpathlen represents the size including "\0".@n
If NULL is specified for out_path, it returns only the length of home directory path.@n
Getting the error value is not required, even if an error occurs.
*/
typedef int    (*wkcFileHomeDirectoryPathProc)(char *out_path, int in_maxpathlen);
/**
@brief Callback function for getting a list of directory entries, set by wkcFileCallbackSetPeer()
@param "in_path" Directory path
@param "in_filter" Filter of entry to get
@param "out_entries" Buffer that stores list of entry
@param "in_maxentrystrlen" Maximum string length per entry
@param "in_maxentrylen" Number of out_entries
@return Number of entry matches
@details
Gets a list of entries that match in_filter from the directory specified by in_path, and stores it in out_entries.@n
'*' must be handled as a string in optional length, since in_filter is specified as wildcard.@n
out_entries can be handled as a C string array, and the maximum length of all strings are represented as in_maxentrystrlen.@n
in_maxentrystrlen represents the size including "\0".@n
If NULL is specified for out_entries, only the number of matched entries must be returned.@n
Getting the error value is not required, even if an error occurs.
*/
typedef int    (*wkcFileListDirectoryProc)(const char* in_path, const char* in_filter, char** out_entries, int in_maxentrystrlen, int in_maxentrylen);
/**
@brief Callback function for getting a directory name from path, set by wkcFileCallbackSetPeer()
@param "in_path" Directory path
@param "out_name" Buffer that stores directory name
@param "in_maxnamelen" Maximum buffer size of out_name
@return Length of directory name
@details
Gets the name of directory from the path.
*/
typedef int    (*wkcFileDirectoryNameProc)(const char* in_path, char* out_name, int in_maxnamelen);
/**
@brief Callback function for creating temporary file, set by wkcFileCallbackSetPeer()
@param "in_prefix" Temporary file name prefix
@param "out_name" Name of path to temporary file
@param "in_maxnamelen" Maximum length of out_name
@return Descriptor of created file
@details
Creates temporary file. This function is used by stream communication or database while using plug-in.@n
If an error occurs, the error value must be able to be obtained using WKCFileProcs::fErrnoProc.@n
The behavior must be the same as the standard C library tmpfile() function, except that a prefix can be set and that a generated filename can be stored in out_name.
*/
typedef void*  (*wkcFileOpenTemporaryFileProc)(const char* in_prefix, char* out_name, int in_maxnamelen);
/**
@brief Callback function for open directory entries, set by wkcFileCallbackSetPeer()
@param "in_usage" Usage
@param "in_path" Path
@param "in_filter" Extension filters
@return Descriptor of directory entries
@details
Open directory entries at specified by in_path.
*/
typedef void*  (*wkcFileOpenDirectoryProc)(int in_usage, const char* in_path, const char *in_filter);
/**
@brief Callback function for close directory entries, set by wkcFileCallbackSetPeer()
@param "in_dir" Descriptor of directory entries
@details
Close directory entries.
*/
typedef void   (*wkcFileCloseDirectoryProc)(void* in_dir);
/**
@brief Callback function for reading directory entries, set by wkcFileCallbackSetPeer()
@param "in_dir" Descriptor of directory entries
@param "out_entryname" Entri name
@param "in_maxentrynamelen" max length of out_entryname
@return Length of entry name
@details
Stores next entry name to out_entryname.
*/
typedef int    (*wkcFileReadDirectoryProc)(void* in_dir, char* out_entryname, int in_maxentrynamelen);

/** @brief Structure for file access callback functions */
struct WKCFileProcs_ {
    /** @brief Callback function for opening files */
    wkcFileFOpenProc fFOpenProc;
    /** @brief Callback function for closing files */
    wkcFileFCloseProc fFCloseProc;
    /** @brief Callback function for reading files */
    wkcFileFReadProc  fFReadProc;
    /** @brief Callback function for writing files */
    wkcFileFWriteProc fFWriteProc;
    /** @brief Callback function for checking end of file */
    wkcFileFeofProc fFeofProc;
    /** @brief Callback function for getting file information */
    wkcFileFStatProc fFStatProc;
    /** @brief Callback function for seeking file */
    wkcFileFSeekProc fFSeekProc;
    /** @brief Callback function for getting file location */
    wkcFileFTellProc fFTellProc;
    /** @brief Callback function for setting file size */
    wkcFileFTruncateProc fFTruncateProc;
    /** @brief Callback function for locking file */
    wkcFileLockProc fLockProc;
    /** @brief Callback function for unlocking file */
    wkcFileUnlockProc fUnlockProc;
    /** @brief Callback function for syncing file */
    wkcFileFSyncProc fFSyncProc;
    /** @brief Callback function for reading one character from file */
    wkcFileFGetcProc fFGetcProc;
    /** @brief Callback function for writing one character in file */
    wkcFileFPutcProc fFPutcProc;
    /** @brief Callback function for writing one line from file */
    wkcFileFGetsProc fFGetsProc;
    /** @brief Callback function for writing one line in file */
    wkcFileFPutsProc fFPutsProc;
    /** @brief Callback function for flushing file */
    wkcFileFFlushProc fFFlushProc;
    /** @brief Callback function for getting file error of file descriptor */
    wkcFileFErrorProc fFErrorProc;
    /** @brief Callback function for getting file access permission */
    wkcFileAccessProc fAccessProc;
    /** @brief Callback function for deleting file */
    wkcFileUnlinkProc fUnlinkProc;
    /** @brief Callback function for getting file information specified by file name */
    wkcFileStatProc fStatProc;
    /** @brief Callback function for getting error value */
    wkcFileErrnoProc fErrnoProc;
    /** @brief Callback function for creating directory */
    wkcFileMakeAllDirectoriesProc fMakeAllDirectoriesProc;
    /** @brief Callback function for getting filename part from path */
    wkcFilePathGetFileNameProc fPathGetFileNameProc;
    /** @brief Callback function for appending path to specified component */
    wkcFilePathByAppendingComponentProc fPathByAppendingComponentProc;
    /** @brief Callback function for getting home directory path */
    wkcFileHomeDirectoryPathProc fHomeDirectoryPathProc;
    /** @brief Callback function for getting list of entry of specified directory */
    wkcFileListDirectoryProc fListDirectoryProc;
    /** @brief Callback function for getting directory name */
    wkcFileDirectoryNameProc fDirectoryNameProc;
    /** @brief Callback function for creating temporary file */
    wkcFileOpenTemporaryFileProc fOpenTemporaryFileProc;
    /** @brief Callback function for opening directory */
    wkcFileOpenDirectoryProc fOpenDirectoryProc;
    /** @brief Callback function for closing directory */
    wkcFileCloseDirectoryProc fCloseDirectoryProc;
    /** @brief Callback function for reading directory */
    wkcFileReadDirectoryProc fReadDirectoryProc;
};
/** @brief Type definition of WKCFileProcs */
typedef struct WKCFileProcs_ WKCFileProcs;

WKC_END_C_LINKAGE
/*@}*/

#endif // _WKCFILEPROCS_H_
