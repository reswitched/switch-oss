/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
 * Copyright (c) 2009,2010 ACCESS CO., LTD. All rights reserved.
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
#include "AccessibilityUIElement.h"

#include <JavaScriptCore/JSStringRef.h>
#include <wtf/Assertions.h>


AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
{
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : m_element(other.m_element)
{
}

AccessibilityUIElement::~AccessibilityUIElement()
{
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>& elements)
{
    // FIXME: implement
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>&)
{
    // FIXME: implement
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>& children)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    int count = childrenCount();
    for (int i = 0; i < count; i++) {
        AtkObject* child = atk_object_ref_accessible_child(ATK_OBJECT(m_element), i);
        children.append(AccessibilityUIElement(child));
    }
#endif
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>& elementVector, unsigned start, unsigned end)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    for (unsigned i = start; i < end; i++) {
        AtkObject* child = atk_object_ref_accessible_child(ATK_OBJECT(m_element), i);
        elementVector.append(AccessibilityUIElement(child));
    }
#endif
}

int AccessibilityUIElement::childrenCount()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    if (!m_element)
        return 0;

    ASSERT(ATK_IS_OBJECT(m_element));

    return atk_object_get_n_accessible_children(ATK_OBJECT(m_element));
#endif
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int x, int y)
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned index)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    Vector<AccessibilityUIElement> children;
    getChildrenWithRange(children, index, index + 1);

    if (children.size() == 1)
        return children.at(0);

    return 0;
#endif
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    ASSERT(m_element);
    AtkObject* parent =  atk_object_get_parent(ATK_OBJECT(m_element));

    return parent ? AccessibilityUIElement(parent) : 0;
#endif
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::role()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return JSStringCreateWithCharacters(0, 0);
#else
    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element));

    if (!role)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_role_get_name(role));
#endif
}

JSStringRef AccessibilityUIElement::subrole()
{
    return 0;
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    return 0;
}

JSStringRef AccessibilityUIElement::title()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return JSStringCreateWithCharacters(0, 0);
#else
    const gchar* name = atk_object_get_name(ATK_OBJECT(m_element));

    if (!name)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(name);
#endif
}

JSStringRef AccessibilityUIElement::description()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return JSStringCreateWithCharacters(0, 0);
#else
    const gchar* description = atk_object_get_description(ATK_OBJECT(m_element));

    if (!description)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(description);
#endif
}

JSStringRef AccessibilityUIElement::stringValue()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::language()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

double AccessibilityUIElement::x()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    int x, y;

    atk_component_get_position(ATK_COMPONENT(m_element), &x, &y, ATK_XY_SCREEN);

    return x;
#endif
}

double AccessibilityUIElement::y()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    int x, y;

    atk_component_get_position(ATK_COMPONENT(m_element), &x, &y, ATK_XY_SCREEN);

    return y;
#endif
}

double AccessibilityUIElement::width()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    int width, height;

    atk_component_get_size(ATK_COMPONENT(m_element), &width, &height);

    return width;
#endif
}

double AccessibilityUIElement::height()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0;
#else
    int width, height;

    atk_component_get_size(ATK_COMPONENT(m_element), &width, &height);

    return height;
#endif
}

double AccessibilityUIElement::clickPointX()
{
    return 0.f;
}

double AccessibilityUIElement::clickPointY()
{
    return 0.f;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    return 0;
}

double AccessibilityUIElement::intValue() const
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0.f;
#else
    GValue value = { 0, { { 0 } } };

    if (!ATK_IS_VALUE(m_element))
        return 0.0f;

    atk_value_get_current_value(ATK_VALUE(m_element), &value);

    if (G_VALUE_HOLDS_DOUBLE(&value))
        return g_value_get_double(&value);
    else if (G_VALUE_HOLDS_INT(&value))
        return static_cast<double>(g_value_get_int(&value));
    else
        return 0.0f;
#endif
}

double AccessibilityUIElement::minValue()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0.f;
#else
    GValue value = { 0, { { 0 } } };

    if (!ATK_IS_VALUE(m_element))
        return 0.0f;

    atk_value_get_minimum_value(ATK_VALUE(m_element), &value);

    if (G_VALUE_HOLDS_DOUBLE(&value))
        return g_value_get_double(&value);
    else if (G_VALUE_HOLDS_INT(&value))
        return static_cast<double>(g_value_get_int(&value));
    else
        return 0.0f;
#endif
}

double AccessibilityUIElement::maxValue()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return 0.f;
#else
    GValue value = { 0, { { 0 } } };

    if (!ATK_IS_VALUE(m_element))
        return 0.0f;

    atk_value_get_maximum_value(ATK_VALUE(m_element), &value);

    if (G_VALUE_HOLDS_DOUBLE(&value))
        return g_value_get_double(&value);
    else if (G_VALUE_HOLDS_INT(&value))
        return static_cast<double>(g_value_get_int(&value));
    else
        return 0.0f;
#endif
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::isEnabled()
{
    // FIXME: implement
    return false;
}


int AccessibilityUIElement::insertionPointLineNumber()
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::isActionSupported(JSStringRef action)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isRequired() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelected() const
{
    // FIXME: implement
    return false;
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    return false;
}

JSStringRef AccessibilityUIElement::ariaDropEffects() const
{
    return 0;
}

