/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
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
#include "CSSDefaultStyleSheets.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "HTMLAnchorElement.h"
#include "HTMLAudioElement.h"
#include "HTMLBRElement.h"
#include "MathMLElement.h"
#include "MediaQueryEvaluator.h"
#include "Page.h"
#include "RenderTheme.h"
#include "RuleSet.h"
#include "SVGElement.h"
#include "StyleSheetContents.h"
#include "UserAgentStyleSheets.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

using namespace HTMLNames;

#if !PLATFORM(WKC)
RuleSet* CSSDefaultStyleSheets::defaultStyle;
RuleSet* CSSDefaultStyleSheets::defaultQuirksStyle;
RuleSet* CSSDefaultStyleSheets::defaultPrintStyle;

StyleSheetContents* CSSDefaultStyleSheets::simpleDefaultStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::defaultStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::quirksStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::svgStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::mathMLStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::mediaControlsStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::fullscreenStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::plugInsStyleSheet;
StyleSheetContents* CSSDefaultStyleSheets::imageControlsStyleSheet;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(RuleSet*, CSSDefaultStyleSheets, defaultStyle, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(RuleSet*, CSSDefaultStyleSheets, defaultQuirksStyle, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(RuleSet*, CSSDefaultStyleSheets, defaultPrintStyle, 0);

WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, simpleDefaultStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, defaultStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, quirksStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, svgStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, mathMLStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, mediaControlsStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, fullscreenStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, plugInsStyleSheet, 0);
WKC_DEFINE_GLOBAL_CLASS_OBJ(StyleSheetContents*, CSSDefaultStyleSheets, imageControlsStyleSheet, 0);
#endif

// FIXME: It would be nice to use some mechanism that guarantees this is in sync with the real UA stylesheet.
static const char* simpleUserAgentStyleSheet = "html,body,div{display:block}head{display:none}body{margin:8px}div:focus,span:focus,a:focus{outline:auto 5px -webkit-focus-ring-color}a:any-link{color:-webkit-link;text-decoration:underline}a:any-link:active{color:-webkit-activelink}";

static inline bool elementCanUseSimpleDefaultStyle(Element& element)
{
    return is<HTMLHtmlElement>(element) || is<HTMLHeadElement>(element)
        || is<HTMLBodyElement>(element) || is<HTMLDivElement>(element)
        || is<HTMLSpanElement>(element) || is<HTMLBRElement>(element)
        || is<HTMLAnchorElement>(element);
}

static const MediaQueryEvaluator& screenEval()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const MediaQueryEvaluator> staticScreenEval("screen");
    return staticScreenEval;
#else
    WKC_DEFINE_STATIC_PTR(const MediaQueryEvaluator*, staticScreenEval, 0);
    if (!staticScreenEval)
        staticScreenEval = new MediaQueryEvaluator("screen");
    return *staticScreenEval;
#endif
}

static const MediaQueryEvaluator& printEval()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const MediaQueryEvaluator> staticPrintEval("print");
    return staticPrintEval;
#else
    WKC_DEFINE_STATIC_PTR(const MediaQueryEvaluator*, staticPrintEval, 0);
    if (!staticPrintEval)
        staticPrintEval = new MediaQueryEvaluator("print");
    return *staticPrintEval;
#endif
}

static StyleSheetContents* parseUASheet(const String& str)
{
    StyleSheetContents& sheet = StyleSheetContents::create().leakRef(); // leak the sheet on purpose
    sheet.parseString(str);
    return &sheet;
}

static StyleSheetContents* parseUASheet(const char* characters, unsigned size)
{
    return parseUASheet(String(characters, size));
}

void CSSDefaultStyleSheets::initDefaultStyle(Element* root)
{
    if (!defaultStyle) {
        if (!root || elementCanUseSimpleDefaultStyle(*root))
            loadSimpleDefaultStyle();
        else
            loadFullDefaultStyle();
    }
}

