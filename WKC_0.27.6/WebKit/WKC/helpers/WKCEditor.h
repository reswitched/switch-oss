/*
 *  Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef _WKC_HELPER_WKCEDITOR_H_
#define _WKC_HELPER_WKCEDITOR_H_

#include <wkc/wkcbase.h>

namespace WKC {

class EditorPrivate;
class Event;
class String;

typedef struct CompositionUnderline_ {
    CompositionUnderline_() 
        : startOffset(0), endOffset(0), thick(false) { }
    CompositionUnderline_(unsigned s, unsigned e, const unsigned& c, bool t) 
        : startOffset(s), endOffset(e), color(c), thick(t) { }
    unsigned startOffset;
    unsigned endOffset;
    unsigned color;
    bool thick;
} CompositionUnderline;

class WKC_API Editor {
public:
    class CommandPrivate;

class WKC_API Command {
public:
    Command(CommandPrivate&);
    ~Command();

    bool execute(Event* triggeringEvent);

private:
    Command& operator=(const Command&); // not needed

    CommandPrivate& m_private;
};

public:
    bool canEdit() const;
    Command command(const String& commandName);
    bool insertText(const String& text, Event* triggeringEvent);
    bool hasComposition() const;
    void confirmComposition();

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    Editor(EditorPrivate&);
    ~Editor();

private:
    Editor(const Editor&);
    Editor& operator=(const Editor&);

    EditorPrivate& m_private;
};

} // namespace

#endif // _WKC_HELPER_WKCEDITOR_H_
