/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ImageControlsButtonElementMac.h"

#if ENABLE(SERVICE_CONTROLS)

#include "ContextMenuController.h"
#include "Event.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLDivElement.h"
#include "Page.h"
#include "Range.h"
#include "RenderBlockFlow.h"
#include "RenderStyle.h"
#include "RenderTheme.h"
#include "ShadowRoot.h"

namespace WebCore {

class RenderImageControlsButton final : public RenderBlockFlow {
public:
    RenderImageControlsButton(HTMLElement&, Ref<RenderStyle>&&);
    virtual ~RenderImageControlsButton();

private:
    virtual void updateLogicalWidth() override;
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const override;

    virtual const char* renderName() const override { return "RenderImageControlsButton"; }
    virtual bool requiresForcedStyleRecalcPropagation() const override { return true; }
};

RenderImageControlsButton::RenderImageControlsButton(HTMLElement& element, Ref<RenderStyle>&& style)
    : RenderBlockFlow(element, WTF::move(style))
{
}

RenderImageControlsButton::~RenderImageControlsButton()
{
}

void RenderImageControlsButton::updateLogicalWidth()
{
    RenderBox::updateLogicalWidth();

    IntSize frameSize = theme().imageControlsButtonSize(*this);
    setLogicalWidth(isHorizontalWritingMode() ? frameSize.width() : frameSize.height());
}

void RenderImageControlsButton::computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    RenderBox::computeLogicalHeight(logicalHeight, logicalTop, computedValues);

    IntSize frameSize = theme().imageControlsButtonSize(*this);
    computedValues.m_extent = isHorizontalWritingMode() ? frameSize.height() : frameSize.width();
}

ImageControlsButtonElementMac::ImageControlsButtonElementMac(Document& document)
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

ImageControlsButtonElementMac::~ImageControlsButtonElementMac()
{
}

PassRefPtr<ImageControlsButtonElementMac> ImageControlsButtonElementMac::maybeCreate(Document& document)
{
    if (!document.page())
        return nullptr;

    RefPtr<ImageControlsButtonElementMac> button = adoptRef(new ImageControlsButtonElementMac(document));
    button->setAttribute(HTMLNames::classAttr, "x-webkit-image-controls-button");

    IntSize positionOffset = document.page()->theme().imageControlsButtonPositionOffset();
    button->setInlineStyleProperty(CSSPropertyTop, positionOffset.height(), CSSPrimitiveValue::CSS_PX);

    // FIXME: Why is right: 0px off the right edge of the parent?
    button->setInlineStyleProperty(CSSPropertyRight, positionOffset.width(), CSSPrimitiveValue::CSS_PX);

    return button.release();
}

void ImageControlsButtonElementMac::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        Frame* frame = document().frame();
        if (!frame)
            return;

        Page* page = document().page();
        if (!page)
            return;

        page->contextMenuController().showImageControlsMenu(event);
        event->setDefaultHandled();
        return;
    }

    HTMLDivElement::defaultEventHandler(event);
}

RenderPtr<RenderElement> ImageControlsButtonElementMac::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderImageControlsButton>(*this, WTF::move(style));
}

} // namespace WebCore

#endif // ENABLE(SERVICE_CONTROLS)
