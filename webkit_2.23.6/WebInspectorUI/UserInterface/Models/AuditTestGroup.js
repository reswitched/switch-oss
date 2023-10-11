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

WI.AuditTestGroup = class AuditTestGroup extends WI.AuditTestBase
{
    constructor(name, tests, options = {})
    {
        console.assert(Array.isArray(tests));

        // Set disabled once `_tests` is set so that it propagates.
        let disabled = options.disabled;
        options.disabled = false;

        super(name, options);

        this._tests = tests;
        this._preventDisabledPropagation = false;

        if (disabled || !this.supported)
            this.disabled = true;

        let hasSupportedTest = false;

        for (let test of this._tests) {
            if (!this.supported)
                test.supported = false;
            else if (test.supported)
                hasSupportedTest = true;

            test.addEventListener(WI.AuditTestBase.Event.Completed, this._handleTestCompleted, this);
            test.addEventListener(WI.AuditTestBase.Event.DisabledChanged, this._handleTestDisabledChanged, this);
            test.addEventListener(WI.AuditTestBase.Event.Progress, this._handleTestProgress, this);

        }

        if (!hasSupportedTest)
            this.supported = false;
    }

    // Static

    static async fromPayload(payload)
    {
        if (typeof payload !== "object" || payload === null)
            return null;

        if (payload.type !== WI.AuditTestGroup.TypeIdentifier)
            return null;

        if (typeof payload.name !== "string") {
            WI.AuditManager.synthesizeError(WI.UIString("\u0022%s\u0022 has a non-string \u0022%s\u0022 value").format(payload.name, WI.unlocalizedString("name")));
            return null;
        }

        if (!Array.isArray(payload.tests)) {
            WI.AuditManager.synthesizeError(WI.UIString("\u0022%s\u0022 has a non-array \u0022%s\u0022 value").format(payload.name, WI.unlocalizedString("tests")));
            return null;
        }

        let tests = await Promise.all(payload.tests.map(async (test) => {
            let testCase = await WI.AuditTestCase.fromPayload(test);
            if (testCase)
                return testCase;

            let testGroup = await WI.AuditTestGroup.fromPayload(test);
            if (testGroup)
                return testGroup;

            return null;
        }));
        tests = tests.filter((test) => !!test);
        if (!tests.length)
            return null;

        let options = {};

        if (typeof payload.description === "string")
            options.description = payload.description;
        else if ("description" in payload)
            WI.AuditManager.synthesizeWarning(WI.UIString("\u0022%s\u0022 has a non-string \u0022%s\u0022 value").format(payload.name, WI.unlocalizedString("description")));

        if (typeof payload.supports === "number")
            options.supports = payload.supports;
        else if ("supports" in payload)
            WI.AuditManager.synthesizeWarning(WI.UIString("\u0022%s\u0022 has a non-number \u0022%s\u0022 value").format(payload.name, WI.unlocalizedString("supports")));

        if (typeof payload.setup === "string")
            options.setup = payload.setup;
        else if ("setup" in payload)
            WI.AuditManager.synthesizeWarning(WI.UIString("\u0022%s\u0022 has a non-string \u0022%s\u0022 value").format(payload.name, WI.unlocalizedString("setup")));

        if (typeof payload.disabled === "boolean")
            options.disabled = payload.disabled;

        return new WI.AuditTestGroup(payload.name, tests, options);
    }

    // Public

    get tests() { return this._tests; }

    get supported()
    {
        return super.supported;
    }

    set supported(supported)
    {
        for (let test of this._tests)
            test.supported = supported;

        super.supported = supported;
    }

    get disabled()
    {
        return super.disabled;
    }

    set disabled(disabled)
    {
        if (!this._preventDisabledPropagation) {
            for (let test of this._tests)
                test.disabled = disabled;
        }

        super.disabled = disabled;
    }

    stop()
    {
        // Called from WI.AuditManager.

        for (let test of this._tests)
            test.stop();

        super.stop();
    }

    clearResult(options = {})
    {
        let cleared = !!this._result;
        for (let test of this._tests) {
            if (test.clearResult(options))
                cleared = true;
        }

        return super.clearResult({
            ...options,
            suppressResultChangedEvent: !cleared,
        });
    }

    toJSON(key)
    {
        let json = super.toJSON(key);
        json.tests = this._tests.map((testCase) => testCase.toJSON(key));
        return json;
    }

    // Protected

    async run()
    {
        let count = this._tests.length;
        for (let index = 0; index < count && this._runningState === WI.AuditManager.RunningState.Active; ++index) {
            let test = this._tests[index];
            if (test.disabled)
                continue;

            await test.start();

            if (test instanceof WI.AuditTestCase)
                this.dispatchEventToListeners(WI.AuditTestBase.Event.Progress, {index, count});
        }

        this._updateResult();
    }

    // Private

    _updateResult()
    {
        let results = this._tests.map((test) => test.result).filter((result) => !!result);
        if (!results.length)
            return;

        this._result = new WI.AuditTestGroupResult(this.name, results, {
            description: this.description,
        });

        this.dispatchEventToListeners(WI.AuditTestBase.Event.ResultChanged);
    }

    _handleTestCompleted(event)
    {
        if (this._runningState === WI.AuditManager.RunningState.Active)
            return;

        this._updateResult();
        this.dispatchEventToListeners(WI.AuditTestBase.Event.Completed);
    }

    _handleTestDisabledChanged(event)
    {
        let enabledTestCount = this._tests.filter((test) => !test.disabled).length;
        if (event.target.disabled && !enabledTestCount)
            this.disabled = true;
        else if (!event.target.disabled && enabledTestCount === 1) {
            this._preventDisabledPropagation = true;
            this.disabled = false;
            this._preventDisabledPropagation = false;
        } else {
            // Don't change `disabled`, as we're currently in an "indeterminate" state.
            this.dispatchEventToListeners(WI.AuditTestBase.Event.DisabledChanged);
        }
    }

    _handleTestProgress(event)
    {
        if (this._runningState !== WI.AuditManager.RunningState.Active)
            return;

        let walk = (tests) => {
            let count = 0;
            for (let test of tests) {
                if (test.disabled)
                    continue;

                if (test instanceof WI.AuditTestCase)
                    ++count;
                else if (test instanceof WI.AuditTestGroup)
                    count += walk(test.tests);
            }
            return count;
        };

        this.dispatchEventToListeners(WI.AuditTestBase.Event.Progress, {
            index: event.data.index + walk(this._tests.slice(0, this._tests.indexOf(event.target))),
            count: walk(this._tests),
        });
    }
};

WI.AuditTestGroup.TypeIdentifier = "test-group";
