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

WebInspector.ProfileNode = class ProfileNode extends WebInspector.Object
{
    constructor(id, type, functionName, sourceCodeLocation, callInfo, calls, childNodes)
    {
        super();

        childNodes = childNodes || [];

        console.assert(id);
        console.assert(!calls || calls instanceof Array);
        console.assert(!calls || calls.length >= 1);
        console.assert(!calls || calls.every(function(call) { return call instanceof WebInspector.ProfileNodeCall; }));
        console.assert(childNodes instanceof Array);
        console.assert(childNodes.every(function(node) { return node instanceof WebInspector.ProfileNode; }));

        this._id = id;
        this._type = type || WebInspector.ProfileNode.Type.Function;
        this._functionName = functionName || null;
        this._sourceCodeLocation = sourceCodeLocation || null;
        this._calls = calls || null;
        this._callInfo = callInfo || null;
        this._childNodes = childNodes;
        this._parentNode = null;
        this._previousSibling = null;
        this._nextSibling = null;
        this._computedTotalTimes = false;

        if (this._callInfo) {            
            this._startTime = this._callInfo.startTime;
            this._endTime = this._callInfo.endTime;
            this._totalTime = this._callInfo.totalTime;
            this._callCount = this._callInfo.callCount;
        }

        for (var i = 0; i < this._childNodes.length; ++i)
            this._childNodes[i].establishRelationships(this, this._childNodes[i - 1], this._childNodes[i + 1]);

        if (this._calls) {
            for (var i = 0; i < this._calls.length; ++i)
                this._calls[i].establishRelationships(this, this._calls[i - 1], this._calls[i + 1]);
        }
    }

    // Public

    get id()
    {
        return this._id;
    }

    get type()
    {
        return this._type;
    }

    get functionName()
    {
        return this._functionName;
    }

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    }

    get startTime()
    {
        if (this._startTime === undefined)
            this._startTime =  Math.max(0, this._calls[0].startTime);
        return this._startTime;
    }

    get endTime()
    {
        if (this._endTime === undefined)
            this._endTime = Math.min(this._calls.lastValue.endTime, Infinity);
        return this._endTime;
    }

    get selfTime()
    {
        this._computeTotalTimesIfNeeded();
        return this._selfTime;
    }

    get totalTime()
    {
        this._computeTotalTimesIfNeeded();
        return this._totalTime;
    }

    get callInfo()
    {
        return this._callInfo;
    }

    get calls()
    {
        return this._calls;
    }

    get previousSibling()
    {
        return this._previousSibling;
    }

    get nextSibling()
    {
        return this._nextSibling;
    }

    get parentNode()
    {
        return this._parentNode;
    }

    get childNodes()
    {
        return this._childNodes;
    }

    computeCallInfoForTimeRange(rangeStartTime, rangeEndTime)
    {
        console.assert(typeof rangeStartTime === "number");
        console.assert(typeof rangeEndTime === "number");

        // With aggregate call info we can't accurately partition self/total/average time
        // in partial ranges because we don't know exactly when each call started. So we
        // always return the entire range.
        if (this._callInfo) {
            if (this._selfTime === undefined) {
                var childNodesTotalTime = 0;
                for (var childNode of this._childNodes)
                    childNodesTotalTime += childNode.totalTime;
                this._selfTime = this._totalTime - childNodesTotalTime;
            }

            return {
                callCount: this._callCount,
                startTime: this._startTime,
                endTime: this._endTime,
                selfTime: this._selfTime,
                totalTime: this._totalTime,
                averageTime: (this._selfTime / this._callCount),
            };
        }

        // COMPATIBILITY (iOS 8): Profiles included per-call information and can be finely partitioned.
        // Compute that below by iterating over all the calls / children for the time range.

        var recordCallCount = true;
        var callCount = 0;

        function totalTimeInRange(previousValue, call)
        {
            if (rangeStartTime > call.endTime || rangeEndTime < call.startTime)
                return previousValue;

            if (recordCallCount)
                ++callCount;

            return previousValue + Math.min(call.endTime, rangeEndTime) - Math.max(rangeStartTime, call.startTime);
        }

        var startTime = Math.max(rangeStartTime, this._calls[0].startTime);
        var endTime = Math.min(this._calls.lastValue.endTime, rangeEndTime);
        var totalTime = this._calls.reduce(totalTimeInRange, 0);

        recordCallCount = false;

        var childNodesTotalTime = 0;
        for (var childNode of this._childNodes)
            childNodesTotalTime += childNode.calls.reduce(totalTimeInRange, 0);

        var selfTime = totalTime - childNodesTotalTime;
        var averageTime = selfTime / callCount;

        return {startTime, endTime, totalTime, selfTime, callCount, averageTime};
    }

    traverseNextProfileNode(stayWithin)
    {
        var profileNode = this._childNodes[0];
        if (profileNode)
            return profileNode;

        if (this === stayWithin)
            return null;

        profileNode = this._nextSibling;
        if (profileNode)
            return profileNode;

        profileNode = this;
        while (profileNode && !profileNode.nextSibling && profileNode.parentNode !== stayWithin)
            profileNode = profileNode.parentNode;

        if (!profileNode)
            return null;

        return profileNode.nextSibling;
    }

    saveIdentityToCookie(cookie)
    {
        cookie[WebInspector.ProfileNode.TypeCookieKey] = this._type || null;
        cookie[WebInspector.ProfileNode.FunctionNameCookieKey] = this._functionName || null;
        cookie[WebInspector.ProfileNode.SourceCodeURLCookieKey] = this._sourceCodeLocation ? this._sourceCodeLocation.sourceCode.url ? this._sourceCodeLocation.sourceCode.url.hash : null : null;
        cookie[WebInspector.ProfileNode.SourceCodeLocationLineCookieKey] = this._sourceCodeLocation ? this._sourceCodeLocation.lineNumber : null;
        cookie[WebInspector.ProfileNode.SourceCodeLocationColumnCookieKey] = this._sourceCodeLocation ? this._sourceCodeLocation.columnNumber : null;
    }

    // Protected

    establishRelationships(parentNode, previousSibling, nextSibling)
    {
        this._parentNode = parentNode || null;
        this._previousSibling = previousSibling || null;
        this._nextSibling = nextSibling || null;
    }

    // Private

    _computeTotalTimesIfNeeded()
    {
        if (this._computedTotalTimes)
            return;

        this._computedTotalTimes = true;

        var info = this.computeCallInfoForTimeRange(0, Infinity);
        this._startTime = info.startTime;
        this._endTime = info.endTime;
        this._selfTime = info.selfTime;
        this._totalTime = info.totalTime;
    }
};

WebInspector.ProfileNode.Type = {
    Function: "profile-node-type-function",
    Program: "profile-node-type-program"
};

WebInspector.ProfileNode.TypeIdentifier = "profile-node";
WebInspector.ProfileNode.TypeCookieKey = "profile-node-type";
WebInspector.ProfileNode.FunctionNameCookieKey = "profile-node-function-name";
WebInspector.ProfileNode.SourceCodeURLCookieKey = "profile-node-source-code-url";
WebInspector.ProfileNode.SourceCodeLocationLineCookieKey = "profile-node-source-code-location-line";
WebInspector.ProfileNode.SourceCodeLocationColumnCookieKey = "profile-node-source-code-location-column";
