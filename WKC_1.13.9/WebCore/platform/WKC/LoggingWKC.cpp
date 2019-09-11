/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Logging.h"
#include "WTFString.h"

#include "NotImplemented.h"

namespace WebCore {

String logLevelString()
{
#if !LOG_DISABLED
    String log;
    //log.append("Animations,");
    log.append("Archives,");
    //log.append("BackForward,");
    //log.append("Compositing,");
    log.append("Editing,");
    log.append("Events,");
    log.append("FTP,");
    log.append("FileAPI,");
    log.append("Frames,");
    log.append("Gamepad,");
    //log.append("History,");
    log.append("LiveConnect,");
    //log.append("Loading,");
    log.append("Media,");
    log.append("MediaSource,");
    log.append("MediaSourceSamples,");
    log.append("MemoryPressure,");
    log.append("Network,");
    log.append("NotYetImplemented,");
    //log.append("PageCache,");
    log.append("PlatformLeaks,");
    log.append("Plugins,");
    log.append("PopupBlocking,");
    //log.append("Progress,");
    log.append("RemoteInspector,");
    //log.append("ResourceLoading,");
    //log.append("SQLDatabase,");
    log.append("SpellingAndGrammar,");
    log.append("StorageAPI,");
    log.append("Threading,");
    log.append("WebAudio,");
    log.append("WebGL,");
    log.append("WebReplay,");
    log.append("Services");

    return log;
#else
    return emptyString();
#endif
}

} // namespace WebCore

namespace PAL {

WTF::String logLevelString()
{
    return WebCore::logLevelString();
}

} // namespace PAL
