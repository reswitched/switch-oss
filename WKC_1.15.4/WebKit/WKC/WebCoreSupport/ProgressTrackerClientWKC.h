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

#ifndef ProgressTrackerClientWKC_h
#define ProgressTrackerClientWKC_h

#include "ProgressTrackerClient.h"

namespace WKC {

class WKCWebViewPrivate;
class ProgressTrackerClientIf;

class ProgressTrackerClientWKC : public WebCore::ProgressTrackerClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static ProgressTrackerClientWKC* create(WKCWebViewPrivate*);

    virtual ~ProgressTrackerClientWKC();

    virtual void progressTrackerDestroyed();

    virtual void willChangeEstimatedProgress();
    virtual void didChangeEstimatedProgress();

    virtual void progressStarted(WebCore::Frame& originatingProgressFrame);
    virtual void progressEstimateChanged(WebCore::Frame& originatingProgressFrame);
    virtual void progressFinished(WebCore::Frame& originatingProgressFrame);

private:
    ProgressTrackerClientWKC(WKCWebViewPrivate*);
    bool construct();

    WKCWebViewPrivate* m_view;
    ProgressTrackerClientIf* m_appClient;
};

} // namespace WKC

#endif // ProgressTrackerClientWKC_h
