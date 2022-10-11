/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2009-2010 Samsung Electronics
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
#include "Pasteboard.h"

#include "NotImplemented.h"

namespace WebCore {

Pasteboard::Pasteboard()
{
}

void Pasteboard::writePlainText(const String&, SmartReplaceOption)
{
    notImplemented();
}

void Pasteboard::clear()
{
    notImplemented();
}

bool Pasteboard::canSmartReplace()
{
    notImplemented();
    return false;
}

std::unique_ptr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    return std::make_unique<Pasteboard>();
}

std::unique_ptr<Pasteboard> Pasteboard::createPrivate()
{
    return std::make_unique<Pasteboard>();
}

#if ENABLE(DRAG_SUPPORT)
std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop()
{
    return std::make_unique<Pasteboard>();
}

std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop(const DragData&)
{
    return std::make_unique<Pasteboard>();
}
#endif

bool Pasteboard::hasData()
{
    notImplemented();
    return false;
}

void Pasteboard::clear(const String&)
{
    notImplemented();
}

void Pasteboard::read(PasteboardPlainText&)
{
    notImplemented();
}

String Pasteboard::readString(const String&)
{
    notImplemented();
    return String();
}

void Pasteboard::write(const PasteboardURL&)
{
    notImplemented();
}

void Pasteboard::writeTrustworthyWebURLsPboardType(const PasteboardURL&)
{
    notImplemented();
}

void Pasteboard::writeString(const String&, const String&)
{
    notImplemented();
}

Vector<String> Pasteboard::types()
{
    notImplemented();
    return Vector<String>();
}

Vector<String> Pasteboard::readFilenames()
{
    notImplemented();
    return Vector<String>();
}

#if ENABLE(DRAG_SUPPORT)
void Pasteboard::setDragImage(DragImageRef, const IntPoint&)
{
    notImplemented();
}
#endif

void Pasteboard::writePasteboard(const Pasteboard&)
{
    notImplemented();
}

}
