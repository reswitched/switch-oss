/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "StyleResolveForDocument.h"

#include "CSSFontSelector.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLIFrameElement.h"
#include "LocaleToScriptMapping.h"
#include "NodeRenderStyle.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#include "RenderView.h"
#include "Settings.h"
#include "StyleFontSizeFunctions.h"
#include "StyleResolver.h"

namespace WebCore {

namespace Style {

Ref<RenderStyle> resolveForDocument(const Document& document)
{
    ASSERT(document.hasLivingRenderTree());

    RenderView& renderView = *document.renderView();

    auto documentStyle = RenderStyle::create();

    documentStyle.get().setDisplay(BLOCK);
    documentStyle.get().setRTLOrdering(document.visuallyOrdered() ? VisualOrder : LogicalOrder);
    documentStyle.get().setZoom(!document.printing() ? renderView.frame().pageZoomFactor() : 1);
    documentStyle.get().setPageScaleTransform(renderView.frame().frameScaleFactor());
    documentStyle.get().setLocale(document.contentLanguage());

    // This overrides any -webkit-user-modify inherited from the parent iframe.
    documentStyle.get().setUserModify(document.inDesignMode() ? READ_WRITE : READ_ONLY);
#if PLATFORM(IOS)
    if (document.inDesignMode())
        documentStyle.get().setTextSizeAdjust(TextSizeAdjustment(NoTextSizeAdjustment));
#endif

    Element* docElement = document.documentElement();
    RenderObject* docElementRenderer = docElement ? docElement->renderer() : nullptr;
    if (docElementRenderer) {
        // Use the direction and writing-mode of the body to set the
        // viewport's direction and writing-mode unless the property is set on the document element.
        // If there is no body, then use the document element.
        auto* body = document.bodyOrFrameset();
        RenderObject* bodyRenderer = body ? body->renderer() : nullptr;
        if (bodyRenderer && !document.writingModeSetOnDocumentElement())
            documentStyle.get().setWritingMode(bodyRenderer->style().writingMode());
        else
            documentStyle.get().setWritingMode(docElementRenderer->style().writingMode());
        if (bodyRenderer && !document.directionSetOnDocumentElement())
            documentStyle.get().setDirection(bodyRenderer->style().direction());
        else
            documentStyle.get().setDirection(docElementRenderer->style().direction());
    }

    const Pagination& pagination = renderView.frameView().pagination();
    if (pagination.mode != Pagination::Unpaginated) {
        documentStyle.get().setColumnStylesFromPaginationMode(pagination.mode);
        documentStyle.get().setColumnGap(pagination.gap);
        if (renderView.multiColumnFlowThread())
            renderView.updateColumnProgressionFromStyle(documentStyle.get());
    }

    const Settings& settings = renderView.frame().settings();

    FontDescription fontDescription;
    fontDescription.setScript(localeToScriptCodeForFontSelection(documentStyle.get().locale()));
    fontDescription.setRenderingMode(settings.fontRenderingMode());
    fontDescription.setOneFamily(standardFamily);

    fontDescription.setKeywordSizeFromIdentifier(CSSValueMedium);
    int size = fontSizeForKeyword(CSSValueMedium, false, document);
    fontDescription.setSpecifiedSize(size);
    bool useSVGZoomRules = document.isSVGDocument();
    fontDescription.setComputedSize(computedFontSizeFromSpecifiedSize(size, fontDescription.isAbsoluteSize(), useSVGZoomRules, documentStyle.ptr(), document));

    FontOrientation fontOrientation;
    NonCJKGlyphOrientation glyphOrientation;
    documentStyle.get().getFontAndGlyphOrientation(fontOrientation, glyphOrientation);
    fontDescription.setOrientation(fontOrientation);
    fontDescription.setNonCJKGlyphOrientation(glyphOrientation);

    documentStyle.get().setFontDescription(fontDescription);

    documentStyle.get().fontCascade().update(&const_cast<Document&>(document).fontSelector());

    return documentStyle;
}

}
}
