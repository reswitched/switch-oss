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

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "PluginDatabase.h"

#include "PluginPackage.h"
#include "CString.h"
#include "npruntime_impl.h"

#include <wkc/wkcpluginpeer.h>

namespace WebCore {

// plugin database

bool
PluginDatabase::isPreferredPluginDirectory(const String& directory)
{
    return true;
}

Vector<String>
PluginDatabase::defaultPluginDirectories()
{
    Vector<String> pathes;

    const char* path = wkcPluginGetPluginPathPeer();
    if (path) {
        pathes.append(path);
    } 
    return pathes;
}


void
PluginDatabase::getPluginPathsInDirectories(HashSet<String>& pathes) const
{
    int count = wkcPluginGetPluginsCountPeer();
    for (int i=0; i<count; i++) {
        const char* path = wkcPluginGetPluginFullPathPeer(i);
        pathes.add(path);
    }
}


void
PluginPackage::determineQuirks(const String& str)
{
    const unsigned int v = wkcPluginGetQuirksInfoPeer(str.utf8().data());
    if (v&WKC_PLUGIN_QUIRK_WANTSMOZILLAUSERAGENT) {
        m_quirks.add(PluginQuirkWantsMozillaUserAgent);
    }
    if (v&WKC_PLUGIN_QUIRK_DEFERFIRSTSETWINDOWCALL) {
        m_quirks.add(PluginQuirkDeferFirstSetWindowCall);
    }
    if (v&WKC_PLUGIN_QUIRK_DEFERFIRSTSETWINDOWCALL) {
        m_quirks.add(PluginQuirkDeferFirstSetWindowCall);
    }
    if (v&WKC_PLUGIN_QUIRK_THROTTLEINVALIDATE) {
        m_quirks.add(PluginQuirkThrottleInvalidate);
    }
    if (v&WKC_PLUGIN_QUIRK_REMOVEWINDOWLESSVIDEOPARAM) {
        m_quirks.add(PluginQuirkRemoveWindowlessVideoParam);
    }
    if (v&WKC_PLUGIN_QUIRK_THROTTLEWMUSERPLUSONEMESSAGES) {
        m_quirks.add(PluginQuirkThrottleWMUserPlusOneMessages);
    }
    if (v&WKC_PLUGIN_QUIRK_DONTUNLOADPLUGIN) {
        m_quirks.add(PluginQuirkDontUnloadPlugin);
    }
    if (v&WKC_PLUGIN_QUIRK_DONTCALLWNDPROCFORSAMEMESSAGERECURSIVELY) {
        m_quirks.add(PluginQuirkDontCallWndProcForSameMessageRecursively);
    }
    if (v&WKC_PLUGIN_QUIRK_HASMODALMESSAGELOOP) {
        m_quirks.add(PluginQuirkHasModalMessageLoop);
    }
    if (v&WKC_PLUGIN_QUIRK_FLASHURLNOTIFYBUG) {
        m_quirks.add(PluginQuirkFlashURLNotifyBug);
    }
    if (v&WKC_PLUGIN_QUIRK_DONTCLIPTOZERORECTWHENSCROLLING) {
        m_quirks.add(PluginQuirkDontClipToZeroRectWhenScrolling);
    }
    if (v&WKC_PLUGIN_QUIRK_DONTSETNULLWINDOWHANDLEONDESTROY) {
        m_quirks.add(PluginQuirkDontSetNullWindowHandleOnDestroy);
    }
    if (v&WKC_PLUGIN_QUIRK_DONTALLOWMULTIPLEINSTANCES) {
        m_quirks.add(PluginQuirkDontAllowMultipleInstances);
    }
    if (v&WKC_PLUGIN_QUIRK_REQUIRESGTKTOOLKIT) {
        m_quirks.add(PluginQuirkRequiresGtkToolKit);
    }
    if (v&WKC_PLUGIN_QUIRK_REQUIRESDEFAULTSCREENDEPTH) {
        m_quirks.add(PluginQuirkRequiresDefaultScreenDepth);
    }
}

bool
PluginPackage::fetchInfo()
{
    if (!load())
        return false;

    WKCPluginInfo info = {0};
    if (!wkcPluginGetInfoPeer(m_module, &info))
        return false;

    CString cname = CString::newUninitialized(info.fNameLen, info.fName);
    CString cdesc = CString::newUninitialized(info.fDescriptionLen, info.fDescription);
    CString cmime = CString::newUninitialized(info.fMIMEDescriptionLen, info.fMIMEDescription);
    CString cexts = CString::newUninitialized(info.fExtensionLen, info.fExtension);
    CString ctypes = CString::newUninitialized(info.fFileOpenNameLen, info.fFileOpenName);

    if (!wkcPluginGetInfoPeer(m_module, &info))
        return false;

    m_name = info.fName;
    m_description = info.fDescription;
    determineModuleVersionFromDescription();

    String smimes = info.fMIMEDescription;
    Vector<String> mimes;

    if (info.fType==WKC_PLUGIN_TYPE_WIN) {
        String e = info.fExtension;
        String d = info.fFileOpenName;
        Vector<String> exts;
        Vector<String> descs;
        Vector<String> iexts;
        smimes.split('|', mimes);
        e.split('|', true, exts);
        d.split('|', true, descs);
        for (int i=0; i<mimes.size(); i++) {
            determineQuirks(mimes[0]);
            String v = i<descs.size() ? descs[i] : "";
            m_mimeToDescriptions.add(mimes[i], v);
            if (i<exts.size()) {
                exts[i].split(',', iexts);
            } else {
                iexts.clear();
            }
            m_mimeToExtensions.add(mimes[i], iexts);
        }
    } else {
        smimes.split(';', mimes);
        for (int i=0; i<mimes.size(); i++) {
            Vector<String> desc;
            Vector<String> exts;
            mimes[i].split(':', true, desc);
            if (desc.size()<3)
                continue;
            desc[1].split(',', exts);

            determineQuirks(desc[0]);
            m_mimeToDescriptions.add(desc[0], desc[2]);
            m_mimeToExtensions.add(desc[0], exts);
        }
    }

    return true;
}

unsigned
PluginPackage::hash() const
{
    const unsigned hashCodes[] = {
        m_name.impl()->hash(),
        m_description.impl()->hash(),
        m_mimeToExtensions.size()
    };

    return StringHasher::hashMemory<sizeof(hashCodes)>(hashCodes);
}

bool
PluginPackage::equal(const PluginPackage& a, const PluginPackage& b)
{
    if (a.m_name != b.m_name)
        return false;

    if (a.m_description != b.m_description)
        return false;

    if (a.m_mimeToExtensions.size() != b.m_mimeToExtensions.size())
        return false;

    MIMEToExtensionsMap::const_iterator::Keys end = a.m_mimeToExtensions.end().keys();
    for (MIMEToExtensionsMap::const_iterator::Keys it = a.m_mimeToExtensions.begin().keys(); it != end; ++it) {
        if (!b.m_mimeToExtensions.contains(*it))
            return false;
    }

    return true;
}

uint16_t
PluginPackage::NPVersion() const
{
    return NP_VERSION_MINOR;
}

bool
PluginPackage::load()
{
    if (m_isLoaded) {
        m_loadCount++;
        return true;
    }

    NP_InitializeFuncPtr NP_Initialize = 0;
    NPError err = 0;

    m_module = wkcPluginLoadPeer(m_path.utf8().data());
    if (!m_module) {
        unloadWithoutShutdown();
        return false;
    }

    NP_Initialize = (NP_InitializeFuncPtr)wkcPluginGetSymbolPeer(m_module, "NP_Initialize");
    m_NPP_Shutdown = (NPP_ShutdownProcPtr)wkcPluginGetSymbolPeer(m_module, "NP_Shutdown");
#ifndef XP_UNIX
    NP_GetEntryPointsFuncPtr NP_GetEntryPoints = (NP_GetEntryPointsFuncPtr)wkcPluginGetSymbolPeer(m_module, "NP_GetEntryPoints");
    if (!NP_Initialize || !NP_GetEntryPoints || !m_NPP_Shutdown) {
#else
    if (!NP_Initialize || !m_NPP_Shutdown) {
#endif
        wkcPluginUnloadPeer(m_module);
        m_module = 0;
        goto error_end;
    }

    m_isLoaded = true;

    ::memset(&m_pluginFuncs, 0, sizeof(m_pluginFuncs));
    m_pluginFuncs.size = sizeof(m_pluginFuncs);

#ifndef XP_UNIX
    err = NP_GetEntryPoints(&m_pluginFuncs);
    if (err!=NPERR_NO_ERROR)
        goto error_end;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    initializeBrowserFuncs();
#endif

#ifdef XP_UNIX
    err = NP_Initialize(&m_browserFuncs, &m_pluginFuncs);
#else
    err = NP_Initialize(&m_browserFuncs);
#endif
    if (err!=NPERR_NO_ERROR)
        goto error_end;

    m_loadCount++;
    return true;

error_end:
    unloadWithoutShutdown();
    return false;
}

bool
unloadModule(PlatformModule module)
{
    wkcPluginUnloadPeer(module);
    return true;
}

} // WebCore
#endif
