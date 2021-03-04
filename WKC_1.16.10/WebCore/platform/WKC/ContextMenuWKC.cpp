/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (c) 2010, 2012, 2013 ACCESS CO., LTD. All rights reserved.
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
#include "NotImplemented.h"
#include "ContextMenuController.h"

namespace WebCore {

// TODO: ref-counting correctness checking.
// See http://bugs.webkit.org/show_bug.cgi?id=16115

ContextMenu::ContextMenu()
{
    m_platformDescription = 0;
}

ContextMenu::~ContextMenu()
{
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
    notImplemented();
}

unsigned ContextMenu::itemCount() const
{
    notImplemented();
    return 0;
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription menu)
{
    notImplemented();
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    notImplemented();
    return 0;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    notImplemented();
    return 0;
}

Vector<ContextMenuItem> contextMenuItemVector(const PlatformMenuDescription menu)
{
    notImplemented();
    Vector<ContextMenuItem> items;
    return items;
}

}

#endif // ENABLE(CONTEXT_MENUS)
