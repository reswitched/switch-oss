/*
 *  Copyright (c) 2011, 2012, 2015 ACCESS CO., LTD. All rights reserved.
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
#include "PluginData.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "PluginDatabase.h"
#include "PluginPackage.h"

namespace WebCore {

PluginData::PluginData(const Page*)
{
}

void PluginData::initPlugins(const Page* page)
{
    PluginDatabase* database = PluginDatabase::installedPlugins();
    if (!database)
       return;

    const Vector<PluginPackage *>& items = database->plugins();
    const int count = (int)items.size();

    for (int i=0; i<count; i++) {
        PluginPackage* package = items[i];
        if (!package) continue;
        const MIMEToDescriptionsMap& map = package->mimeToDescriptions();

        PluginInfo* info = new PluginInfo;
        info->name = package->name();
        info->file = package->fileName();
        info->desc = package->description();

        MIMEToDescriptionsMap::const_iterator last = map.end();
        for (MIMEToDescriptionsMap::const_iterator it = map.begin(); it!=last; ++it) {
            MimeClassInfo minfo;
            minfo.type = it->key;
            minfo.desc = it->value;
            minfo.extensions = package->mimeToExtensions().get(minfo.type);

            info->mimes.append(minfo);
        }
        m_plugins.append(*info);
        delete info;
    }
}

void PluginData::refresh()
{
    PluginDatabase *database = 0;

    database = PluginDatabase::installedPlugins();
    database->refresh();
}

bool PluginData::supportsMimeType(const String& mimeType, const AllowedPluginTypes) const
{
    return false;
}

String PluginData::pluginNameForMimeType(const String& mimeType) const
{
    return String();
}

String PluginData::pluginFileForMimeType(const String& mimeType) const
{
    return String();
}

};
#endif // ENABLE(NETSCAPE_PLUGIN_API)
