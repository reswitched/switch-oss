/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008, 2016 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ErrorInstance.h"

#include "JSScope.h"
#include "JSCInlines.h"
#include "JSGlobalObjectFunctions.h"
#include <wtf/Vector.h>

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ErrorInstance);

const ClassInfo ErrorInstance::s_info = { "Error", &JSNonFinalObject::s_info, 0, CREATE_METHOD_TABLE(ErrorInstance) };

ErrorInstance::ErrorInstance(VM& vm, Structure* structure)
    : JSNonFinalObject(vm, structure)
{
}

static void appendSourceToError(CallFrame* callFrame, ErrorInstance* exception, unsigned bytecodeOffset)
{
    ErrorInstance::SourceAppender appender = exception->sourceAppender();
    exception->clearSourceAppender();
    RuntimeType type = exception->runtimeTypeForCause();
    exception->clearRuntimeTypeForCause();

    if (!callFrame->codeBlock()->hasExpressionInfo())
        return;
    
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    unsigned line = 0;
    unsigned column = 0;
    
    CodeBlock* codeBlock = callFrame->codeBlock();
    codeBlock->expressionRangeForBytecodeOffset(bytecodeOffset, divotPoint, startOffset, endOffset, line, column);
    
    int expressionStart = divotPoint - startOffset;
    int expressionStop = divotPoint + endOffset;

    const String& sourceString = codeBlock->source()->source();
    if (!expressionStop || expressionStart > static_cast<int>(sourceString.length()))
        return;
    
    VM* vm = &callFrame->vm();
    JSValue jsMessage = exception->getDirect(*vm, vm->propertyNames->message);
    if (!jsMessage || !jsMessage.isString())
        return;
    
    String message = asString(jsMessage)->value(callFrame);
    if (expressionStart < expressionStop)
        message = appender(message, codeBlock->source()->getRange(expressionStart, expressionStop) , type, ErrorInstance::FoundExactSource);
    else {
        // No range information, so give a few characters of context.
        const StringImpl* data = sourceString.impl();
        int dataLength = sourceString.length();
        int start = expressionStart;
        int stop = expressionStart;
        // Get up to 20 characters of context to the left and right of the divot, clamping to the line.
        // Then strip whitespace.
        while (start > 0 && (expressionStart - start < 20) && (*data)[start - 1] != '\n')
            start--;
        while (start < (expressionStart - 1) && isStrWhiteSpace((*data)[start]))
            start++;
        while (stop < dataLength && (stop - expressionStart < 20) && (*data)[stop] != '\n')
            stop++;
        while (stop > expressionStart && isStrWhiteSpace((*data)[stop - 1]))
            stop--;
        message = appender(message, codeBlock->source()->getRange(start, stop), type, ErrorInstance::FoundApproximateSource);
    }
    exception->putDirect(*vm, vm->propertyNames->message, jsString(vm, message));

}

class FindFirstCallerFrameWithCodeblockFunctor {
public:
    FindFirstCallerFrameWithCodeblockFunctor(CallFrame* startCallFrame)
        : m_startCallFrame(startCallFrame)
        , m_foundCallFrame(nullptr)
        , m_foundStartCallFrame(false)
        , m_index(0)
    { }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        if (!m_foundStartCallFrame && (visitor->callFrame() == m_startCallFrame))
            m_foundStartCallFrame = true;

        if (m_foundStartCallFrame) {
            if (visitor->callFrame()->codeBlock()) {
                m_foundCallFrame = visitor->callFrame();
                return StackVisitor::Done;
            }
            m_index++;
        }

        return StackVisitor::Continue;
    }

    CallFrame* foundCallFrame() const { return m_foundCallFrame; }
    unsigned index() const { return m_index; }

private:
    CallFrame* m_startCallFrame;
    CallFrame* m_foundCallFrame;
    bool m_foundStartCallFrame;
    unsigned m_index;
};

