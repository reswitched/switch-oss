/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPER_WKCFRAME_H_
#define _WKC_HELPER_WKCFRAME_H_

#include <wkc/wkcbase.h>

namespace WKC {

class Document;
class HTMLFrameOwnerElement;
class FrameView;
class Page;
class FrameLoader;
class FrameLoaderPrivate;
class FrameSelection;
class FrameTree;
class MainFrame;
class Editor;

class FramePrivate;

class WKC_API Frame {
public:
    static Frame* create(Frame*, bool needsRef = false);
    static void destroy(Frame*);

    bool compare(const Frame*) const ;

    Document* document() const;
    Page* page() const;
    FrameView* view() const;
    FrameSelection* selection() const;
    FrameLoader* loader();
    FrameTree* tree() const;
    Editor* editor() const;

    HTMLFrameOwnerElement* ownerElement() const;

    const unsigned short* name();

    bool isMainFrame() const;
    Frame* mainFrame();

    FramePrivate& priv() { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    Frame(FramePrivate&);
    Frame(Frame*, bool needsRef);
    ~Frame();

private:
    Frame(const Frame&);
    Frame& operator=(const Frame&);

    FramePrivate* m_ownedPrivate;
    FramePrivate& m_private;
    bool m_needsRef;
};
} // namespace

#endif // _WKC_HELPER_WKCFRAME_H_
