/*
 *  Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
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

#include "ProgressTrackerClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "helpers/ProgressTrackerClientIf.h"
#include "helpers/privates/WKCFramePrivate.h"

namespace WKC {

ProgressTrackerClientWKC::ProgressTrackerClientWKC(WKCWebViewPrivate* view)
    : m_view(view)
    , m_appClient(0)
{
}

ProgressTrackerClientWKC::~ProgressTrackerClientWKC()
{
    if (m_view && m_appClient)
        m_view->clientBuilders().deleteProgressTrackerClient(m_appClient);
}

ProgressTrackerClientWKC*
ProgressTrackerClientWKC::create(WKC::WKCWebViewPrivate* view)
{
    ProgressTrackerClientWKC* self = 0;
    self = new ProgressTrackerClientWKC(view);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
ProgressTrackerClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createProgressTrackerClient(m_view->parent());
    return true;
}

void
ProgressTrackerClientWKC::progressTrackerDestroyed()
{
    m_appClient->progressTrackerDestroyed();
}

void
ProgressTrackerClientWKC::willChangeEstimatedProgress()
{
    m_appClient->willChangeEstimatedProgress();
}

void
ProgressTrackerClientWKC::didChangeEstimatedProgress()
{
    m_appClient->didChangeEstimatedProgress();
}

void
ProgressTrackerClientWKC::progressStarted(WebCore::Frame& originatingProgressFrame)
{
    FramePrivate f(&originatingProgressFrame);
    m_appClient->progressStarted(f.wkc());
}

void
ProgressTrackerClientWKC::progressEstimateChanged(WebCore::Frame& originatingProgressFrame)
{
    FramePrivate f(&originatingProgressFrame);
    m_appClient->progressEstimateChanged(f.wkc());
}

void
ProgressTrackerClientWKC::progressFinished(WebCore::Frame& originatingProgressFrame)
{
    FramePrivate f(&originatingProgressFrame);
    m_appClient->progressFinished(f.wkc());
}

} // namespace
