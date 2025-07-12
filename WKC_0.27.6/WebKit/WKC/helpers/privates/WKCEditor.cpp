/*
 * Copyright (c) 2011, 2016 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCEditor.h"
#include "helpers/privates/WKCEditorPrivate.h"

#include "helpers/privates/WKCEventPrivate.h"

#include "Editor.h"
#include "Frame.h"
#include "WTFString.h"

namespace WKC {

class Editor::CommandPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    CommandPrivate(WebCore::Editor::Command&);
    ~CommandPrivate();

    WebCore::Editor::Command& webcore() { return m_webcore; }
    Editor::Command& wkc() { return m_wkc; }

    bool execute(Event* triggeringEvent);

private:
    WebCore::Editor::Command m_webcore;
    Editor::Command m_wkc;
};

Editor::CommandPrivate::CommandPrivate(WebCore::Editor::Command& parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

Editor::CommandPrivate::~CommandPrivate()
{
}

bool
Editor::CommandPrivate::execute(Event* triggeringEvent)
{
    if (triggeringEvent) {
        return m_webcore.execute(triggeringEvent->priv().webcore());
    } else {
        return m_webcore.execute(0);
    }
}

EditorPrivate::EditorPrivate(WebCore::Editor* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_command(0)
{
}

EditorPrivate::~EditorPrivate()
{
    delete m_command;
}

bool
EditorPrivate::canEdit() const
{
    return m_webcore->canEdit();
}

Editor::Command
EditorPrivate::command(const String& commandName)
{
    delete m_command;
    WebCore::Editor::Command cmd = m_webcore->command(commandName);
    m_command = new Editor::CommandPrivate(cmd);
    return m_command->wkc();
}

bool
EditorPrivate::insertText(const String& text, Event* triggeringEvent)
{
    if (triggeringEvent) {
        return m_webcore->insertText(text, triggeringEvent->priv().webcore());
    } else {
        return m_webcore->insertText(text, 0);
    }
}

bool
EditorPrivate::hasComposition() const
{
    return m_webcore->hasComposition();
}

void
EditorPrivate::confirmComposition()
{
    m_webcore->confirmComposition();
}

Editor::Editor(EditorPrivate& parent)
    : m_private(parent)
{
}

Editor::~Editor()
{
}

bool
Editor::canEdit() const
{
    return m_private.canEdit();
}

Editor::Command
Editor::command(const String& commandName)
{
    return m_private.command(commandName);
}

bool
Editor::insertText(const String& text, Event* triggeringEvent)
{
    return m_private.insertText(text, triggeringEvent);
}

bool
Editor::hasComposition() const
{
    return m_private.hasComposition();
}

void
Editor::confirmComposition()
{
    m_private.confirmComposition();
}

Editor::Command::Command(Editor::CommandPrivate& parent)
    : m_private(parent)
{
}

Editor::Command::~Command()
{
}

bool
Editor::Command::execute(Event* triggeringEvent)
{
    return m_private.execute(triggeringEvent);
}

} // namespace
