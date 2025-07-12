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

#ifndef _PLATFORMSPEECHSYNTHESIZERWKC_H_
#define _PLATFORMSPEECHSYNTHESIZERWKC_H_

#if ENABLE(SPEECH_SYNTHESIS)

#include "PlatformSpeechSynthesisUtterance.h"

namespace WebCore {

class PlatformSpeechSynthesizerWKC {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PlatformSpeechSynthesizerWKC();
    ~PlatformSpeechSynthesizerWKC();

    void speak(RefPtr<PlatformSpeechSynthesisUtterance>&&);
    void pause();
    void resume();
    void cancel();
    void initializeVoiceList();

private:
    RefPtr<PlatformSpeechSynthesisUtterance> m_utterance;
};

} // namespace

#endif

#endif // _PLATFORMSPEECHSYNTHESIZERWKC_H_

