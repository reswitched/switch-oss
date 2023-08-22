/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

WI.AuditTestBase = class AuditTestBase extends WI.Object
{
    constructor(name, {description, supports, setup, disabled} = {})
    {
        console.assert(typeof name === "string");
        console.assert(!description || typeof description === "string");
        console.assert(supports === undefined || typeof supports === "number");
        console.assert(!setup || typeof setup === "string");
        console.assert(disabled === undefined || typeof disabled === "boolean");

        super();

        // This class should not be instantiated directly. Create a concrete subclass instead.
        console.assert(this.constructor !== WI.AuditTestBase && this instanceof WI.AuditTestBase);

        this._name = name;
        this._description = description || null;
        this._supports = supports;
        this._setup = setup || null;

        this._supported = true;
        if (typeof this._supports === "number") {
            if (this._supports > WI.AuditTestBase.Version) {
                WI.AuditManager.synthesizeWarning(WI.UIString("\u0022%s\u0022 is too new to run in this Web Inspector").format(this.name));
                this._supported = false;
            } else if (this._supports > InspectorBackend.getVersion("Audit")) {
                WI.AuditManager.synthesizeWarning(WI.UIString("\u0022%s\u0022 is too new to run on this inspected page").format(this.name));
                this._supported = false;
            }
        }

        if (!this.supported)
            disabled = true;

        this._runningState = disabled ? WI.AuditManager.RunningState.Disabled : WI.AuditManager.RunningState.Inactive;
        this._result = null;
    }

    // Public

    get name() { return this._name; }
    get description() { return this._description; }
    get runningState() { return this._runningState; }
    get result() { return this._result; }

    get supported()
    {
        return this._supported;
    }

    set supported(supported)
    {
        this._supported = supported;
        if (!this._supported)
            this.disabled = true;
    }

    get disabled()
    {
        return this._runningState === WI.AuditManager.RunningState.Disabled;
    }

    set disabled(disabled)
    {
        console.assert(this._runningState === WI.AuditManager.RunningState.Disabled || this._runningState === WI.AuditManager.RunningState.Inactive);
        if (this._runningState !== WI.AuditManager.RunningState.Disabled && this._runningState !== WI.AuditManager.RunningState.Inactive)
            return;

        if (!this.supported)
            disabled = true;

        let runningState = disabled ? WI.AuditManager.RunningState.Disabled : WI.AuditManager.RunningState.Inactive;
        if (runningState === this._runningState)
            return;

        this._runningState = runningState;

        this.dispatchEventToListeners(WI.AuditTestBase.Event.DisabledChanged);
    }

    async setup()
    {
        if (!this._setup)
            return;

        let target = WI.assumingMainTarget();

        let agentCommandFunction = null;
        let agentCommandArguments = {};
        if (target.hasDomain("Audit")) {
            agentCommandFunction = target.AuditAgent.run;
            agentCommandArguments.test = this._setup;
        } else {
            agentCommandFunction = target.RuntimeAgent.evaluate;
            agentCommandArguments.expression = `(function() { "use strict"; return eval(\`(${this._setup.replace(/`/g, "\\`")})\`)(); })()`;
            agentCommandArguments.objectGroup = AuditTestBase.ObjectGroup;
            agentCommandArguments.doNotPauseOnExceptionsAndMuteConsole = true;
        }

        try {
            let response = await agentCommandFunction.invoke(agentCommandArguments);

            if (response.result.type === "object" && response.result.className === "Promise") {
                if (WI.RuntimeManager.supportsAwaitPromise())
                    response = await target.RuntimeAgent.awaitPromise(response.result.objectId);
                else {
                    response = null;
                    WI.AuditManager.synthesizeError(WI.UIString("Async audits are not supported."));
                }
            }

            if (response) {
                let remoteObject = WI.RemoteObject.fromPayload(response.result, WI.mainTarget);
                if (response.wasThrown || (remoteObject.type === "object" && remoteObject.subtype === "error"))
                    WI.AuditManager.synthesizeError(remoteObject.description);
            }
        } catch (error) {
            WI.AuditManager.synthesizeError(error.message);
        }
    }

    async start()
    {
        // Called from WI.AuditManager.

        if (this.disabled)
            return;

        console.assert(this.supported);

        console.assert(WI.auditManager.runningState === WI.AuditManager.RunningState.Active);

        console.assert(this._runningState === WI.AuditManager.RunningState.Inactive);
        if (this._runningState !== WI.AuditManager.RunningState.Inactive)
            return;

        this._runningState = WI.AuditManager.RunningState.Active;
        this.dispatchEventToListeners(WI.AuditTestBase.Event.Scheduled);

        await this.run();

        this._runningState = WI.AuditManager.RunningState.Inactive;
        this.dispatchEventToListeners(WI.AuditTestBase.Event.Completed);
    }

    stop()
    {
        // Called from WI.AuditManager.

        if (this.disabled)
            return;

        console.assert(this.supported);

        console.assert(WI.auditManager.runningState === WI.AuditManager.RunningState.Stopping);

        if (this._runningState !== WI.AuditManager.RunningState.Active)
            return;

        this._runningState = WI.AuditManager.RunningState.Stopping;
        this.dispatchEventToListeners(WI.AuditTestBase.Event.Stopping);
    }

    clearResult(options = {})
    {
        if (!this._result)
            return false;

        this._result = null;

        if (!options.suppressResultChangedEvent)
            this.dispatchEventToListeners(WI.AuditTestBase.Event.ResultChanged);

        return true;
    }

    saveIdentityToCookie(cookie)
    {
        cookie["audit-" + this.constructor.TypeIdentifier + "-name"] = this._name;
    }

    toJSON(key)
    {
        let json = {
            type: this.constructor.TypeIdentifier,
            name: this._name,
        };
        if (this._description)
            json.description = this._description;
        if (typeof this._supports === "number")
            json.supports = this._supports;
        if (this._setup)
            json.setup = this._setup;
        if (key === WI.ObjectStore.toJSONSymbol)
            json.disabled = this.disabled;
        return json;
    }

    // Protected

    async run()
    {
        throw WI.NotImplementedError.subclassMustOverride();
    }
};

// Keep this in sync with Inspector::Protocol::Audit::VERSION.
WI.AuditTestBase.Version = 3;

WI.AuditTestBase.ObjectGroup = "audit";

WI.AuditTestBase.Event = {
    Completed: "audit-test-base-completed",
    DisabledChanged: "audit-test-base-disabled-changed",
    Progress: "audit-test-base-progress",
    ResultChanged: "audit-test-base-result-changed",
    Scheduled: "audit-test-base-scheduled",
    Stopping: "audit-test-base-stopping",
};
