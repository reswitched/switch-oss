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

#ifndef _WKC_HELPERS_PLUGINDATABASEPRIVATE_H_
#define _WKC_HELPERS_PLUGINDATABASEPRIVATE_H_

#include "helpers/WKCPluginDatabase.h"

namespace WebCore {
class PluginDatabase;
} // namespace WebCore

namespace WKC {

class String;

class PluginDatabaseWrap : public PluginDatabase {
WTF_MAKE_FAST_ALLOCATED;
public:
    PluginDatabaseWrap(PluginDatabasePrivate& parent) : PluginDatabase(parent) {}
    ~PluginDatabaseWrap() {}
};

class PluginDatabasePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    PluginDatabasePrivate(WebCore::PluginDatabase*);
    ~PluginDatabasePrivate();

    WebCore::PluginDatabase* webcore() const { return m_webcore; }
    PluginDatabase& wkc() { return m_wkc; }

    static PluginDatabasePrivate* installedPlugins(bool populate=true);
    bool isMIMETypeRegistered(const String&);

private:
    WebCore::PluginDatabase* m_webcore;
    PluginDatabaseWrap m_wkc;
};

} // namespace WKC

#endif // _WKC_HELPERS_PLUGINDATABASEPRIVATE_H_
