/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef CSSDefaultStyleSheets_h
#define CSSDefaultStyleSheets_h

namespace WebCore {

class Element;
class RuleSet;
class StyleSheetContents;

class CSSDefaultStyleSheets {
public:
#if !PLATFORM(WKC)
    static RuleSet* defaultStyle;
    static RuleSet* defaultQuirksStyle;
    static RuleSet* defaultPrintStyle;

    static StyleSheetContents* simpleDefaultStyleSheet;
    static StyleSheetContents* defaultStyleSheet;
    static StyleSheetContents* quirksStyleSheet;
    static StyleSheetContents* svgStyleSheet;
    static StyleSheetContents* mathMLStyleSheet;
    static StyleSheetContents* mediaControlsStyleSheet;
    static StyleSheetContents* fullscreenStyleSheet;
    static StyleSheetContents* plugInsStyleSheet;
    static StyleSheetContents* imageControlsStyleSheet;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(RuleSet*, defaultStyle);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(RuleSet*, defaultQuirksStyle);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(RuleSet*, defaultPrintStyle);

    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, simpleDefaultStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, defaultStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, quirksStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, svgStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, mathMLStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, mediaControlsStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, fullscreenStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, plugInsStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(StyleSheetContents*, imageControlsStyleSheet);
#endif

    static void ensureDefaultStyleSheetsForElement(Element&, bool& changedDefaultStyle);
    static void loadFullDefaultStyle();
    static void loadSimpleDefaultStyle();
    static void initDefaultStyle(Element*);
};

} // namespace WebCore

#endif // CSSDefaultStyleSheets_h