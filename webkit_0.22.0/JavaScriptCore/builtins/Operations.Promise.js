/*
 * Copyright (C) 2015 Yusuke Suzuki <utatane.tea@gmail.com>.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

function isPromise(promise)
{
    "use strict";

    return @isObject(promise) && !!promise.@promiseState;
}

function newPromiseReaction(capability, handler)
{
    "use strict";

    return {
        @capabilities: capability,
        @handler: handler
    };
}

function newPromiseDeferred()
{
    "use strict";

    return @newPromiseCapability(@Promise);
}

function newPromiseCapability(constructor)
{
    "use strict";

    // FIXME: Check isConstructor(constructor).
    if (typeof constructor !== "function")
        throw new @TypeError("promise capability requires a constructor function");

    var promiseCapability = {
        @promise: @undefined,
        @resolve: @undefined,
        @reject: @undefined
    };

    function executor(resolve, reject)
    {
        if (promiseCapability.@resolve !== @undefined)
            throw new @TypeError("resolve function is already set");
        if (promiseCapability.@reject !== @undefined)
            throw new @TypeError("reject function is already set");

        promiseCapability.@resolve = resolve;
        promiseCapability.@reject = reject;
    }

    var promise = new constructor(executor);

    if (typeof promiseCapability.@resolve !== "function")
        throw new @TypeError("executor did not take a resolve function");

    if (typeof promiseCapability.@reject !== "function")
        throw new @TypeError("executor did not take a reject function");

    promiseCapability.@promise = promise;

    return promiseCapability;
}

function triggerPromiseReactions(reactions, argument)
{
    "use strict";

    for (var index = 0, length = reactions.length; index < length; ++index)
        @enqueueJob(@promiseReactionJob, [reactions[index], argument]);
}

function rejectPromise(promise, reason)
{
    "use strict";

    var reactions = promise.@promiseRejectReactions;
    promise.@promiseResult = reason;
    promise.@promiseFulfillReactions = @undefined;
    promise.@promiseRejectReactions = @undefined;
    promise.@promiseState = @promiseRejected;
    @triggerPromiseReactions(reactions, reason);
}

function fulfillPromise(promise, value)
{
    "use strict";

    var reactions = promise.@promiseFulfillReactions;
    promise.@promiseResult = value;
    promise.@promiseFulfillReactions = @undefined;
    promise.@promiseRejectReactions = @undefined;
    promise.@promiseState = @promiseFulfilled;
    @triggerPromiseReactions(reactions, value);
}

function createResolvingFunctions(promise)
{
    "use strict";

    var alreadyResolved = false;

    var resolve = function (resolution) {
        if (alreadyResolved)
            return @undefined;
        alreadyResolved = true;

        if (resolution === promise)
            return @rejectPromise(promise, new @TypeError("Resolve a promise with itself"));

        if (!@isObject(resolution))
            return @fulfillPromise(promise, resolution);

        var then;
        try {
            then = resolution.then;
        } catch (error) {
            return @rejectPromise(promise, error);
        }

        if (typeof then !== 'function')
            return @fulfillPromise(promise, resolution);

        @enqueueJob(@promiseResolveThenableJob, [promise, resolution, then]);

        return @undefined;
    };

    var reject = function (reason) {
        if (alreadyResolved)
            return @undefined;
        alreadyResolved = true;

        return @rejectPromise(promise, reason);
    };

    return {
        @resolve: resolve,
        @reject: reject
    };
}

function promiseReactionJob(reaction, argument)
{
    "use strict";

    var promiseCapability = reaction.@capabilities;

    var result;
    try {
        result = reaction.@handler.@call(@undefined, argument);
    } catch (error) {
        return promiseCapability.@reject.@call(@undefined, error);
    }

    return promiseCapability.@resolve.@call(@undefined, result);
}

function promiseResolveThenableJob(promiseToResolve, thenable, then)
{
    "use strict";

    var resolvingFunctions = @createResolvingFunctions(promiseToResolve);

    try {
        return then.@call(thenable, resolvingFunctions.@resolve, resolvingFunctions.@reject);
    } catch (error) {
        return resolvingFunctions.@reject.@call(@undefined, error);
    }
}

function initializePromise(executor)
{
    "use strict";

    if (typeof executor !== 'function')
        throw new @TypeError("Promise constructor takes a function argument");

    this.@promiseState = @promisePending;
    this.@promiseFulfillReactions = [];
    this.@promiseRejectReactions = [];

    var resolvingFunctions = @createResolvingFunctions(this);
    try {
        executor(resolvingFunctions.@resolve, resolvingFunctions.@reject);
    } catch (error) {
        return resolvingFunctions.@reject.@call(@undefined, error);
    }

    return this;
}
