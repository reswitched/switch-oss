/*
 *  Copyright (c) 2017-2019 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#pragma once

#include "MediaSample.h"

namespace WebCore {

class MediaSampleWKC : public MediaSample {
public:
    static Ref<MediaSampleWKC> create(int id, int64_t presentationTime, int64_t decodeTime, int64_t duration, bool isSync) { return adoptRef(*new MediaSampleWKC(id, presentationTime, decodeTime, duration, isSync)); }
    virtual ~MediaSampleWKC() { }

    virtual MediaTime presentationTime() const override { return m_presentationTime; }
    virtual MediaTime decodeTime() const override { return m_decodeTime; }
    virtual MediaTime duration() const override { return m_duration; }

    virtual AtomicString trackID() const override { return m_trackID; }
    virtual void setTrackID(const String& id) override { m_trackID = id; }

    virtual size_t sizeInBytes() const { return 0; }
    virtual FloatSize presentationSize() const { return { 0, 0 }; }

    virtual SampleFlags flags() const override { return m_flags; }
    virtual PlatformSample platformSample() { PlatformSample sample = { PlatformSample::None, }; return sample; }
    virtual void dump(PrintStream&) const { }
    virtual void offsetTimestampsBy(const MediaTime& timestampOffset)
    {
        m_presentationTime += timestampOffset;
        m_decodeTime += timestampOffset;
    }
    void setTimestamps(const MediaTime&, const MediaTime&) override { }
    bool isDivisable() const override { return false; }
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime) override { return { nullptr, nullptr }; }
    Ref<MediaSample> createNonDisplayingCopy() const override
    {
        auto copy = MediaSampleWKC::create(m_trackID.toInt(), m_presentationTime.toDouble(), m_decodeTime.toDouble(), m_duration.toDouble(), false);
        return WTFMove(copy);
    }

protected:
    MediaSampleWKC(int id, int64_t presentationTime, int64_t decodeTime, int64_t duration, bool isSync)
        : m_presentationTime(presentationTime, 1000000)
        , m_decodeTime(decodeTime, 1000000)
        , m_duration(duration, 1000000)
        , m_trackID(String::format("%d", id))
        , m_flags(isSync ? IsSync : None)
    {
    }

private:
    MediaTime m_presentationTime;
    MediaTime m_decodeTime;
    MediaTime m_duration;
    AtomicString m_trackID;
    SampleFlags m_flags;
};

}
