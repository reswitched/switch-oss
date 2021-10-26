/*
 * Copyright (C) 2007 Kevin Ollivier.  All rights reserved.
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Font.h"

#include "FontCascade.h"
#include "FontPlatformDataWKC.h"
#include "GlyphBuffer.h"
#include "GraphicsContext.h"
#if USE(WKC_CAIRO)
#include "CairoOperations.h"
#include "CairoUtilities.h"
#include "PlatformContextCairo.h"
#include "ShadowBlur.h"
#else /* USE(WKC_CAIRO) */
#include "ImageBuffer.h"
#endif /* USE(WKC_CAIRO) */
#include "IntRect.h"
#include "TextRun.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>

namespace WebCore {

static const int cGlyphBufMaxLen = 1024;

static const UChar cU3099_normal[] = {
    0x304b, 0x304d, 0x304f, 0x3051, 0x3053, // ka-ko
    0x3055, 0x3057, 0x3059, 0x305b, 0x305d, // sa-so
    0x305f, 0x3061, 0x3064, 0x3066, 0x3068, // ta-to
    0x306f, 0x3072, 0x3075, 0x3078, 0x307b, // ha-ho
    0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3, // ka-ko
    0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd, // sa-so
    0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, // ta-to
    0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, // ha-ho
    0x3046, 0x309d, 0x30a6, 0x30ef, 0x30f0, 0x30f1, 0x30f2, 0x30fd, // specials
    0
};

static const UChar cU3099_voiced[] = {
    0x304c, 0x304e, 0x3050, 0x3052, 0x3054, // ga-go
    0x3056, 0x3058, 0x305a, 0x305c, 0x305e, // za-zo
    0x3060, 0x3062, 0x3065, 0x3067, 0x3069, // da-do
    0x3070, 0x3073, 0x3076, 0x3079, 0x307c, // ba-bo
    0x30ac, 0x30ae, 0x30b0, 0x30b2, 0x30b4, // ga-go
    0x30b6, 0x30b8, 0x30ba, 0x30bc, 0x30be, // za-zo
    0x30c0, 0x30c2, 0x30c5, 0x30c7, 0x30c9, // da-do
    0x30d0, 0x30d3, 0x30d6, 0x30d9, 0x30dc, // ba-bo
    0x3094, 0x309e, 0x30f4, 0x30f7, 0x30f8, 0x30f9, 0x30fa, 0x30fe, // specials
    0
};

static const UChar cU309a_normal[] = {
    0x306f, 0x3072, 0x3075, 0x3078, 0x307b, // ha-ho
    0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, // ha-ho
    0
};

static const UChar cU309a_voiced[] = {
    0x3071, 0x3074, 0x3077, 0x307a, 0x307d, // pa-po
    0x30d1, 0x30d4, 0x30d7, 0x30da, 0x30dd, // pa-po
    0
};

static bool
combineCharsInText(const UChar*& inout_str, int& inout_len)
{
    if (inout_len == 0)
        return false;

    static const int cSupportsMn = wkcFontEngineSupportsNonSpacingMarksPeer();
    if (cSupportsMn)
        return false;

    int i = 0;
    for (i = 1; i<inout_len; i++) {
        if (wkcUnicodeCategoryPeer(inout_str[i]) == WKC_UNICODE_CATEGORY_MARKNONSPACING)
            break;
    }
    if (i == inout_len)
        return false;

    WKC_DEFINE_STATIC_TYPE(UChar*, gGlyphBuf, 0);
    if (!gGlyphBuf) {
        gGlyphBuf = (UChar *)fastMalloc(cGlyphBufMaxLen * sizeof(UChar));
    }
    UChar* buf = gGlyphBuf;
    bool needdelete = false;
    if (inout_len >= cGlyphBufMaxLen - 1) {
        buf = (UChar *)fastMalloc(inout_len * sizeof(UChar) + 1);
        needdelete = true;
    }

    int len = 0;
    for (i = 0; i<inout_len; i++) {
        const UChar c = inout_str[i];
        buf[len++] = c;
        if (wkcUnicodeCategoryPeer(c) != WKC_UNICODE_CATEGORY_MARKNONSPACING || len == 1)
            continue;

        const UChar* tbl_normal = 0;
        const UChar* tbl_voiced = 0;
        if (c == 0x3099) {
            tbl_normal = cU3099_normal;
            tbl_voiced = cU3099_voiced;
        }
        else if (c == 0x309a) {
            tbl_normal = cU309a_normal;
            tbl_voiced = cU309a_voiced;
        }
        else
            continue;
        const UChar b = buf[len - 2];
        for (int k = 0; tbl_normal[k]; k++) {
            if (tbl_normal[k] != b) continue;
            len--;
            buf[len - 1] = tbl_voiced[k];
            break;
        }
    }

    inout_str = buf;
    inout_len = len;
    return needdelete;
}

float
FontCascade::getTextWidth(const FontPlatformData& in_font, int in_flags, const unsigned short* in_str, int in_len, float* out_clip_width)
{
    bool needdelete = combineCharsInText(in_str, in_len);
    float width = 0;
    float clipwidth = 0;
    float cw = 0;

    size_t start = 0;
    for (size_t i=0; i<in_len;) {
        if (i==in_len-1) {
            i++;
            continue;
        }

        size_t emojilen = wkcDrawContextIsEmojiSequencePeer(&in_str[i], in_len-i);
        if (!emojilen) {
            i++;
            continue;
        }

        if (i-start > 0) {
            if (i-start==1 && in_str[start]==0x200d) {
                width += 0;
            } else {
                width += wkcFontGetTextWidthPeer(in_font.font()->font(), in_flags, &in_str[start], i-start, &cw);
                clipwidth += cw;
            }
        }

        width += wkcDrawContextGetEmojiWidthPeer(in_font.font()->font(), &in_str[i], emojilen, &cw);
        clipwidth += cw;

        i+=emojilen;
        start = i;
    }

    if (start!=in_len) {
        width += wkcFontGetTextWidthPeer(in_font.font()->font(), in_flags, &in_str[start], in_len-start, &cw);
        clipwidth += cw;
    }

    if (needdelete)
        fastFree((void *)in_str);
    if (out_clip_width)
        *out_clip_width = clipwidth;
    return width;
}

static void
drawPlatformText(void* in_context, const unsigned short* in_str, int in_len, const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip, void* in_font, int in_flag)
{
    bool needdelete = combineCharsInText(in_str, in_len);
    wkcDrawContextDrawTextPeer(in_context, in_str, in_len, in_textbox, in_clip, (WKCPeerFont *)in_font, in_flag);
    if (needdelete)
        fastFree((void *)in_str);
}

#if USE(WKC_CAIRO)
static void
drawTextShadow(GraphicsContext& graphicscontext, void* in_platformcontext, const unsigned short* in_str, int in_len, const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip, WKCPeerFont* in_font, int in_flag)
{
    ShadowBlur shadow(graphicscontext.state());
    if (!(graphicscontext.textDrawingMode() & TextModeFill) || shadow.type() == ShadowBlur::NoShadow)
        return;

    WKCFloatRect textbox(*in_textbox);
    WKCFloatRect clip(*in_clip);
    FloatSize shadowOffset(graphicscontext.state().shadowOffset);
    textbox.fX += shadowOffset.width();
    textbox.fY += shadowOffset.height();
    clip.fX += shadowOffset.width();
    clip.fY += shadowOffset.height();

    if (shadow.type()==ShadowBlur::NoShadow) {
        // Optimize non-blurry shadows, by just drawing text without the ContextShadow.
        graphicscontext.save();
        graphicscontext.setFillColor(graphicscontext.state().shadowColor);
        drawPlatformText(in_platformcontext, in_str, in_len, &textbox, &clip, in_font, in_flag);
        graphicscontext.restore();
        return;
    }

    FloatRect fontRect(*in_clip);
//    fontRect.inflate(graphicscontext.state().blurDistance);
    PlatformContextCairo& platformContext = *graphicscontext.platformContext();
    shadow.drawShadowLayer(Cairo::State::getCTM(platformContext), Cairo::State::getClipBounds(platformContext), fontRect,
        [&in_str, &in_len, &in_textbox, &in_clip, &in_font, &in_flag](GraphicsContext& shadowContext)
        {
            PlatformContextCairo* pf = new PlatformContextCairo(shadowContext.platformContext()->cr());
            wkcDrawContextSaveStatePeer(pf);
            wkcDrawContextSetFillColorPeer(pf, 0xff000000);
            drawPlatformText(pf, in_str, in_len, in_textbox, in_clip, in_font, in_flag);
            wkcDrawContextRestoreStatePeer(pf);
            delete pf;
        },
        [&graphicscontext](ImageBuffer& layerImage, const FloatPoint& layerOrigin, const FloatSize& layerSize, const FloatRect& sourceRect)
        {
            GraphicsContextStateSaver stateSaver(graphicscontext);
            graphicscontext.clearShadow();
            graphicscontext.drawImageBuffer(layerImage, FloatRect(roundedIntPoint(layerOrigin), layerSize), FloatRect(FloatPoint(), layerSize), graphicscontext.compositeOperation());
        });
}
#else /* USE(WKC_CAIRO) */
static void
drawTextShadow(GraphicsContext& graphicscontext, void* in_platformcontext, const unsigned short* in_str, int in_len, const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip, WKCPeerFont* in_font, int in_flag)
{
    FloatSize offset;
    float blur;
    Color color;
    if (!graphicscontext.getShadow(offset, blur, color))
        return;

    int type = wkcDrawContextGetOffscreenTypePeer(in_platformcontext);

    if (!blur || type==WKC_OFFSCREEN_TYPE_POLYGON) {
        // Optimize non-blurry shadows, by just drawing text without the ContextShadow.
        WKCFloatRect textbox(*in_textbox);
        WKCFloatRect clip(*in_clip);
        textbox.fX += offset.width();
        textbox.fY += offset.height();
        clip.fX += offset.width();
        clip.fY += offset.height();

        graphicscontext.save();
        graphicscontext.setFillColor(color);
        drawPlatformText(in_platformcontext, in_str, in_len, &textbox, &clip, in_font, in_flag);
        graphicscontext.restore();
        return;
    }

    ShadowBlur shadow(FloatSize(blur, blur), offset, color);

    const IntSize bufferSize(in_textbox->fWidth+blur*2, in_textbox->fHeight+blur*2);
    const FloatRect bufferRect(FloatPoint(), bufferSize);
    OwnPtr<ImageBuffer> shadowBuffer = ImageBuffer::create(bufferSize);
    if (!shadowBuffer) return;
    GraphicsContext *shadowContext = shadowBuffer->context();

    shadowContext->clearRect(bufferRect);
    shadowContext->setFillColor(Color::black);
    const WKCFloatRect textbox = {blur, blur, in_textbox->fWidth, in_textbox->fHeight};
    const WKCFloatRect tcr = { in_clip->fX-in_textbox->fX, in_clip->fY-in_textbox->fY, in_clip->fWidth, in_clip->fHeight };
    void* buf_dc = (void *)shadowContext->platformContext();
    drawPlatformText(buf_dc, in_str, in_len, &textbox, &tcr, in_font, in_flag);

    IntRect br(bufferRect);
    RefPtr<Uint8ClampedArray> layerData = shadowBuffer->getUnmultipliedImageData(br);
    if (!layerData) return;
    shadow.blurLayerImage(layerData->data(), bufferSize, bufferSize.width() * 4);
    shadowBuffer->putByteArray(Unmultiplied, layerData.get(), bufferSize, br, IntPoint());

    shadowContext->setCompositeOperation(CompositeSourceIn);
    shadowContext->setFillColor(color);
    shadowContext->fillRect(bufferRect);

    graphicscontext.drawImageBuffer(shadowBuffer.get(), IntPoint(in_textbox->fX+offset.width()-blur, in_textbox->fY+offset.height()-blur));
}
#endif /* USE(WKC_CAIRO) */

inline static void
drawTextWithShadow(GraphicsContext& graphicscontext, void* in_platformcontext, const unsigned short* in_str, int in_len, const WKCFloatRect* in_textbox, const WKCFloatRect* in_clip, WKCPeerFont* in_font, int in_flag)
{
    if (!in_platformcontext) return;
    TextDrawingModeFlags mode = graphicscontext.textDrawingMode();

    graphicscontext.setTextDrawingMode(TextModeFill);
    drawTextShadow(graphicscontext, in_platformcontext, in_str, in_len, in_textbox, in_clip, in_font, in_flag);
    graphicscontext.setTextDrawingMode(mode);
    drawPlatformText(in_platformcontext, in_str, in_len, in_textbox, in_clip, in_font, in_flag);
}

UChar*
FontCascade::fixedGlyphs(int sch, bool isemoji, const UChar* str, int& len, bool& needfree)
{
    WKC_DEFINE_STATIC_TYPE(UChar*, gGlyphBuf, 0);
    if (!gGlyphBuf) {
        gGlyphBuf = (UChar *)WTF::fastMalloc(cGlyphBufMaxLen * sizeof(UChar));
    }

    UChar* buf = gGlyphBuf;
    needfree = false;
    int j=0;

    if (isemoji) {
        for (int i=0; i<len; i++) {
            const UChar32 c = str[i] | 0x010000;
            buf[j++] = U16_LEAD(c);
            buf[j++] = U16_TRAIL(c);
        }
    } else if (sch==0) {
        // BMP
        if (len>=cGlyphBufMaxLen-1) {
            buf = (UChar *)WTF::fastMalloc((len+1) * sizeof(UChar));
            needfree = true;
        }
        for (int i=0; i<len; i++) {
            const UChar c = str[i];
            UChar c_next = 0;
            if (i<(len-1))
                c_next = str[i+1];
            UChar c_prev = 0;
            if (i>0)
                c_prev = str[i-1];
            UChar c_after_next = 0;
            if (i<(len - 2))
                c_after_next = str[i+2];

            if (FontCascade::treatAsSpace(c)) {
                buf[j++] = 0x20;
            } else if (c==0x200d) {
                buf[j++] = c;
            } else if ((c==0x200b || FontCascade::treatAsZeroWidthSpace(c) || c==0xfeff)) {
                buf[j++] = 0x200b;
            // Instead of implementing Emoji variation sequences, temporarily convert Variation Selector-15 (U+fe0e) and Variation Selector-16 (U+feof) to zero width space (U+200b).
            // If Variation Selector is followed by Zero Width Joiner (U+200d), don't convert to zero width space.
            } else if ((c==0xfe0e || c==0xfe0f) && c_next!=0x200d) {
                buf[j++] = 0x200b;
            // Instead of implementing Emoji variation sequences, temporarily convert Variation Selector-15 (U+fe0e) and Variation Selector-16 (U+feof) to zero width space (U+200b).
            // If Variation Selector is followed by Zero Width Joiner (U+200d) and Zero Width Joiner is followed by Gender Symbol (U+2640 or U+2642), convert to zero width space.
            } else if ((c==0xfe0e || c==0xfe0f) && c_next==0x200d && (c_after_next==0x2640 || c_after_next==0x2642)) {
                buf[j++] = 0x200b;
                i++;
            // Instead of implementing Emoji variation sequences, temporarily convert VS1 - VS14 (U+fe00 - U+fe0d) to zero width space (U+200b).
            } else if (c>=0xfe00 && c<=0xfe0d) {
                buf[j++] = 0x200b;
            // Instead of implementing Emoji variation sequences, temporarily convert VS17 - VS256 (U+db40 U+dd00 - U+db40 U+ddef) to zero width spaces (U+200b U+200b).
            } else if (c == 0xdb40 && c_next>=0xdd00 && c_next<=0xddef) {
                buf[j++] = 0x200b;
            } else if (c_prev == 0xdb40 && c>=0xdd00 && c<=0xddef) {
                buf[j++] = 0x200b;
            } else {
                buf[j++] = c;
            }
        }
    } else {
        if (U_IS_BMP(sch)) {
            if (U_IS_SURROGATE(sch))
                sch = 0xfffd;
            for (int i=0; i<len; i++) {
                buf[j++] = sch;
            }
        } else {
            for (int i=0; i<len; i++) {
                buf[j++] = U16_LEAD(sch);
                buf[j++] = U16_TRAIL(sch);
            }
        }
    }

    const UChar* cbuf = (const UChar *)buf;
    bool combine_needfree = combineCharsInText(cbuf, j);
    if (combine_needfree) {
        if (needfree)
            WTF::fastFree(buf);
        needfree = true;
    }
    buf = (UChar*)cbuf;

    buf[j] = 0;
    len = j;
    return buf;
}

void FontCascade::drawGlyphs(GraphicsContext& graphicsContext, const Font& font, const GlyphBuffer& glyphBuffer,
                             unsigned from, unsigned numGlyphs, const FloatPoint& point, FontSmoothingMode)
{
    FloatPoint pos(point);
    void* dc = (void *)graphicsContext.platformContext();
    WKCPeerFont pf = {0};

    if (!dc) return;

    const FontPlatformData& pd(font.platformData());
    WKC::WKCFontInfo* info = pd.font();
    if (!info)
        return;
    pf.fFont = info->font();
    if (!pf.fFont) return;

    const float scale = info->scale();

    pf.fRequestedSize = info->requestSize();
    pf.fCreatedSize = info->createdSize();
    pf.fWeight = info->weight();
    pf.fItalic = info->isItalic();
    pf.fScale = info->scale();
    pf.fiScale = info->iscale();
    pf.fCanScale = info->canScale();
    pf.fFontId = (void *)static_cast<uintptr_t>(pd.hash());
    pf.fHorizontal = info->horizontal();

    pos.setY(pos.y() - (float)info->ascent()*scale);
    float h = (info->lineSpacing()) * scale;

    const GlyphBufferGlyph* str = glyphBuffer.glyphs(from);
    bool needfree = false;
    int len = numGlyphs;
    UChar* buf = fixedGlyphs(info->specificUnicodeChar(), pd.isSupplemental(), str, len, needfree);

    /*
     * the following code is a copy from GraphicsContextWKC.cpp::GraphicsContext::fillPath().
     */
    WKCPeerPattern* pt = 0;
    Pattern* pattern = graphicsContext.fillPattern();
    Gradient* gradient = graphicsContext.fillGradient().get();
    AffineTransform affine;
    if (pattern) {
        pt = (WKCPeerPattern *)pattern->createPlatformPattern(affine);
        if (NULL == pt) {
            if (needfree) {
                WTF::fastFree((void *)buf);
            }
            return;
        }
    } else if (gradient) {
        pt = (WKCPeerPattern *)gradient->createPlatformGradient(1);
        if (NULL == pt) {
            if (needfree) {
                WTF::fastFree((void *)buf);
            }
            return;
        }
    } else {
        /*
         * ? why ? even if the alpha value is zero, we have to draw (e.g. SRCCOPY).
         */
        Color c = graphicsContext.fillColor();
        Color sc = graphicsContext.strokeColor();
        if (0 == c.alpha() && 0==sc.alpha()) {
            if (needfree) {
                WTF::fastFree((void *)buf);
            }
            return;
        }
    }
    wkcDrawContextSetPatternPeer(dc, pt);

    const unsigned short* tstart = buf;
    int tlen = 0;
    int glen = -1;
    float tofs = 0;
    float aw = 0;
    float w = 0;
    float cw = 0;
    if (pd.isSupplemental()) {
        w = (len/2) * info->createdSize();
        const WKCFloatRect textbox = { pos.x()+tofs, pos.y(), w, h };
        const WKCFloatRect cr = { pos.x()+tofs, pos.y(), w, h };
        drawTextWithShadow(graphicsContext, dc, (const unsigned short *)buf, len, &textbox, &cr, &pf, WKC_DRAWTEXT_OVERRIDE_BIDI);
    } else {
        for (int i=0; i<len; i++) {
            tlen++;
            glen++;
            if (U16_IS_LEAD(tstart[tlen-1]))
                continue;
            if (tstart[tlen-1] == zeroWidthJoiner)
                continue;

            if (from + glen < glyphBuffer.size()) {
                Glyph gc = glyphBuffer.glyphAt(from + glen);
                if (gc == 0x3099 || gc == 0x309a) {
                    if (glen > 0) {
                        Glyph previousGc = glyphBuffer.glyphAt(from + glen - 1);
                        if (previousGc != 0x3099 && previousGc != 0x309a)
                            glen++;
                    }
                }

                if ((tstart[tlen - 1] == 0x3099 || tstart[tlen - 1] == 0x309a) && gc != 0x3099 && gc != 0x309a)
                    glen--;
            }

            w += font.widthForGlyph(tstart[tlen-1]);
            const FloatRect br = font.boundsForGlyph(tstart[tlen-1]);
            cw += br.width();

            if (i < len - 1 && from + glen < glyphBuffer.size())
                aw += glyphBuffer.advanceAt(from + glen).width();
            else
                aw = -1;
            if (w==aw)
                continue;

            const WKCFloatRect textbox = { pos.x()+tofs, pos.y(), w, h };
            const WKCFloatRect cr = { pos.x()+tofs, pos.y(), cw, h };
            drawTextWithShadow(graphicsContext, dc, (const unsigned short *)tstart, tlen, &textbox, &cr, &pf, WKC_DRAWTEXT_OVERRIDE_BIDI);
            tstart += tlen;
            tofs += aw;
            tlen = 0;
            aw = 0;
            w = 0;
            cw = 0;
        }
    }

    wkcDrawContextSetPatternPeer(dc, 0);
#if USE(WKC_CAIRO)
    if (pt) {
        if (gradient) {
            gradient->destroyPlatformGradient();
        } else {
            cairo_pattern_destroy((cairo_pattern_t *)pt);
        }
    }
#else
    if (pattern) {
        graphicsContext.tidyPattern_i(pattern);
    }
#endif

    if (needfree)
        WTF::fastFree((void *)buf);
}

void FontCascade::adjustSelectionRectForComplexText(const TextRun& run, LayoutRect& selectionRect, unsigned from, unsigned to) const
{
    void* font = 0;
    UChar* str = 0;
    int len;
    float x0,x1,x2;
    float scale = 0;

    len = run.length();
    if (!len) return;
    if (from>=len || to>len) return;
    if (from>=to) return;
    if (from<0) from = 0;
    if (to>len) to = len;

    const Font& sfont = primaryFont();
    if (!sfont.platformData().font())
        return;
    font = sfont.platformData().font()->font();
    if (!font) return;
    scale = sfont.platformData().font()->scale();

    if (run.is8Bit()) {
        const LChar* str8 = run.characters8();
        if (!str8)
            return;
        str = (UChar *)WTF::fastMalloc(len * sizeof(UChar) + 1);
        for (int i=0; i<len; i++) {
            str[i] = (UChar)str8[i];
        }
    } else {
        str = (UChar *)run.characters16();
    }

    int fflags = WKC_FONT_FLAG_NONE;
    if (run.directionalOverride()) {
        fflags |= WKC_FONT_FLAG_OVERRIDE_BIDI;
    }

    UChar* glyphs = 0;
    bool needfree = false;
    int plen = 0;

    x0=x1=x2=0;

    plen = from;
    if (plen) {
        glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), str, plen, needfree);
        if (plen) {
            x0 = getTextWidth(sfont.platformData(), fflags, glyphs, plen, 0) * scale;
        }
        if (needfree)
            WTF::fastFree((void *)glyphs);
    }
    plen = to-from;
    if (plen) {
        glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), &str[from], plen, needfree);
        if (plen) {
            x1 = getTextWidth(sfont.platformData(), fflags, glyphs, plen, 0) * scale;
        }
        if (needfree)
            WTF::fastFree((void *)glyphs);
    }
    plen = len-to;
    if (plen) {
        glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), &str[to], plen, needfree);
        if (plen) {
            x2 = getTextWidth(sfont.platformData(), fflags, glyphs, plen, 0) * scale;
        }
        if (needfree)
            WTF::fastFree((void *)glyphs);
    }

    if (run.rtl() && !run.directionalOverride()) {
        selectionRect = IntRect(selectionRect.x() + x2, selectionRect.y(), x1, selectionRect.height());
    } else {
        selectionRect = IntRect(selectionRect.x() + x0, selectionRect.y(), x1, selectionRect.height());
    }
    if (run.is8Bit()) {
        WTF::fastFree(str);
        str = 0;
    }
}

