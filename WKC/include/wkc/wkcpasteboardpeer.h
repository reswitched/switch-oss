/*
 *  wkcpasteboardpeer.h
 *
 *  Copyright(c) 2009-2014 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_PASTEBOARDPEER_H_
#define _WKC_PASTEBOARDPEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>
#include <wkc/wkcpasteboardprocs.h>

/**
   @file
   @brief WKC pasteboard peers.
 */
/*@{*/

WKC_BEGIN_C_LINKAGE

/**
@brief Clears pasteboard
@return None
*/
WKC_PEER_API void wkcPasteboardClearPeer(void);
/**
@brief Write plain text to pasteboard
@param in_str Text
@param in_len Length of text
@return None
*/
WKC_PEER_API void wkcPasteboardWritePlainTextPeer(const char* in_str, int in_len);
/**
@brief Write html to pasteboard
@param in_html HTML
@param in_html_len Length of HTML
@param in_uri uri
@param in_uri_len Length of URI
@param in_text text
@param in_text_len Length of text
@param in_can_smart_copy_or_delete whether can smart copy or delete
@return None
*/
WKC_PEER_API void wkcPasteboardWriteHTMLPeer(const char* in_html, int in_html_len, const char* in_uri, int in_uri_len, const char* in_text, int in_text_len, bool in_can_smart_copy_or_delete);
/**
@brief Write URI to pasteboard
@param in_uri URI
@param in_uri_len Length of URI
@param in_label Label of URI
@param in_label_len Length of label
@return None
*/
WKC_PEER_API void wkcPasteboardWriteURIPeer(const char* in_uri, int in_uri_len, const char* in_label, int in_label_len);
/**
@brief Write image to pasteboard
@param in_type Type
@param in_bitmap Bitmap
@param in_rowbytes Row bytes
@param in_size Size of image
@return None
*/
WKC_PEER_API void wkcPasteboardWriteImagePeer(int in_type, void* in_bitmap, int in_rowbytes, const WKCSize* in_size);
/**
@brief Read plain text from pasteboard
@param out_buf buffer for storing text
@param in_buflen Max length of out_buf
@return Length of text in pasteboard
*/
WKC_PEER_API int wkcPasteboardReadPlainTextPeer(char* out_buf, int in_buflen);
/**
@brief Read HTML from pasteboard
@param out_buf buffer for storing HTML
@param in_buflen Max length of out_buf
@return Length of HTML in pasteboard
*/
WKC_PEER_API int wkcPasteboardReadHTMLPeer(char* out_buf, int in_buflen);
/**
@brief Read URI from pasteboard
@param out_buf buffer for storing URI
@param in_buflen Max length of out_buf
@return Length of URI in pasteboard
*/
WKC_PEER_API int wkcPasteboardReadHTMLURIPeer(char* out_buf, int in_buflen);

/**
@brief Checks whether the format is available in platform pasteboard
@param in_format format (WKC_PASTEBOARD_XXX)
@return supported or not
*/
WKC_PEER_API bool wkcPasteboardIsFormatAvailablePeer(int in_format);

enum {
    WKC_PASTEBOARD_FORMAT_HTML,
    WKC_PASTEBOARD_FORMAT_TEXT,
    WKC_PASTEBOARD_FORMAT_SMART_PASTE,
    WKC_PASTEBOARD_FORMATS
};

/** @brief Sets callback functions of pasteboard */
WKC_PEER_API void wkcPasteboardCallbackSetPeer(const WKCPasteboardProcs* in_procs);

WKC_END_C_LINKAGE
/*@}*/

#endif // _WKC_PASTEBOARDPEER_H_
