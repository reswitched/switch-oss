/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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


#include "config.h"

#include "PropertyDescriptor.h"

#include "GetterSetter.h"
#include "JSObject.h"
#include "JSCInlines.h"

namespace JSC {
unsigned PropertyDescriptor::defaultAttributes = DontDelete | DontEnum | ReadOnly;

bool PropertyDescriptor::writable() const
{
    ASSERT(!isAccessorDescriptor());
    return !(m_attributes & ReadOnly);
}

bool PropertyDescriptor::enumerable() const
{
    return !(m_attributes & DontEnum);
}

bool PropertyDescriptor::configurable() const
{
    return !(m_attributes & DontDelete);
}

bool PropertyDescriptor::isDataDescriptor() const
{
    return m_value || (m_seenAttributes & WritablePresent);
}

bool PropertyDescriptor::isGenericDescriptor() const
{
    return !isAccessorDescriptor() && !isDataDescriptor();
}

bool PropertyDescriptor::isAccessorDescriptor() const
{
    return m_getter || m_setter;
}

void PropertyDescriptor::setUndefined()
{
    m_value = jsUndefined();
    m_attributes = ReadOnly | DontDelete | DontEnum;
}

JSValue PropertyDescriptor::getter() const
{
    ASSERT(isAccessorDescriptor());
    return m_getter;
}

JSValue PropertyDescriptor::setter() const
{
    ASSERT(isAccessorDescriptor());
    return m_setter;
}

JSObject* PropertyDescriptor::getterObject() const
{
    ASSERT(isAccessorDescriptor() && getterPresent());
    return m_getter.isObject() ? asObject(m_getter) : 0;
}

JSObject* PropertyDescriptor::setterObject() const
{
    ASSERT(isAccessorDescriptor() && setterPresent());
    return m_setter.isObject() ? asObject(m_setter) : 0;
}

void PropertyDescriptor::setDescriptor(JSValue value, unsigned attributes)
{
    ASSERT(value);
    ASSERT(value.isGetterSetter() == !!(attributes & Accessor));

    // We need to mask off the PropertyAttribute::CustomValue bit because
    // PropertyDescriptor::attributesEqual() does an equivalent test on
    // m_attributes, and a property that has a CustomValue should be indistinguishable
    // from a property that has a normal value as far as JS code is concerned.
    // PropertyAttribute does not need knowledge of the underlying implementation
    // actually being a CustomValue. So, we'll just mask it off up front here.
    m_attributes = attributes & ~CustomValue;
    if (value.isGetterSetter()) {
        m_attributes &= ~ReadOnly; // FIXME: we should be able to ASSERT this!

        GetterSetter* accessor = asGetterSetter(value);
        m_getter = !accessor->isGetterNull() ? accessor->getter() : jsUndefined();
        m_setter = !accessor->isSetterNull() ? accessor->setter() : jsUndefined();
        m_seenAttributes = EnumerablePresent | ConfigurablePresent;
    } else {
        m_value = value;
        m_seenAttributes = EnumerablePresent | ConfigurablePresent | WritablePresent;
    }
}

void PropertyDescriptor::setCustomDescriptor(unsigned attributes)
{
    ASSERT(!(attributes & CustomValue));
    m_attributes = attributes | Accessor | CustomAccessor;
    m_attributes &= ~ReadOnly;
    m_seenAttributes = EnumerablePresent | ConfigurablePresent;
    setGetter(jsUndefined());
    setSetter(jsUndefined());
    m_value = JSValue();
}

void PropertyDescriptor::setAccessorDescriptor(GetterSetter* accessor, unsigned attributes)
{
    ASSERT(attributes & Accessor);
    ASSERT(!(attributes & CustomValue));
    attributes &= ~ReadOnly; // FIXME: we should be able to ASSERT this!

    m_attributes = attributes;
    m_getter = !accessor->isGetterNull() ? accessor->getter() : jsUndefined();
    m_setter = !accessor->isSetterNull() ? accessor->setter() : jsUndefined();
    m_seenAttributes = EnumerablePresent | ConfigurablePresent;
}

void PropertyDescriptor::setWritable(bool writable)
{
    if (writable)
        m_attributes &= ~ReadOnly;
    else
        m_attributes |= ReadOnly;
    m_seenAttributes |= WritablePresent;
}

void PropertyDescriptor::setEnumerable(bool enumerable)
{
    if (enumerable)
        m_attributes &= ~DontEnum;
    else
        m_attributes |= DontEnum;
    m_seenAttributes |= EnumerablePresent;
}

void PropertyDescriptor::setConfigurable(bool configurable)
{
    if (configurable)
        m_attributes &= ~DontDelete;
    else
        m_attributes |= DontDelete;
    m_seenAttributes |= ConfigurablePresent;
}

void PropertyDescriptor::setSetter(JSValue setter)
{
    m_setter = setter;
    m_attributes |= Accessor;
    m_attributes &= ~ReadOnly;
}

void PropertyDescriptor::setGetter(JSValue getter)
{
    m_getter = getter;
    m_attributes |= Accessor;
    m_attributes &= ~ReadOnly;
}

// See ES5.1 9.12
bool sameValue(ExecState* exec, JSValue a, JSValue b)
{
    if (!a.isNumber())
        return JSValue::strictEqual(exec, a, b);
    if (!b.isNumber())
        return false;
    double x = a.asNumber();
    double y = b.asNumber();
    bool xIsNaN = std::isnan(x);
    bool yIsNaN = std::isnan(y);
    if (xIsNaN || yIsNaN)
        return xIsNaN && yIsNaN;
    return bitwise_cast<uint64_t>(x) == bitwise_cast<uint64_t>(y);
}

bool PropertyDescriptor::equalTo(ExecState* exec, const PropertyDescriptor& other) const
{
    if (other.m_value.isEmpty() != m_value.isEmpty()
        || other.m_getter.isEmpty() != m_getter.isEmpty()
        || other.m_setter.isEmpty() != m_setter.isEmpty())
        return false;
    return (!m_value || sameValue(exec, other.m_value, m_value))
        && (!m_getter || JSValue::strictEqual(exec, other.m_getter, m_getter))
        && (!m_setter || JSValue::strictEqual(exec, other.m_setter, m_setter))
        && attributesEqual(other);
}

bool PropertyDescriptor::attributesEqual(const PropertyDescriptor& other) const
{
    unsigned mismatch = other.m_attributes ^ m_attributes;
    unsigned sharedSeen = other.m_seenAttributes & m_seenAttributes;
    if (sharedSeen & WritablePresent && mismatch & ReadOnly)
        return false;
    if (sharedSeen & ConfigurablePresent && mismatch & DontDelete)
        return false;
    if (sharedSeen & EnumerablePresent && mismatch & DontEnum)
        return false;
    return true;
}

unsigned PropertyDescriptor::attributesOverridingCurrent(const PropertyDescriptor& current) const
{
    unsigned currentAttributes = current.m_attributes;
    if (isDataDescriptor() && current.isAccessorDescriptor())
        currentAttributes |= ReadOnly;
    unsigned overrideMask = 0;
    if (writablePresent())
        overrideMask |= ReadOnly;
    if (enumerablePresent())
        overrideMask |= DontEnum;
    if (configurablePresent())
        overrideMask |= DontDelete;
    if (isAccessorDescriptor())
        overrideMask |= Accessor;
    return (m_attributes & overrideMask) | (currentAttributes & ~overrideMask);
}

}
