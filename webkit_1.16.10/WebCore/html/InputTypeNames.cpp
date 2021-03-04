/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010, 2012 Google Inc. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "InputTypeNames.h"

#include <wtf/NeverDestroyed.h>

namespace WebCore {

namespace InputTypeNames {

// The type names must be lowercased because they will be the return values of
// input.type and input.type must be lowercase according to DOM Level 2.

const AtomicString& button()
{
    static NeverDestroyed<AtomicString> name("button", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("button", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& checkbox()
{
    static NeverDestroyed<AtomicString> name("checkbox", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("checkbox", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& color()
{
    static NeverDestroyed<AtomicString> name("color", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("color", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& date()
{
    static NeverDestroyed<AtomicString> name("date", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("date", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& datetime()
{
    static NeverDestroyed<AtomicString> name("datetime", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("datetime", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& datetimelocal()
{
    static NeverDestroyed<AtomicString> name("datetime-local", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("datetime-local", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& email()
{
    static NeverDestroyed<AtomicString> name("email", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("email", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& file()
{
    static NeverDestroyed<AtomicString> name("file", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("file", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& hidden()
{
    static NeverDestroyed<AtomicString> name("hidden", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("hidden", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& image()
{
    static NeverDestroyed<AtomicString> name("image", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("image", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& month()
{
    static NeverDestroyed<AtomicString> name("month", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("month", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& number()
{
    static NeverDestroyed<AtomicString> name("number", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("number", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& password()
{
    static NeverDestroyed<AtomicString> name("password", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("password", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& radio()
{
    static NeverDestroyed<AtomicString> name("radio", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("radio", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& range()
{
    static NeverDestroyed<AtomicString> name("range", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("range", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& reset()
{
    static NeverDestroyed<AtomicString> name("reset", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("reset", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& search()
{
    static NeverDestroyed<AtomicString> name("search", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("search", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& submit()
{
    static NeverDestroyed<AtomicString> name("submit", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("submit", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& telephone()
{
    static NeverDestroyed<AtomicString> name("tel", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("tel", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& text()
{
    static NeverDestroyed<AtomicString> name("text", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("text", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& time()
{
    static NeverDestroyed<AtomicString> name("time", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("time", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& url()
{
    static NeverDestroyed<AtomicString> name("url", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("url", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

const AtomicString& week()
{
    static NeverDestroyed<AtomicString> name("week", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (name.isNull())
        name.construct("week", AtomicString::ConstructFromLiteral);
#endif
    return name;
}

} // namespace WebCore::InputTypeNames

} // namespace WebCore
