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

#if ENABLE(SPEECH_SYNTHESIS)

#include "PlatformSpeechSynthesizer.h"
#include "PlatformSpeechSynthesizerWKC.h"

namespace WebCore {

PlatformSpeechSynthesizerWKC::PlatformSpeechSynthesizerWKC()
{
}

PlatformSpeechSynthesizerWKC::~PlatformSpeechSynthesizerWKC()
{
}

void
PlatformSpeechSynthesizerWKC::initializeVoiceList()
{
}

void
PlatformSpeechSynthesizerWKC::speak(RefPtr<PlatformSpeechSynthesisUtterance>&& utterance)
{
    m_utterance = WTFMove(utterance);
}

void
PlatformSpeechSynthesizerWKC::pause()
{
}

void
PlatformSpeechSynthesizerWKC::resume()
{
}

void
PlatformSpeechSynthesizerWKC::cancel()
{
}

// APIs

PlatformSpeechSynthesizer::PlatformSpeechSynthesizer(PlatformSpeechSynthesizerClient* client)
    : m_speechSynthesizerClient(client)
{
    m_platformSpeechWrapper = std::make_unique<PlatformSpeechSynthesizerWKC>();
}

PlatformSpeechSynthesizer::~PlatformSpeechSynthesizer()
{
}

void
PlatformSpeechSynthesizer::initializeVoiceList()
{
    m_platformSpeechWrapper->initializeVoiceList();
}

void
PlatformSpeechSynthesizer::speak(RefPtr<PlatformSpeechSynthesisUtterance>&& utterance)
{
    m_platformSpeechWrapper->speak(WTFMove(utterance));
}

void
PlatformSpeechSynthesizer::pause()
{
    m_platformSpeechWrapper->pause();
}

void
PlatformSpeechSynthesizer::resume()
{
    m_platformSpeechWrapper->resume();
}

void
PlatformSpeechSynthesizer::cancel()
{
    m_platformSpeechWrapper->cancel();
}

} // namespace

#endif