static bool addErrorInfoAndGetBytecodeOffset(ExecState* exec, VM& vm, JSObject* obj, bool useCurrentFrame, CallFrame*& callFrame, unsigned &bytecodeOffset)
{
    Vector<StackFrame> stackTrace = Vector<StackFrame>();

    if (exec && stackTrace.isEmpty())
        vm.interpreter->getStackTrace(stackTrace);

    if (!stackTrace.isEmpty()) {

        ASSERT(exec == vm.topCallFrame || exec == exec->lexicalGlobalObject()->globalExec() || exec == exec->vmEntryGlobalObject()->globalExec());

        StackFrame* stackFrame;
        for (unsigned i = 0 ; i < stackTrace.size(); ++i) {
            stackFrame = &stackTrace.at(i);
            if (stackFrame->bytecodeOffset)
                break;
        }

        if (bytecodeOffset) {
            FindFirstCallerFrameWithCodeblockFunctor functor(exec);
            vm.topCallFrame->iterate(functor);
            callFrame = functor.foundCallFrame();
            unsigned stackIndex = functor.index();
            bytecodeOffset = stackTrace.at(stackIndex).bytecodeOffset;
        }
        
        unsigned line;
        unsigned column;
        stackFrame->computeLineAndColumn(line, column);
        obj->putDirect(vm, vm.propertyNames->line, jsNumber(line), ReadOnly | DontDelete);
        obj->putDirect(vm, vm.propertyNames->column, jsNumber(column), ReadOnly | DontDelete);

        if (!stackFrame->sourceURL.isEmpty())
            obj->putDirect(vm, vm.propertyNames->sourceURL, jsString(&vm, stackFrame->sourceURL), ReadOnly | DontDelete);
    
        if (!useCurrentFrame)
            stackTrace.remove(0);
        obj->putDirect(vm, vm.propertyNames->stack, vm.interpreter->stackTraceAsString(vm.topCallFrame, stackTrace), DontEnum);

        return true;
    }
    return false;
}

void ErrorInstance::finishCreation(ExecState* exec, VM& vm, const String& message, bool useCurrentFrame)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    if (!message.isNull())
        putDirect(vm, vm.propertyNames->message, jsString(&vm, message), DontEnum);

    unsigned bytecodeOffset = hasSourceAppender();
    CallFrame* callFrame = nullptr;
    bool hasTrace = addErrorInfoAndGetBytecodeOffset(exec, vm, this, useCurrentFrame, callFrame, bytecodeOffset);

    if (hasTrace && callFrame && hasSourceAppender()) {
        if (callFrame && callFrame->codeBlock()) 
            appendSourceToError(callFrame, this, bytecodeOffset);
    }
}

// Based on ErrorPrototype's errorProtoFuncToString(), but is modified to
// have no observable side effects to the user (i.e. does not call proxies,
// and getters).
String ErrorInstance::sanitizedToString(ExecState* exec)
{
    VM& vm = exec->vm();

    JSValue nameValue;
    auto namePropertName = vm.propertyNames->name;
    PropertySlot nameSlot(this);

    JSValue currentObj = this;
    unsigned prototypeDepth = 0;

    // We only check the current object and its prototype (2 levels) because normal
    // Error objects may have a name property, and if not, its prototype should have
    // a name property for the type of error e.g. "SyntaxError".
    while (currentObj.isCell() && prototypeDepth++ < 2) {
        JSObject* obj = jsCast<JSObject*>(currentObj);
        if (JSObject::getOwnPropertySlot(obj, exec, namePropertName, nameSlot) && nameSlot.isValue()) {
            nameValue = nameSlot.getValue(exec, namePropertName);
            break;
        }
        currentObj = obj->structure()->storedPrototype();
    }
    ASSERT(!vm.exception());

    String nameString;
    if (!nameValue)
        nameString = ASCIILiteral("Error");
    else {
        nameString = nameValue.toString(exec)->value(exec);
        if (vm.exception())
            return String();
    }

    JSValue messageValue;
    auto messagePropertName = vm.propertyNames->message;
    PropertySlot messageSlot(this);
    if (JSObject::getOwnPropertySlot(this, exec, messagePropertName, messageSlot) && messageSlot.isValue())
        messageValue = messageSlot.getValue(exec, messagePropertName);
    ASSERT(!vm.exception());

    String messageString;
    if (!messageValue)
        messageString = String();
    else {
        messageString = messageValue.toString(exec)->value(exec);
        if (vm.exception())
            return String();
    }

    if (!nameString.length())
        return messageString;

    if (!messageString.length())
        return nameString;

    StringBuilder builder;
    builder.append(nameString);
    builder.append(": ");
    builder.append(messageString);
    return builder.toString();
}

} // namespace JSC