#if 0
float FontCascade::floatWidthForComplexText(const TextRun& run, HashSet<const Font*>* /*fallbackFonts*/, GlyphOverflow* /*overflow*/) const
{
    void* font = 0;
    float w = 0;
    UChar* str = 0;
    const Font& sfont = primaryFont();
    float scale = 0;
    int len;

    if (!sfont.platformData().font()) return 0.f;

    len = run.length();
    if (!len) return 0.f;

    font = sfont.platformData().font()->font();
    if (!font) return 0.f;

    if (run.is8Bit()) {
        const LChar* str8 = run.characters8();
        if (!str8)
            return 0.f;
        str = (UChar *)WTF::fastMalloc(len * sizeof(UChar) + 1);
        for (int i=0; i<len; i++) {
            str[i] = (UChar)str8[i];
        }
    } else {
        str = (UChar *)run.characters16();
    }

    scale = sfont.platformData().font()->scale();

    bool needfree = false;
    UChar* glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), str, len, needfree);

    if (run.is8Bit()) {
        WTF::fastFree(str);
        str = 0;
    }
    int fflags = WKC_FONT_FLAG_NONE;
    if (run.directionalOverride()) {
        fflags |= WKC_FONT_FLAG_OVERRIDE_BIDI;
    }

    if (len) {
        w = getTextWidth(sfont.platformData(), fflags, glyphs, len, 0);
    }
    if (needfree)
        WTF::fastFree((void *)glyphs);

    return w * scale;
}
#endif

