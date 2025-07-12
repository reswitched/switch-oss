/*
 * Copyright (c) 2010-2011 ACCESS CO., LTD. All rights reserved.
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

#ifndef FONTPLATFORMDATAWKC_H
#define FONTPLATFORMDATAWKC_H

#include "CString.h"

namespace WKC {

class WKCFontInfo {
WTF_MAKE_FAST_ALLOCATED;
public:
    ~WKCFontInfo();

    static WKCFontInfo* create(float size, int weight, bool italic, bool horizontal, int family, const char* familyName);
    static WKCFontInfo* copy(const WKCFontInfo* other);

    bool isEqual(const WKCFontInfo* other);
    const CString& familyName() const { return m_familyName; }

public:
    inline void* font() const { return m_font; }
    inline float scale() const { return m_scale; }
    inline float iscale() const { return m_iscale; }
    inline float requestSize() const { return m_requestSize; }
    inline float createdSize() const { return m_createdSize; }
    inline int weight() const { return m_weight; }
    inline bool isItalic() const { return m_isItalic; }
    inline bool canScale() const { return m_canScale; }
    inline float ascent() const { return m_ascent; }
    inline float descent() const { return m_descent; }
    inline float lineSpacing() const { return m_lineSpacing; }

    inline void setSpecificUnicodeChar(int ch) { m_unicodeChar = ch; }
    inline int specificUnicodeChar() const { return m_unicodeChar; }

    inline bool horizontal() const { return m_horizontal; }

private:
    WKCFontInfo(const char* familyName);
    bool construct(float size, int weight, bool italic, bool horizontal, int family);

private:
    void* m_font;
    float m_scale;
    float m_iscale;
    float m_requestSize;
    float m_createdSize;
    int m_weight;
    bool m_isItalic;
    bool m_canScale;
    float m_ascent;
    float m_descent;
    float m_lineSpacing;
    int m_unicodeChar;
    bool m_horizontal;

    WTF::CString m_familyName;
};

} // namespace

#endif // FONTPLATFORMDATAWKC_H