void CSSDefaultStyleSheets::loadFullDefaultStyle()
{
    if (simpleDefaultStyleSheet) {
        ASSERT(defaultStyle);
        ASSERT(defaultPrintStyle == defaultStyle);
        delete defaultStyle;
        simpleDefaultStyleSheet->deref();
        defaultStyle = std::make_unique<RuleSet>().release();
        defaultPrintStyle = std::make_unique<RuleSet>().release();
        simpleDefaultStyleSheet = 0;
    } else {
        ASSERT(!defaultStyle);
        defaultStyle = std::make_unique<RuleSet>().release();
        defaultPrintStyle = std::make_unique<RuleSet>().release();
        defaultQuirksStyle = std::make_unique<RuleSet>().release();
    }

    // Strict-mode rules.
    String defaultRules = String(htmlUserAgentStyleSheet, sizeof(htmlUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraDefaultStyleSheet();
    defaultStyleSheet = parseUASheet(defaultRules);
    defaultStyle->addRulesFromSheet(defaultStyleSheet, screenEval());
    defaultPrintStyle->addRulesFromSheet(defaultStyleSheet, printEval());

    // Quirks-mode rules.
    String quirksRules = String(quirksUserAgentStyleSheet, sizeof(quirksUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraQuirksStyleSheet();
    quirksStyleSheet = parseUASheet(quirksRules);
    defaultQuirksStyle->addRulesFromSheet(quirksStyleSheet, screenEval());
}

void CSSDefaultStyleSheets::loadSimpleDefaultStyle()
{
    ASSERT(!defaultStyle);
    ASSERT(!simpleDefaultStyleSheet);

    defaultStyle = std::make_unique<RuleSet>().release();
    // There are no media-specific rules in the simple default style.
    defaultPrintStyle = defaultStyle;
    defaultQuirksStyle = std::make_unique<RuleSet>().release();

    simpleDefaultStyleSheet = parseUASheet(simpleUserAgentStyleSheet, strlen(simpleUserAgentStyleSheet));
    defaultStyle->addRulesFromSheet(simpleDefaultStyleSheet, screenEval());

    // No need to initialize quirks sheet yet as there are no quirk rules for elements allowed in simple default style.
}

void CSSDefaultStyleSheets::ensureDefaultStyleSheetsForElement(Element& element, bool& changedDefaultStyle)
{
    if (simpleDefaultStyleSheet && !elementCanUseSimpleDefaultStyle(element)) {
        loadFullDefaultStyle();
        changedDefaultStyle = true;
    }

    if (is<HTMLElement>(element)) {
        if (is<HTMLObjectElement>(element) || is<HTMLEmbedElement>(element)) {
            if (!plugInsStyleSheet) {
                String plugInsRules = RenderTheme::themeForPage(element.document().page())->extraPlugInsStyleSheet() + element.document().page()->chrome().client().plugInExtraStyleSheet();
                if (plugInsRules.isEmpty())
                    plugInsRules = String(plugInsUserAgentStyleSheet, sizeof(plugInsUserAgentStyleSheet));
                plugInsStyleSheet = parseUASheet(plugInsRules);
                defaultStyle->addRulesFromSheet(plugInsStyleSheet, screenEval());
                changedDefaultStyle = true;
            }
        }
#if ENABLE(VIDEO)
        else if (is<HTMLMediaElement>(element)) {
            if (!mediaControlsStyleSheet) {
                String mediaRules = RenderTheme::themeForPage(element.document().page())->mediaControlsStyleSheet();
                if (mediaRules.isEmpty())
                    mediaRules = String(mediaControlsUserAgentStyleSheet, sizeof(mediaControlsUserAgentStyleSheet)) + RenderTheme::themeForPage(element.document().page())->extraMediaControlsStyleSheet();
                mediaControlsStyleSheet = parseUASheet(mediaRules);
                defaultStyle->addRulesFromSheet(mediaControlsStyleSheet, screenEval());
                defaultPrintStyle->addRulesFromSheet(mediaControlsStyleSheet, printEval());
                changedDefaultStyle = true;
            }
        }
#endif // ENABLE(VIDEO)
#if ENABLE(SERVICE_CONTROLS)
        else if (is<HTMLDivElement>(element) && element.isImageControlsRootElement()) {
            if (!imageControlsStyleSheet) {
                String imageControlsRules = RenderTheme::themeForPage(element.document().page())->imageControlsStyleSheet();
                imageControlsStyleSheet = parseUASheet(imageControlsRules);
                defaultStyle->addRulesFromSheet(imageControlsStyleSheet, screenEval());
                defaultPrintStyle->addRulesFromSheet(imageControlsStyleSheet, printEval());
                changedDefaultStyle = true;
            }
        }
#endif // ENABLE(SERVICE_CONTROLS)
    } else if (is<SVGElement>(element)) {
        if (!svgStyleSheet) {
            // SVG rules.
            svgStyleSheet = parseUASheet(svgUserAgentStyleSheet, sizeof(svgUserAgentStyleSheet));
            defaultStyle->addRulesFromSheet(svgStyleSheet, screenEval());
            defaultPrintStyle->addRulesFromSheet(svgStyleSheet, printEval());
            changedDefaultStyle = true;
        }
    }
#if ENABLE(MATHML)
    else if (is<MathMLElement>(element)) {
        if (!mathMLStyleSheet) {
            // MathML rules.
            mathMLStyleSheet = parseUASheet(mathmlUserAgentStyleSheet, sizeof(mathmlUserAgentStyleSheet));
            defaultStyle->addRulesFromSheet(mathMLStyleSheet, screenEval());
            defaultPrintStyle->addRulesFromSheet(mathMLStyleSheet, printEval());
            changedDefaultStyle = true;
        }
    }
#endif // ENABLE(MATHML)

#if ENABLE(FULLSCREEN_API)
    if (!fullscreenStyleSheet && element.document().webkitIsFullScreen()) {
        String fullscreenRules = String(fullscreenUserAgentStyleSheet, sizeof(fullscreenUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraFullScreenStyleSheet();
        fullscreenStyleSheet = parseUASheet(fullscreenRules);
        defaultStyle->addRulesFromSheet(fullscreenStyleSheet, screenEval());
        defaultQuirksStyle->addRulesFromSheet(fullscreenStyleSheet, screenEval());
        changedDefaultStyle = true;
    }
#endif // ENABLE(FULLSCREEN_API)

    ASSERT(defaultStyle->features().idsInRules.isEmpty());
    ASSERT(mathMLStyleSheet || defaultStyle->features().siblingRules.isEmpty());
}

} // namespace WebCore