int FontCascade::offsetForPositionForComplexText(const TextRun& run, float x, bool includePartialGlyphs) const
{
    void* font = 0;
    float w = 0.f;
    UChar* str = 0;
    int i, len;
    float delta = 0;
    float scale = 0;

    len = run.length();
    if (!len) return 0;

   const Font&  sfont = primaryFont();
    if (!sfont.platformData().font())
        return 0;
    font = sfont.platformData().font()->font();
    if (!font) return 0;
    scale = sfont.platformData().font()->scale();

    if (run.is8Bit()) {
        const LChar* str8 = run.characters8();
        if (!str8)
            return 0.f;
        str = (UChar *)WTF::fastMalloc(len * sizeof(UChar) + 1);
        for (int i=0; i<len; i++) {
            str[i] = (UChar)str8[i];
        }
    } else {
        str = (UChar *)run.characters16();
    }

    bool needfree = 0;
    int ulen = len;
    UChar* glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), str, ulen, needfree);
    if (run.is8Bit()) {
        WTF::fastFree(str);
        str = 0;
    }
    if (!ulen) {
        if (needfree)
            WTF::fastFree((void *)glyphs);
        return 0;
    }

    int fflags = WKC_FONT_FLAG_NONE;
    if (run.directionalOverride()) {
        fflags |= WKC_FONT_FLAG_OVERRIDE_BIDI;
    }

    i = 0;
    float lw = 0;
    float dw = 0;

    if (run.rtl()) {
        float ofs = x - floatWidthForComplexText(run, 0);
        while (i<len) {
            w = getTextWidth(sfont.platformData(), fflags, glyphs, i + 1, 0) * scale;
            dw = w - lw;
            lw = w;
            delta = ofs + w;
            if (includePartialGlyphs) {
                if (delta - dw / 2 >= 0)
                    break;
            } else {
                if (delta >= 0)
                    break;
            }
            i++;
        }
    } else {
        float ofs = x;
        while (i<len) {
            w = getTextWidth(sfont.platformData(), fflags, glyphs, i + 1, 0) * scale;
            dw = w - lw;
            lw = w;
            delta = ofs - w;
            if (includePartialGlyphs) {
                if (delta + dw / 2 <= 0)
                    break;
            } else {
                if (delta <= 0)
                    break;
            }
            i++;
        }
    }

    if (needfree)
        WTF::fastFree((void *)glyphs);

    return i;
}

