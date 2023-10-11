/*
    Copyright (C) 2000 Harri Porten (porten@kde.org)
    Copyright (C) 2000 Daniel Molkentin (molkentin@kde.org)
    Copyright (C) 2000 Stefan Schimanski (schimmi@kde.org)
    Copyright (C) 2003, 2004, 2005, 2006, 2007, 2015 Apple Inc. All Rights Reserved.
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "PluginData.h"

#include "PlatformStrategies.h"
#include "PluginStrategy.h"

namespace WebCore {

PluginData::PluginData(const Page* page)
{
    ASSERT_ARG(page, page);

    m_page = page;
    initPlugins();
}

Vector<PluginInfo> PluginData::webVisiblePlugins() const
{
    Vector<PluginInfo> plugins;
    platformStrategies()->pluginStrategy()->getWebVisiblePluginInfo(m_page, plugins);
    return plugins;
}

void PluginData::getWebVisibleMimesAndPluginIndices(Vector<MimeClassInfo>& mimes, Vector<size_t>& mimePluginIndices) const
{
    ASSERT_ARG(mimes, mimes.isEmpty());
    ASSERT_ARG(mimePluginIndices, mimePluginIndices.isEmpty());

    const Vector<PluginInfo>& plugins = webVisiblePlugins();
    for (unsigned i = 0; i < plugins.size(); ++i) {
        const PluginInfo& plugin = plugins[i];
        for (auto& mime : plugin.mimes) {
            mimes.append(mime);
            mimePluginIndices.append(i);
        }
    }
}

bool PluginData::supportsWebVisibleMimeType(const String& mimeType, const AllowedPluginTypes allowedPluginTypes) const
{
    Vector<MimeClassInfo> mimes;
    Vector<size_t> mimePluginIndices;
    const Vector<PluginInfo>& plugins = webVisiblePlugins();
    getWebVisibleMimesAndPluginIndices(mimes, mimePluginIndices);

    for (unsigned i = 0; i < mimes.size(); ++i) {
        if (mimes[i].type == mimeType && (allowedPluginTypes == AllPlugins || plugins[mimePluginIndices[i]].isApplicationPlugin))
            return true;
    }
    return false;
}

bool PluginData::getPluginInfoForWebVisibleMimeType(const String& mimeType, PluginInfo& pluginInfoRef) const
{
    Vector<MimeClassInfo> mimes;
    Vector<size_t> mimePluginIndices;
    const Vector<PluginInfo>& plugins = webVisiblePlugins();
    getWebVisibleMimesAndPluginIndices(mimes, mimePluginIndices);

    for (unsigned i = 0; i < mimes.size(); ++i) {
        const MimeClassInfo& info = mimes[i];

        if (info.type == mimeType) {
            pluginInfoRef = plugins[mimePluginIndices[i]];
            return true;
        }
    }

    return false;
}

String PluginData::pluginNameForWebVisibleMimeType(const String& mimeType) const
{
    PluginInfo info;
    if (getPluginInfoForWebVisibleMimeType(mimeType, info))
        return info.name;
    return String();
}

String PluginData::pluginFileForWebVisibleMimeType(const String& mimeType) const
{
    PluginInfo info;
    if (getPluginInfoForWebVisibleMimeType(mimeType, info))
        return info.file;
    return String();
}

void PluginData::refresh()
{
    platformStrategies()->pluginStrategy()->refreshPlugins();
}

void PluginData::initPlugins()
{
    ASSERT(m_plugins.isEmpty());

    platformStrategies()->pluginStrategy()->getPluginInfo(m_page, m_plugins);
}

} // namespace WebCore
