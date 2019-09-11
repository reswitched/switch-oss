/*
 autotoolsconfig.h

 Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AUTOTOOLSCONFIG_H_
#define _AUTOTOOLSCONFIG_H_

// remove pre-defined values

#ifdef _MSC_VER
// workaround for windef.h
// see also WebCore/config.h
# ifndef max
#  define max max
# endif
# ifndef min
#  define min min
# endif
# ifndef INFINITY
#  include <limits>
#  ifndef INFINITY
#   define INFINITY std::numeric_limits<float>::infinity()
#  endif
# endif
# define __BUILDING_IN_VS__
#endif // _MSC_VER

#define MAX_PATH            260

#include <wkc/wkcconfig.h>
#include "wkcglobalwrapper.h"
#include "wkcclibwrapper.h"

#endif // _AUTOTOOLSCONFIG_H_
