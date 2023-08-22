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

// In the inspector token types have been modified to include extra mode information
// after the actual token type. So we can't do token === "foo". So instead we do
// /\bfoo\b/.test(token).

CodeMirror.extendMode("javascript", {
    shouldHaveSpaceBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!token) {
            if (content === "(") // Most keywords like "if (" but not "function(" or "typeof(".
                return lastToken && /\bkeyword\b/.test(lastToken) && (lastContent !== "function" && lastContent !== "typeof" && lastContent !== "instanceof");
            if (content === ":") // Ternary.
                return (state.lexical.type === "stat" || state.lexical.type === ")");
            return false;
        }

        if (isComment)
            return true;

        if (/\boperator\b/.test(token)) {
            if (content === "!") // Unary ! should not be confused with "!=".
                return false;
            return "+-/*&&||!===+=-=>=<=?".indexOf(content) >= 0; // Operators.
        }

        if (/\bkeyword\b/.test(token)) { // Most keywords require spaces before them, unless a '}' can come before it.
            if (content === "else" || content === "catch" || content === "finally")
                return lastContent === "}";
            return false;
        }

        return false;
    },

    shouldHaveSpaceAfterLastToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (lastToken && /\bkeyword\b/.test(lastToken)) {  // Most keywords require spaces after them, unless a '{' or ';' can come after it.
            if (lastContent === "else")
                return true;
            if (lastContent === "catch")
                return true;
            if (lastContent === "return")
                return content !== ";";
            if (lastContent === "throw")
                return true;
            if (lastContent === "try")
                return true;
            if (lastContent === "finally")
                return true;
            if (lastContent === "do")
                return true;
            return false;
        }

        if (lastToken && /\bcomment\b/.test(lastToken)) // Embedded /* comment */.
            return true;
        if (lastContent === ")") // "){".
            return content === "{";
        if (lastContent === ";") // In for loop.
            return state.lexical.type === ")";
        if (lastContent === "!") // Unary ! should not be confused with "!=".
            return false;

        return ",+-/*&&||:!===+=-=>=<=?".indexOf(lastContent) >= 0; // Operators.
    },

    newlinesAfterToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!token) {
            if (content === ",") // In object literals, like in {a:1,b:2}, but not in param lists or vardef lists.
                return state.lexical.type === "}" ? 1 : 0;
            if (content === ";") // Everywhere except in for loop conditions.
                return state.lexical.type !== ")" ? 1 : 0;
            if (content === ":" && state.lexical.type === "}" && state.lexical.prev && state.lexical.prev.type === "form") // Switch case/default.
                return 1;
            return content.length === 1 && "{}".indexOf(content) >= 0 ? 1 : 0; // After braces.
        }

        if (isComment)
            return 1;

        return 0;
    },

    removeLastWhitespace: function(lastToken, lastContent, token, state, content, isComment)
    {
        return false;
    },

    removeLastNewline: function(lastToken, lastContent, token, state, content, isComment, firstTokenOnLine)
    {
        if (!token) {
            if (content === "}") // "{}".
                return lastContent === "{";
            if (content === ";") // "x = {};" or ";;".
                return "};".indexOf(lastContent) >= 0;
            if (content === ":") // Ternary.
                return lastContent === "}" && (state.lexical.type === "stat" || state.lexical.type === ")");
            if (",().".indexOf(content) >= 0) // "})", "}.bind", "function() { ... }()", or "}, false)".
                return lastContent === "}";
            return false;
        }

        if (isComment) { // Comment after semicolon.
            if (!firstTokenOnLine && lastContent === ";")
                return true;
            return false;
        }

        if (/\bkeyword\b/.test(token)) {
            if (content === "else" || content === "catch" || content === "finally") // "} else", "} catch", "} finally"
                return lastContent === "}";
            return false;
        }

        return false;
    },

    indentAfterToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (content === "{")
            return true;

        if (content === "case" || content === "default")
            return state.lexical.type === "}" && state.lexical.prev && state.lexical.prev.type === "form"; // Switch case/default.

        return false;
    },

    newlineBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (state._jsPrettyPrint.shouldIndent)
            return true;

        return content === "}" && lastContent !== "{"; // "{}"
    },

    indentBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (state._jsPrettyPrint.shouldIndent)
            return true;

        return false;
    },

    dedentsBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        var dedent = 0;

        if (state._jsPrettyPrint.shouldDedent)
            dedent += state._jsPrettyPrint.dedentSize;

        if (!token && content === "}")
            dedent += 1;
        else if (token && /\bkeyword\b/.test(token) && (content === "case" || content === "default"))
            dedent += 1;

        return dedent;
    },

    modifyStateForTokenPre: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!state._jsPrettyPrint) {
            state._jsPrettyPrint = {
                indentCount: 0,       // How far have we indented because of single statement blocks.
                shouldIndent: false,  // Signal we should indent on entering a single statement block.
                shouldDedent: false,  // Signal we should dedent on leaving a single statement block.
                dedentSize: 0,        // How far we should dedent when leaving a single statement block.
                lastIfIndentCount: 0, // Keep track of the indent the last time we saw an if without braces.
                openBraceStartMarkers: [],  // Keep track of non-single statement blocks.
                openBraceTrackingCount: -1, // Keep track of "{" and "}" in non-single statement blocks.
            };
        }

        // - Entering:
        //   - Preconditions:
        //     - last lexical was a "form" we haven't encountered before
        //     - last content was ")", "else", or "do"
        //     - current lexical is not ")" (in an expression or condition)
        //   - Cases:
        //     1. "{"
        //       - indent +0
        //       - save this indent size so when we encounter the "}" we know how far to dedent
        //     2. "else if"
        //       - indent +0 and do not signal to add a newline and indent
        //       - mark the last if location so when we encounter an "else" we know how far to dedent
        //       - mark the lexical state so we know we are inside a single statement block
        //     3. Token without brace.
        //       - indent +1 and signal to add a newline and indent
        //       - mark the last if location so when we encounter an "else" we know how far to dedent
        //       - mark the lexical state so we know we are inside a single statement block
        if (!isComment && state.lexical.prev && state.lexical.prev.type === "form" && !state.lexical.prev._jsPrettyPrintMarker && (lastContent === ")" || lastContent === "else" || lastContent === "do") && (state.lexical.type !== ")")) {
            if (content === "{") {
                // Save the state at the opening brace so we can return to it when we see "}".
                var savedState = {indentCount: state._jsPrettyPrint.indentCount, openBraceTrackingCount: state._jsPrettyPrint.openBraceTrackingCount};
                state._jsPrettyPrint.openBraceStartMarkers.push(savedState);
                state._jsPrettyPrint.openBraceTrackingCount = 1;
            } else if (state.lexical.type !== "}") {
                // Increase the indent count. Signal for a newline and indent if needed.
                if (!(lastContent === "else" && content === "if")) {
                    state._jsPrettyPrint.indentCount++;
                    state._jsPrettyPrint.shouldIndent = true;
                }
                state.lexical.prev._jsPrettyPrintMarker = true;
                if (state._jsPrettyPrint.enteringIf)
                    state._jsPrettyPrint.lastIfIndentCount = state._jsPrettyPrint.indentCount - 1;
            }
        }

        // - Leaving:
        //   - Preconditions:
        //     - we must be indented
        //     - ignore ";", wait for the next token instead.
        //   - Cases:
        //     1. "else"
        //       - dedent to the last "if"
        //     2. "}" and all braces we saw are balanced
        //       - dedent to the last "{"
        //     3. Token without a marker on the stack
        //       - dedent all the way
        else if (state._jsPrettyPrint.indentCount) {
            console.assert(!state._jsPrettyPrint.shouldDedent);
            console.assert(!state._jsPrettyPrint.dedentSize);

            // Track "{" and "}" to know when the "}" is really closing a block.
            if (!isComment) {
                if (content === "{")
                    state._jsPrettyPrint.openBraceTrackingCount++;
                else if (content === "}")
                    state._jsPrettyPrint.openBraceTrackingCount--;
            }

            if (content === ";") {
                // Ignore.
            } else if (content === "else") {
                // Dedent to the last "if".
                if (lastContent !== "}") {
                    state._jsPrettyPrint.shouldDedent = true;
                    state._jsPrettyPrint.dedentSize = state._jsPrettyPrint.indentCount - state._jsPrettyPrint.lastIfIndentCount;
                    state._jsPrettyPrint.lastIfIndentCount = 0;
                }
            } else if (content === "}" && !state._jsPrettyPrint.openBraceTrackingCount && state._jsPrettyPrint.openBraceStartMarkers.length) {
                // Dedent to the last "{".
                var savedState = state._jsPrettyPrint.openBraceStartMarkers.pop();
                state._jsPrettyPrint.shouldDedent = true;
                state._jsPrettyPrint.dedentSize = state._jsPrettyPrint.indentCount - savedState.indentCount;
                state._jsPrettyPrint.openBraceTrackingCount = savedState.openBraceTrackingCount;
            } else {
                // Dedent all the way.
                var shouldDedent = true;
                var lexical = state.lexical.prev;
                while (lexical) {
                    if (lexical._jsPrettyPrintMarker) {
                        shouldDedent = false;
                        break;
                    }
                    lexical = lexical.prev;
                }
                if (shouldDedent) {
                    state._jsPrettyPrint.shouldDedent = true;
                    state._jsPrettyPrint.dedentSize = state._jsPrettyPrint.indentCount;
                }
            }
        }

        // Signal for when we will be entering an if.
        if (token && state.lexical.type === "form" && state.lexical.prev && state.lexical.prev !== "form" && /\bkeyword\b/.test(token))
            state._jsPrettyPrint.enteringIf = (content === "if");
    },

    modifyStateForTokenPost: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (state._jsPrettyPrint.shouldIndent)
            state._jsPrettyPrint.shouldIndent = false;

        if (state._jsPrettyPrint.shouldDedent) {
            state._jsPrettyPrint.indentCount -= state._jsPrettyPrint.dedentSize;
            state._jsPrettyPrint.dedentSize = 0;
            state._jsPrettyPrint.shouldDedent = false;
        }
    }
});

