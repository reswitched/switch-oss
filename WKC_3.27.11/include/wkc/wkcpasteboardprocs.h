/*
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
 *
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
