/*
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
 *
 */

#ifndef _WKCPROGRESSTRACKERCLIENTIF_H_
#define _WKCPROGRESSTRACKERCLIENTIF_H_

#include <wkc/wkcbase.h>
#include "WKCHelpersEnums.h"

namespace WKC {

class Frame;

class WKC_API ProgressTrackerClientIf {
public:
    virtual ~ProgressTrackerClientIf() {};

    virtual void progressTrackerDestroyed() = 0;

    virtual void willChangeEstimatedProgress() = 0;
    virtual void didChangeEstimatedProgress() = 0;
    virtual void progressStarted(Frame&) = 0;
    virtual void progressEstimateChanged(Frame&) = 0;
    virtual void progressFinished(Frame&) = 0;
};

} // namespace

#endif // _WKCPROGRESSTRACKERCLIENTIF_H_
