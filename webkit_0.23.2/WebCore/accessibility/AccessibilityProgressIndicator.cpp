/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "config.h"
#include "AccessibilityProgressIndicator.h"

#include "FloatConversion.h"
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "HTMLProgressElement.h"
#include "RenderMeter.h"
#include "RenderObject.h"
#include "RenderProgress.h"

namespace WebCore {
    
using namespace HTMLNames;

AccessibilityProgressIndicator::AccessibilityProgressIndicator(RenderProgress* renderer)
    : AccessibilityRenderObject(renderer)
{
}

Ref<AccessibilityProgressIndicator> AccessibilityProgressIndicator::create(RenderProgress* renderer)
{
    return adoptRef(*new AccessibilityProgressIndicator(renderer));
}
    
#if ENABLE(METER_ELEMENT)
AccessibilityProgressIndicator::AccessibilityProgressIndicator(RenderMeter* renderer)
    : AccessibilityRenderObject(renderer)
{
}

Ref<AccessibilityProgressIndicator> AccessibilityProgressIndicator::create(RenderMeter* renderer)
{
    return adoptRef(*new AccessibilityProgressIndicator(renderer));
}
#endif

bool AccessibilityProgressIndicator::computeAccessibilityIsIgnored() const
{
    return accessibilityIsIgnoredByDefault();
}
    
float AccessibilityProgressIndicator::valueForRange() const
{
    if (!m_renderer)
        return 0.0;
    
    if (m_renderer->isProgress()) {
        HTMLProgressElement* progress = progressElement();
        if (progress && progress->position() >= 0)
            return narrowPrecisionToFloat(progress->value());
    }

#if ENABLE(METER_ELEMENT)
    if (m_renderer->isMeter()) {
        if (HTMLMeterElement* meter = meterElement())
            return narrowPrecisionToFloat(meter->value());
    }
#endif

    // Indeterminate progress bar should return 0.
    return 0.0;
}

float AccessibilityProgressIndicator::maxValueForRange() const
{
    if (!m_renderer)
        return 0.0;

    if (m_renderer->isProgress()) {
        if (HTMLProgressElement* progress = progressElement())
            return narrowPrecisionToFloat(progress->max());
    }
    
#if ENABLE(METER_ELEMENT)
    if (m_renderer->isMeter()) {
        if (HTMLMeterElement* meter = meterElement())
            return narrowPrecisionToFloat(meter->max());
    }
#endif

    return 0.0;
}

float AccessibilityProgressIndicator::minValueForRange() const
{
    if (!m_renderer)
        return 0.0;
    
    if (m_renderer->isProgress())
        return 0.0;
    
#if ENABLE(METER_ELEMENT)
    if (m_renderer->isMeter()) {
        if (HTMLMeterElement* meter = meterElement())
            return narrowPrecisionToFloat(meter->min());
    }
#endif
    
    return 0.0;
}

HTMLProgressElement* AccessibilityProgressIndicator::progressElement() const
{
    if (!is<RenderProgress>(*m_renderer))
        return nullptr;
    
    return downcast<RenderProgress>(*m_renderer).progressElement();
}

#if ENABLE(METER_ELEMENT)
HTMLMeterElement* AccessibilityProgressIndicator::meterElement() const
{
    if (!is<RenderMeter>(*m_renderer))
        return nullptr;

    return downcast<RenderMeter>(*m_renderer).meterElement();
}
#endif

Element* AccessibilityProgressIndicator::element() const
{
    if (m_renderer->isProgress())
        return progressElement();

#if ENABLE(METER_ELEMENT)
    if (m_renderer->isMeter())
        return meterElement();
#endif

    return AccessibilityObject::element();
}

} // namespace WebCore

