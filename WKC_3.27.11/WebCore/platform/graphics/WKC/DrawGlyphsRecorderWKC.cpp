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

#include "config.h"

#include "DrawGlyphsRecorder.h"
#include "GraphicsContext.h"

namespace WebCore {

DrawGlyphsRecorder::DrawGlyphsRecorder(GraphicsContext& context, DeconstructDrawGlyphs deconstructDrawGlyphs, DeriveFontFromContext deriveFontFromContext)
	: m_owner(context)
	, m_deconstructDrawGlyphs(deconstructDrawGlyphs)
	, m_deriveFontFromContext(deriveFontFromContext)
{

}

void DrawGlyphsRecorder::drawGlyphs(const Font& font, const GlyphBufferGlyph* glyphs, const GlyphBufferAdvance* advances, unsigned numGlyphs, const FloatPoint& anchorPoint, FontSmoothingMode fontSmoothingMode)
{
	m_owner.drawGlyphsAndCacheFont(font, glyphs, advances, numGlyphs, anchorPoint, fontSmoothingMode);
}

}