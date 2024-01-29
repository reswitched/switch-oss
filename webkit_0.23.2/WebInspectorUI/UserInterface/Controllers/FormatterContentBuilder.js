/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2015 Tobias Reiss <tobi+webkit@basecode.de>
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

WebInspector.FormatterContentBuilder = class FormatterContentBuilder
{
    constructor(mapping, originalLineEndings, formattedLineEndings, originalOffset, formattedOffset, indentString)
    {
        this._originalContent = null;
        this._formattedContent = [];
        this._formattedContentLength = 0;

        this._startOfLine = true;
        this.lastTokenWasNewline = false;
        this.lastTokenWasWhitespace = false;
        this.lastNewlineAppendWasMultiple = false;

        this._indent = 0;
        this._indentString = indentString;
        this._indentCache = ["", this._indentString];

        this._mapping = mapping;
        this._originalLineEndings = originalLineEndings || [];
        this._formattedLineEndings = formattedLineEndings || [];
        this._originalOffset = originalOffset || 0;
        this._formattedOffset = formattedOffset || 0;

        this._lastOriginalPosition = 0;
        this._lastFormattedPosition = 0;
    }

    // Public

    get originalContent()
    {
        return this._originalContent;
    }

    get formattedContent()
    {
        var formatted = this._formattedContent.join("");
        console.assert(formatted.length === this._formattedContentLength);
        return formatted;
    }

    get mapping()
    {
        return this._mapping;
    }

    get originalLineEndings()
    {
        return this._originalLineEndings;
    }

    get formattedLineEndings()
    {
        return this._formattedLineEndings;
    }

    setOriginalContent(originalContent)
    {
        console.assert(!this._originalContent);
        this._originalContent = originalContent;
    }

    appendToken(string, originalPosition)
    {
        if (this._startOfLine)
            this._appendIndent();

        this._addMappingIfNeeded(originalPosition);

        this._append(string);
        this._startOfLine = false;
        this.lastTokenWasNewline = false;
        this.lastTokenWasWhitespace = false;
    }

    appendSpace()
    {
        if (!this._startOfLine) {
            this._append(" ");
            this.lastTokenWasNewline = false;
            this.lastTokenWasWhitespace = true;
        }
    }

    appendNewline(force)
    {
        if ((!this.lastTokenWasNewline && !this._startOfLine) || force) {
            this._append("\n");
            this._addFormattedLineEnding();
            this._startOfLine = true;
            this.lastTokenWasNewline = true;
            this.lastTokenWasWhitespace = false;
            this.lastNewlineAppendWasMultiple = false;
        }
    }

    appendMultipleNewlines(newlines)
    {
        console.assert(newlines > 0);

        var wasMultiple = newlines > 1;

        while (newlines-- > 0)
            this.appendNewline(true);

        if (wasMultiple)
            this.lastNewlineAppendWasMultiple = true;
    }

    removeLastNewline()
    {
        console.assert(this.lastTokenWasNewline);
        console.assert(this._formattedContent.lastValue === "\n");
        if (this.lastTokenWasNewline) {
            this._popFormattedContent();
            this._formattedLineEndings.pop();
            this._startOfLine = false;
            this.lastTokenWasNewline = false;
            this.lastTokenWasWhitespace = false;
        }
    }

    removeLastWhitespace()
    {
        console.assert(this.lastTokenWasWhitespace);
        console.assert(this._formattedContent.lastValue === " ");
        if (this.lastTokenWasWhitespace) {
            this._popFormattedContent();
            // No need to worry about `_startOfLine` and `lastTokenWasNewline`
            // because `appendSpace` takes care of not adding whitespace
            // to the beginning of a line.
            this.lastTokenWasWhitespace = false;
        }
    }

    indent()
    {
        ++this._indent;
    }

    dedent()
    {
        --this._indent;

        console.assert(this._indent >= 0);
        if (this._indent < 0)
            this._indent = 0;
    }

    addOriginalLineEnding(originalPosition)
    {
        this._originalLineEndings.push(originalPosition);
    }

    finish()
    {
        this.appendNewline();
    }

    // Private

    _popFormattedContent()
    {
        var removed = this._formattedContent.pop();
        this._formattedContentLength -= removed.length;
    }

    _append(str)
    {
        this._formattedContent.push(str);
        this._formattedContentLength += str.length;
    }

    _appendIndent()
    {
        // Indent is already in the cache.
        if (this._indent < this._indentCache.length) {
            this._append(this._indentCache[this._indent]);
            return;
        }

        // Indent was not in the cache, fill up the cache up with what was needed.
        var maxCacheIndent = 20;
        var max = Math.min(this._indent, maxCacheIndent);
        for (var i = this._indentCache.length; i <= max; ++i)
            this._indentCache[i] = this._indentCache[i - 1] + this._indentString;

        // Append indents as needed.
        var indent = this._indent;
        do {
            if (indent >= maxCacheIndent)
                this._append(this._indentCache[maxCacheIndent]);
            else
                this._append(this._indentCache[indent]);
            indent -= maxCacheIndent;
        } while (indent > 0);
    }

    _addMappingIfNeeded(originalPosition)
    {
        if (originalPosition - this._lastOriginalPosition === this._formattedContentLength - this._lastFormattedPosition)
            return;

        this._mapping.original.push(this._originalOffset + originalPosition);
        this._mapping.formatted.push(this._formattedOffset + this._formattedContentLength);

        this._lastOriginalPosition = originalPosition;
        this._lastFormattedPosition = this._formattedContentLength;
    }

    _addFormattedLineEnding()
    {
        console.assert(this._formattedContent.lastValue === "\n");
        this._formattedLineEndings.push(this._formattedContentLength - 1);
    }
};
