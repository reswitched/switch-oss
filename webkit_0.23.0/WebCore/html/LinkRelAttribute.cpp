/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "LinkRelAttribute.h"

#include <wtf/text/WTFString.h>

namespace WebCore {

LinkRelAttribute::LinkRelAttribute()
{
}

LinkRelAttribute::LinkRelAttribute(const String& rel)
{
    if (equalIgnoringCase(rel, "stylesheet"))
        isStyleSheet = true;
    else if (equalIgnoringCase(rel, "icon") || equalIgnoringCase(rel, "shortcut icon"))
        iconType = Favicon;
#if ENABLE(TOUCH_ICON_LOADING)
    else if (equalIgnoringCase(rel, "apple-touch-icon"))
        iconType = TouchIcon;
    else if (equalIgnoringCase(rel, "apple-touch-icon-precomposed"))
        iconType = TouchPrecomposedIcon;
#endif
    else if (equalIgnoringCase(rel, "dns-prefetch"))
        isDNSPrefetch = true;
    else if (equalIgnoringCase(rel, "alternate stylesheet") || equalIgnoringCase(rel, "stylesheet alternate")) {
        isStyleSheet = true;
        isAlternate = true;
    } else {
        // Tokenize the rel attribute and set bits based on specific keywords that we find.
        String relCopy = rel;
        relCopy.replace('\n', ' ');
        Vector<String> list;
        relCopy.split(' ', list);
        for (auto& word : list) {
            if (equalIgnoringCase(word, "stylesheet"))
                isStyleSheet = true;
            else if (equalIgnoringCase(word, "alternate"))
                isAlternate = true;
            else if (equalIgnoringCase(word, "icon"))
                iconType = Favicon;
#if ENABLE(TOUCH_ICON_LOADING)
            else if (equalIgnoringCase(word, "apple-touch-icon"))
                iconType = TouchIcon;
            else if (equalIgnoringCase(word, "apple-touch-icon-precomposed"))
                iconType = TouchPrecomposedIcon;
#endif
#if ENABLE(LINK_PREFETCH)
            else if (equalIgnoringCase(word, "prefetch"))
                isLinkPrefetch = true;
            else if (equalIgnoringCase(word, "subresource"))
                isLinkSubresource = true;
#endif
        }
    }
}

}
