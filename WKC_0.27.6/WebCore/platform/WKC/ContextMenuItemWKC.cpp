/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (c) 2010, 2012 ACCESS CO., LTD. All rights reserved.
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
 */

#include "config.h"

#if ENABLE(CONTEXT_MENUS)

#include "ContextMenu.h"
#include "ContextMenuItem.h"
#include "CString.h"
#include "NotImplemented.h"

#define WEBKIT_CONTEXT_MENU_ACTION "webkit-context-menu"

namespace WebCore {

ContextMenuItem::ContextMenuItem(ContextMenu*)
{
    m_platformDescription = 0;
    notImplemented();
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType type, ContextMenuAction action, const String& title, ContextMenu* subMenu)
{
    m_platformDescription = 0;
    notImplemented();
}

ContextMenuItem::~ContextMenuItem()
{
    notImplemented();
}

ContextMenuItemType ContextMenuItem::type() const
{
    notImplemented();
    return (ContextMenuItemType)0;
}

void ContextMenuItem::setType(ContextMenuItemType type)
{
    notImplemented();
}

ContextMenuAction ContextMenuItem::action() const
{
    notImplemented();
    return (ContextMenuAction)0;
}

void ContextMenuItem::setAction(ContextMenuAction action)
{
    notImplemented();
}

String ContextMenuItem::title() const
{
    notImplemented();
    return String();
}

void ContextMenuItem::setTitle(const String&)
{
    notImplemented();
}

PlatformMenuDescription ContextMenuItem::platformSubMenu() const
{
    notImplemented();
    return 0;
}

void ContextMenuItem::setSubMenu(ContextMenu* menu)
{
    notImplemented();
}

void ContextMenuItem::setChecked(bool shouldCheck)
{
    notImplemented();
}

void ContextMenuItem::setEnabled(bool shouldEnable)
{
    notImplemented();
}

}
#endif // ENABLE(CONTEXT_MENUS)
