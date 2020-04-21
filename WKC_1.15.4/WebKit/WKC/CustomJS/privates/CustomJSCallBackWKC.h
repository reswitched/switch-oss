/*
 * CustomJSCallBackWKC.h
 *
 * Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef CustomJSCallBackWKC_h
#define CustomJSCallBackWKC_h

#ifdef WKC_ENABLE_CUSTOMJS

#include <JSContextRef.h>
#include <JSObjectRef.h>
#include <JSValueRef.h>

namespace WKC {

JSValueRef CustomJSCallBackWKC(JSContextRef ctx, JSObjectRef jobj, JSObjectRef jobjThis, size_t argLen, const JSValueRef args[], JSValueRef* jobjExp);
JSValueRef CustomJSStringCallBackWKC(JSContextRef ctx, JSObjectRef jobj, JSObjectRef jobjThis, size_t argLen, const JSValueRef args[], JSValueRef* jobjExp);

} // namespace
#endif // WKC_ENABLE_CUSTOMJS
#endif // CustomJSCallBackWKC_h