CodeMirror.extendMode("css", {
    shouldHaveSpaceBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!token) {
            if (content === "{")
                return true;
            return ">+~-*/".indexOf(content) >= 0; // calc() expression or child/sibling selectors
        }

        if (isComment)
            return true;

        if (/\bkeyword\b/.test(token)) {
            if (content.charAt(0) === "!") // "!important".
                return true;
            return false;
        }

        return false;
    },

    shouldHaveSpaceAfterLastToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!lastToken) {
            if (lastContent === ",")
                return true;
            if (lastContent === ":") // Space in "prop: value" but not in a selectors "a:link" or "div::after" or media queries "(max-device-width:480px)".
                return state.state === "prop";
            if (lastContent === ")" && (content !== ")" && content !== ",")) {
                if (/\bnumber\b/.test(token)) // linear-gradient(rgb(...)0%,rgb(...)100%)
                    return true;
                if (state.state === "prop") // -webkit-transform:rotate(...)translate(...);
                    return true;
                if (state.state === "media" || state.state === "atBlock_parens") // Space in "not(foo)and" but not at the end of "not(not(foo))"
                    return true;
                return false; // color: rgb(...);
            }
            return ">+~-*/".indexOf(lastContent) >= 0; // calc() expression or child/sibling selectors
        }

        if (/\bcomment\b/.test(lastToken))
            return true;

        if (/\bkeyword\b/.test(lastToken)) // media-query keywords
            return state.state === "media" || (state.state === "atBlock_parens" && content !== ")");

        return false;
    },

    newlinesAfterToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!token) {
            if (content === ";")
                return 1;
            if (content === ",") { // "a,b,c,...,z{}" rule list at top level or in @media top level and only if the line length will be large.
                if ((state.state === "top" || state.state === "media") && state._cssPrettyPrint.lineLength > 60) {
                    state._cssPrettyPrint.lineLength = 0;
                    return 1;
                }
                return 0;
            }
            if (content === "{")
                return 1;
            if (content === "}") // 2 newlines between rule declarations.
                return 2;
            return 0;
        }

        if (isComment)
            return 1;

        return 0;
    },

    removeLastWhitespace: function(lastToken, lastContent, token, state, content, isComment)
    {
        return false;
    },

    removeLastNewline: function(lastToken, lastContent, token, state, content, isComment, firstTokenOnLine)
    {
        if (isComment) { // Comment after semicolon.
            if (!firstTokenOnLine && lastContent === ";")
                return true;
            return false;
        }

        return content === "}" && (lastContent === "{" || lastContent === "}"); // "{}" and "}\n}" when closing @media.
    },

    indentAfterToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return content === "{";
    },

    newlineBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return content === "}" && (lastContent !== "{" && lastContent !== "}"); // "{}" and "}\n}" when closing @media.
    },

    indentBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return false;
    },

    dedentsBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return content === "}" ? 1 : 0;
    },

    modifyStateForTokenPost: function(lastToken, lastContent, token, state, content, isComment)
    {
        if (!state._cssPrettyPrint)
            state._cssPrettyPrint = {lineLength: 0};

        // In order insert newlines in selector lists we need keep track of the length of the current line.
        // This isn't exact line length, only the builder knows that, but it is good enough to get an idea.
        // If we are at a top level, keep track of the current line length, otherwise we reset to 0.
        if (!isComment && (state.state === "top" || state.state === "media" || state.state === "pseudo"))
            state._cssPrettyPrint.lineLength += content.length;
        else
            state._cssPrettyPrint.lineLength = 0;
    }
});

