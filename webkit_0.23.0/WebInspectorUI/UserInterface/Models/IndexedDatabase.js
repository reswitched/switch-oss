/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

WebInspector.IndexedDatabase = class IndexedDatabase extends WebInspector.Object
{
    constructor(name, securityOrigin, version, objectStores)
    {
        super();

        this._name = name;
        this._securityOrigin = securityOrigin;
        this._host = parseSecurityOrigin(securityOrigin).host;
        this._version = version;
        this._objectStores = objectStores || [];

        for (var objectStore of this._objectStores)
            objectStore.establishRelationship(this);
    }

    // Public

    get name()
    {
        return this._name;
    }

    get securityOrigin()
    {
        return this._securityOrigin;
    }

    get host()
    {
        return this._host;
    }

    get version()
    {
        return this._version;
    }

    get objectStores()
    {
        return this._objectStores;
    }

    saveIdentityToCookie(cookie)
    {
        cookie[WebInspector.IndexedDatabase.NameCookieKey] = this._name;
        cookie[WebInspector.IndexedDatabase.HostCookieKey] = this._host;
    }
};

WebInspector.IndexedDatabase.TypeIdentifier = "indexed-database";
WebInspector.IndexedDatabase.NameCookieKey = "indexed-database-name";
WebInspector.IndexedDatabase.HostCookieKey = "indexed-database-host";
