/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "JSElement.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "HTMLFrameElementBase.h"
#include "HTMLNames.h"
#include "JSAttr.h"
#include "JSDOMBinding.h"
#include "JSHTMLElementWrapperFactory.h"
#include "JSNodeList.h"
#include "JSNodeOrString.h"
#include "JSSVGElementWrapperFactory.h"
#include "NodeList.h"
#include "SVGElement.h"

using namespace JSC;

namespace WebCore {

using namespace HTMLNames;

JSValue toJSNewlyCreated(ExecState*, JSDOMGlobalObject* globalObject, Element* element)
{
    if (!element)
        return jsNull();

    ASSERT(!getCachedWrapper(globalObject->world(), element));

    JSDOMWrapper* wrapper;        
    if (is<HTMLElement>(*element))
        wrapper = createJSHTMLWrapper(globalObject, downcast<HTMLElement>(element));
    else if (is<SVGElement>(*element))
        wrapper = createJSSVGWrapper(globalObject, downcast<SVGElement>(element));
    else
        wrapper = CREATE_DOM_WRAPPER(globalObject, Element, element);

    return wrapper;    
}

JSValue JSElement::before(ExecState* state)
{
    ExceptionCode ec = 0;
    impl().before(toNodeOrStringVector(*state), ec);
    setDOMException(state, ec);

    return jsUndefined();
}

JSValue JSElement::after(ExecState* state)
{
    ExceptionCode ec = 0;
    impl().after(toNodeOrStringVector(*state), ec);
    setDOMException(state, ec);

    return jsUndefined();
}

JSValue JSElement::replaceWith(ExecState* state)
{
    ExceptionCode ec = 0;
    impl().replaceWith(toNodeOrStringVector(*state), ec);
    setDOMException(state, ec);

    return jsUndefined();
}

JSValue JSElement::prepend(ExecState* state)
{
    ExceptionCode ec = 0;
    impl().prepend(toNodeOrStringVector(*state), ec);
    setDOMException(state, ec);

    return jsUndefined();
}

JSValue JSElement::append(ExecState* state)
{
    ExceptionCode ec = 0;
    impl().append(toNodeOrStringVector(*state), ec);
    setDOMException(state, ec);

    return jsUndefined();
}

} // namespace WebCore
