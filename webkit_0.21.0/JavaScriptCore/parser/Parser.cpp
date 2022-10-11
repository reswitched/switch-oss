/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Parser.h"

#include "ASTBuilder.h"
#include "CodeBlock.h"
#include "Debugger.h"
#include "JSCJSValueInlines.h"
#include "Lexer.h"
#include "JSCInlines.h"
#include "SourceProvider.h"
#include "VM.h"
#include <utility>
#include <wtf/HashFunctions.h>
#include <wtf/StringPrintStream.h>
#include <wtf/WTFThreadData.h>


#define updateErrorMessage(shouldPrintToken, ...) do {\
    propagateError(); \
    logError(shouldPrintToken, __VA_ARGS__); \
} while (0)

#define propagateError() do { if (hasError()) return 0; } while (0)
#define internalFailWithMessage(shouldPrintToken, ...) do { updateErrorMessage(shouldPrintToken, __VA_ARGS__); return 0; } while (0)
#define handleErrorToken() do { if (m_token.m_type == EOFTOK || m_token.m_type & ErrorTokenFlag) { failDueToUnexpectedToken(); } } while (0)
#define failWithMessage(...) do { { handleErrorToken(); updateErrorMessage(true, __VA_ARGS__); } return 0; } while (0)
#define failWithStackOverflow() do { updateErrorMessage(false, "Stack exhausted"); m_hasStackOverflow = true; return 0; } while (0)
#define failIfFalse(cond, ...) do { if (!(cond)) { handleErrorToken(); internalFailWithMessage(true, __VA_ARGS__); } } while (0)
#define failIfTrue(cond, ...) do { if (cond) { handleErrorToken(); internalFailWithMessage(true, __VA_ARGS__); } } while (0)
#define failIfTrueIfStrict(cond, ...) do { if ((cond) && strictMode()) internalFailWithMessage(false, __VA_ARGS__); } while (0)
#define failIfFalseIfStrict(cond, ...) do { if ((!(cond)) && strictMode()) internalFailWithMessage(false, __VA_ARGS__); } while (0)
#define consumeOrFail(tokenType, ...) do { if (!consume(tokenType)) { handleErrorToken(); internalFailWithMessage(true, __VA_ARGS__); } } while (0)
#define consumeOrFailWithFlags(tokenType, flags, ...) do { if (!consume(tokenType, flags)) { handleErrorToken(); internalFailWithMessage(true, __VA_ARGS__); } } while (0)
#define matchOrFail(tokenType, ...) do { if (!match(tokenType)) { handleErrorToken(); internalFailWithMessage(true, __VA_ARGS__); } } while (0)
#define failIfStackOverflow() do { if (!canRecurse()) failWithStackOverflow(); } while (0)
#define semanticFail(...) do { internalFailWithMessage(false, __VA_ARGS__); } while (0)
#define semanticFailIfTrue(cond, ...) do { if (cond) internalFailWithMessage(false, __VA_ARGS__); } while (0)
#define semanticFailIfFalse(cond, ...) do { if (!(cond)) internalFailWithMessage(false, __VA_ARGS__); } while (0)
#define regexFail(failure) do { setErrorMessage(failure); return 0; } while (0)
#define failDueToUnexpectedToken() do {\
        logError(true);\
    return 0;\
} while (0)

#define handleProductionOrFail(token, tokenString, operation, production) do {\
    consumeOrFail(token, "Expected '", tokenString, "' to ", operation, " a ", production);\
} while (0)

#define semanticFailureDueToKeyword(...) do { \
    if (strictMode() && m_token.m_type == RESERVED_IF_STRICT) \
        semanticFail("Cannot use the reserved word '", getToken(), "' as a ", __VA_ARGS__, " in strict mode"); \
    if (m_token.m_type == RESERVED || m_token.m_type == RESERVED_IF_STRICT) \
        semanticFail("Cannot use the reserved word '", getToken(), "' as a ", __VA_ARGS__); \
    if (m_token.m_type & KeywordTokenFlag) \
        semanticFail("Cannot use the keyword '", getToken(), "' as a ", __VA_ARGS__); \
} while (0)

using namespace std;

namespace JSC {

template <typename LexerType>
void Parser<LexerType>::logError(bool)
{
    if (hasError())
        return;
    StringPrintStream stream;
    printUnexpectedTokenText(stream);
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A, typename B>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1, const B& value2)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, value2, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A, typename B, typename C>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1, const B& value2, const C& value3)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, value2, value3, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A, typename B, typename C, typename D>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1, const B& value2, const C& value3, const D& value4)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, value2, value3, value4, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A, typename B, typename C, typename D, typename E>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1, const B& value2, const C& value3, const D& value4, const E& value5)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, value2, value3, value4, value5, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A, typename B, typename C, typename D, typename E, typename F>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1, const B& value2, const C& value3, const D& value4, const E& value5, const F& value6)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, value2, value3, value4, value5, value6, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType> template <typename A, typename B, typename C, typename D, typename E, typename F, typename G>
void Parser<LexerType>::logError(bool shouldPrintToken, const A& value1, const B& value2, const C& value3, const D& value4, const E& value5, const F& value6, const G& value7)
{
    if (hasError())
        return;
    StringPrintStream stream;
    if (shouldPrintToken) {
        printUnexpectedTokenText(stream);
        stream.print(". ");
    }
    stream.print(value1, value2, value3, value4, value5, value6, value7, ".");
    setErrorMessage(stream.toString());
}

template <typename LexerType>
Parser<LexerType>::Parser(
    VM* vm, const SourceCode& source, JSParserBuiltinMode builtinMode, 
    JSParserStrictMode strictMode, JSParserCodeType codeType, 
    ConstructorKind defaultConstructorKind, ThisTDZMode thisTDZMode)
    : m_vm(vm)
    , m_source(&source)
    , m_hasStackOverflow(false)
    , m_allowsIn(true)
    , m_assignmentCount(0)
    , m_nonLHSCount(0)
    , m_syntaxAlreadyValidated(source.provider()->isValid())
    , m_statementDepth(0)
    , m_nonTrivialExpressionCount(0)
    , m_lastIdentifier(0)
    , m_lastFunctionName(nullptr)
    , m_sourceElements(0)
    , m_parsingBuiltin(builtinMode == JSParserBuiltinMode::Builtin)
    , m_defaultConstructorKind(defaultConstructorKind)
    , m_thisTDZMode(thisTDZMode)
{
    m_lexer = std::make_unique<LexerType>(vm, builtinMode);
    m_lexer->setCode(source, &m_parserArena);
    m_token.m_location.line = source.firstLine();
    m_token.m_location.startOffset = source.startOffset();
    m_token.m_location.endOffset = source.startOffset();
    m_token.m_location.lineStartOffset = source.startOffset();
    m_functionCache = vm->addSourceProviderCache(source.provider());
    ScopeRef scope = pushScope();
    if (codeType == JSParserCodeType::Function)
        scope->setIsFunction();
    if (strictMode == JSParserStrictMode::Strict)
        scope->setStrictMode();

    next();
}

template <typename LexerType>
Parser<LexerType>::~Parser()
{
}

