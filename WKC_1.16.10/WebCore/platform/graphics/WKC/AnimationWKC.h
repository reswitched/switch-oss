/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (c) 2019 ACCESS CO., LTD. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#pragma once

// copy from WebCore/platform/graphics/texmap/TextureMapperAnimation.h

#include "GraphicsLayer.h"

namespace WebCore {

class TransformationMatrix;

class AnimationWKC {
public:
    enum class AnimationState { Playing, Paused, Stopped };

    struct ApplicationResult {
        std::optional<TransformationMatrix> transform;
        std::optional<double> opacity;
        std::optional<FilterOperations> filters;
        bool hasRunningAnimations { false };
    };

    AnimationWKC()
        : m_keyframes(AnimatedPropertyInvalid)
    { }
    AnimationWKC(const WTF::String&, const KeyframeValueList&, const FloatSize&, const Animation&, bool, MonotonicTime, Seconds, AnimationState);
    AnimationWKC(const AnimationWKC&);

    void apply(ApplicationResult&, MonotonicTime);
    void pause(Seconds);
    void resume();
    bool isActive() const;

    const WTF::String& name() const { return m_name; }
    const KeyframeValueList& keyframes() const { return m_keyframes; }
    const RefPtr<Animation> animation() const { return m_animation; }
    AnimationState state() const { return m_state; }

private:
    void applyInternal(ApplicationResult&, const AnimationValue& from, const AnimationValue& to, float progress);
    Seconds computeTotalRunningTime(MonotonicTime);

    WTF::String m_name;
    KeyframeValueList m_keyframes;
    FloatSize m_boxSize;
    RefPtr<Animation> m_animation;
    bool m_listsMatch;
    MonotonicTime m_startTime;
    Seconds m_pauseTime;
    Seconds m_totalRunningTime;
    MonotonicTime m_lastRefreshedTime;
    AnimationState m_state;
};

class AnimationsWKC {
public:
    AnimationsWKC() = default;

    void add(const AnimationWKC&);
    void remove(const WTF::String& name);
    void remove(const WTF::String& name, AnimatedPropertyID);
    void pause(const WTF::String&, Seconds);
    void suspend(MonotonicTime);
    void resume();

    void apply(AnimationWKC::ApplicationResult&, MonotonicTime);

    bool isEmpty() const { return m_animations.isEmpty(); }
    size_t size() const { return m_animations.size(); }
    const Vector<AnimationWKC>& animations() const { return m_animations; }
    Vector<AnimationWKC>& animations() { return m_animations; }

    bool hasRunningAnimations() const;
    bool hasActiveAnimationsOfType(AnimatedPropertyID type) const;
    AnimationsWKC getActiveAnimations() const;

private:
    Vector<AnimationWKC> m_animations;
};

}
