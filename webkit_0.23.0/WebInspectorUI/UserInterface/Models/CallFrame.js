/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.CallFrame = class CallFrame extends WebInspector.Object
{
    constructor(id, sourceCodeLocation, functionName, thisObject, scopeChain, nativeCode, programCode)
    {
        super();

        console.assert(!sourceCodeLocation || sourceCodeLocation instanceof WebInspector.SourceCodeLocation);
        console.assert(!thisObject || thisObject instanceof WebInspector.RemoteObject);
        console.assert(!scopeChain || scopeChain instanceof Array);

        this._id = id || null;
        this._sourceCodeLocation = sourceCodeLocation || null;
        this._functionName = functionName || null;
        this._thisObject = thisObject || null;
        this._scopeChain = scopeChain || [];
        this._nativeCode = nativeCode || false;
        this._programCode = programCode || false;
    }

    // Public

    get id()
    {
        return this._id;
    }

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    }

    get functionName()
    {
        return this._functionName;
    }

    get nativeCode()
    {
        return this._nativeCode;
    }

    get programCode()
    {
        return this._programCode;
    }

    get thisObject()
    {
        return this._thisObject;
    }

    get scopeChain()
    {
        return this._scopeChain;
    }

    saveIdentityToCookie()
    {
        // Do nothing. The call frame is torn down when the inspector closes, and
        // we shouldn't restore call frame content views across debugger pauses.
    }

    collectScopeChainVariableNames(callback)
    {
        var result = {this: true, __proto__: null};

        var pendingRequests = this._scopeChain.length;

        function propertiesCollected(properties)
        {
            for (var i = 0; properties && i < properties.length; ++i)
                result[properties[i].name] = true;

            if (--pendingRequests)
                return;

            callback(result);
        }

        for (var i = 0; i < this._scopeChain.length; ++i)
            this._scopeChain[i].object.deprecatedGetAllProperties(propertiesCollected);
    }

    // Static

    static fromPayload(payload)
    {
        console.assert(payload);

        var url = payload.url;
        var nativeCode = false;
        var programCode = false;
        var sourceCodeLocation = null;

        if (!url || url === "[native code]") {
            nativeCode = true;
            url = null;
        } else {
            var sourceCode = WebInspector.frameResourceManager.resourceForURL(url);
            if (!sourceCode)
                sourceCode = WebInspector.debuggerManager.scriptsForURL(url)[0];

            if (sourceCode) {
                // The lineNumber is 1-based, but we expect 0-based.
                var lineNumber = payload.lineNumber - 1;
                sourceCodeLocation = sourceCode.createLazySourceCodeLocation(lineNumber, payload.columnNumber);
            }
        }

        var functionName = payload.functionName;
        if (payload.functionName === "global code"
            || payload.functionName === "eval code"
            || payload.functionName === "module code")
            programCode = true;

        return new WebInspector.CallFrame(null, sourceCodeLocation, functionName, null, null, nativeCode, programCode);
    }
};
