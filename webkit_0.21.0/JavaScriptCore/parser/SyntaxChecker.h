/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef SyntaxChecker_h
#define SyntaxChecker_h

#include "Lexer.h"
#include "ParserFunctionInfo.h"
#include "YarrSyntaxChecker.h"

namespace JSC {
    
class SyntaxChecker {
public:
    struct BinaryExprContext {
        BinaryExprContext(SyntaxChecker& context)
            : m_context(&context)
        {
            m_context->m_topBinaryExprs.append(m_context->m_topBinaryExpr);
            m_context->m_topBinaryExpr = 0;
        }
        ~BinaryExprContext()
        {
            m_context->m_topBinaryExpr = m_context->m_topBinaryExprs.last();
            m_context->m_topBinaryExprs.removeLast();
        }
    private:
        SyntaxChecker* m_context;
    };
    struct UnaryExprContext {
        UnaryExprContext(SyntaxChecker& context)
            : m_context(&context)
        {
            m_context->m_topUnaryTokens.append(m_context->m_topUnaryToken);
            m_context->m_topUnaryToken = 0;
        }
        ~UnaryExprContext()
        {
            m_context->m_topUnaryToken = m_context->m_topUnaryTokens.last();
            m_context->m_topUnaryTokens.removeLast();
        }
    private:
        SyntaxChecker* m_context;
    };
    
    SyntaxChecker(VM* , void*)
    {
    }

    enum { NoneExpr = 0,
        ResolveEvalExpr, ResolveExpr, IntegerExpr, DoubleExpr, StringExpr,
        ThisExpr, NullExpr, BoolExpr, RegExpExpr, ObjectLiteralExpr,
        FunctionExpr, ClassExpr, SuperExpr, BracketExpr, DotExpr, CallExpr,
        NewExpr, PreExpr, PostExpr, UnaryExpr, BinaryExpr,
        ConditionalExpr, AssignmentExpr, TypeofExpr,
        DeleteExpr, ArrayLiteralExpr, BindingDestructuring,
        ArrayDestructuring, ObjectDestructuring, SourceElementsResult,
        FunctionBodyResult, SpreadExpr, ArgumentsResult,
        PropertyListResult, ArgumentsListResult, ElementsListResult,
        StatementResult, FormalParameterListResult, ClauseResult,
        ClauseListResult, CommaExpr, DestructuringAssignment,
        TemplateStringResult, TemplateStringListResult,
        TemplateExpressionListResult, TemplateExpr,
        TaggedTemplateExpr
    };
    typedef int ExpressionType;

    typedef ExpressionType Expression;
    typedef int SourceElements;
    typedef int Arguments;
    typedef ExpressionType Comma;
    struct Property {
        ALWAYS_INLINE Property(void* = 0)
            : type((PropertyNode::Type)0)
        {
        }
        ALWAYS_INLINE Property(const Identifier* ident, PropertyNode::Type ty)
            : name(ident)
            , type(ty)
        {
        }
        ALWAYS_INLINE Property(PropertyNode::Type ty)
            : name(0)
            , type(ty)
        {
        }
        ALWAYS_INLINE bool operator!() { return !type; }
        const Identifier* name;
        PropertyNode::Type type;
    };
    typedef int PropertyList;
    typedef int ElementList;
    typedef int ArgumentsList;
#if ENABLE(ES6_TEMPLATE_LITERAL_SYNTAX)
    typedef int TemplateExpressionList;
    typedef int TemplateString;
    typedef int TemplateStringList;
    typedef int TemplateLiteral;
#endif
    typedef int FormalParameterList;
    typedef int FunctionBody;
#if ENABLE(ES6_CLASS_SYNTAX)
    typedef int ClassExpression;
#endif
    typedef int Statement;
    typedef int ClauseList;
    typedef int Clause;
    typedef int ConstDeclList;
    typedef int BinaryOperand;
    typedef int DestructuringPattern;
    typedef DestructuringPattern ArrayPattern;
    typedef DestructuringPattern ObjectPattern;

    static const bool CreatesAST = false;
    static const bool NeedsFreeVariableInfo = false;
    static const bool CanUseFunctionCache = true;
    static const unsigned DontBuildKeywords = LexexFlagsDontBuildKeywords;
    static const unsigned DontBuildStrings = LexerFlagsDontBuildStrings;

