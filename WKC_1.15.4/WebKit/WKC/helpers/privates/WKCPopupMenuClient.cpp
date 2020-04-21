/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCPopupMenuClient.h"
#include "helpers/privates/WKCPopupMenuClientPrivate.h"

#include "WTFString.h"
#include "PopupMenuClient.h"
#include "helpers/WKCString.h"

namespace WKC {

PopupMenuClientPrivate::PopupMenuClientPrivate(WebCore::PopupMenuClient* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

PopupMenuClientPrivate::~PopupMenuClientPrivate()
{
}

void
PopupMenuClientPrivate::valueChanged(unsigned listIndex, bool fireEvents)
{
    m_webcore->valueChanged(listIndex, fireEvents);
}


String
PopupMenuClientPrivate::itemText(unsigned listIndex) const
{
    return m_webcore->itemText(listIndex);
}

String
PopupMenuClientPrivate::itemToolTip(unsigned listIndex) const
{
    return m_webcore->itemToolTip(listIndex);
}

bool
PopupMenuClientPrivate::itemIsEnabled(unsigned listIndex) const
{
    return m_webcore->itemIsEnabled(listIndex);
}

bool
PopupMenuClientPrivate::itemIsEnabledRespectingGroup(unsigned listIndex) const
{
    return m_webcore->itemIsEnabledRespectingGroup(listIndex);
}

int
PopupMenuClientPrivate::menuStyle_font_height() const
{
    return m_webcore->menuStyle().font().pixelSize();
}

int
PopupMenuClientPrivate::clientInsetLeft() const
{
    return m_webcore->clientInsetLeft();
}

int
PopupMenuClientPrivate::clientInsetRight() const
{
    return m_webcore->clientInsetRight();
}

int
PopupMenuClientPrivate::clientPaddingLeft() const
{
    return m_webcore->clientPaddingLeft();
}

int
PopupMenuClientPrivate::clientPaddingRight() const
{
    return m_webcore->clientPaddingRight();
}

int
PopupMenuClientPrivate::listSize() const
{
    return m_webcore->listSize();
}

int
PopupMenuClientPrivate::selectedIndex() const
{
    return m_webcore->selectedIndex();
}

void
PopupMenuClientPrivate::popupDidHide()
{
    m_webcore->popupDidHide();
}

bool
PopupMenuClientPrivate::itemIsSeparator(unsigned listIndex) const
{
    return m_webcore->itemIsSeparator(listIndex);
}

bool
PopupMenuClientPrivate::itemIsLabel(unsigned listIndex) const
{
    return m_webcore->itemIsLabel(listIndex);
}

bool
PopupMenuClientPrivate::itemIsSelected(unsigned listIndex) const
{
    return m_webcore->itemIsSelected(listIndex);
}

bool
PopupMenuClientPrivate::shouldPopOver() const
{
    return m_webcore->shouldPopOver();
}

bool
PopupMenuClientPrivate::valueShouldChangeOnHotTrack() const
{
    return m_webcore->valueShouldChangeOnHotTrack();
}

void
PopupMenuClientPrivate::setTextFromItem(unsigned listIndex)
{
    m_webcore->setTextFromItem(listIndex);
}

void
PopupMenuClientPrivate::listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow)
{
    m_webcore->listBoxSelectItem(listIndex, allowMultiplySelections, shift, fireOnChangeNow);
}

bool
PopupMenuClientPrivate::multiple()
{
    return m_webcore->multiple();
}

////////////////////////////////////////////////////////////////////////////////

PopupMenuClient::PopupMenuClient(PopupMenuClientPrivate& parent)
    : m_private(parent)
{
}

PopupMenuClient::~PopupMenuClient()
{
}

void
PopupMenuClient::valueChanged(unsigned listIndex, bool fireEvents)
{
    m_private.valueChanged(listIndex, fireEvents);
}


String
PopupMenuClient::itemText(unsigned listIndex) const
{
    return m_private.itemText(listIndex);
}

String
PopupMenuClient::itemToolTip(unsigned listIndex) const
{
    return m_private.itemToolTip(listIndex);
}

bool
PopupMenuClient::itemIsEnabledRespectingGroup(unsigned listIndex) const
{
    return m_private.itemIsEnabledRespectingGroup(listIndex);
}

bool
PopupMenuClient::itemIsEnabled(unsigned listIndex) const
{
    return m_private.itemIsEnabled(listIndex);
}

int
PopupMenuClient::menuStyle_font_height() const
{
    return m_private.menuStyle_font_height();
}

int
PopupMenuClient::clientInsetLeft() const
{
    return m_private.clientInsetLeft();
}

int
PopupMenuClient::clientInsetRight() const
{
    return m_private.clientInsetRight();
}

int
PopupMenuClient::clientPaddingLeft() const
{
    return m_private.clientPaddingLeft();
}

int
PopupMenuClient::clientPaddingRight() const
{
    return m_private.clientPaddingRight();
}

int
PopupMenuClient::listSize() const
{
    return m_private.listSize();
}

int
PopupMenuClient::selectedIndex() const
{
    return m_private.selectedIndex();
}

void
PopupMenuClient::popupDidHide()
{
    m_private.popupDidHide();
}

bool
PopupMenuClient::itemIsSeparator(unsigned listIndex) const
{
    return m_private.itemIsSeparator(listIndex);
}

bool
PopupMenuClient::itemIsLabel(unsigned listIndex) const
{
    return m_private.itemIsLabel(listIndex);
}

bool
PopupMenuClient::itemIsSelected(unsigned listIndex) const
{
    return m_private.itemIsSelected(listIndex);
}

bool
PopupMenuClient::shouldPopOver() const
{
    return m_private.shouldPopOver();
}

bool
PopupMenuClient::valueShouldChangeOnHotTrack() const
{
    return m_private.valueShouldChangeOnHotTrack();
}

void
PopupMenuClient::setTextFromItem(unsigned listIndex)
{
    m_private.setTextFromItem(listIndex);
}

void
PopupMenuClient::listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow)
{
    m_private.listBoxSelectItem(listIndex, allowMultiplySelections, shift, fireOnChangeNow);
}

bool
PopupMenuClient::multiple()
{
    return m_private.multiple();
}

} // namespace