template <typename LexerType>
String Parser<LexerType>::parseInner(const Identifier& calleeName, FunctionParseMode parseMode)
{
    String parseError = String();
    
    ASTBuilder context(const_cast<VM*>(m_vm), m_parserArena, const_cast<SourceCode*>(m_source));
    ScopeRef scope = currentScope();

    bool isArrowFunctionBodyExpression = false;
    if (m_lexer->isReparsingFunction()) {
        ParserFunctionInfo<ASTBuilder> functionInfo;
        parseFunctionParameters(context, parseMode, functionInfo);
        m_parameters = functionInfo.parameters;

#if ENABLE(ES6_ARROWFUNCTION_SYNTAX)
        if (parseMode == ArrowFunctionMode && !hasError()) {
            // The only way we could have an error wile reparsing is if we run out of stack space.
            RELEASE_ASSERT(match(ARROWFUNCTION));
            next();
            isArrowFunctionBodyExpression = !match(OPENBRACE);
        }
#endif
    }

    if (!calleeName.isNull())
        scope->declareCallee(&calleeName);

    if (m_lexer->isReparsingFunction())
        m_statementDepth--;

    SourceElements* sourceElements = nullptr;
    // The only way we can error this early is if we reparse a function and we run out of stack space.
    if (!hasError()) {
        if (isArrowFunctionBodyExpression)
            sourceElements = parseArrowFunctionSingleExpressionBodySourceElements(context);
        else
            sourceElements = parseSourceElements(context, CheckForStrictMode);
    }

    bool validEnding;
    if (isArrowFunctionBodyExpression) {
        ASSERT(m_lexer->isReparsingFunction());
        // When we reparse and stack overflow, we're not guaranteed a valid ending. If we don't run out of stack space,
        // then of course this will always be valid because we already parsed for syntax errors. But we must
        // be cautious in case we run out of stack space.
        validEnding = isEndOfArrowFunction(); 
    } else
        validEnding = consume(EOFTOK);

    if (!sourceElements || !validEnding) {
        if (hasError())
            parseError = m_errorMessage;
        else
            parseError = ASCIILiteral("Parser error");
    }

    IdentifierSet capturedVariables;
    bool modifiedParameter = false;
    bool modifiedArguments = false;
    scope->getCapturedVariables(capturedVariables, modifiedParameter, modifiedArguments);
    
    CodeFeatures features = context.features();
    if (scope->strictMode())
        features |= StrictModeFeature;
    if (scope->shadowsArguments())
        features |= ShadowsArgumentsFeature;
    if (modifiedParameter)
        features |= ModifiedParameterFeature;
    if (modifiedArguments)
        features |= ModifiedArgumentsFeature;
    Vector<RefPtr<UniquedStringImpl>> closedVariables;
    if (m_parsingBuiltin) {
        IdentifierSet usedVariables;
        scope->getUsedVariables(usedVariables);
        for (const auto& variable : usedVariables) {
            Identifier identifier = Identifier::fromUid(m_vm, variable.get());
            if (scope->hasDeclaredVariable(identifier))
                continue;
            
            if (scope->hasDeclaredParameter(identifier))
                continue;

            if (variable == m_vm->propertyNames->arguments.impl())
                continue;

            closedVariables.append(variable);
        }

        if (!capturedVariables.isEmpty()) {
            for (const auto& capturedVariable : capturedVariables) {
                Identifier identifier = Identifier::fromUid(m_vm, capturedVariable.get());
                if (scope->hasDeclaredVariable(identifier))
                    continue;

                if (scope->hasDeclaredParameter(identifier))
                    continue;

                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }
    didFinishParsing(sourceElements, context.varDeclarations(), context.funcDeclarations(), features,
        context.numConstants(), capturedVariables, WTF::move(closedVariables));

    return parseError;
}

template <typename LexerType>
void Parser<LexerType>::didFinishParsing(SourceElements* sourceElements, DeclarationStacks::VarStack& varStack, 
    DeclarationStacks::FunctionStack& funcStack, CodeFeatures features, int numConstants, IdentifierSet& capturedVars, const Vector<RefPtr<UniquedStringImpl>>&& closedVariables)
{
    m_sourceElements = sourceElements;
    m_varDeclarations.swap(varStack);
    m_funcDeclarations.swap(funcStack);
    m_capturedVariables.swap(capturedVars);
    m_closedVariables = closedVariables;
    m_features = features;
    m_numConstants = numConstants;
}

template <typename LexerType>
bool Parser<LexerType>::allowAutomaticSemicolon()
{
    return match(CLOSEBRACE) || match(EOFTOK) || m_lexer->prevTerminator();
}

template <typename LexerType>
template <class TreeBuilder> TreeSourceElements Parser<LexerType>::parseSourceElements(TreeBuilder& context, SourceElementsMode mode)
{
    const unsigned lengthOfUseStrictLiteral = 12; // "use strict".length
    TreeSourceElements sourceElements = context.createSourceElements();
    bool seenNonDirective = false;
    const Identifier* directive = 0;
    unsigned directiveLiteralLength = 0;
    auto savePoint = createSavePoint();
    bool hasSetStrict = false;
    
    while (TreeStatement statement = parseStatementListItem(context, directive, &directiveLiteralLength)) {
        if (mode == CheckForStrictMode && !seenNonDirective) {
            if (directive) {
                // "use strict" must be the exact literal without escape sequences or line continuation.
                if (!hasSetStrict && directiveLiteralLength == lengthOfUseStrictLiteral && m_vm->propertyNames->useStrictIdentifier == *directive) {
                    setStrictMode();
                    hasSetStrict = true;
                    if (!isValidStrictMode()) {
                        if (m_lastFunctionName) {
                            if (m_vm->propertyNames->arguments == *m_lastFunctionName)
                                semanticFail("Cannot name a function 'arguments' in strict mode");
                            if (m_vm->propertyNames->eval == *m_lastFunctionName)
                                semanticFail("Cannot name a function 'eval' in strict mode");
                        }
                        if (hasDeclaredVariable(m_vm->propertyNames->arguments))
                            semanticFail("Cannot declare a variable named 'arguments' in strict mode");
                        if (hasDeclaredVariable(m_vm->propertyNames->eval))
                            semanticFail("Cannot declare a variable named 'eval' in strict mode");
                        semanticFailIfFalse(isValidStrictMode(), "Invalid parameters or function name in strict mode");
                    }
                    restoreSavePoint(savePoint);
                    propagateError();
                    continue;
                }
            } else
                seenNonDirective = true;
        }
        context.appendStatement(sourceElements, statement);
    }

    propagateError();
    return sourceElements;
}
template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseStatementListItem(TreeBuilder& context, const Identifier*& directive, unsigned* directiveLiteralLength)
{
    // The grammar is documented here:
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-statements
    TreeStatement result = 0;
    switch (m_token.m_type) {
    case CONSTTOKEN:
        result = parseConstDeclaration(context);
        break;
#if ENABLE(ES6_CLASS_SYNTAX)
    case CLASSTOKEN:
        result = parseClassDeclaration(context);
        break;
#endif
    default:
        // FIXME: This needs to consider 'let' in bug:
        // https://bugs.webkit.org/show_bug.cgi?id=142944
        result = parseStatement(context, directive, directiveLiteralLength);
        break;
    }

    return result;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseVarDeclaration(TreeBuilder& context)
{
    ASSERT(match(VAR));
    JSTokenLocation location(tokenLocation());
    int start = tokenLine();
    int end = 0;
    int scratch;
    TreeDestructuringPattern scratch1 = 0;
    TreeExpression scratch2 = 0;
    JSTextPosition scratch3;
    TreeExpression varDecls = parseVarDeclarationList(context, scratch, scratch1, scratch2, scratch3, scratch3, scratch3, VarDeclarationContext);
    propagateError();
    failIfFalse(autoSemiColon(), "Expected ';' after var declaration");
    
    return context.createVarStatement(location, varDecls, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseConstDeclaration(TreeBuilder& context)
{
    ASSERT(match(CONSTTOKEN));
    JSTokenLocation location(tokenLocation());
    int start = tokenLine();
    int end = 0;
    TreeConstDeclList constDecls = parseConstDeclarationList(context);
    propagateError();
    failIfFalse(autoSemiColon(), "Expected ';' after const declaration");
    
    return context.createConstStatement(location, constDecls, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseDoWhileStatement(TreeBuilder& context)
{
    ASSERT(match(DO));
    int startLine = tokenLine();
    next();
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement, "Expected a statement following 'do'");
    int endLine = tokenLine();
    JSTokenLocation location(tokenLocation());
    handleProductionOrFail(WHILE, "while", "end", "do-while loop");
    handleProductionOrFail(OPENPAREN, "(", "start", "do-while loop condition");
    semanticFailIfTrue(match(CLOSEPAREN), "Must provide an expression as a do-while loop condition");
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Unable to parse do-while loop condition");
    handleProductionOrFail(CLOSEPAREN, ")", "end", "do-while loop condition");
    if (match(SEMICOLON))
        next(); // Always performs automatic semicolon insertion.
    return context.createDoWhileStatement(location, statement, expr, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseWhileStatement(TreeBuilder& context)
{
    ASSERT(match(WHILE));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    next();
    
    handleProductionOrFail(OPENPAREN, "(", "start", "while loop condition");
    semanticFailIfTrue(match(CLOSEPAREN), "Must provide an expression as a while loop condition");
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Unable to parse while loop condition");
    int endLine = tokenLine();
    handleProductionOrFail(CLOSEPAREN, ")", "end", "while loop condition");
    
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement, "Expected a statement as the body of a while loop");
    return context.createWhileStatement(location, expr, statement, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseVarDeclarationList(TreeBuilder& context, int& declarations, TreeDestructuringPattern& lastPattern, TreeExpression& lastInitializer, JSTextPosition& identStart, JSTextPosition& initStart, JSTextPosition& initEnd, VarDeclarationListContext declarationListContext)
{
    TreeExpression head = 0;
    TreeExpression tail = 0;
    const Identifier* lastIdent;
    JSToken lastIdentToken; 
    do {
        lastIdent = 0;
        lastPattern = TreeDestructuringPattern(0);
        JSTokenLocation location(tokenLocation());
        next();
        TreeExpression node = 0;
        declarations++;
        bool hasInitializer = false;
        if (match(IDENT)) {
            JSTextPosition varStart = tokenStartPosition();
            JSTokenLocation varStartLocation(tokenLocation());
            identStart = varStart;
            const Identifier* name = m_token.m_data.ident;
            lastIdent = name;
            lastIdentToken = m_token;
            next();
            hasInitializer = match(EQUAL);
            failIfFalseIfStrict(declareVariable(name), "Cannot declare a variable named ", name->impl(), " in strict mode");
            context.addVar(name, (hasInitializer || (!m_allowsIn && (match(INTOKEN) || isofToken()))) ? DeclarationStacks::HasInitializer : 0);
            if (hasInitializer) {
                JSTextPosition varDivot = tokenStartPosition() + 1;
                initStart = tokenStartPosition();
                next(TreeBuilder::DontBuildStrings); // consume '='
                TreeExpression initializer = parseAssignmentExpression(context);
                initEnd = lastTokenEndPosition();
                lastInitializer = initializer;
                failIfFalse(initializer, "Expected expression as the intializer for the variable '", name->impl(), "'");
                
                node = context.createAssignResolve(location, *name, initializer, varStart, varDivot, lastTokenEndPosition());
            } else
                node = context.createEmptyVarExpression(varStartLocation, *name);
        } else {
            lastIdent = 0;
            auto pattern = parseDestructuringPattern(context, DestructureToVariables);
            failIfFalse(pattern, "Cannot parse this destructuring pattern");
            hasInitializer = match(EQUAL);
            failIfTrue(declarationListContext == VarDeclarationContext && !hasInitializer, "Expected an initializer in destructuring variable declaration");
            lastPattern = pattern;
            if (hasInitializer) {
                next(TreeBuilder::DontBuildStrings); // consume '='
                TreeExpression rhs = parseAssignmentExpression(context);
                node = context.createDestructuringAssignment(location, pattern, rhs);
                lastInitializer = rhs;
            }
        }
        
        if (!head)
            head = node;
        else if (!tail) {
            head = context.createCommaExpr(location, head);
            tail = context.appendToCommaExpr(location, head, head, node);
        } else
            tail = context.appendToCommaExpr(location, head, tail, node);
    } while (match(COMMA));
    if (lastIdent)
        lastPattern = createBindingPattern(context, DestructureToVariables, *lastIdent, 0, lastIdentToken);
    return head;
}

template <typename LexerType>
template <class TreeBuilder> TreeDestructuringPattern Parser<LexerType>::createBindingPattern(TreeBuilder& context, DestructuringKind kind, const Identifier& name, int depth, JSToken token)
{
    ASSERT(!name.isNull());

#if !PLATFORM(WKC)
    ASSERT(name.impl()->isAtomic() || name.impl()->isSymbol());
#endif
    if (depth) {
        if (kind == DestructureToVariables)
            failIfFalseIfStrict(declareVariable(&name), "Cannot destructure to a variable named '", name.impl(), "' in strict mode");
        if (kind == DestructureToParameters) {
            auto bindingResult = declareBoundParameter(&name);
            if (bindingResult == Scope::StrictBindingFailed && strictMode()) {
                semanticFailIfTrue(m_vm->propertyNames->arguments == name || m_vm->propertyNames->eval == name, "Cannot destructure to a parameter name '", name.impl(), "' in strict mode");
                if (m_lastFunctionName && name == *m_lastFunctionName)
                    semanticFail("Cannot destructure to '", name.impl(), "' as it shadows the name of a strict mode function");
                semanticFailureDueToKeyword("bound parameter name");
                if (hasDeclaredParameter(name))
                    semanticFail("Cannot destructure to '", name.impl(), "' as it has already been declared");
                semanticFail("Cannot bind to a parameter named '", name.impl(), "' in strict mode");
            }
            if (bindingResult == Scope::BindingFailed) {
                semanticFailureDueToKeyword("bound parameter name");
                if (hasDeclaredParameter(name))
                    semanticFail("Cannot destructure to '", name.impl(), "' as it has already been declared");
                semanticFail("Cannot destructure to a parameter named '", name.impl(), "'");
            }
        }
        if (kind != DestructureToExpressions)
            context.addVar(&name, DeclarationStacks::HasInitializer);

    } else {
        if (kind == DestructureToVariables) {
            failIfFalseIfStrict(declareVariable(&name), "Cannot declare a variable named '", name.impl(), "' in strict mode");
            context.addVar(&name, DeclarationStacks::HasInitializer);
        }
        
        if (kind == DestructureToParameters) {
            bool declarationResult = declareParameter(&name);
            if (!declarationResult && strictMode()) {
                semanticFailIfTrue(m_vm->propertyNames->arguments == name || m_vm->propertyNames->eval == name, "Cannot destructure to a parameter name '", name.impl(), "' in strict mode");
                if (m_lastFunctionName && name == *m_lastFunctionName)
                    semanticFail("Cannot declare a parameter named '", name.impl(), "' as it shadows the name of a strict mode function");
                semanticFailureDueToKeyword("parameter name");
                if (hasDeclaredParameter(name))
                    semanticFail("Cannot declare a parameter named '", name.impl(), "' in strict mode as it has already been declared");
                semanticFail("Cannot declare a parameter named '", name.impl(), "' in strict mode");
            }
        }
    }
    return context.createBindingLocation(token.m_location, name, token.m_startPosition, token.m_endPosition);
}

template <typename LexerType>
template <class TreeBuilder> TreeSourceElements Parser<LexerType>::parseArrowFunctionSingleExpressionBodySourceElements(TreeBuilder& context)
{
    ASSERT(!match(OPENBRACE));

    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();

    failIfStackOverflow();
    TreeExpression expr = parseAssignmentExpression(context);
    failIfFalse(expr, "Cannot parse the arrow function expression");
    
    context.setEndOffset(expr, m_lastTokenEndPosition.offset);

    failIfFalse(isEndOfArrowFunction(), "Expected a ';', ']', '}', ')', ',', line terminator or EOF following a arrow function statement");

    JSTextPosition end = tokenEndPosition();
    
    if (!m_lexer->prevTerminator())
        setEndOfStatement();

    TreeSourceElements sourceElements = context.createSourceElements();
    TreeStatement body = context.createReturnStatement(location, expr, start, end);
    context.setEndOffset(body, m_lastTokenEndPosition.offset);
    context.appendStatement(sourceElements, body);

    return sourceElements;
}

template <typename LexerType>
template <class TreeBuilder> TreeDestructuringPattern Parser<LexerType>::tryParseDestructuringPatternExpression(TreeBuilder& context)
{
    return parseDestructuringPattern(context, DestructureToExpressions);
}

template <typename LexerType>
template <class TreeBuilder> TreeDestructuringPattern Parser<LexerType>::parseDestructuringPattern(TreeBuilder& context, DestructuringKind kind, int depth)
{
    failIfStackOverflow();
    int nonLHSCount = m_nonLHSCount;
    TreeDestructuringPattern pattern;
    switch (m_token.m_type) {
    case OPENBRACKET: {
        JSTextPosition divotStart = tokenStartPosition();
        auto arrayPattern = context.createArrayPattern(m_token.m_location);
        next();

        bool restElementWasFound = false;

        do {
            while (match(COMMA)) {
                context.appendArrayPatternSkipEntry(arrayPattern, m_token.m_location);
                next();
            }
            propagateError();

            if (match(CLOSEBRACKET))
                break;

            if (UNLIKELY(match(DOTDOTDOT))) {
                JSTokenLocation location = m_token.m_location;
                next();
                auto innerPattern = parseDestructuringPattern(context, kind, depth + 1);
                if (kind == DestructureToExpressions && !innerPattern)
                    return 0;
                failIfFalse(innerPattern, "Cannot parse this destructuring pattern");

                failIfTrue(kind != DestructureToExpressions && !context.isBindingNode(innerPattern),  "Expected identifier for a rest element destructuring pattern");

                context.appendArrayPatternRestEntry(arrayPattern, location, innerPattern);
                restElementWasFound = true;
                break;
            }

            JSTokenLocation location = m_token.m_location;
            auto innerPattern = parseDestructuringPattern(context, kind, depth + 1);
            if (kind == DestructureToExpressions && !innerPattern)
                return 0;
            failIfFalse(innerPattern, "Cannot parse this destructuring pattern");
            TreeExpression defaultValue = parseDefaultValueForDestructuringPattern(context);
            failIfTrue(kind == DestructureToParameters && defaultValue,  "Default values in destructuring parameters are currently not supported");
            context.appendArrayPatternEntry(arrayPattern, location, innerPattern, defaultValue);
        } while (consume(COMMA));

        if (kind == DestructureToExpressions && !match(CLOSEBRACKET))
            return 0;
        consumeOrFail(CLOSEBRACKET, restElementWasFound ? "Expected a closing ']' following a rest element destructuring pattern" : "Expected either a closing ']' or a ',' following an element destructuring pattern");
        context.finishArrayPattern(arrayPattern, divotStart, divotStart, lastTokenEndPosition());
        pattern = arrayPattern;
        break;
    }
    case OPENBRACE: {
        auto objectPattern = context.createObjectPattern(m_token.m_location);
        next();

        do {
            bool wasString = false;

            if (match(CLOSEBRACE))
                break;

            Identifier propertyName;
            TreeDestructuringPattern innerPattern = 0;
            JSTokenLocation location = m_token.m_location;
            if (match(IDENT)) {
                propertyName = *m_token.m_data.ident;
                JSToken identifierToken = m_token;
                next();
                if (consume(COLON))
                    innerPattern = parseDestructuringPattern(context, kind, depth + 1);
                else
                    innerPattern = createBindingPattern(context, kind, propertyName, depth, identifierToken);
            } else {
                JSTokenType tokenType = m_token.m_type;
                switch (m_token.m_type) {
                case DOUBLE:
                case INTEGER:
                    propertyName = Identifier::from(m_vm, m_token.m_data.doubleValue);
                    break;
                case STRING:
                    propertyName = *m_token.m_data.ident;
                    wasString = true;
                    break;
                default:
                    if (m_token.m_type != RESERVED && m_token.m_type != RESERVED_IF_STRICT && !(m_token.m_type & KeywordTokenFlag)) {
                        if (kind == DestructureToExpressions)
                            return 0;
                        failWithMessage("Expected a property name");
                    }
                    propertyName = *m_token.m_data.ident;
                    break;
                }
                next();
                if (!consume(COLON)) {
                    if (kind == DestructureToExpressions)
                        return 0;
                    semanticFailIfTrue(tokenType == RESERVED, "Cannot use abbreviated destructuring syntax for reserved name '", propertyName.impl(), "'");
                    semanticFailIfTrue(tokenType == RESERVED_IF_STRICT, "Cannot use abbreviated destructuring syntax for reserved name '", propertyName.impl(), "' in strict mode");
                    semanticFailIfTrue(tokenType & KeywordTokenFlag, "Cannot use abbreviated destructuring syntax for keyword '", propertyName.impl(), "'");
                    
                    failWithMessage("Expected a ':' prior to a named destructuring property");
                }
                innerPattern = parseDestructuringPattern(context, kind, depth + 1);
            }
            if (kind == DestructureToExpressions && !innerPattern)
                return 0;
            failIfFalse(innerPattern, "Cannot parse this destructuring pattern");
            TreeExpression defaultValue = parseDefaultValueForDestructuringPattern(context);
            failIfTrue(kind == DestructureToParameters && defaultValue, "Default values in destructuring parameters are currently not supported");
            context.appendObjectPatternEntry(objectPattern, location, wasString, propertyName, innerPattern, defaultValue);
        } while (consume(COMMA));

        if (kind == DestructureToExpressions && !match(CLOSEBRACE))
            return 0;
        consumeOrFail(CLOSEBRACE, "Expected either a closing '}' or an ',' after a property destructuring pattern");
        pattern = objectPattern;
        break;
    }

    default: {
        if (!match(IDENT)) {
            if (kind == DestructureToExpressions)
                return 0;
            semanticFailureDueToKeyword("variable name");
            failWithMessage("Expected a parameter pattern or a ')' in parameter list");
        }
        pattern = createBindingPattern(context, kind, *m_token.m_data.ident, depth, m_token);
        next();
        break;
    }
    }
    m_nonLHSCount = nonLHSCount;
    return pattern;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseDefaultValueForDestructuringPattern(TreeBuilder& context)
{
    if (!match(EQUAL))
        return 0;

    next(TreeBuilder::DontBuildStrings); // consume '='
    return parseAssignmentExpression(context);
}

template <typename LexerType>
template <class TreeBuilder> TreeConstDeclList Parser<LexerType>::parseConstDeclarationList(TreeBuilder& context)
{
    failIfTrue(strictMode(), "Const declarations are not supported in strict mode");
    TreeConstDeclList constDecls = 0;
    TreeConstDeclList tail = 0;
    do {
        JSTokenLocation location(tokenLocation());
        next();
        matchOrFail(IDENT, "Expected an identifier name in const declaration");
        const Identifier* name = m_token.m_data.ident;
        next();
        bool hasInitializer = match(EQUAL);
        declareVariable(name);
        context.addVar(name, DeclarationStacks::IsConstant | (hasInitializer ? DeclarationStacks::HasInitializer : 0));

        TreeExpression initializer = 0;
        if (hasInitializer) {
            next(TreeBuilder::DontBuildStrings); // consume '='
            initializer = parseAssignmentExpression(context);
            failIfFalse(!!initializer, "Unable to parse initializer");
        }
        tail = context.appendConstDecl(location, tail, name, initializer);
        if (!constDecls)
            constDecls = tail;
    } while (match(COMMA));
    return constDecls;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseForStatement(TreeBuilder& context)
{
    ASSERT(match(FOR));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    next();
    handleProductionOrFail(OPENPAREN, "(", "start", "for-loop header");
    int nonLHSCount = m_nonLHSCount;
    int declarations = 0;
    JSTextPosition declsStart;
    JSTextPosition declsEnd;
    TreeExpression decls = 0;
    TreeDestructuringPattern pattern = 0;
    if (match(VAR)) {
        /*
         for (var IDENT in expression) statement
         for (var varDeclarationList; expressionOpt; expressionOpt)
         */
        TreeDestructuringPattern forInTarget = 0;
        TreeExpression forInInitializer = 0;
        m_allowsIn = false;
        JSTextPosition initStart;
        JSTextPosition initEnd;
        decls = parseVarDeclarationList(context, declarations, forInTarget, forInInitializer, declsStart, initStart, initEnd, ForLoopContext);
        m_allowsIn = true;
        propagateError();

        // Remainder of a standard for loop is handled identically
        if (match(SEMICOLON))
            goto standardForLoop;
        
        failIfFalse(declarations == 1, "can only declare a single variable in an enumeration");
        failIfTrueIfStrict(forInInitializer, "Cannot use initialiser syntax in a strict mode enumeration");

        if (forInInitializer)
            failIfFalse(context.isBindingNode(forInTarget), "Cannot use initialiser syntax when binding to a pattern during enumeration");

        // Handle for-in with var declaration
        JSTextPosition inLocation = tokenStartPosition();
        bool isOfEnumeration = false;
        if (!consume(INTOKEN)) {
            failIfFalse(match(IDENT) && *m_token.m_data.ident == m_vm->propertyNames->of, "Expected either 'in' or 'of' in enumeration syntax");
            isOfEnumeration = true;
            failIfTrue(forInInitializer, "Cannot use initialiser syntax in a for-of enumeration");
            next();
        }
        TreeExpression expr = parseExpression(context);
        failIfFalse(expr, "Expected expression to enumerate");
        JSTextPosition exprEnd = lastTokenEndPosition();
        
        int endLine = tokenLine();
        
        handleProductionOrFail(CLOSEPAREN, ")", "end", (isOfEnumeration ? "for-of header" : "for-in header"));
        
        const Identifier* unused = 0;
        startLoop();
        TreeStatement statement = parseStatement(context, unused);
        endLoop();
        failIfFalse(statement, "Expected statement as body of for-", isOfEnumeration ? "of" : "in", " statement");
        if (isOfEnumeration)
            return context.createForOfLoop(location, forInTarget, expr, statement, declsStart, inLocation, exprEnd, startLine, endLine);
        return context.createForInLoop(location, forInTarget, expr, statement, declsStart, inLocation, exprEnd, startLine, endLine);
    }
    
    if (!match(SEMICOLON)) {
        if (match(OPENBRACE) || match(OPENBRACKET)) {
            SavePoint savePoint = createSavePoint();
            declsStart = tokenStartPosition();
            pattern = tryParseDestructuringPatternExpression(context);
            declsEnd = lastTokenEndPosition();
            if (pattern && (match(INTOKEN) || (match(IDENT) && *m_token.m_data.ident == m_vm->propertyNames->of)))
                goto enumerationLoop;
            pattern = TreeDestructuringPattern(0);
            restoreSavePoint(savePoint);
        }
        m_allowsIn = false;
        declsStart = tokenStartPosition();
        decls = parseExpression(context);
        declsEnd = lastTokenEndPosition();
        m_allowsIn = true;
        failIfFalse(decls, "Cannot parse for loop declarations");
    }
    
    if (match(SEMICOLON)) {
    standardForLoop:
        // Standard for loop
        next();
        TreeExpression condition = 0;
        
        if (!match(SEMICOLON)) {
            condition = parseExpression(context);
            failIfFalse(condition, "Cannot parse for loop condition expression");
        }
        consumeOrFail(SEMICOLON, "Expected a ';' after the for loop condition expression");
        
        TreeExpression increment = 0;
        if (!match(CLOSEPAREN)) {
            increment = parseExpression(context);
            failIfFalse(increment, "Cannot parse for loop iteration expression");
        }
        int endLine = tokenLine();
        handleProductionOrFail(CLOSEPAREN, ")", "end", "for-loop header");
        const Identifier* unused = 0;
        startLoop();
        TreeStatement statement = parseStatement(context, unused);
        endLoop();
        failIfFalse(statement, "Expected a statement as the body of a for loop");
        return context.createForLoop(location, decls, condition, increment, statement, startLine, endLine);
    }
    
    // For-in loop
enumerationLoop:
    failIfFalse(nonLHSCount == m_nonLHSCount, "Expected a reference on the left hand side of an enumeration statement");
    bool isOfEnumeration = false;
    if (!consume(INTOKEN)) {
        failIfFalse(match(IDENT) && *m_token.m_data.ident == m_vm->propertyNames->of, "Expected either 'in' or 'of' in enumeration syntax");
        isOfEnumeration = true;
        next();
    }
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Cannot parse subject for-", isOfEnumeration ? "of" : "in", " statement");
    JSTextPosition exprEnd = lastTokenEndPosition();
    int endLine = tokenLine();
    
    handleProductionOrFail(CLOSEPAREN, ")", "end", (isOfEnumeration ? "for-of header" : "for-in header"));
    const Identifier* unused = 0;
    startLoop();
    TreeStatement statement = parseStatement(context, unused);
    endLoop();
    failIfFalse(statement, "Expected a statement as the body of a for-", isOfEnumeration ? "of" : "in", "loop");
    if (pattern) {
        ASSERT(!decls);
        if (isOfEnumeration)
            return context.createForOfLoop(location, pattern, expr, statement, declsStart, declsEnd, exprEnd, startLine, endLine);
        return context.createForInLoop(location, pattern, expr, statement, declsStart, declsEnd, exprEnd, startLine, endLine);
    }
    if (isOfEnumeration)
        return context.createForOfLoop(location, decls, expr, statement, declsStart, declsEnd, exprEnd, startLine, endLine);
    return context.createForInLoop(location, decls, expr, statement, declsStart, declsEnd, exprEnd, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseBreakStatement(TreeBuilder& context)
{
    ASSERT(match(BREAK));
    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();
    JSTextPosition end = tokenEndPosition();
    next();
    
    if (autoSemiColon()) {
        semanticFailIfFalse(breakIsValid(), "'break' is only valid inside a switch or loop statement");
        return context.createBreakStatement(location, &m_vm->propertyNames->nullIdentifier, start, end);
    }
    matchOrFail(IDENT, "Expected an identifier as the target for a break statement");
    const Identifier* ident = m_token.m_data.ident;
    semanticFailIfFalse(getLabel(ident), "Cannot use the undeclared label '", ident->impl(), "'");
    end = tokenEndPosition();
    next();
    failIfFalse(autoSemiColon(), "Expected a ';' following a targeted break statement");
    return context.createBreakStatement(location, ident, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseContinueStatement(TreeBuilder& context)
{
    ASSERT(match(CONTINUE));
    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();
    JSTextPosition end = tokenEndPosition();
    next();
    
    if (autoSemiColon()) {
        semanticFailIfFalse(continueIsValid(), "'continue' is only valid inside a loop statement");
        return context.createContinueStatement(location, &m_vm->propertyNames->nullIdentifier, start, end);
    }
    matchOrFail(IDENT, "Expected an identifier as the target for a continue statement");
    const Identifier* ident = m_token.m_data.ident;
    ScopeLabelInfo* label = getLabel(ident);
    semanticFailIfFalse(label, "Cannot use the undeclared label '", ident->impl(), "'");
    semanticFailIfFalse(label->isLoop, "Cannot continue to the label '", ident->impl(), "' as it is not targeting a loop");
    end = tokenEndPosition();
    next();
    failIfFalse(autoSemiColon(), "Expected a ';' following a targeted continue statement");
    return context.createContinueStatement(location, ident, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseReturnStatement(TreeBuilder& context)
{
    ASSERT(match(RETURN));
    JSTokenLocation location(tokenLocation());
    semanticFailIfFalse(currentScope()->isFunction(), "Return statements are only valid inside functions");
    JSTextPosition start = tokenStartPosition();
    JSTextPosition end = tokenEndPosition();
    next();
    // We do the auto semicolon check before attempting to parse expression
    // as we need to ensure the a line break after the return correctly terminates
    // the statement
    if (match(SEMICOLON))
        end = tokenEndPosition();

    if (autoSemiColon())
        return context.createReturnStatement(location, 0, start, end);
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Cannot parse the return expression");
    end = lastTokenEndPosition();
    if (match(SEMICOLON))
        end  = tokenEndPosition();
    if (!autoSemiColon())
        failWithMessage("Expected a ';' following a return statement");
    return context.createReturnStatement(location, expr, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseThrowStatement(TreeBuilder& context)
{
    ASSERT(match(THROW));
    JSTokenLocation location(tokenLocation());
    JSTextPosition start = tokenStartPosition();
    next();
    failIfTrue(match(SEMICOLON), "Expected expression after 'throw'");
    semanticFailIfTrue(autoSemiColon(), "Cannot have a newline after 'throw'");
    
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Cannot parse expression for throw statement");
    JSTextPosition end = lastTokenEndPosition();
    failIfFalse(autoSemiColon(), "Expected a ';' after a throw statement");
    
    return context.createThrowStatement(location, expr, start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseWithStatement(TreeBuilder& context)
{
    ASSERT(match(WITH));
    JSTokenLocation location(tokenLocation());
    semanticFailIfTrue(strictMode(), "'with' statements are not valid in strict mode");
    currentScope()->setNeedsFullActivation();
    int startLine = tokenLine();
    next();

    handleProductionOrFail(OPENPAREN, "(", "start", "subject of a 'with' statement");
    int start = tokenStart();
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Cannot parse 'with' subject expression");
    JSTextPosition end = lastTokenEndPosition();
    int endLine = tokenLine();
    handleProductionOrFail(CLOSEPAREN, ")", "start", "subject of a 'with' statement");
    const Identifier* unused = 0;
    TreeStatement statement = parseStatement(context, unused);
    failIfFalse(statement, "A 'with' statement must have a body");
    
    return context.createWithStatement(location, expr, statement, start, end, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseSwitchStatement(TreeBuilder& context)
{
    ASSERT(match(SWITCH));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    next();
    handleProductionOrFail(OPENPAREN, "(", "start", "subject of a 'switch'");
    TreeExpression expr = parseExpression(context);
    failIfFalse(expr, "Cannot parse switch subject expression");
    int endLine = tokenLine();
    
    handleProductionOrFail(CLOSEPAREN, ")", "end", "subject of a 'switch'");
    handleProductionOrFail(OPENBRACE, "{", "start", "body of a 'switch'");
    startSwitch();
    TreeClauseList firstClauses = parseSwitchClauses(context);
    propagateError();
    
    TreeClause defaultClause = parseSwitchDefaultClause(context);
    propagateError();
    
    TreeClauseList secondClauses = parseSwitchClauses(context);
    propagateError();
    endSwitch();
    handleProductionOrFail(CLOSEBRACE, "}", "end", "body of a 'switch'");
    
    return context.createSwitchStatement(location, expr, firstClauses, defaultClause, secondClauses, startLine, endLine);
    
}

template <typename LexerType>
template <class TreeBuilder> TreeClauseList Parser<LexerType>::parseSwitchClauses(TreeBuilder& context)
{
    if (!match(CASE))
        return 0;
    unsigned startOffset = tokenStart();
    next();
    TreeExpression condition = parseExpression(context);
    failIfFalse(condition, "Cannot parse switch clause");
    consumeOrFail(COLON, "Expected a ':' after switch clause expression");
    TreeSourceElements statements = parseSourceElements(context, DontCheckForStrictMode);
    failIfFalse(statements, "Cannot parse the body of a switch clause");
    TreeClause clause = context.createClause(condition, statements);
    context.setStartOffset(clause, startOffset);
    TreeClauseList clauseList = context.createClauseList(clause);
    TreeClauseList tail = clauseList;
    
    while (match(CASE)) {
        startOffset = tokenStart();
        next();
        TreeExpression condition = parseExpression(context);
        failIfFalse(condition, "Cannot parse switch case expression");
        consumeOrFail(COLON, "Expected a ':' after switch clause expression");
        TreeSourceElements statements = parseSourceElements(context, DontCheckForStrictMode);
        failIfFalse(statements, "Cannot parse the body of a switch clause");
        clause = context.createClause(condition, statements);
        context.setStartOffset(clause, startOffset);
        tail = context.createClauseList(tail, clause);
    }
    return clauseList;
}

template <typename LexerType>
template <class TreeBuilder> TreeClause Parser<LexerType>::parseSwitchDefaultClause(TreeBuilder& context)
{
    if (!match(DEFAULT))
        return 0;
    unsigned startOffset = tokenStart();
    next();
    consumeOrFail(COLON, "Expected a ':' after switch default clause");
    TreeSourceElements statements = parseSourceElements(context, DontCheckForStrictMode);
    failIfFalse(statements, "Cannot parse the body of a switch default clause");
    TreeClause result = context.createClause(0, statements);
    context.setStartOffset(result, startOffset);
    return result;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseTryStatement(TreeBuilder& context)
{
    ASSERT(match(TRY));
    JSTokenLocation location(tokenLocation());
    TreeStatement tryBlock = 0;
    const Identifier* ident = &m_vm->propertyNames->nullIdentifier;
    TreeStatement catchBlock = 0;
    TreeStatement finallyBlock = 0;
    int firstLine = tokenLine();
    next();
    matchOrFail(OPENBRACE, "Expected a block statement as body of a try statement");
    
    tryBlock = parseBlockStatement(context);
    failIfFalse(tryBlock, "Cannot parse the body of try block");
    int lastLine = m_lastTokenEndPosition.line;
    
    if (match(CATCH)) {
        currentScope()->setNeedsFullActivation();
        next();
        
        handleProductionOrFail(OPENPAREN, "(", "start", "'catch' target");
        if (!match(IDENT)) {
            semanticFailureDueToKeyword("catch variable name");
            failWithMessage("Expected identifier name as catch target");
        }
        ident = m_token.m_data.ident;
        next();
        AutoPopScopeRef catchScope(this, pushScope());
        failIfFalseIfStrict(declareVariable(ident), "Cannot declare a catch variable named '", ident->impl(), "' in strict mode");
        catchScope->preventNewDecls();
        handleProductionOrFail(CLOSEPAREN, ")", "end", "'catch' target");
        matchOrFail(OPENBRACE, "Expected exception handler to be a block statement");
        catchBlock = parseBlockStatement(context);
        failIfFalse(catchBlock, "Unable to parse 'catch' block");
        failIfFalse(popScope(catchScope, TreeBuilder::NeedsFreeVariableInfo), "Parse error");
    }
    
    if (match(FINALLY)) {
        next();
        matchOrFail(OPENBRACE, "Expected block statement for finally body");
        finallyBlock = parseBlockStatement(context);
        failIfFalse(finallyBlock, "Cannot parse finally body");
    }
    failIfFalse(catchBlock || finallyBlock, "Try statements must have at least a catch or finally block");
    return context.createTryStatement(location, tryBlock, ident, catchBlock, finallyBlock, firstLine, lastLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseDebuggerStatement(TreeBuilder& context)
{
    ASSERT(match(DEBUGGER));
    JSTokenLocation location(tokenLocation());
    int startLine = tokenLine();
    int endLine = startLine;
    next();
    if (match(SEMICOLON))
        startLine = tokenLine();
    failIfFalse(autoSemiColon(), "Debugger keyword must be followed by a ';'");
    return context.createDebugger(location, startLine, endLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseBlockStatement(TreeBuilder& context)
{
    ASSERT(match(OPENBRACE));
    JSTokenLocation location(tokenLocation());
    int startOffset = m_token.m_data.offset;
    int start = tokenLine();
    next();
    if (match(CLOSEBRACE)) {
        int endOffset = m_token.m_data.offset;
        next();
        TreeStatement result = context.createBlockStatement(location, 0, start, m_lastTokenEndPosition.line);
        context.setStartOffset(result, startOffset);
        context.setEndOffset(result, endOffset);
        return result;
    }
    TreeSourceElements subtree = parseSourceElements(context, DontCheckForStrictMode);
    failIfFalse(subtree, "Cannot parse the body of the block statement");
    matchOrFail(CLOSEBRACE, "Expected a closing '}' at the end of a block statement");
    int endOffset = m_token.m_data.offset;
    next();
    TreeStatement result = context.createBlockStatement(location, subtree, start, m_lastTokenEndPosition.line);
    context.setStartOffset(result, startOffset);
    context.setEndOffset(result, endOffset);
    return result;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseStatement(TreeBuilder& context, const Identifier*& directive, unsigned* directiveLiteralLength)
{
    DepthManager statementDepth(&m_statementDepth);
    m_statementDepth++;
    directive = 0;
    int nonTrivialExpressionCount = 0;
    failIfStackOverflow();
    TreeStatement result = 0;
    bool shouldSetEndOffset = true;

    switch (m_token.m_type) {
    case OPENBRACE:
        result = parseBlockStatement(context);
        shouldSetEndOffset = false;
        break;
    case VAR:
        result = parseVarDeclaration(context);
        break;
    case FUNCTION:
        failIfFalseIfStrict(m_statementDepth == 1, "Strict mode does not allow function declarations in a lexically nested statement");
        result = parseFunctionDeclaration(context);
        break;
    case SEMICOLON: {
        JSTokenLocation location(tokenLocation());
        next();
        result = context.createEmptyStatement(location);
        break;
    }
    case IF:
        result = parseIfStatement(context);
        break;
    case DO:
        result = parseDoWhileStatement(context);
        break;
    case WHILE:
        result = parseWhileStatement(context);
        break;
    case FOR:
        result = parseForStatement(context);
        break;
    case CONTINUE:
        result = parseContinueStatement(context);
        break;
    case BREAK:
        result = parseBreakStatement(context);
        break;
    case RETURN:
        result = parseReturnStatement(context);
        break;
    case WITH:
        result = parseWithStatement(context);
        break;
    case SWITCH:
        result = parseSwitchStatement(context);
        break;
    case THROW:
        result = parseThrowStatement(context);
        break;
    case TRY:
        result = parseTryStatement(context);
        break;
    case DEBUGGER:
        result = parseDebuggerStatement(context);
        break;
    case EOFTOK:
    case CASE:
    case CLOSEBRACE:
    case DEFAULT:
        // These tokens imply the end of a set of source elements
        return 0;
    case IDENT:
        result = parseExpressionOrLabelStatement(context);
        break;
    case STRING:
        directive = m_token.m_data.ident;
        if (directiveLiteralLength)
            *directiveLiteralLength = m_token.m_location.endOffset - m_token.m_location.startOffset;
        nonTrivialExpressionCount = m_nonTrivialExpressionCount;
        FALLTHROUGH;
    default:
        TreeStatement exprStatement = parseExpressionStatement(context);
        if (directive && nonTrivialExpressionCount != m_nonTrivialExpressionCount)
            directive = 0;
        result = exprStatement;
        break;
    }

    if (result && shouldSetEndOffset)
        context.setEndOffset(result, m_lastTokenEndPosition.offset);
    return result;
}

template <typename LexerType>
template <class TreeBuilder> bool Parser<LexerType>::parseFormalParameters(TreeBuilder& context, TreeFormalParameterList list, unsigned& parameterCount)
{
    auto parameter = parseDestructuringPattern(context, DestructureToParameters);
    failIfFalse(parameter, "Cannot parse parameter pattern");
    context.appendParameter(list, parameter);
    parameterCount++;
    while (consume(COMMA)) {
        parameter = parseDestructuringPattern(context, DestructureToParameters);
        failIfFalse(parameter, "Cannot parse parameter pattern");
        context.appendParameter(list, parameter);
        parameterCount++;
    }
    return true;
}

template <typename LexerType>
template <class TreeBuilder> TreeFunctionBody Parser<LexerType>::parseFunctionBody(
    TreeBuilder& context, const JSTokenLocation& startLocation, int startColumn, int functionKeywordStart, int functionNameStart, int parametersStart, 
    ConstructorKind constructorKind, FunctionBodyType bodyType, unsigned parameterCount, FunctionParseMode parseMode)
{
    bool isArrowFunction = FunctionBodyType::StandardFunctionBodyBlock != bodyType;
    if (bodyType == StandardFunctionBodyBlock || bodyType == ArrowFunctionBodyBlock) {
        next();
        if (match(CLOSEBRACE)) {
            unsigned endColumn = tokenColumn();
            return context.createFunctionBody(startLocation, tokenLocation(), startColumn, endColumn, functionKeywordStart, functionNameStart, parametersStart, strictMode(), constructorKind, parameterCount, parseMode, isArrowFunction);
        }
    }

    DepthManager statementDepth(&m_statementDepth);
    m_statementDepth = 0;
    SyntaxChecker syntaxChecker(const_cast<VM*>(m_vm), m_lexer.get());
    if (bodyType == ArrowFunctionBodyExpression)
        failIfFalse(parseArrowFunctionSingleExpressionBodySourceElements(syntaxChecker), "Cannot parse body of this arrow function");
    else
        failIfFalse(parseSourceElements(syntaxChecker, CheckForStrictMode), bodyType == StandardFunctionBodyBlock ? "Cannot parse body of this function" : "Cannot parse body of this arrow function");
    unsigned endColumn = tokenColumn();
    return context.createFunctionBody(startLocation, tokenLocation(), startColumn, endColumn, functionKeywordStart, functionNameStart, parametersStart, strictMode(), constructorKind, parameterCount, parseMode, isArrowFunction);
}

static const char* stringForFunctionMode(FunctionParseMode mode)
{
    switch (mode) {
    case GetterMode:
        return "getter";
    case SetterMode:
        return "setter";
    case NormalFunctionMode:
        return "function";
    case MethodMode:
        return "method";
    case ArrowFunctionMode:
        return "arrow function";
    case NotAFunctionMode:
        RELEASE_ASSERT_NOT_REACHED();
        return "";
    }
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

template <typename LexerType> template <class TreeBuilder> int Parser<LexerType>::parseFunctionParameters(TreeBuilder& context, FunctionParseMode mode, ParserFunctionInfo<TreeBuilder>& functionInfo)
{
    RELEASE_ASSERT(mode != NotAFunctionMode);
    int parametersStart = m_token.m_location.startOffset;
    TreeFormalParameterList parameterList = context.createFormalParameterList();
    functionInfo.parameters = parameterList;
    functionInfo.startOffset = parametersStart;
    
    if (mode == ArrowFunctionMode) {
        if (!match(IDENT) && !match(OPENPAREN)) {
            semanticFailureDueToKeyword(stringForFunctionMode(mode), " name");
            failWithMessage("Expected an arrow function input parameter");
        } else {
            if (match(OPENPAREN)) {
                next();
                
                if (match(CLOSEPAREN))
                    functionInfo.parameterCount = 0;
                else
                    failIfFalse(parseFormalParameters(context, parameterList, functionInfo.parameterCount), "Cannot parse parameters for this ", stringForFunctionMode(mode));
                
                consumeOrFail(CLOSEPAREN, "Expected a ')' or a ',' after a parameter declaration");
            } else {
                functionInfo.parameterCount = 1;
                auto parameter = parseDestructuringPattern(context, DestructureToParameters);
                failIfFalse(parameter, "Cannot parse parameter pattern");
                context.appendParameter(parameterList, parameter);
            }
        }

        return parametersStart;
    }

    if (!consume(OPENPAREN)) {
        semanticFailureDueToKeyword(stringForFunctionMode(mode), " name");
        failWithMessage("Expected an opening '(' before a ", stringForFunctionMode(mode), "'s parameter list");
    }

    if (mode == GetterMode) {
        consumeOrFail(CLOSEPAREN, "getter functions must have no parameters");
        functionInfo.parameterCount = 0;
    } else if (mode == SetterMode) {
        failIfTrue(match(CLOSEPAREN), "setter functions must have one parameter");
        auto parameter = parseDestructuringPattern(context, DestructureToParameters);
        failIfFalse(parameter, "setter functions must have one parameter");
        context.appendParameter(parameterList, parameter);
        functionInfo.parameterCount = 1;
        failIfTrue(match(COMMA), "setter functions must have one parameter");
        consumeOrFail(CLOSEPAREN, "Expected a ')' after a parameter declaration");
    } else {
        if (match(CLOSEPAREN))
            functionInfo.parameterCount = 0;
        else
            failIfFalse(parseFormalParameters(context, parameterList, functionInfo.parameterCount), "Cannot parse parameters for this ", stringForFunctionMode(mode));
        consumeOrFail(CLOSEPAREN, "Expected a ')' or a ',' after a parameter declaration");
    }

    return parametersStart;
}

template <typename LexerType>
template <class TreeBuilder> bool Parser<LexerType>::parseFunctionInfo(TreeBuilder& context, FunctionRequirements requirements, FunctionParseMode mode, bool nameIsInContainingScope, ConstructorKind constructorKind, SuperBinding expectedSuperBinding, int functionKeywordStart, ParserFunctionInfo<TreeBuilder>& functionInfo, FunctionParseType parseType)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    RELEASE_ASSERT(mode != NotAFunctionMode);

    AutoPopScopeRef functionScope(this, pushScope());
    functionScope->setIsFunction();
    int functionNameStart = m_token.m_location.startOffset;
    const Identifier* lastFunctionName = m_lastFunctionName;
    m_lastFunctionName = nullptr;
    int parametersStart;
    JSTokenLocation startLocation;
    int startColumn;
    FunctionBodyType functionBodyType;
    
    switch (parseType) {
    case StandardFunctionParseType: {
        RELEASE_ASSERT(mode != ArrowFunctionMode);
        if (match(IDENT)) {
            functionInfo.name = m_token.m_data.ident;
            m_lastFunctionName = functionInfo.name;
            next();
            if (!nameIsInContainingScope)
                failIfFalseIfStrict(functionScope->declareVariable(functionInfo.name), "'", functionInfo.name->impl(), "' is not a valid ", stringForFunctionMode(mode), " name in strict mode");
        } else if (requirements == FunctionNeedsName) {
            if (match(OPENPAREN) && mode == NormalFunctionMode)
                semanticFail("Function statements must have a name");
            semanticFailureDueToKeyword(stringForFunctionMode(mode), " name");
            failDueToUnexpectedToken();
            return false;
        }

        startLocation = tokenLocation();
        functionInfo.startLine = tokenLine();
        startColumn = tokenColumn();

        parametersStart = parseFunctionParameters(context, mode, functionInfo);
        propagateError();
        
        matchOrFail(OPENBRACE, "Expected an opening '{' at the start of a ", stringForFunctionMode(mode), " body");
        
        // BytecodeGenerator emits code to throw TypeError when a class constructor is "call"ed.
        // Set ConstructorKind to None for non-constructor methods of classes.
    
        if (m_defaultConstructorKind != ConstructorKind::None) {
            constructorKind = m_defaultConstructorKind;
            expectedSuperBinding = m_defaultConstructorKind == ConstructorKind::Derived ? SuperBinding::Needed : SuperBinding::NotNeeded;
        }

        functionBodyType = StandardFunctionBodyBlock;
        
        break;
    }
#if ENABLE(ES6_ARROWFUNCTION_SYNTAX)
    case ArrowFunctionParseType: {
        RELEASE_ASSERT(mode == ArrowFunctionMode);

        startLocation = tokenLocation();
        functionInfo.startLine = tokenLine();
        startColumn = tokenColumn();

        parametersStart = parseFunctionParameters(context, mode, functionInfo);
        propagateError();
        
        matchOrFail(ARROWFUNCTION, "Expected a '=>' after arrow function parameter declaration");
        
        if (m_lexer->prevTerminator())
            failDueToUnexpectedToken();

        ASSERT(constructorKind == ConstructorKind::None);
        
        // Check if arrow body start with {. If it true it mean that arrow function is Fat arrow function
        // and we need use common approach to parse function body
        next();
        functionBodyType = match(OPENBRACE) ? ArrowFunctionBodyBlock : ArrowFunctionBodyExpression;
        
        break;
    }
#else
    default:
        RELEASE_ASSERT_NOT_REACHED();
#endif
    }
    
    functionScope->setConstructorKind(constructorKind);
    functionScope->setExpectedSuperBinding(expectedSuperBinding);

    functionInfo.bodyStartColumn = startColumn;
    
    // If we know about this function already, we can use the cached info and skip the parser to the end of the function.
    if (const SourceProviderCacheItem* cachedInfo = TreeBuilder::CanUseFunctionCache ? findCachedFunctionInfo(functionInfo.startOffset) : 0) {
        // If we're in a strict context, the cached function info must say it was strict too.
        ASSERT(!strictMode() || cachedInfo->strictMode);
        JSTokenLocation endLocation;

        endLocation.line = cachedInfo->lastTockenLine;
        endLocation.startOffset = cachedInfo->lastTockenStartOffset;
        endLocation.lineStartOffset = cachedInfo->lastTockenLineStartOffset;

        bool endColumnIsOnStartLine = (endLocation.line == functionInfo.startLine);
        ASSERT(endLocation.startOffset >= endLocation.lineStartOffset);
        unsigned bodyEndColumn = endColumnIsOnStartLine ?
            endLocation.startOffset - m_token.m_data.lineStartOffset :
            endLocation.startOffset - endLocation.lineStartOffset;
        unsigned currentLineStartOffset = m_token.m_location.lineStartOffset;

        bool isArrowFunction = parseType == ArrowFunctionParseType;

        functionInfo.body = context.createFunctionBody(
            startLocation, endLocation, functionInfo.bodyStartColumn, bodyEndColumn, 
            functionKeywordStart, functionNameStart, parametersStart, 
            cachedInfo->strictMode, constructorKind, cachedInfo->parameterCount, mode, isArrowFunction);
        
        functionScope->restoreFromSourceProviderCache(cachedInfo);
        failIfFalse(popScope(functionScope, TreeBuilder::NeedsFreeVariableInfo), "Parser error");
        
        m_token = cachedInfo->endFunctionToken();
        
        if (endColumnIsOnStartLine)
            m_token.m_location.lineStartOffset = currentLineStartOffset;

        m_lexer->setOffset(m_token.m_location.endOffset, m_token.m_location.lineStartOffset);
        m_lexer->setLineNumber(m_token.m_location.line);
        functionInfo.endOffset = cachedInfo->endFunctionOffset;

        if (isArrowFunction)
            functionBodyType = cachedInfo->isBodyArrowExpression ?  ArrowFunctionBodyExpression : ArrowFunctionBodyBlock;
        else
            functionBodyType = StandardFunctionBodyBlock;
        
        switch (functionBodyType) {
        case ArrowFunctionBodyExpression:
            next();
            context.setEndOffset(functionInfo.body, m_lexer->currentOffset());
            break;
        case ArrowFunctionBodyBlock:
        case StandardFunctionBodyBlock:
            context.setEndOffset(functionInfo.body, m_lexer->currentOffset());
            next();
            break;
        }
        functionInfo.endLine = m_lastTokenEndPosition.line;
        return true;
    }
    
    m_lastFunctionName = lastFunctionName;
    ParserState oldState = saveState();
    
    functionInfo.body = parseFunctionBody(context, startLocation, startColumn, functionKeywordStart, functionNameStart, parametersStart, constructorKind, functionBodyType, functionInfo.parameterCount, mode);
    
    restoreState(oldState);
    failIfFalse(functionInfo.body, "Cannot parse the body of this ", stringForFunctionMode(mode));
    context.setEndOffset(functionInfo.body, m_lexer->currentOffset());
    if (functionScope->strictMode() && functionInfo.name) {
        RELEASE_ASSERT(mode == NormalFunctionMode || mode == MethodMode || mode == ArrowFunctionMode);
        semanticFailIfTrue(m_vm->propertyNames->arguments == *functionInfo.name, "'", functionInfo.name->impl(), "' is not a valid function name in strict mode");
        semanticFailIfTrue(m_vm->propertyNames->eval == *functionInfo.name, "'", functionInfo.name->impl(), "' is not a valid function name in strict mode");
    }

    JSTokenLocation location = JSTokenLocation(m_token.m_location);
    functionInfo.endOffset = m_token.m_data.offset;
    
    if (functionBodyType == ArrowFunctionBodyExpression) {
        location = locationBeforeLastToken();
        functionInfo.endOffset = location.endOffset;
    }
    
    // Cache the tokenizer state and the function scope the first time the function is parsed.
    // Any future reparsing can then skip the function.
    // For arrow function is 8 = x=>x + 4 symbols;
    // For ordinary function is 16  = function(){} + 4 symbols
    const int minimumFunctionLengthToCache = functionBodyType == StandardFunctionBodyBlock ? 16 : 8;
    std::unique_ptr<SourceProviderCacheItem> newInfo;
    int functionLength = functionInfo.endOffset - functionInfo.startOffset;
    if (TreeBuilder::CanUseFunctionCache && m_functionCache && functionLength > minimumFunctionLengthToCache) {
        SourceProviderCacheItemCreationParameters parameters;
        parameters.endFunctionOffset = functionInfo.endOffset;
        parameters.functionNameStart = functionNameStart;
        parameters.lastTockenLine = location.line;
        parameters.lastTockenStartOffset = location.startOffset;
        parameters.lastTockenEndOffset = location.endOffset;
        parameters.lastTockenLineStartOffset = location.lineStartOffset;
        parameters.parameterCount = functionInfo.parameterCount;
        if (functionBodyType == ArrowFunctionBodyExpression) {
            parameters.isBodyArrowExpression = true;
            parameters.tokenType = m_token.m_type;
        }

        functionScope->fillParametersForSourceProviderCache(parameters);
        newInfo = SourceProviderCacheItem::create(parameters);
    }
    
    failIfFalse(popScope(functionScope, TreeBuilder::NeedsFreeVariableInfo), "Parser error");
    
    if (functionBodyType == ArrowFunctionBodyExpression)
        failIfFalse(isEndOfArrowFunction(), "Expected the closing ';' ',' ']' ')' '}', line terminator or EOF after arrow function");
    else {
        matchOrFail(CLOSEBRACE, "Expected a closing '}' after a ", stringForFunctionMode(mode), " body");
        next();
    }
    
    if (newInfo)
        m_functionCache->add(functionInfo.startOffset, WTF::move(newInfo));
    
    functionInfo.endLine = m_lastTokenEndPosition.line;
    return true;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseFunctionDeclaration(TreeBuilder& context)
{
    ASSERT(match(FUNCTION));
    JSTokenLocation location(tokenLocation());
    unsigned functionKeywordStart = tokenStart();
    next();
    ParserFunctionInfo<TreeBuilder> functionInfo;
    failIfFalse((parseFunctionInfo(context, FunctionNeedsName, NormalFunctionMode, true, ConstructorKind::None, SuperBinding::NotNeeded,
        functionKeywordStart, functionInfo, StandardFunctionParseType)), "Cannot parse this function");
    failIfFalse(functionInfo.name, "Function statements must have a name");
    failIfFalseIfStrict(declareVariable(functionInfo.name), "Cannot declare a function named '", functionInfo.name->impl(), "' in strict mode");
    return context.createFuncDeclStatement(location, functionInfo);
}

#if ENABLE(ES6_CLASS_SYNTAX)
template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseClassDeclaration(TreeBuilder& context)
{
    ASSERT(match(CLASSTOKEN));
    JSTokenLocation location(tokenLocation());
    JSTextPosition classStart = tokenStartPosition();
    unsigned classStartLine = tokenLine();

    ParserClassInfo<TreeBuilder> info;
    TreeClassExpression classExpr = parseClass(context, FunctionNeedsName, info);
    failIfFalse(classExpr, "Failed to parse class");
    declareVariable(info.className);

    // FIXME: This should be like `let`, not `var`.
    context.addVar(info.className, DeclarationStacks::HasInitializer);

    JSTextPosition classEnd = lastTokenEndPosition();
    unsigned classEndLine = tokenLine();

    return context.createClassDeclStatement(location, classExpr, classStart, classEnd, classStartLine, classEndLine);
}

template <typename LexerType>
template <class TreeBuilder> TreeClassExpression Parser<LexerType>::parseClass(TreeBuilder& context, FunctionRequirements requirements, ParserClassInfo<TreeBuilder>& info)
{
    ASSERT(match(CLASSTOKEN));
    JSTokenLocation location(tokenLocation());
    next();

    AutoPopScopeRef classScope(this, pushScope());
    classScope->setStrictMode();

    const Identifier* className = nullptr;
    if (match(IDENT)) {
        className = m_token.m_data.ident;
        info.className = className;
        next();
        failIfFalse(classScope->declareVariable(className), "'", className->impl(), "' is not a valid class name");
    } else if (requirements == FunctionNeedsName) {
        if (match(OPENBRACE))
            semanticFail("Class statements must have a name");
        semanticFailureDueToKeyword("class name");
        failDueToUnexpectedToken();
    } else
        className = &m_vm->propertyNames->nullIdentifier;
    ASSERT(className);

    TreeExpression parentClass = 0;
    if (consume(EXTENDS)) {
        parentClass = parseMemberExpression(context);
        failIfFalse(parentClass, "Cannot parse the parent class name");
    }
    const ConstructorKind constructorKind = parentClass ? ConstructorKind::Derived : ConstructorKind::Base;

    consumeOrFail(OPENBRACE, "Expected opening '{' at the start of a class body");

    TreeExpression constructor = 0;
    TreePropertyList staticMethods = 0;
    TreePropertyList instanceMethods = 0;
    TreePropertyList instanceMethodsTail = 0;
    TreePropertyList staticMethodsTail = 0;
    while (!match(CLOSEBRACE)) {
        if (match(SEMICOLON)) {
            next();
            continue;
        }

        JSTokenLocation methodLocation(tokenLocation());
        unsigned methodStart = tokenStart();

        // For backwards compatibility, "static" is a non-reserved keyword in non-strict mode.
        bool isStaticMethod = match(RESERVED_IF_STRICT) && *m_token.m_data.ident == m_vm->propertyNames->staticKeyword;
        if (isStaticMethod)
            next();

        // FIXME: Figure out a way to share more code with parseProperty.
        const CommonIdentifiers& propertyNames = *m_vm->propertyNames;
        const Identifier* ident = nullptr;
        bool isGetter = false;
        bool isSetter = false;
        switch (m_token.m_type) {
        case STRING:
            ident = m_token.m_data.ident;
            ASSERT(ident);
            next();
            break;
        case IDENT:
            ident = m_token.m_data.ident;
            isGetter = *ident == propertyNames.get;
            isSetter = *ident == propertyNames.set;
            ASSERT(ident);
            break;
        case DOUBLE:
        case INTEGER:
            ident = &m_parserArena.identifierArena().makeNumericIdentifier(const_cast<VM*>(m_vm), m_token.m_data.doubleValue);
            ASSERT(ident);
            next();
            break;
        default:
            failDueToUnexpectedToken();
        }

        TreeProperty property;
        const bool alwaysStrictInsideClass = true;
        if (isGetter || isSetter) {
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords);
            property = parseGetterSetter(context, alwaysStrictInsideClass, isGetter ? PropertyNode::Getter : PropertyNode::Setter, methodStart,
                ConstructorKind::None, SuperBinding::Needed);
            failIfFalse(property, "Cannot parse this method");
        } else {
            ParserFunctionInfo<TreeBuilder> methodInfo;
            bool isConstructor = !isStaticMethod && *ident == propertyNames.constructor;
            failIfFalse((parseFunctionInfo(context, FunctionNoRequirements, isStaticMethod ? NormalFunctionMode : MethodMode, false, isConstructor ? constructorKind : ConstructorKind::None, SuperBinding::Needed, methodStart, methodInfo, StandardFunctionParseType)), "Cannot parse this method");
            failIfFalse(ident && declareVariable(ident), "Cannot declare a method named '", methodInfo.name->impl(), "'");
            methodInfo.name = isConstructor ? className : ident;

            TreeExpression method = context.createFunctionExpr(methodLocation, methodInfo);
            if (isConstructor) {
                semanticFailIfTrue(constructor, "Cannot declare multiple constructors in a single class");
                constructor = method;
                continue;
            }

            // FIXME: Syntax error when super() is called
            semanticFailIfTrue(isStaticMethod && methodInfo.name && *methodInfo.name == propertyNames.prototype,
                "Cannot declare a static method named 'prototype'");
            property = context.createProperty(methodInfo.name, method, PropertyNode::Constant, PropertyNode::Unknown, alwaysStrictInsideClass, SuperBinding::Needed);
        }

        TreePropertyList& tail = isStaticMethod ? staticMethodsTail : instanceMethodsTail;
        if (tail)
            tail = context.createPropertyList(methodLocation, property, tail);
        else {
            tail = context.createPropertyList(methodLocation, property);
            if (isStaticMethod)
                staticMethods = tail;
            else
                instanceMethods = tail;
        }
    }

    failIfFalse(popScope(classScope, TreeBuilder::NeedsFreeVariableInfo), "Parser error");
    consumeOrFail(CLOSEBRACE, "Expected a closing '}' after a class body");

    return context.createClassExpr(location, *className, constructor, parentClass, instanceMethods, staticMethods);
}
#endif

struct LabelInfo {
    LabelInfo(const Identifier* ident, const JSTextPosition& start, const JSTextPosition& end)
    : m_ident(ident)
    , m_start(start)
    , m_end(end)
    {
    }
    
    const Identifier* m_ident;
    JSTextPosition m_start;
    JSTextPosition m_end;
};

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseExpressionOrLabelStatement(TreeBuilder& context)
{
    
    /* Expression and Label statements are ambiguous at LL(1), so we have a
     * special case that looks for a colon as the next character in the input.
     */
    Vector<LabelInfo> labels;
    JSTokenLocation location;
    do {
        JSTextPosition start = tokenStartPosition();
        location = tokenLocation();
        if (!nextTokenIsColon()) {
            // If we hit this path we're making a expression statement, which
            // by definition can't make use of continue/break so we can just
            // ignore any labels we might have accumulated.
            TreeExpression expression = parseExpression(context);
            failIfFalse(expression, "Cannot parse expression statement");
            if (!autoSemiColon())
                failDueToUnexpectedToken();
            return context.createExprStatement(location, expression, start, m_lastTokenEndPosition.line);
        }
        const Identifier* ident = m_token.m_data.ident;
        JSTextPosition end = tokenEndPosition();
        next();
        consumeOrFail(COLON, "Labels must be followed by a ':'");
        if (!m_syntaxAlreadyValidated) {
            // This is O(N^2) over the current list of consecutive labels, but I
            // have never seen more than one label in a row in the real world.
            for (size_t i = 0; i < labels.size(); i++)
                failIfTrue(ident->impl() == labels[i].m_ident->impl(), "Attempted to redeclare the label '", ident->impl(), "'");
            failIfTrue(getLabel(ident), "Cannot find scope for the label '", ident->impl(), "'");
            labels.append(LabelInfo(ident, start, end));
        }
    } while (match(IDENT));
    bool isLoop = false;
    switch (m_token.m_type) {
    case FOR:
    case WHILE:
    case DO:
        isLoop = true;
        break;
        
    default:
        break;
    }
    const Identifier* unused = 0;
    if (!m_syntaxAlreadyValidated) {
        for (size_t i = 0; i < labels.size(); i++)
            pushLabel(labels[i].m_ident, isLoop);
    }
    TreeStatement statement = parseStatement(context, unused);
    if (!m_syntaxAlreadyValidated) {
        for (size_t i = 0; i < labels.size(); i++)
            popLabel();
    }
    failIfFalse(statement, "Cannot parse statement");
    for (size_t i = 0; i < labels.size(); i++) {
        const LabelInfo& info = labels[labels.size() - i - 1];
        statement = context.createLabelStatement(location, info.m_ident, statement, info.m_start, info.m_end);
    }
    return statement;
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseExpressionStatement(TreeBuilder& context)
{
    switch (m_token.m_type) {
    // Consult: http://www.ecma-international.org/ecma-262/6.0/index.html#sec-expression-statement
    // The ES6 spec mandates that we should fail from FUNCTION token here. We handle this case 
    // in parseStatement() which is the only caller of parseExpressionStatement().
    // We actually allow FUNCTION in situations where it should not be allowed unless we're in strict mode.
    case CLASSTOKEN:
        failWithMessage("'class' declaration is not directly within a block statement");
        break;
    default:
        // FIXME: when implementing 'let' we should fail when we see the token sequence "let [".
        // https://bugs.webkit.org/show_bug.cgi?id=142944
        break;
    }
    JSTextPosition start = tokenStartPosition();
    JSTokenLocation location(tokenLocation());
    TreeExpression expression = parseExpression(context);
    failIfFalse(expression, "Cannot parse expression statement");
    failIfFalse(autoSemiColon(), "Parse error");
    return context.createExprStatement(location, expression, start, m_lastTokenEndPosition.line);
}

template <typename LexerType>
template <class TreeBuilder> TreeStatement Parser<LexerType>::parseIfStatement(TreeBuilder& context)
{
    ASSERT(match(IF));
    JSTokenLocation ifLocation(tokenLocation());
    int start = tokenLine();
    next();
    handleProductionOrFail(OPENPAREN, "(", "start", "'if' condition");

    TreeExpression condition = parseExpression(context);
    failIfFalse(condition, "Expected a expression as the condition for an if statement");
    int end = tokenLine();
    handleProductionOrFail(CLOSEPAREN, ")", "end", "'if' condition");

    const Identifier* unused = 0;
    TreeStatement trueBlock = parseStatement(context, unused);
    failIfFalse(trueBlock, "Expected a statement as the body of an if block");

    if (!match(ELSE))
        return context.createIfStatement(ifLocation, condition, trueBlock, 0, start, end);

    Vector<TreeExpression> exprStack;
    Vector<std::pair<int, int>> posStack;
    Vector<JSTokenLocation> tokenLocationStack;
    Vector<TreeStatement> statementStack;
    bool trailingElse = false;
    do {
        JSTokenLocation tempLocation = tokenLocation();
        next();
        if (!match(IF)) {
            const Identifier* unused = 0;
            TreeStatement block = parseStatement(context, unused);
            failIfFalse(block, "Expected a statement as the body of an else block");
            statementStack.append(block);
            trailingElse = true;
            break;
        }
        int innerStart = tokenLine();
        next();
        
        handleProductionOrFail(OPENPAREN, "(", "start", "'if' condition");

        TreeExpression innerCondition = parseExpression(context);
        failIfFalse(innerCondition, "Expected a expression as the condition for an if statement");
        int innerEnd = tokenLine();
        handleProductionOrFail(CLOSEPAREN, ")", "end", "'if' condition");
        const Identifier* unused = 0;
        TreeStatement innerTrueBlock = parseStatement(context, unused);
        failIfFalse(innerTrueBlock, "Expected a statement as the body of an if block");
        tokenLocationStack.append(tempLocation);
        exprStack.append(innerCondition);
        posStack.append(std::make_pair(innerStart, innerEnd));
        statementStack.append(innerTrueBlock);
    } while (match(ELSE));

    if (!trailingElse) {
        TreeExpression condition = exprStack.last();
        exprStack.removeLast();
        TreeStatement trueBlock = statementStack.last();
        statementStack.removeLast();
        std::pair<int, int> pos = posStack.last();
        posStack.removeLast();
        JSTokenLocation elseLocation = tokenLocationStack.last();
        tokenLocationStack.removeLast();
        TreeStatement ifStatement = context.createIfStatement(elseLocation, condition, trueBlock, 0, pos.first, pos.second);
        context.setEndOffset(ifStatement, context.endOffset(trueBlock));
        statementStack.append(ifStatement);
    }

    while (!exprStack.isEmpty()) {
        TreeExpression condition = exprStack.last();
        exprStack.removeLast();
        TreeStatement falseBlock = statementStack.last();
        statementStack.removeLast();
        TreeStatement trueBlock = statementStack.last();
        statementStack.removeLast();
        std::pair<int, int> pos = posStack.last();
        posStack.removeLast();
        JSTokenLocation elseLocation = tokenLocationStack.last();
        tokenLocationStack.removeLast();
        TreeStatement ifStatement = context.createIfStatement(elseLocation, condition, trueBlock, falseBlock, pos.first, pos.second);
        context.setEndOffset(ifStatement, context.endOffset(falseBlock));
        statementStack.append(ifStatement);
    }

    return context.createIfStatement(ifLocation, condition, trueBlock, statementStack.last(), start, end);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    JSTokenLocation location(tokenLocation());
    TreeExpression node = parseAssignmentExpression(context);
    failIfFalse(node, "Cannot parse expression");
    context.setEndOffset(node, m_lastTokenEndPosition.offset);
    if (!match(COMMA))
        return node;
    next();
    m_nonTrivialExpressionCount++;
    m_nonLHSCount++;
    TreeExpression right = parseAssignmentExpression(context);
    failIfFalse(right, "Cannot parse expression in a comma expression");
    context.setEndOffset(right, m_lastTokenEndPosition.offset);
    typename TreeBuilder::Comma head = context.createCommaExpr(location, node);
    typename TreeBuilder::Comma tail = context.appendToCommaExpr(location, head, head, right);
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        right = parseAssignmentExpression(context);
        failIfFalse(right, "Cannot parse expression in a comma expression");
        context.setEndOffset(right, m_lastTokenEndPosition.offset);
        tail = context.appendToCommaExpr(location, head, tail, right);
    }
    context.setEndOffset(head, m_lastTokenEndPosition.offset);
    return head;
}

    
template <typename LexerType>
template <typename TreeBuilder> TreeExpression Parser<LexerType>::parseAssignmentExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    JSTextPosition start = tokenStartPosition();
    JSTokenLocation location(tokenLocation());
    int initialAssignmentCount = m_assignmentCount;
    int initialNonLHSCount = m_nonLHSCount;
    if (match(OPENBRACE) || match(OPENBRACKET)) {
        SavePoint savePoint = createSavePoint();
        auto pattern = tryParseDestructuringPatternExpression(context);
        if (pattern && consume(EQUAL)) {
            auto rhs = parseAssignmentExpression(context);
            if (rhs)
                return context.createDestructuringAssignment(location, pattern, rhs);
        }
        restoreSavePoint(savePoint);
    }
    
#if ENABLE(ES6_ARROWFUNCTION_SYNTAX)
    if (isArrowFunctionParamters())
        return parseArrowFunctionExpression(context);
#endif
    
    TreeExpression lhs = parseConditionalExpression(context);
    failIfFalse(lhs, "Cannot parse expression");
    if (initialNonLHSCount != m_nonLHSCount) {
        if (m_token.m_type >= EQUAL && m_token.m_type <= OREQUAL)
            semanticFail("Left hand side of operator '", getToken(), "' must be a reference");

        return lhs;
    }
    
    int assignmentStack = 0;
    Operator op;
    bool hadAssignment = false;
    while (true) {
        switch (m_token.m_type) {
        case EQUAL: op = OpEqual; break;
        case PLUSEQUAL: op = OpPlusEq; break;
        case MINUSEQUAL: op = OpMinusEq; break;
        case MULTEQUAL: op = OpMultEq; break;
        case DIVEQUAL: op = OpDivEq; break;
        case LSHIFTEQUAL: op = OpLShift; break;
        case RSHIFTEQUAL: op = OpRShift; break;
        case URSHIFTEQUAL: op = OpURShift; break;
        case ANDEQUAL: op = OpAndEq; break;
        case XOREQUAL: op = OpXOrEq; break;
        case OREQUAL: op = OpOrEq; break;
        case MODEQUAL: op = OpModEq; break;
        default:
            goto end;
        }
        m_nonTrivialExpressionCount++;
        hadAssignment = true;
        context.assignmentStackAppend(assignmentStack, lhs, start, tokenStartPosition(), m_assignmentCount, op);
        start = tokenStartPosition();
        m_assignmentCount++;
        next(TreeBuilder::DontBuildStrings);
        if (strictMode() && m_lastIdentifier && context.isResolve(lhs)) {
            failIfTrueIfStrict(m_vm->propertyNames->eval == *m_lastIdentifier, "Cannot modify 'eval' in strict mode");
            failIfTrueIfStrict(m_vm->propertyNames->arguments == *m_lastIdentifier, "Cannot modify 'arguments' in strict mode");
            declareWrite(m_lastIdentifier);
            m_lastIdentifier = 0;
        }
        lhs = parseAssignmentExpression(context);
        failIfFalse(lhs, "Cannot parse the right hand side of an assignment expression");
        if (initialNonLHSCount != m_nonLHSCount) {
            if (m_token.m_type >= EQUAL && m_token.m_type <= OREQUAL)
                semanticFail("Left hand side of operator '", getToken(), "' must be a reference");
            break;
        }
    }
end:
    if (hadAssignment)
        m_nonLHSCount++;
    
    if (!TreeBuilder::CreatesAST)
        return lhs;
    
    while (assignmentStack)
        lhs = context.createAssignment(location, assignmentStack, lhs, initialAssignmentCount, m_assignmentCount, lastTokenEndPosition());
    
    return lhs;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseConditionalExpression(TreeBuilder& context)
{
    JSTokenLocation location(tokenLocation());
    TreeExpression cond = parseBinaryExpression(context);
    failIfFalse(cond, "Cannot parse expression");
    if (!match(QUESTION))
        return cond;
    m_nonTrivialExpressionCount++;
    m_nonLHSCount++;
    next(TreeBuilder::DontBuildStrings);
    TreeExpression lhs = parseAssignmentExpression(context);
    failIfFalse(lhs, "Cannot parse left hand side of ternary operator");
    context.setEndOffset(lhs, m_lastTokenEndPosition.offset);
    consumeOrFailWithFlags(COLON, TreeBuilder::DontBuildStrings, "Expected ':' in ternary operator");
    
    TreeExpression rhs = parseAssignmentExpression(context);
    failIfFalse(rhs, "Cannot parse right hand side of ternary operator");
    context.setEndOffset(rhs, m_lastTokenEndPosition.offset);
    return context.createConditionalExpr(location, cond, lhs, rhs);
}

ALWAYS_INLINE static bool isUnaryOp(JSTokenType token)
{
    return token & UnaryOpTokenFlag;
}

template <typename LexerType>
int Parser<LexerType>::isBinaryOperator(JSTokenType token)
{
    if (m_allowsIn)
        return token & (BinaryOpTokenPrecedenceMask << BinaryOpTokenAllowsInPrecedenceAdditionalShift);
    return token & BinaryOpTokenPrecedenceMask;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseBinaryExpression(TreeBuilder& context)
{
    int operandStackDepth = 0;
    int operatorStackDepth = 0;
    typename TreeBuilder::BinaryExprContext binaryExprContext(context);
    JSTokenLocation location(tokenLocation());
    while (true) {
        JSTextPosition exprStart = tokenStartPosition();
        int initialAssignments = m_assignmentCount;
        TreeExpression current = parseUnaryExpression(context);
        failIfFalse(current, "Cannot parse expression");
        
        context.appendBinaryExpressionInfo(operandStackDepth, current, exprStart, lastTokenEndPosition(), lastTokenEndPosition(), initialAssignments != m_assignmentCount);
        int precedence = isBinaryOperator(m_token.m_type);
        if (!precedence)
            break;
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        int operatorToken = m_token.m_type;
        next(TreeBuilder::DontBuildStrings);
        
        while (operatorStackDepth &&  context.operatorStackHasHigherPrecedence(operatorStackDepth, precedence)) {
            ASSERT(operandStackDepth > 1);
            
            typename TreeBuilder::BinaryOperand rhs = context.getFromOperandStack(-1);
            typename TreeBuilder::BinaryOperand lhs = context.getFromOperandStack(-2);
            context.shrinkOperandStackBy(operandStackDepth, 2);
            context.appendBinaryOperation(location, operandStackDepth, operatorStackDepth, lhs, rhs);
            context.operatorStackPop(operatorStackDepth);
        }
        context.operatorStackAppend(operatorStackDepth, operatorToken, precedence);
    }
    while (operatorStackDepth) {
        ASSERT(operandStackDepth > 1);
        
        typename TreeBuilder::BinaryOperand rhs = context.getFromOperandStack(-1);
        typename TreeBuilder::BinaryOperand lhs = context.getFromOperandStack(-2);
        context.shrinkOperandStackBy(operandStackDepth, 2);
        context.appendBinaryOperation(location, operandStackDepth, operatorStackDepth, lhs, rhs);
        context.operatorStackPop(operatorStackDepth);
    }
    return context.popOperandStack(operandStackDepth);
}

template <typename LexerType>
template <class TreeBuilder> TreeProperty Parser<LexerType>::parseProperty(TreeBuilder& context, bool complete)
{
    bool wasIdent = false;
    switch (m_token.m_type) {
    namedProperty:
    case IDENT:
        wasIdent = true;
        FALLTHROUGH;
    case STRING: {
        const Identifier* ident = m_token.m_data.ident;
        unsigned getterOrSetterStartOffset = tokenStart();
        if (complete || (wasIdent && (*ident == m_vm->propertyNames->get || *ident == m_vm->propertyNames->set)))
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords);
        else
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords | TreeBuilder::DontBuildKeywords);

        if (match(COLON)) {
            next();
            TreeExpression node = parseAssignmentExpression(context);
            failIfFalse(node, "Cannot parse expression for property declaration");
            context.setEndOffset(node, m_lexer->currentOffset());
            return context.createProperty(ident, node, PropertyNode::Constant, PropertyNode::Unknown, complete);
        }

        if (match(OPENPAREN)) {
            auto method = parsePropertyMethod(context, ident);
            propagateError();
            return context.createProperty(ident, method, PropertyNode::Constant, PropertyNode::KnownDirect, complete);
        }

        failIfFalse(wasIdent, "Expected an identifier as property name");

        if (match(COMMA) || match(CLOSEBRACE)) {
            JSTextPosition start = tokenStartPosition();
            JSTokenLocation location(tokenLocation());
            currentScope()->useVariable(ident, m_vm->propertyNames->eval == *ident);
            TreeExpression node = context.createResolve(location, *ident, start, lastTokenEndPosition());
            return context.createProperty(ident, node, static_cast<PropertyNode::Type>(PropertyNode::Constant | PropertyNode::Shorthand), PropertyNode::KnownDirect, complete);
        }

        PropertyNode::Type type;
        if (*ident == m_vm->propertyNames->get)
            type = PropertyNode::Getter;
        else if (*ident == m_vm->propertyNames->set)
            type = PropertyNode::Setter;
        else
            failWithMessage("Expected a ':' following the property name '", ident->impl(), "'");
        return parseGetterSetter(context, complete, type, getterOrSetterStartOffset);
    }
    case DOUBLE:
    case INTEGER: {
        double propertyName = m_token.m_data.doubleValue;
        next();

        if (match(OPENPAREN)) {
            const Identifier& ident = m_parserArena.identifierArena().makeNumericIdentifier(const_cast<VM*>(m_vm), propertyName);
            auto method = parsePropertyMethod(context, &ident);
            propagateError();
            return context.createProperty(&ident, method, PropertyNode::Constant, PropertyNode::Unknown, complete);
        }

        consumeOrFail(COLON, "Expected ':' after property name");
        TreeExpression node = parseAssignmentExpression(context);
        failIfFalse(node, "Cannot parse expression for property declaration");
        context.setEndOffset(node, m_lexer->currentOffset());
        return context.createProperty(const_cast<VM*>(m_vm), m_parserArena, propertyName, node, PropertyNode::Constant, PropertyNode::Unknown, complete);
    }
    case OPENBRACKET: {
        next();
        auto propertyName = parseAssignmentExpression(context);
        failIfFalse(propertyName, "Cannot parse computed property name");
        handleProductionOrFail(CLOSEBRACKET, "]", "end", "computed property name");

        if (match(OPENPAREN)) {
            auto method = parsePropertyMethod(context, &m_vm->propertyNames->nullIdentifier);
            propagateError();
            return context.createProperty(propertyName, method, static_cast<PropertyNode::Type>(PropertyNode::Constant | PropertyNode::Computed), PropertyNode::KnownDirect, complete);
        }

        consumeOrFail(COLON, "Expected ':' after property name");
        TreeExpression node = parseAssignmentExpression(context);
        failIfFalse(node, "Cannot parse expression for property declaration");
        context.setEndOffset(node, m_lexer->currentOffset());
        return context.createProperty(propertyName, node, static_cast<PropertyNode::Type>(PropertyNode::Constant | PropertyNode::Computed), PropertyNode::Unknown, complete);
    }
    default:
        failIfFalse(m_token.m_type & KeywordTokenFlag, "Expected a property name");
        goto namedProperty;
    }
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parsePropertyMethod(TreeBuilder& context, const Identifier* methodName)
{
    JSTokenLocation methodLocation(tokenLocation());
    unsigned methodStart = tokenStart();
    ParserFunctionInfo<TreeBuilder> methodInfo;
    failIfFalse((parseFunctionInfo(context, FunctionNoRequirements, MethodMode, false, ConstructorKind::None, SuperBinding::NotNeeded, methodStart, methodInfo, StandardFunctionParseType)), "Cannot parse this method");
    methodInfo.name = methodName;
    return context.createFunctionExpr(methodLocation, methodInfo);
}

template <typename LexerType>
template <class TreeBuilder> TreeProperty Parser<LexerType>::parseGetterSetter(TreeBuilder& context, bool strict, PropertyNode::Type type, unsigned getterOrSetterStartOffset,
    ConstructorKind constructorKind, SuperBinding superBinding)
{
    const Identifier* stringPropertyName = 0;
    double numericPropertyName = 0;
    if (m_token.m_type == IDENT || m_token.m_type == STRING) {
        stringPropertyName = m_token.m_data.ident;
        semanticFailIfTrue(superBinding == SuperBinding::Needed && *stringPropertyName == m_vm->propertyNames->prototype,
            "Cannot declare a static method named 'prototype'");
        semanticFailIfTrue(superBinding == SuperBinding::Needed && *stringPropertyName == m_vm->propertyNames->constructor,
            "Cannot declare a getter or setter named 'constructor'");
    } else if (m_token.m_type == DOUBLE || m_token.m_type == INTEGER)
        numericPropertyName = m_token.m_data.doubleValue;
    else
        failDueToUnexpectedToken();
    JSTokenLocation location(tokenLocation());
    next();
    ParserFunctionInfo<TreeBuilder> info;
    if (type & PropertyNode::Getter) {
        failIfFalse(match(OPENPAREN), "Expected a parameter list for getter definition");
        failIfFalse((parseFunctionInfo(context, FunctionNoRequirements, GetterMode, false, constructorKind, superBinding,
            getterOrSetterStartOffset, info, StandardFunctionParseType)), "Cannot parse getter definition");
    } else {
        failIfFalse(match(OPENPAREN), "Expected a parameter list for setter definition");
        failIfFalse((parseFunctionInfo(context, FunctionNoRequirements, SetterMode, false, constructorKind, superBinding,
            getterOrSetterStartOffset, info, StandardFunctionParseType)), "Cannot parse setter definition");
    }
    if (stringPropertyName)
        return context.createGetterOrSetterProperty(location, type, strict, stringPropertyName, info, superBinding);
    return context.createGetterOrSetterProperty(const_cast<VM*>(m_vm), m_parserArena, location, type, strict, numericPropertyName, info, superBinding);
}

template <typename LexerType>
template <class TreeBuilder> bool Parser<LexerType>::shouldCheckPropertyForUnderscoreProtoDuplicate(TreeBuilder& context, const TreeProperty& property)
{
    if (m_syntaxAlreadyValidated)
        return false;

    if (!context.getName(property))
        return false;

    // A Constant property that is not a Computed or Shorthand Constant property.
    return context.getType(property) == PropertyNode::Constant;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseObjectLiteral(TreeBuilder& context)
{
    auto savePoint = createSavePoint();
    consumeOrFailWithFlags(OPENBRACE, TreeBuilder::DontBuildStrings, "Expected opening '{' at the start of an object literal");

    int oldNonLHSCount = m_nonLHSCount;

    JSTokenLocation location(tokenLocation());    
    if (match(CLOSEBRACE)) {
        next();
        return context.createObjectLiteral(location);
    }
    
    TreeProperty property = parseProperty(context, false);
    failIfFalse(property, "Cannot parse object literal property");

    if (!m_syntaxAlreadyValidated && context.getType(property) & (PropertyNode::Getter | PropertyNode::Setter)) {
        restoreSavePoint(savePoint);
        return parseStrictObjectLiteral(context);
    }

    bool seenUnderscoreProto = false;
    if (shouldCheckPropertyForUnderscoreProtoDuplicate(context, property))
        seenUnderscoreProto = *context.getName(property) == m_vm->propertyNames->underscoreProto;

    TreePropertyList propertyList = context.createPropertyList(location, property);
    TreePropertyList tail = propertyList;
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        if (match(CLOSEBRACE))
            break;
        JSTokenLocation propertyLocation(tokenLocation());
        property = parseProperty(context, false);
        failIfFalse(property, "Cannot parse object literal property");
        if (!m_syntaxAlreadyValidated && context.getType(property) & (PropertyNode::Getter | PropertyNode::Setter)) {
            restoreSavePoint(savePoint);
            return parseStrictObjectLiteral(context);
        }
        if (shouldCheckPropertyForUnderscoreProtoDuplicate(context, property)) {
            if (*context.getName(property) == m_vm->propertyNames->underscoreProto) {
                semanticFailIfTrue(seenUnderscoreProto, "Attempted to redefine __proto__ property");
                seenUnderscoreProto = true;
            }
        }
        tail = context.createPropertyList(propertyLocation, property, tail);
    }

    location = tokenLocation();
    handleProductionOrFail(CLOSEBRACE, "}", "end", "object literal");
    
    m_nonLHSCount = oldNonLHSCount;
    
    return context.createObjectLiteral(location, propertyList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseStrictObjectLiteral(TreeBuilder& context)
{
    consumeOrFail(OPENBRACE, "Expected opening '{' at the start of an object literal");
    
    int oldNonLHSCount = m_nonLHSCount;

    JSTokenLocation location(tokenLocation());
    if (match(CLOSEBRACE)) {
        next();
        return context.createObjectLiteral(location);
    }
    
    TreeProperty property = parseProperty(context, true);
    failIfFalse(property, "Cannot parse object literal property");

    bool seenUnderscoreProto = false;
    if (shouldCheckPropertyForUnderscoreProtoDuplicate(context, property))
        seenUnderscoreProto = *context.getName(property) == m_vm->propertyNames->underscoreProto;

    TreePropertyList propertyList = context.createPropertyList(location, property);
    TreePropertyList tail = propertyList;
    while (match(COMMA)) {
        next();
        if (match(CLOSEBRACE))
            break;
        JSTokenLocation propertyLocation(tokenLocation());
        property = parseProperty(context, true);
        failIfFalse(property, "Cannot parse object literal property");
        if (shouldCheckPropertyForUnderscoreProtoDuplicate(context, property)) {
            if (*context.getName(property) == m_vm->propertyNames->underscoreProto) {
                semanticFailIfTrue(seenUnderscoreProto, "Attempted to redefine __proto__ property");
                seenUnderscoreProto = true;
            }
        }
        tail = context.createPropertyList(propertyLocation, property, tail);
    }

    location = tokenLocation();
    handleProductionOrFail(CLOSEBRACE, "}", "end", "object literal");

    m_nonLHSCount = oldNonLHSCount;

    return context.createObjectLiteral(location, propertyList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseArrayLiteral(TreeBuilder& context)
{
    consumeOrFailWithFlags(OPENBRACKET, TreeBuilder::DontBuildStrings, "Expected an opening '[' at the beginning of an array literal");
    
    int oldNonLHSCount = m_nonLHSCount;
    
    int elisions = 0;
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        elisions++;
    }
    if (match(CLOSEBRACKET)) {
        JSTokenLocation location(tokenLocation());
        next(TreeBuilder::DontBuildStrings);
        return context.createArray(location, elisions);
    }
    
    TreeExpression elem;
    if (UNLIKELY(match(DOTDOTDOT))) {
        auto spreadLocation = m_token.m_location;
        auto start = m_token.m_startPosition;
        auto divot = m_token.m_endPosition;
        next();
        auto spreadExpr = parseAssignmentExpression(context);
        failIfFalse(spreadExpr, "Cannot parse subject of a spread operation");
        elem = context.createSpreadExpression(spreadLocation, spreadExpr, start, divot, m_lastTokenEndPosition);
    } else
        elem = parseAssignmentExpression(context);
    failIfFalse(elem, "Cannot parse array literal element");
    typename TreeBuilder::ElementList elementList = context.createElementList(elisions, elem);
    typename TreeBuilder::ElementList tail = elementList;
    elisions = 0;
    while (match(COMMA)) {
        next(TreeBuilder::DontBuildStrings);
        elisions = 0;
        
        while (match(COMMA)) {
            next();
            elisions++;
        }
        
        if (match(CLOSEBRACKET)) {
            JSTokenLocation location(tokenLocation());
            next(TreeBuilder::DontBuildStrings);
            return context.createArray(location, elisions, elementList);
        }
        if (UNLIKELY(match(DOTDOTDOT))) {
            auto spreadLocation = m_token.m_location;
            auto start = m_token.m_startPosition;
            auto divot = m_token.m_endPosition;
            next();
            TreeExpression elem = parseAssignmentExpression(context);
            failIfFalse(elem, "Cannot parse subject of a spread operation");
            auto spread = context.createSpreadExpression(spreadLocation, elem, start, divot, m_lastTokenEndPosition);
            tail = context.createElementList(tail, elisions, spread);
            continue;
        }
        TreeExpression elem = parseAssignmentExpression(context);
        failIfFalse(elem, "Cannot parse array literal element");
        tail = context.createElementList(tail, elisions, elem);
    }

    JSTokenLocation location(tokenLocation());
    if (!consume(CLOSEBRACKET)) {
        failIfFalse(match(DOTDOTDOT), "Expected either a closing ']' or a ',' following an array element");
        semanticFail("The '...' operator should come before a target expression");
    }
    
    m_nonLHSCount = oldNonLHSCount;
    
    return context.createArray(location, elementList);
}

#if ENABLE(ES6_TEMPLATE_LITERAL_SYNTAX)
template <typename LexerType>
template <class TreeBuilder> typename TreeBuilder::TemplateString Parser<LexerType>::parseTemplateString(TreeBuilder& context, bool isTemplateHead, typename LexerType::RawStringsBuildMode rawStringsBuildMode, bool& elementIsTail)
{
    if (!isTemplateHead) {
        matchOrFail(CLOSEBRACE, "Expected a closing '}' following an expression in template literal");
        // Re-scan the token to recognize it as Template Element.
        m_token.m_type = m_lexer->scanTrailingTemplateString(&m_token, rawStringsBuildMode);
    }
    matchOrFail(TEMPLATE, "Expected an template element");
    const Identifier* cooked = m_token.m_data.cooked;
    const Identifier* raw = m_token.m_data.raw;
    elementIsTail = m_token.m_data.isTail;
    JSTokenLocation location(tokenLocation());
    next();
    return context.createTemplateString(location, *cooked, *raw);
}

template <typename LexerType>
template <class TreeBuilder> typename TreeBuilder::TemplateLiteral Parser<LexerType>::parseTemplateLiteral(TreeBuilder& context, typename LexerType::RawStringsBuildMode rawStringsBuildMode)
{
    JSTokenLocation location(tokenLocation());
    bool elementIsTail = false;

    auto headTemplateString = parseTemplateString(context, true, rawStringsBuildMode, elementIsTail);
    failIfFalse(headTemplateString, "Cannot parse head template element");

    typename TreeBuilder::TemplateStringList templateStringList = context.createTemplateStringList(headTemplateString);
    typename TreeBuilder::TemplateStringList templateStringTail = templateStringList;

    if (elementIsTail)
        return context.createTemplateLiteral(location, templateStringList);

    failIfTrue(match(CLOSEBRACE), "Template literal expression cannot be empty");
    TreeExpression expression = parseExpression(context);
    failIfFalse(expression, "Cannot parse expression in template literal");

    typename TreeBuilder::TemplateExpressionList templateExpressionList = context.createTemplateExpressionList(expression);
    typename TreeBuilder::TemplateExpressionList templateExpressionTail = templateExpressionList;

    auto templateString = parseTemplateString(context, false, rawStringsBuildMode, elementIsTail);
    failIfFalse(templateString, "Cannot parse template element");
    templateStringTail = context.createTemplateStringList(templateStringTail, templateString);

    while (!elementIsTail) {
        failIfTrue(match(CLOSEBRACE), "Template literal expression cannot be empty");
        TreeExpression expression = parseExpression(context);
        failIfFalse(expression, "Cannot parse expression in template literal");

        templateExpressionTail = context.createTemplateExpressionList(templateExpressionTail, expression);

        auto templateString = parseTemplateString(context, false, rawStringsBuildMode, elementIsTail);
        failIfFalse(templateString, "Cannot parse template element");
        templateStringTail = context.createTemplateStringList(templateStringTail, templateString);
    }

    return context.createTemplateLiteral(location, templateStringList, templateExpressionList);
}
#endif

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parsePrimaryExpression(TreeBuilder& context)
{
    failIfStackOverflow();
    switch (m_token.m_type) {
    case FUNCTION: {
        JSTokenLocation location(tokenLocation());
        unsigned functionKeywordStart = tokenStart();
        next();
        ParserFunctionInfo<TreeBuilder> info;
        info.name = &m_vm->propertyNames->nullIdentifier;
        failIfFalse((parseFunctionInfo(context, FunctionNoRequirements, NormalFunctionMode, false, ConstructorKind::None, SuperBinding::NotNeeded, functionKeywordStart, info, StandardFunctionParseType)), "Cannot parse function expression");
        return context.createFunctionExpr(location, info);
    }
#if ENABLE(ES6_CLASS_SYNTAX)
    case CLASSTOKEN: {
        ParserClassInfo<TreeBuilder> info;
        return parseClass(context, FunctionNoRequirements, info);
    }
#endif
    case OPENBRACE:
        if (strictMode())
            return parseStrictObjectLiteral(context);
        return parseObjectLiteral(context);
    case OPENBRACKET:
        return parseArrayLiteral(context);
    case OPENPAREN: {
        next();
        int oldNonLHSCount = m_nonLHSCount;
        TreeExpression result = parseExpression(context);
        m_nonLHSCount = oldNonLHSCount;
        handleProductionOrFail(CLOSEPAREN, ")", "end", "compound expression");
        return result;
    }
    case THISTOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.thisExpr(location, m_thisTDZMode);
    }
    case IDENT: {
        JSTextPosition start = tokenStartPosition();
        const Identifier* ident = m_token.m_data.ident;
        JSTokenLocation location(tokenLocation());
        next();
        currentScope()->useVariable(ident, m_vm->propertyNames->eval == *ident);
        m_lastIdentifier = ident;
        return context.createResolve(location, *ident, start, lastTokenEndPosition());
    }
    case STRING: {
        const Identifier* ident = m_token.m_data.ident;
        JSTokenLocation location(tokenLocation());
        next();
        return context.createString(location, ident);
    }
    case DOUBLE: {
        double d = m_token.m_data.doubleValue;
        JSTokenLocation location(tokenLocation());
        next();
        return context.createDoubleExpr(location, d);
    }
    case INTEGER: {
        double d = m_token.m_data.doubleValue;
        JSTokenLocation location(tokenLocation());
        next();
        return context.createIntegerExpr(location, d);
    }
    case NULLTOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createNull(location);
    }
    case TRUETOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createBoolean(location, true);
    }
    case FALSETOKEN: {
        JSTokenLocation location(tokenLocation());
        next();
        return context.createBoolean(location, false);
    }
    case DIVEQUAL:
    case DIVIDE: {
        /* regexp */
        const Identifier* pattern;
        const Identifier* flags;
        if (match(DIVEQUAL))
            failIfFalse(m_lexer->scanRegExp(pattern, flags, '='), "Invalid regular expression");
        else
            failIfFalse(m_lexer->scanRegExp(pattern, flags), "Invalid regular expression");
        
        JSTextPosition start = tokenStartPosition();
        JSTokenLocation location(tokenLocation());
        next();
        TreeExpression re = context.createRegExp(location, *pattern, *flags, start);
        if (!re) {
            const char* yarrErrorMsg = Yarr::checkSyntax(pattern->string());
            regexFail(yarrErrorMsg);
        }
        return re;
    }
#if ENABLE(ES6_TEMPLATE_LITERAL_SYNTAX)
    case TEMPLATE:
        return parseTemplateLiteral(context, LexerType::RawStringsBuildMode::DontBuildRawStrings);
#endif
    default:
        failDueToUnexpectedToken();
    }
}

template <typename LexerType>
template <class TreeBuilder> TreeArguments Parser<LexerType>::parseArguments(TreeBuilder& context, SpreadMode mode)
{
    consumeOrFailWithFlags(OPENPAREN, TreeBuilder::DontBuildStrings, "Expected opening '(' at start of argument list");
    JSTokenLocation location(tokenLocation());
    if (match(CLOSEPAREN)) {
        next(TreeBuilder::DontBuildStrings);
        return context.createArguments();
    }
    if (match(DOTDOTDOT) && mode == AllowSpread) {
        JSTokenLocation spreadLocation(tokenLocation());
        auto start = m_token.m_startPosition;
        auto divot = m_token.m_endPosition;
        next();
        auto spreadExpr = parseAssignmentExpression(context);
        auto end = m_lastTokenEndPosition;
        if (!spreadExpr)
            failWithMessage("Cannot parse spread expression");
        if (!consume(CLOSEPAREN)) {
            if (match(COMMA))
                semanticFail("Spread operator may only be applied to the last argument passed to a function");
            handleProductionOrFail(CLOSEPAREN, ")", "end", "argument list");
        }
        auto spread = context.createSpreadExpression(spreadLocation, spreadExpr, start, divot, end);
        TreeArgumentsList argList = context.createArgumentsList(location, spread);
        return context.createArguments(argList);
    }
    TreeExpression firstArg = parseAssignmentExpression(context);
    failIfFalse(firstArg, "Cannot parse function argument");
    
    TreeArgumentsList argList = context.createArgumentsList(location, firstArg);
    TreeArgumentsList tail = argList;
    while (match(COMMA)) {
        JSTokenLocation argumentLocation(tokenLocation());
        next(TreeBuilder::DontBuildStrings);
        TreeExpression arg = parseAssignmentExpression(context);
        failIfFalse(arg, "Cannot parse function argument");
        tail = context.createArgumentsList(argumentLocation, tail, arg);
    }
    semanticFailIfTrue(match(DOTDOTDOT), "The '...' operator should come before the target expression");
    handleProductionOrFail(CLOSEPAREN, ")", "end", "argument list");
    return context.createArguments(argList);
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseMemberExpression(TreeBuilder& context)
{
    TreeExpression base = 0;
    JSTextPosition expressionStart = tokenStartPosition();
    int newCount = 0;
    JSTokenLocation startLocation = tokenLocation();
    JSTokenLocation location;
    while (match(NEW)) {
        next();
        newCount++;
    }

#if ENABLE(ES6_CLASS_SYNTAX)
    bool baseIsSuper = match(SUPER);
    semanticFailIfTrue(baseIsSuper && newCount, "Cannot use new with super");
#else
    bool baseIsSuper = false;
#endif

    if (baseIsSuper) {
        base = context.superExpr(location);
        next();
        ScopeRef functionScope = currentFunctionScope();
        if (!functionScope->setNeedsSuperBinding()) {
            // It unnecessary to check of using super during reparsing one more time. Also it can lead to syntax error
            // in case of arrow function because during reparsing we don't know whether we currently parse the arrow function
            // inside of the constructor or method.
            if (!m_lexer->isReparsingFunction()) {
                SuperBinding functionSuperBinding = functionScope->expectedSuperBinding();
                semanticFailIfTrue(functionSuperBinding == SuperBinding::NotNeeded, "super is not valid in this context");
            }
        }
    } else
        base = parsePrimaryExpression(context);

    failIfFalse(base, "Cannot parse base expression");
    while (true) {
        location = tokenLocation();
        switch (m_token.m_type) {
        case OPENBRACKET: {
            m_nonTrivialExpressionCount++;
            JSTextPosition expressionEnd = lastTokenEndPosition();
            next();
            int nonLHSCount = m_nonLHSCount;
            int initialAssignments = m_assignmentCount;
            TreeExpression property = parseExpression(context);
            failIfFalse(property, "Cannot parse subscript expression");
            base = context.createBracketAccess(location, base, property, initialAssignments != m_assignmentCount, expressionStart, expressionEnd, tokenEndPosition());
            handleProductionOrFail(CLOSEBRACKET, "]", "end", "subscript expression");
            m_nonLHSCount = nonLHSCount;
            break;
        }
        case OPENPAREN: {
            m_nonTrivialExpressionCount++;
            int nonLHSCount = m_nonLHSCount;
            if (newCount) {
                newCount--;
                JSTextPosition expressionEnd = lastTokenEndPosition();
                TreeArguments arguments = parseArguments(context, AllowSpread);
                failIfFalse(arguments, "Cannot parse call arguments");
                base = context.createNewExpr(location, base, arguments, expressionStart, expressionEnd, lastTokenEndPosition());
            } else {
                JSTextPosition expressionEnd = lastTokenEndPosition();
                TreeArguments arguments = parseArguments(context, AllowSpread);
                failIfFalse(arguments, "Cannot parse call arguments");
                if (baseIsSuper) {
                    ScopeRef functionScope = currentFunctionScope();
                    if (!functionScope->setHasDirectSuper()) {
                        // It unnecessary to check of using super during reparsing one more time. Also it can lead to syntax error
                        // in case of arrow function because during reparsing we don't know whether we currently parse the arrow function
                        // inside of the constructor or method.
                        if (!m_lexer->isReparsingFunction()) {
                            ConstructorKind functionConstructorKind = functionScope->constructorKind();
                            semanticFailIfTrue(functionConstructorKind == ConstructorKind::None, "super is not valid in this context");
                            semanticFailIfTrue(functionConstructorKind != ConstructorKind::Derived, "super is not valid in this context");
                        }
                    }
                }
                base = context.makeFunctionCallNode(startLocation, base, arguments, expressionStart, expressionEnd, lastTokenEndPosition());
            }
            m_nonLHSCount = nonLHSCount;
            break;
        }
        case DOT: {
            m_nonTrivialExpressionCount++;
            JSTextPosition expressionEnd = lastTokenEndPosition();
            nextExpectIdentifier(LexerFlagsIgnoreReservedWords | TreeBuilder::DontBuildKeywords);
            matchOrFail(IDENT, "Expected a property name after '.'");
            base = context.createDotAccess(location, base, m_token.m_data.ident, expressionStart, expressionEnd, tokenEndPosition());
            next();
            break;
        }
#if ENABLE(ES6_TEMPLATE_LITERAL_SYNTAX)
        case TEMPLATE: {
            semanticFailIfTrue(baseIsSuper, "Cannot use super as tag for tagged templates");
            JSTextPosition expressionEnd = lastTokenEndPosition();
            int nonLHSCount = m_nonLHSCount;
            typename TreeBuilder::TemplateLiteral templateLiteral = parseTemplateLiteral(context, LexerType::RawStringsBuildMode::BuildRawStrings);
            failIfFalse(templateLiteral, "Cannot parse template literal");
            base = context.createTaggedTemplate(location, base, templateLiteral, expressionStart, expressionEnd, lastTokenEndPosition());
            m_nonLHSCount = nonLHSCount;
            break;
        }
#endif
        default:
            goto endMemberExpression;
        }
        baseIsSuper = false;
    }
endMemberExpression:
    semanticFailIfTrue(baseIsSuper, "Cannot reference super");
    while (newCount--)
        base = context.createNewExpr(location, base, expressionStart, lastTokenEndPosition());
    return base;
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseArrowFunctionExpression(TreeBuilder& context)
{
    JSTokenLocation location;

    unsigned functionKeywordStart = tokenStart();
    location = tokenLocation();
    ParserFunctionInfo<TreeBuilder> info;
    info.name = &m_vm->propertyNames->nullIdentifier;
    failIfFalse((parseFunctionInfo(context, FunctionNoRequirements, ArrowFunctionMode, true, ConstructorKind::None, SuperBinding::NotNeeded, functionKeywordStart, info, ArrowFunctionParseType)), "Cannot parse arrow function expression");

    return context.createArrowFunctionExpr(location, info);
}

static const char* operatorString(bool prefix, unsigned tok)
{
    switch (tok) {
    case MINUSMINUS:
    case AUTOMINUSMINUS:
        return prefix ? "prefix-decrement" : "decrement";

    case PLUSPLUS:
    case AUTOPLUSPLUS:
        return prefix ? "prefix-increment" : "increment";

    case EXCLAMATION:
        return "logical-not";

    case TILDE:
        return "bitwise-not";
    
    case TYPEOF:
        return "typeof";
    
    case VOIDTOKEN:
        return "void";
    
    case DELETETOKEN:
        return "delete";
    }
    RELEASE_ASSERT_NOT_REACHED();
    return "error";
}

template <typename LexerType>
template <class TreeBuilder> TreeExpression Parser<LexerType>::parseUnaryExpression(TreeBuilder& context)
{
    typename TreeBuilder::UnaryExprContext unaryExprContext(context);
    AllowInOverride allowInOverride(this);
    int tokenStackDepth = 0;
    bool modifiesExpr = false;
    bool requiresLExpr = false;
    unsigned lastOperator = 0;
    while (isUnaryOp(m_token.m_type)) {
        if (strictMode()) {
            switch (m_token.m_type) {
            case PLUSPLUS:
            case MINUSMINUS:
            case AUTOPLUSPLUS:
            case AUTOMINUSMINUS:
                semanticFailIfTrue(requiresLExpr, "The ", operatorString(true, lastOperator), " operator requires a reference expression");
                modifiesExpr = true;
                requiresLExpr = true;
                break;
            case DELETETOKEN:
                semanticFailIfTrue(requiresLExpr, "The ", operatorString(true, lastOperator), " operator requires a reference expression");
                requiresLExpr = true;
                break;
            default:
                semanticFailIfTrue(requiresLExpr, "The ", operatorString(true, lastOperator), " operator requires a reference expression");
                break;
            }
        }
        lastOperator = m_token.m_type;
        m_nonLHSCount++;
        context.appendUnaryToken(tokenStackDepth, m_token.m_type, tokenStartPosition());
        next();
        m_nonTrivialExpressionCount++;
    }
    JSTextPosition subExprStart = tokenStartPosition();
    ASSERT(subExprStart.offset >= subExprStart.lineStartOffset);
    JSTokenLocation location(tokenLocation());
    TreeExpression expr = parseMemberExpression(context);
    if (!expr) {
        if (lastOperator)
            failWithMessage("Cannot parse subexpression of ", operatorString(true, lastOperator), "operator");
        failWithMessage("Cannot parse member expression");
    }
    bool isEvalOrArguments = false;
    if (strictMode() && !m_syntaxAlreadyValidated) {
        if (context.isResolve(expr))
            isEvalOrArguments = *m_lastIdentifier == m_vm->propertyNames->eval || *m_lastIdentifier == m_vm->propertyNames->arguments;
    }
    failIfTrueIfStrict(isEvalOrArguments && modifiesExpr, "Cannot modify '", m_lastIdentifier->impl(), "' in strict mode");
    switch (m_token.m_type) {
    case PLUSPLUS:
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        expr = context.makePostfixNode(location, expr, OpPlusPlus, subExprStart, lastTokenEndPosition(), tokenEndPosition());
        m_assignmentCount++;
        failIfTrueIfStrict(isEvalOrArguments, "Cannot modify '", m_lastIdentifier->impl(), "' in strict mode");
        semanticFailIfTrue(requiresLExpr, "The ", operatorString(false, lastOperator), " operator requires a reference expression");
        lastOperator = PLUSPLUS;
        next();
        break;
    case MINUSMINUS:
        m_nonTrivialExpressionCount++;
        m_nonLHSCount++;
        expr = context.makePostfixNode(location, expr, OpMinusMinus, subExprStart, lastTokenEndPosition(), tokenEndPosition());
        m_assignmentCount++;
        failIfTrueIfStrict(isEvalOrArguments, "'", m_lastIdentifier->impl(), "' cannot be modified in strict mode");
        semanticFailIfTrue(requiresLExpr, "The ", operatorString(false, lastOperator), " operator requires a reference expression");
        lastOperator = PLUSPLUS;
        next();
        break;
    default:
        break;
    }
    
    JSTextPosition end = lastTokenEndPosition();

    if (!TreeBuilder::CreatesAST && (m_syntaxAlreadyValidated || !strictMode()))
        return expr;

    location = tokenLocation();
    location.line = m_lexer->lastLineNumber();
    while (tokenStackDepth) {
        switch (context.unaryTokenStackLastType(tokenStackDepth)) {
        case EXCLAMATION:
            expr = context.createLogicalNot(location, expr);
            break;
        case TILDE:
            expr = context.makeBitwiseNotNode(location, expr);
            break;
        case MINUS:
            expr = context.makeNegateNode(location, expr);
            break;
        case PLUS:
            expr = context.createUnaryPlus(location, expr);
            break;
        case PLUSPLUS:
        case AUTOPLUSPLUS:
            expr = context.makePrefixNode(location, expr, OpPlusPlus, context.unaryTokenStackLastStart(tokenStackDepth), subExprStart + 1, end);
            m_assignmentCount++;
            break;
        case MINUSMINUS:
        case AUTOMINUSMINUS:
            expr = context.makePrefixNode(location, expr, OpMinusMinus, context.unaryTokenStackLastStart(tokenStackDepth), subExprStart + 1, end);
            m_assignmentCount++;
            break;
        case TYPEOF:
            expr = context.makeTypeOfNode(location, expr);
            break;
        case VOIDTOKEN:
            expr = context.createVoid(location, expr);
            break;
        case DELETETOKEN:
            failIfTrueIfStrict(context.isResolve(expr), "Cannot delete unqualified property '", m_lastIdentifier->impl(), "' in strict mode");
            expr = context.makeDeleteNode(location, expr, context.unaryTokenStackLastStart(tokenStackDepth), end, end);
            break;
        default:
            // If we get here something has gone horribly horribly wrong
            CRASH();
        }
        subExprStart = context.unaryTokenStackLastStart(tokenStackDepth);
        context.unaryTokenStackRemoveLast(tokenStackDepth);
    }
    return expr;
}


template <typename LexerType> void Parser<LexerType>::printUnexpectedTokenText(WTF::PrintStream& out)
{
    switch (m_token.m_type) {
    case EOFTOK:
        out.print("Unexpected end of script");
        return;
    case UNTERMINATED_IDENTIFIER_ESCAPE_ERRORTOK:
    case UNTERMINATED_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK:
        out.print("Incomplete unicode escape in identifier: '", getToken(), "'");
        return;
    case UNTERMINATED_MULTILINE_COMMENT_ERRORTOK:
        out.print("Unterminated multiline comment");
        return;
    case UNTERMINATED_NUMERIC_LITERAL_ERRORTOK:
        out.print("Unterminated numeric literal '", getToken(), "'");
        return;
    case UNTERMINATED_STRING_LITERAL_ERRORTOK:
        out.print("Unterminated string literal '", getToken(), "'");
        return;
    case INVALID_IDENTIFIER_ESCAPE_ERRORTOK:
        out.print("Invalid escape in identifier: '", getToken(), "'");
        return;
    case INVALID_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK:
        out.print("Invalid unicode escape in identifier: '", getToken(), "'");
        return;
    case INVALID_NUMERIC_LITERAL_ERRORTOK:
        out.print("Invalid numeric literal: '", getToken(), "'");
        return;
    case INVALID_OCTAL_NUMBER_ERRORTOK:
        out.print("Invalid use of octal: '", getToken(), "'");
        return;
    case INVALID_STRING_LITERAL_ERRORTOK:
        out.print("Invalid string literal: '", getToken(), "'");
        return;
    case ERRORTOK:
        out.print("Unrecognized token '", getToken(), "'");
        return;
    case STRING:
        out.print("Unexpected string literal ", getToken());
        return;
    case INTEGER:
    case DOUBLE:
        out.print("Unexpected number '", getToken(), "'");
        return;
    
    case RESERVED_IF_STRICT:
        out.print("Unexpected use of reserved word '", getToken(), "' in strict mode");
        return;
        
    case RESERVED:
        out.print("Unexpected use of reserved word '", getToken(), "'");
        return;

    case INVALID_PRIVATE_NAME_ERRORTOK:
        out.print("Invalid private name '", getToken(), "'");
        return;
            
    case IDENT:
        out.print("Unexpected identifier '", getToken(), "'");
        return;

    default:
        break;
    }

    if (m_token.m_type & KeywordTokenFlag) {
        out.print("Unexpected keyword '", getToken(), "'");
        return;
    }
    
    out.print("Unexpected token '", getToken(), "'");
}

// Instantiate the two flavors of Parser we need instead of putting most of this file in Parser.h
template class Parser<Lexer<LChar>>;
template class Parser<Lexer<UChar>>;
    
} // namespace JSC
