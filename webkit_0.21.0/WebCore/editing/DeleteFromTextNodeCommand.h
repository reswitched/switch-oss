/*
 * Copyright (C) 2005, 2006, 2008, 2015 Apple Inc. All rights reserved.
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

#ifndef DeleteFromTextNodeCommand_h
#define DeleteFromTextNodeCommand_h

#include "EditCommand.h"

namespace WebCore {

class Text;

class DeleteFromTextNodeCommand : public SimpleEditCommand {
public:
    static Ref<DeleteFromTextNodeCommand> create(RefPtr<Text>&& node, unsigned offset, unsigned count, EditAction editingAction = EditActionDelete)
    {
        return adoptRef(*new DeleteFromTextNodeCommand(WTF::move(node), offset, count, editingAction));
    }

    const String& deletedText();

protected:
    DeleteFromTextNodeCommand(RefPtr<Text>&&, unsigned offset, unsigned count, EditAction);

private:
    virtual void doApply() override;
    virtual void doUnapply() override;
    
#ifndef NDEBUG
    virtual void getNodesInCommand(HashSet<Node*>&) override;
#endif
    
    RefPtr<Text> m_node;
    unsigned m_offset;
    unsigned m_count;
    String m_text;
};

inline const String& DeleteFromTextNodeCommand::deletedText()
{
    return m_text;
}

} // namespace WebCore

#endif // DeleteFromTextNodeCommand_h