CodeMirror.extendMode("css-rule", {
    shouldHaveSpaceBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        // Add whitespace before ":_value"
        if (lastContent === ":" && !lastToken)
            return true;

        // Add whitespace between "1px_solid_green"
        var tokenRegExp = /\b(?:keyword|atom|number)\b/;
        if (tokenRegExp.test(lastToken) && tokenRegExp.test(token))
            return true;

        return false;
    },

    shouldHaveSpaceAfterLastToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return lastContent === "," && !lastToken;
    },

    newlinesAfterToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return 0;
    },

    removeLastWhitespace: function(lastToken, lastContent, token, state, content, isComment)
    {
        // Remove whitespace before a comment which moves the comment to the beginning of the line.
        if (isComment)
            return true;

        // A semicolon indicates the end of line. So remove whitespace before next line.
        if (!lastToken)
            return lastContent === ";";

        // Remove whitespace before semicolon. Like `prop: value ;`.
        // Remove whitespace before colon. Like `prop : value;`.
        if (!token)
            return content === ";" || content === ":";

        // A comment is supposed to be in its own line. So remove whitespace before next line.
        if (/\bcomment\b/.test(lastToken))
            return true;

        return false;
    },

    removeLastNewline: function(lastToken, lastContent, token, state, content, isComment, firstTokenOnLine)
    {
        // Each property should be formatted to one line each with no extra newlines.
        return true;
    },

    indentAfterToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return false;
    },

    newlineBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        // Add new line before comments.
        if (isComment)
            return true;

        // Add new line before a prefixed property like `-webkit-animation`.
        if (state.state === "block")
            return /\bmeta\b/.test(token);

        // Add new line after comment
        if (/\bcomment\b/.test(lastToken))
            return true;

        // Add new line before a regular property like `display`.
        if (/\bproperty\b/.test(token))
            return !(/\bmeta\b/.test(lastToken));

        return false;
    },

    indentBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return false;
    },

    dedentsBeforeToken: function(lastToken, lastContent, token, state, content, isComment)
    {
        return 0;
    }
});
