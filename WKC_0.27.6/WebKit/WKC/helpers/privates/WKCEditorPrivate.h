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

#ifndef _WKC_HELPERS_PRIVATE_EDITOR_H_
#define _WKC_HELPERS_PRIVATE_EDITOR_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCEditor.h"

namespace WebCore {
class Editor;
class Command;
} // namespace

namespace WKC {

class String;
class Event;

class EditorWrap : public Editor {
WTF_MAKE_FAST_ALLOCATED;
public:
    EditorWrap(EditorPrivate& parent) : Editor(parent) {}
    ~EditorWrap() {}
};

class EditorPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    EditorPrivate(WebCore::Editor*);
    ~EditorPrivate();

    WebCore::Editor* webcore() const { return m_webcore; }
    Editor& wkc() { return m_wkc; }

    bool canEdit() const;
    Editor::Command command(const String& commandName);
    bool insertText(const String& text, Event* triggeringEvent);
    bool hasComposition() const;
    void confirmComposition();

private:
    WebCore::Editor* m_webcore;
    EditorWrap m_wkc;

    Editor::CommandPrivate* m_command;
};


} // namespace

#endif // _WKC_HELPERS_PRIVATE_EDITOR_H_
