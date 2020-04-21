/*
 *  Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "helpers/WKCPluginDatabase.h"
#include "helpers/privates/WKCPluginDatabasePrivate.h"

#include "helpers/WKCString.h"

#include "PluginDatabase.h"

namespace WKC {

PluginDatabasePrivate::PluginDatabasePrivate(WebCore::PluginDatabase* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

PluginDatabasePrivate::~PluginDatabasePrivate()
{
}
    
PluginDatabasePrivate*
PluginDatabasePrivate::installedPlugins(bool populate)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    WebCore::PluginDatabase* parent = WebCore::PluginDatabase::installedPlugins(populate);
    WKC_DEFINE_STATIC_TYPE(PluginDatabasePrivate, gInstalledPlugins, PluginDatabasePrivate(parent));

    return &gInstalledPlugins;
#else
    return 0;
#endif
}

bool
PluginDatabasePrivate::isMIMETypeRegistered(const String& mime)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    return m_webcore->isMIMETypeRegistered(mime);
#else
    return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////

PluginDatabase::PluginDatabase(PluginDatabasePrivate& priv)
    : m_private(priv)
{
}

PluginDatabase::~PluginDatabase()
{
}

PluginDatabase*
PluginDatabase::installedPlugins(bool populate)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    return &PluginDatabasePrivate::installedPlugins(populate)->wkc();
#else
    return 0;
#endif
}

bool
PluginDatabase::isMIMETypeRegistered(const String& mime)
{
    return priv().isMIMETypeRegistered(mime);
}

} // namespace WKC

#else

#include "helpers/WKCPluginDatabase.h"
#include "helpers/privates/WKCPluginDatabasePrivate.h"

namespace WKC {

PluginDatabase::PluginDatabase(PluginDatabasePrivate& priv)
    : m_private(priv)
{
}

PluginDatabase::~PluginDatabase()
{
}

PluginDatabase*
PluginDatabase::installedPlugins(bool populate)
{
    return 0;
}

bool
PluginDatabase::isMIMETypeRegistered(const String& mime)
{
    return false;
}

} // namespace WKC

#endif