float
FontCascade::getGlyphsAndAdvancesForComplexText(const TextRun& run, unsigned from, unsigned to, GlyphBuffer& glyphBuffer, ForTextEmphasisOrNot forTextEmphasis) const
{
    if (forTextEmphasis) {
        return 0.f;
    }

    int len = run.length();
    if (!len) return 0.f;
    if (from>=len || to>len) return 0.f;
    if (from>=to) return 0.f;
    if (from<0) from = 0;
    if (to>len) to = len;

    const Font& sfont = primaryFont();
    if (!sfont.platformData().font()) return 0.f;
    void* font = sfont.platformData().font()->font();
    if (!font) return 0.f;
    const float scale = sfont.platformData().font()->scale();

    UChar* str;
    if (run.is8Bit()) {
        const LChar* str8 = run.characters8();
        if (!str8)
            return 0.f;
        str = (UChar *)WTF::fastMalloc(len * sizeof(UChar) + 1);
        for (int i=0; i<len; i++) {
            str[i] = (UChar)str8[i];
        }
    } else {
        str = (UChar *)run.characters16();
    }

    UChar* glyphs = 0;
    bool needfree = false;
    int plen = 0;

    float x0 = 0.f;
    float x1 = 0.f;
    float x2 = 0.f;

    int fflags = WKC_FONT_FLAG_NONE;
    if (run.directionalOverride()) {
        fflags |= WKC_FONT_FLAG_OVERRIDE_BIDI;
    }

    plen = from;
    if (plen) {
        glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), str, plen, needfree);
        if (plen) {
            x0 = getTextWidth(sfont.platformData(), fflags, glyphs, plen, 0) * scale;
        }
        if (needfree)
            WTF::fastFree((void *)glyphs);
    }
    plen = to-from;
    if (plen) {
        glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), &str[from], plen, needfree);
        if (plen) {
            x1 = getTextWidth(sfont.platformData(), fflags, glyphs, plen, 0) * scale;
        }
        for (int i=0; i<plen; i++) {
            const float fw = getTextWidth(sfont.platformData(), fflags, &glyphs[i], 1, 0) * scale;
            glyphBuffer.add(glyphs[i], &sfont, fw, 0);
        }
        if (needfree)
            WTF::fastFree((void *)glyphs);
    }
    plen = len-to;
    if (plen) {
        glyphs = fixedGlyphs(sfont.platformData().font()->specificUnicodeChar(), sfont.platformData().isSupplemental(), &str[to], plen, needfree);
        if (plen) {
            x2 = getTextWidth(sfont.platformData(), fflags, glyphs, plen, 0) * scale;
        }
        if (needfree)
            WTF::fastFree((void *)glyphs);
    }

    if (run.is8Bit()) {
        WTF::fastFree(str);
        str = 0;
    }

    (void)x1;

    if (run.rtl() && !run.directionalOverride()) {
        return x2;
    } else {
        return x0;
    }
}
 
bool
FontCascade::canExpandAroundIdeographsInComplexText()
{
    return true;
}
bool FontCascade::canReturnFallbackFontsForComplexText()
{
    return false;
}

}
