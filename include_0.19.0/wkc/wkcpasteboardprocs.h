/*
 *  wkcpasteboardprocs.h
 *
 *  Copyright(c) 2014 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCPASTEBOARDPROCS_H_
#define _WKCPASTEBOARDPROCS_H_

#include <wkc/wkcbase.h>

/**
   @file
   @brief WKC pasteboard procs.
 */
/*@{*/

WKC_BEGIN_C_LINKAGE

// callbacks
typedef void (*wkcPasteboardClearProc)(void);
typedef void (*wkcPasteboardWriteHTMLProc)(const char* in_html, int in_html_len, const char* in_uri, int in_uri_len, const char* in_text, int in_text_len, bool in_can_smart_copy_or_delete);
typedef void (*wkcPasteboardWritePlainTextProc)(const char* in_str, int in_len);
typedef void (*wkcPasteboardWriteURIProc)(const char* in_uri, int in_uri_len, const char* in_label, int in_label_len);
typedef int (*wkcPasteboardReadHTMLProc)(char* out_buf, int in_buflen);
typedef int (*wkcPasteboardReadHTMLURIProc)(char* out_buf, int in_buflen);
typedef int (*wkcPasteboardReadPlainTextProc)(char* out_buf, int in_buflen);
typedef void (*wkcPasteboardWriteImageProc)(int in_type, void* in_bitmap, int in_rowbytes, const WKCSize* in_size);
typedef bool (*wkcPasteboardIsFormatAvailableProc)(int in_format);

struct WKCPasteboardProcs_ {
    wkcPasteboardClearProc fClearProc;
    wkcPasteboardWriteHTMLProc fWriteHTMLProc;
    wkcPasteboardWritePlainTextProc fWritePlainTextProc;
    wkcPasteboardWriteURIProc fWriteURIProc;
    wkcPasteboardReadHTMLProc fReadHTMLProc;
    wkcPasteboardReadHTMLURIProc fReadHTMLURIProc;
    wkcPasteboardReadPlainTextProc fReadPlainTextProc;
    wkcPasteboardWriteImageProc fWriteImageProc;
    wkcPasteboardIsFormatAvailableProc fIsFormatAvailableProc;
};

/** @brief Type definition of WKCPasteboardProcs */
typedef struct WKCPasteboardProcs_ WKCPasteboardProcs;

WKC_END_C_LINKAGE
/*@}*/

#endif // _WKCPASTEBOARDPROCS__H
