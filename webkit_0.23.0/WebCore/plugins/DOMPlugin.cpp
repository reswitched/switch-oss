/*
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DOMPlugin.h"

#include "PluginData.h"
#include "Frame.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

DOMPlugin::DOMPlugin(PluginData* pluginData, Frame* frame, PluginInfo pluginInfo)
    : FrameDestructionObserver(frame)
    , m_pluginData(pluginData)
    , m_pluginInfo(WTF::move(pluginInfo))
{
}

DOMPlugin::~DOMPlugin()
{
}

String DOMPlugin::name() const
{
    return m_pluginInfo.name;
}

String DOMPlugin::filename() const
{
    return m_pluginInfo.file;
}

String DOMPlugin::description() const
{
    return m_pluginInfo.desc;
}

unsigned DOMPlugin::length() const
{
    return m_pluginInfo.mimes.size();
}

PassRefPtr<DOMMimeType> DOMPlugin::item(unsigned index)
{
    if (index >= m_pluginInfo.mimes.size())
        return 0;

    MimeClassInfo mime = m_pluginInfo.mimes[index];

    Vector<MimeClassInfo> mimes;
    Vector<size_t> mimePluginIndices;
    Vector<PluginInfo> plugins = m_pluginData->webVisiblePlugins();
    m_pluginData->getWebVisibleMimesAndPluginIndices(mimes, mimePluginIndices);
    for (unsigned i = 0; i < mimes.size(); ++i) {
        if (mimes[i] == mime && plugins[mimePluginIndices[i]] == m_pluginInfo)
            return DOMMimeType::create(m_pluginData.get(), m_frame, i);
    }
    return 0;
}

bool DOMPlugin::canGetItemsForName(const AtomicString& propertyName)
{
    Vector<MimeClassInfo> mimes;
    Vector<size_t> mimePluginIndices;
    m_pluginData->getWebVisibleMimesAndPluginIndices(mimes, mimePluginIndices);
    for (auto& mime : mimes) {
        if (mime.type == propertyName)
            return true;
    }
    return false;
}

PassRefPtr<DOMMimeType> DOMPlugin::namedItem(const AtomicString& propertyName)
{
    Vector<MimeClassInfo> mimes;
    Vector<size_t> mimePluginIndices;
    m_pluginData->getWebVisibleMimesAndPluginIndices(mimes, mimePluginIndices);
    for (unsigned i = 0; i < mimes.size(); ++i)
        if (mimes[i].type == propertyName)
            return DOMMimeType::create(m_pluginData.get(), m_frame, i);
    return 0;
}

} // namespace WebCore