bool AccessibilityUIElement::isExpanded() const
{
    // FIXME: implement
    return false;
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::indexInTable()
{
    // FIXME: implement
    return 0;
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::lineForIndex(int)
{
    // FIXME: implement
    return 0;
}

JSStringRef AccessibilityUIElement::boundsForRange(unsigned location, unsigned length)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned, unsigned)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned column, unsigned row)
{
    // FIXME: implement
    return 0;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

void AccessibilityUIElement::setSelectedTextRange(unsigned location, unsigned length)
{
    // FIXME: implement
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef attribute)
{
    return false;
}

void AccessibilityUIElement::increment()
{
    // FIXME: implement
}

void AccessibilityUIElement::decrement()
{
    // FIXME: implement
}

void AccessibilityUIElement::showMenu()
{
    // FIXME: implement
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    return 0;
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentEncoding()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return JSStringCreateWithCharacters(0, 0);
#else
    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element));
    if (role != ATK_ROLE_DOCUMENT_FRAME)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_document_get_attribute_value(ATK_DOCUMENT(m_element), "Encoding"));
#endif
}

JSStringRef AccessibilityUIElement::documentURI()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return JSStringCreateWithCharacters(0, 0);
#else
    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element));
    if (role != ATK_ROLE_DOCUMENT_FRAME)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_document_get_attribute_value(ATK_DOCUMENT(m_element), "URI"));
#endif
}

JSStringRef AccessibilityUIElement::url()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

    
JSObjectRef AccessibilityUIElement::makeJSAccessibilityUIElement(JSContextRef, const AccessibilityUIElement&)
{
    return 0;
}

bool isEqual(AccessibilityUIElement* otherElement)
{
    return false;
}


unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement*)
{
    return 0;
}

void AccessibilityUIElement::takeFocus()
{
    return;
}
void AccessibilityUIElement::takeSelection()
{
    return;
}
void AccessibilityUIElement::addSelection()
{
    return;
}
void AccessibilityUIElement::removeSelection()
{
    return;
}

AccessibilityUIElement AccessibilityUIElement::linkedUIElementAtIndex(unsigned)
{
    return 0;
}    

void AccessibilityUIElement::press()
{
    return;
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef attribute)
{
    return 0;
}
bool AccessibilityUIElement::boolAttributeValue(JSStringRef attribute)
{
    return false;
}
JSStringRef AccessibilityUIElement::helpText() const
{
    return 0;
}
bool AccessibilityUIElement::isFocused() const
{
    return false;
}
bool AccessibilityUIElement::isFocusable() const
{
    return false;
}
bool AccessibilityUIElement::isSelectable() const
{
    return false;
}
bool AccessibilityUIElement::isMultiSelectable() const
{
    return false;
}
void AccessibilityUIElement::setSelectedChild(AccessibilityUIElement*) const
{
    return;
}
unsigned AccessibilityUIElement::selectedChildrenCount() const
{
    return 0;
}
AccessibilityUIElement AccessibilityUIElement::selectedChildAtIndex(unsigned) const
{
    return 0;
}    

bool AccessibilityUIElement::isChecked() const
{
    return false;
}
bool AccessibilityUIElement::isVisible() const
{
    return false;
}
bool AccessibilityUIElement::isOffScreen() const
{
    return false;
}
bool AccessibilityUIElement::isCollapsed() const
{
    return false;
}
bool AccessibilityUIElement::isIgnored() const
{
    return false;
}
bool AccessibilityUIElement::hasPopup() const
{
    return false;
}

JSStringRef AccessibilityUIElement::speak()
{
    return 0;
}    

int AccessibilityUIElement::rowCount()
{
    return 0;
}
int AccessibilityUIElement::columnCount()
{
    return 0;
}


JSStringRef AccessibilityUIElement::rangeForLine(int)
{
    return 0;
}
JSStringRef AccessibilityUIElement::attributedStringForRange(unsigned location, unsigned length)
{
    return 0;
}
bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned location, unsigned length)
{
    return 0;
}


AccessibilityTextMarkerRange AccessibilityUIElement::textMarkerRangeForElement(AccessibilityUIElement*)
{
    return 0;
}

AccessibilityTextMarkerRange AccessibilityUIElement::textMarkerRangeForMarkers(AccessibilityTextMarker* startMarker, AccessibilityTextMarker* endMarker)
{
    return 0;
}
AccessibilityTextMarker AccessibilityUIElement::startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*)
{
    return 0;
}
AccessibilityTextMarker AccessibilityUIElement::endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange*)
{
    return 0;
}
AccessibilityTextMarker AccessibilityUIElement::textMarkerForPoint(int x, int y)
{
    return 0;
}
AccessibilityTextMarker AccessibilityUIElement::previousTextMarker(AccessibilityTextMarker*)
{
    return 0;
}
AccessibilityTextMarker AccessibilityUIElement::nextTextMarker(AccessibilityTextMarker*)
{
    return 0;
}
AccessibilityUIElement AccessibilityUIElement::accessibilityElementForTextMarker(AccessibilityTextMarker*)
{
    return 0;
}
JSStringRef AccessibilityUIElement::stringForTextMarkerRange(AccessibilityTextMarkerRange*)
{
    return 0;
}
int AccessibilityUIElement::textMarkerRangeLength(AccessibilityTextMarkerRange*)
{
    return 0;
}    

bool AccessibilityUIElement::addNotificationListener(JSObjectRef functionCallback)
{
    return false;
}
void AccessibilityUIElement::removeNotificationListener()
{
    return;
}