    int createSourceElements() { return SourceElementsResult; }
    ExpressionType makeFunctionCallNode(const JSTokenLocation&, int, int, int, int, int) { return CallExpr; }
    ExpressionType createCommaExpr(const JSTokenLocation&, ExpressionType expr) { return expr; }
    ExpressionType appendToCommaExpr(const JSTokenLocation&, ExpressionType& head, ExpressionType, ExpressionType next) { head = next; return next; }
    ExpressionType makeAssignNode(const JSTokenLocation&, ExpressionType, Operator, ExpressionType, bool, bool, int, int, int) { return AssignmentExpr; }
    ExpressionType makePrefixNode(const JSTokenLocation&, ExpressionType, Operator, int, int, int) { return PreExpr; }
    ExpressionType makePostfixNode(const JSTokenLocation&, ExpressionType, Operator, int, int, int) { return PostExpr; }
    ExpressionType makeTypeOfNode(const JSTokenLocation&, ExpressionType) { return TypeofExpr; }
    ExpressionType makeDeleteNode(const JSTokenLocation&, ExpressionType, int, int, int) { return DeleteExpr; }
    ExpressionType makeNegateNode(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType makeBitwiseNotNode(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType createLogicalNot(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType createUnaryPlus(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType createVoid(const JSTokenLocation&, ExpressionType) { return UnaryExpr; }
    ExpressionType thisExpr(const JSTokenLocation&, ThisTDZMode) { return ThisExpr; }
    ExpressionType superExpr(const JSTokenLocation&) { return SuperExpr; }
    ExpressionType createResolve(const JSTokenLocation&, const Identifier&, int, int) { return ResolveExpr; }
    ExpressionType createObjectLiteral(const JSTokenLocation&) { return ObjectLiteralExpr; }
    ExpressionType createObjectLiteral(const JSTokenLocation&, int) { return ObjectLiteralExpr; }
    ExpressionType createArray(const JSTokenLocation&, int) { return ArrayLiteralExpr; }
    ExpressionType createArray(const JSTokenLocation&, int, int) { return ArrayLiteralExpr; }
    ExpressionType createDoubleExpr(const JSTokenLocation&, double) { return DoubleExpr; }
    ExpressionType createIntegerExpr(const JSTokenLocation&, double) { return IntegerExpr; }
    ExpressionType createString(const JSTokenLocation&, const Identifier*) { return StringExpr; }
    ExpressionType createBoolean(const JSTokenLocation&, bool) { return BoolExpr; }
    ExpressionType createNull(const JSTokenLocation&) { return NullExpr; }
    ExpressionType createBracketAccess(const JSTokenLocation&, ExpressionType, ExpressionType, bool, int, int, int) { return BracketExpr; }
    ExpressionType createDotAccess(const JSTokenLocation&, ExpressionType, const Identifier*, int, int, int) { return DotExpr; }
    ExpressionType createRegExp(const JSTokenLocation&, const Identifier& pattern, const Identifier&, int) { return Yarr::checkSyntax(pattern.string()) ? 0 : RegExpExpr; }
    ExpressionType createNewExpr(const JSTokenLocation&, ExpressionType, int, int, int, int) { return NewExpr; }
    ExpressionType createNewExpr(const JSTokenLocation&, ExpressionType, int, int) { return NewExpr; }
    ExpressionType createConditionalExpr(const JSTokenLocation&, ExpressionType, ExpressionType, ExpressionType) { return ConditionalExpr; }
    ExpressionType createAssignResolve(const JSTokenLocation&, const Identifier&, ExpressionType, int, int, int) { return AssignmentExpr; }
    ExpressionType createEmptyVarExpression(const JSTokenLocation&, const Identifier&) { return AssignmentExpr; }
#if ENABLE(ES6_CLASS_SYNTAX)
    ClassExpression createClassExpr(const JSTokenLocation&, const Identifier&, ExpressionType, ExpressionType, PropertyList, PropertyList) { return ClassExpr; }
#endif
    ExpressionType createFunctionExpr(const JSTokenLocation&, const ParserFunctionInfo<SyntaxChecker>&) { return FunctionExpr; }
    int createFunctionBody(const JSTokenLocation&, const JSTokenLocation&, int, int, bool, int, int, int, ConstructorKind, unsigned, int, bool) { return FunctionBodyResult; }
    ExpressionType createArrowFunctionExpr(const JSTokenLocation&, const ParserFunctionInfo<SyntaxChecker>&) { return FunctionExpr; }
    void setFunctionNameStart(int, int) { }
    int createArguments() { return ArgumentsResult; }
    int createArguments(int) { return ArgumentsResult; }
    ExpressionType createSpreadExpression(const JSTokenLocation&, ExpressionType, int, int, int) { return SpreadExpr; }
#if ENABLE(ES6_TEMPLATE_LITERAL_SYNTAX)
    TemplateString createTemplateString(const JSTokenLocation&, const Identifier&, const Identifier&) { return TemplateStringResult; }
    TemplateStringList createTemplateStringList(TemplateString) { return TemplateStringListResult; }
    TemplateStringList createTemplateStringList(TemplateStringList, TemplateString) { return TemplateStringListResult; }
    TemplateExpressionList createTemplateExpressionList(Expression) { return TemplateExpressionListResult; }
    TemplateExpressionList createTemplateExpressionList(TemplateExpressionList, Expression) { return TemplateExpressionListResult; }
    TemplateLiteral createTemplateLiteral(const JSTokenLocation&, TemplateStringList) { return TemplateExpr; }
    TemplateLiteral createTemplateLiteral(const JSTokenLocation&, TemplateStringList, TemplateExpressionList) { return TemplateExpr; }
    ExpressionType createTaggedTemplate(const JSTokenLocation&, ExpressionType, TemplateLiteral, int, int, int) { return TaggedTemplateExpr; }
#endif

    int createArgumentsList(const JSTokenLocation&, int) { return ArgumentsListResult; }
    int createArgumentsList(const JSTokenLocation&, int, int) { return ArgumentsListResult; }
    Property createProperty(const Identifier* name, int, PropertyNode::Type type, PropertyNode::PutType, bool complete, SuperBinding = SuperBinding::NotNeeded)
    {
        if (!complete)
            return Property(type);
        ASSERT(name);
        return Property(name, type);
    }
    Property createProperty(VM* vm, ParserArena& parserArena, double name, int, PropertyNode::Type type, PropertyNode::PutType, bool complete)
    {
        if (!complete)
            return Property(type);
        return Property(&parserArena.identifierArena().makeNumericIdentifier(vm, name), type);
    }
    Property createProperty(int, int, PropertyNode::Type type, PropertyNode::PutType, bool)
    {
        return Property(type);
    }
    int createPropertyList(const JSTokenLocation&, Property) { return PropertyListResult; }
    int createPropertyList(const JSTokenLocation&, Property, int) { return PropertyListResult; }
    int createElementList(int, int) { return ElementsListResult; }
    int createElementList(int, int, int) { return ElementsListResult; }
    int createFormalParameterList() { return FormalParameterListResult; }
    void appendParameter(int, DestructuringPattern) { }
    int createClause(int, int) { return ClauseResult; }
    int createClauseList(int) { return ClauseListResult; }
    int createClauseList(int, int) { return ClauseListResult; }
    int createFuncDeclStatement(const JSTokenLocation&, const ParserFunctionInfo<SyntaxChecker>&) { return StatementResult; }
#if ENABLE(ES6_CLASS_SYNTAX)
    int createClassDeclStatement(const JSTokenLocation&, ClassExpression,
        const JSTextPosition&, const JSTextPosition&, int, int) { return StatementResult; }
#endif
    int createBlockStatement(const JSTokenLocation&, int, int, int) { return StatementResult; }
    int createExprStatement(const JSTokenLocation&, int, int, int) { return StatementResult; }
    int createIfStatement(const JSTokenLocation&, int, int, int, int) { return StatementResult; }
    int createIfStatement(const JSTokenLocation&, int, int, int, int, int) { return StatementResult; }
    int createForLoop(const JSTokenLocation&, int, int, int, int, int, int) { return StatementResult; }
    int createForInLoop(const JSTokenLocation&, int, int, int, int, int, int, int, int) { return StatementResult; }
    int createForOfLoop(const JSTokenLocation&, int, int, int, int, int, int, int, int) { return StatementResult; }
    int createEmptyStatement(const JSTokenLocation&) { return StatementResult; }
    int createVarStatement(const JSTokenLocation&, int, int, int) { return StatementResult; }
    int createReturnStatement(const JSTokenLocation&, int, int, int) { return StatementResult; }
    int createBreakStatement(const JSTokenLocation&, int, int) { return StatementResult; }
    int createBreakStatement(const JSTokenLocation&, const Identifier*, int, int) { return StatementResult; }
    int createContinueStatement(const JSTokenLocation&, int, int) { return StatementResult; }
    int createContinueStatement(const JSTokenLocation&, const Identifier*, int, int) { return StatementResult; }
    int createTryStatement(const JSTokenLocation&, int, const Identifier*, int, int, int, int) { return StatementResult; }
    int createSwitchStatement(const JSTokenLocation&, int, int, int, int, int, int) { return StatementResult; }
    int createWhileStatement(const JSTokenLocation&, int, int, int, int) { return StatementResult; }
    int createWithStatement(const JSTokenLocation&, int, int, int, int, int, int) { return StatementResult; }
    int createDoWhileStatement(const JSTokenLocation&, int, int, int, int) { return StatementResult; }
    int createLabelStatement(const JSTokenLocation&, const Identifier*, int, int, int) { return StatementResult; }
    int createThrowStatement(const JSTokenLocation&, int, int, int) { return StatementResult; }
    int createDebugger(const JSTokenLocation&, int, int) { return StatementResult; }
    int createConstStatement(const JSTokenLocation&, int, int, int) { return StatementResult; }
    int appendConstDecl(const JSTokenLocation&, int, const Identifier*, int) { return StatementResult; }
    Property createGetterOrSetterProperty(const JSTokenLocation&, PropertyNode::Type type, bool strict, const Identifier* name, const ParserFunctionInfo<SyntaxChecker>&, SuperBinding)
    {
        ASSERT(name);
        if (!strict)
            return Property(type);
        return Property(name, type);
    }
    Property createGetterOrSetterProperty(VM* vm, ParserArena& parserArena, const JSTokenLocation&, PropertyNode::Type type, bool strict, double name, const ParserFunctionInfo<SyntaxChecker>&, SuperBinding)
    {
        if (!strict)
            return Property(type);
        return Property(&parserArena.identifierArena().makeNumericIdentifier(vm, name), type);
    }

    void appendStatement(int, int) { }
    void addVar(const Identifier*, bool) { }
    int combineCommaNodes(const JSTokenLocation&, int, int) { return CommaExpr; }
    int evalCount() const { return 0; }
    void appendBinaryExpressionInfo(int& operandStackDepth, int expr, int, int, int, bool)
    {
        if (!m_topBinaryExpr)
            m_topBinaryExpr = expr;
        else
            m_topBinaryExpr = BinaryExpr;
        operandStackDepth++;
    }
    
    // Logic to handle datastructures used during parsing of binary expressions
    void operatorStackPop(int& operatorStackDepth) { operatorStackDepth--; }
    bool operatorStackHasHigherPrecedence(int&, int) { return true; }
    BinaryOperand getFromOperandStack(int) { return m_topBinaryExpr; }
    void shrinkOperandStackBy(int& operandStackDepth, int amount) { operandStackDepth -= amount; }
    void appendBinaryOperation(const JSTokenLocation&, int& operandStackDepth, int&, BinaryOperand, BinaryOperand) { operandStackDepth++; }
    void operatorStackAppend(int& operatorStackDepth, int, int) { operatorStackDepth++; }
    int popOperandStack(int&) { int res = m_topBinaryExpr; m_topBinaryExpr = 0; return res; }
    
    void appendUnaryToken(int& stackDepth, int tok, int) { stackDepth = 1; m_topUnaryToken = tok; }
    int unaryTokenStackLastType(int&) { return m_topUnaryToken; }
    JSTextPosition unaryTokenStackLastStart(int&) { return JSTextPosition(0, 0, 0); }
    void unaryTokenStackRemoveLast(int& stackDepth) { stackDepth = 0; }
    
    void assignmentStackAppend(int, int, int, int, int, Operator) { }
    int createAssignment(const JSTokenLocation&, int, int, int, int, int) { RELEASE_ASSERT_NOT_REACHED(); return AssignmentExpr; }
    const Identifier* getName(const Property& property) const { return property.name; }
    PropertyNode::Type getType(const Property& property) const { return property.type; }
    bool isResolve(ExpressionType expr) const { return expr == ResolveExpr || expr == ResolveEvalExpr; }
    ExpressionType createDestructuringAssignment(const JSTokenLocation&, int, ExpressionType)
    {
        return DestructuringAssignment;
    }
    
    ArrayPattern createArrayPattern(const JSTokenLocation&)
    {
        return ArrayDestructuring;
    }
    void appendArrayPatternSkipEntry(ArrayPattern, const JSTokenLocation&)
    {
    }
    void appendArrayPatternEntry(ArrayPattern, const JSTokenLocation&, DestructuringPattern, int)
    {
    }
    void appendArrayPatternRestEntry(ArrayPattern, const JSTokenLocation&, DestructuringPattern)
    {
    }
    void finishArrayPattern(ArrayPattern, const JSTextPosition&, const JSTextPosition&, const JSTextPosition&)
    {
    }
    ObjectPattern createObjectPattern(const JSTokenLocation&)
    {
        return ObjectDestructuring;
    }
    void appendObjectPatternEntry(ArrayPattern, const JSTokenLocation&, bool, const Identifier&, DestructuringPattern, int)
    {
    }
    DestructuringPattern createBindingLocation(const JSTokenLocation&, const Identifier&, const JSTextPosition&, const JSTextPosition&)
    {
        return BindingDestructuring;
    }

    bool isBindingNode(DestructuringPattern pattern)
    {
        return pattern == BindingDestructuring;
    }

    void setEndOffset(int, int) { }
    int endOffset(int) { return 0; }
    void setStartOffset(int, int) { }

private:
    int m_topBinaryExpr;
    int m_topUnaryToken;
    Vector<int, 8> m_topBinaryExprs;
    Vector<int, 8> m_topUnaryTokens;
};

}

#endif
