function shouldBe(actual, expected) {
    if (actual !== expected)
        throw new Error('bad value: ' + actual);
}

shouldBe(JSON.stringify(Object.getOwnPropertyNames(RegExp.prototype).sort()), '["compile","constructor","exec","flags","global","ignoreCase","lastIndex","multiline","source","test","toString"]');
shouldBe(JSON.stringify(Object.getOwnPropertyNames(/Cocoa/).sort()), '["lastIndex"]');
