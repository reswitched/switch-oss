/*
 * Copyright (c) 2014-2016 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCPolicyChecker.h"
#include "helpers/privates/WKCPolicyCheckerPrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

#include "PolicyChecker.h"

namespace WKC {

PolicyCheckerPrivate::PolicyCheckerPrivate(WebCore::PolicyChecker* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

PolicyCheckerPrivate::~PolicyCheckerPrivate()
{
}

FrameLoadType
PolicyCheckerPrivate::loadType() const
{
    return toWKCFrameLoadType(m_webcore->loadType());
}

////////////////////////////////////////////////////////////////////////////////

PolicyChecker::PolicyChecker(PolicyCheckerPrivate& parent)
    : m_private(parent)
{
}

PolicyChecker::~PolicyChecker()
{
}

FrameLoadType
PolicyChecker::loadType() const
{
    return m_private.loadType();
}


} // namespace
