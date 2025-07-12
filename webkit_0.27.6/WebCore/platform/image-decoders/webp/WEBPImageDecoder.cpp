/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WEBPImageDecoder.h"

#if USE(WEBP)

#ifdef QCMS_WEBP_COLOR_CORRECTION
#include <qcms.h>
#endif

#if !ENABLE(WKC_BLINK_AWEBP)
#ifdef QCMS_WEBP_COLOR_CORRECTION
#include <webp/demux.h>
#else
#undef ICCP_FLAG
#define ICCP_FLAG 0
#endif
#endif

// Backward emulation for earlier versions than 0.1.99.
#if (WEBP_DECODER_ABI_VERSION < 0x0163)
#define MODE_rgbA MODE_RGBA
#define MODE_bgrA MODE_BGRA
#endif

#if CPU(BIG_ENDIAN) || CPU(MIDDLE_ENDIAN)
inline WEBP_CSP_MODE outputMode(bool hasAlpha) { return hasAlpha ? MODE_rgbA : MODE_RGBA; }
#else // LITTLE_ENDIAN, output BGRA pixels.
inline WEBP_CSP_MODE outputMode(bool hasAlpha) { return hasAlpha ? MODE_bgrA : MODE_BGRA; }
#endif

#if ENABLE(WKC_BLINK_AWEBP)
#if CPU(BIG_ENDIAN) || CPU(MIDDLE_ENDIAN)
#  define R_SHIFT 0
#  define G_SHIFT 8
#  define B_SHIFT 16
#  define A_SHIFT 24
#else
#  define A_SHIFT 24
#  define R_SHIFT 16
#  define G_SHIFT 8
#  define B_SHIFT 0
#endif
#endif

namespace WebCore {

#if ENABLE(WKC_BLINK_AWEBP)

static inline unsigned getAFromColor(uint32_t color) {
    return (uint32_t)(color << (24 - A_SHIFT)) >> 24;
}

static inline unsigned getRFromColor(uint32_t color) {
    return (uint32_t)(color << (24 - R_SHIFT)) >> 24;
}

static inline unsigned getGFromColor(uint32_t color) {
    return (uint32_t)(color << (24 - G_SHIFT)) >> 24;
}

static inline unsigned getBFromColor(uint32_t color) {
    return (uint32_t)(color << (24 - B_SHIFT)) >> 24;
}

static inline unsigned calcAlpha255To256(unsigned alpha) {
    return alpha + 1;
}

static inline uint32_t packARGB(unsigned a, unsigned r, unsigned g, unsigned b) {
    return (a << A_SHIFT) | (r << R_SHIFT) | (g << G_SHIFT) | (b << B_SHIFT);
}

static inline uint32_t alphaMulQ(uint32_t c, unsigned scale) {
    uint32_t mask = 0xFF00FF;

    uint32_t rb = ((c & mask) * scale) >> 8;
    uint32_t ag = ((c >> 8) & mask) * scale;
    return (rb & mask) | (ag & ~mask);
}

static inline uint32_t blendSrcOverDstPremultiplied(uint32_t src, uint32_t dst) {
    return src + alphaMulQ(dst, calcAlpha255To256(255 - getAFromColor(src)));
}

inline uint8_t blendChannel(uint8_t src, uint8_t srcA, uint8_t dst, uint8_t dstA, unsigned scale)
{
    unsigned blendUnscaled = src * srcA + dst * dstA;
    ASSERT(blendUnscaled < (1ULL << 32) / scale);
    return (blendUnscaled * scale) >> 24;
}

inline uint32_t blendSrcOverDstNonPremultiplied(uint32_t src, uint32_t dst)
{
    uint8_t srcA = getAFromColor(src);
    if (srcA == 0)
        return dst;

    uint8_t dstA = getAFromColor(dst);
    uint8_t dstFactorA = (dstA * calcAlpha255To256(255 - srcA)) >> 8;
    ASSERT(srcA + dstFactorA < (1U << 8));
    uint8_t blendA = srcA + dstFactorA;
    unsigned scale = (1UL << 24) / blendA;

    uint8_t blendR = blendChannel(getRFromColor(src), srcA, getRFromColor(dst), dstFactorA, scale);
    uint8_t blendG = blendChannel(getGFromColor(src), srcA, getGFromColor(dst), dstFactorA, scale);
    uint8_t blendB = blendChannel(getBFromColor(src), srcA, getBFromColor(dst), dstFactorA, scale);

    return packARGB(blendA, blendR, blendG, blendB);
}

// Returns two point ranges (<left, width> pairs) at row 'canvasY', that belong to 'src' but not 'dst'.
// A point range is empty if the corresponding width is 0.
inline void findBlendRangeAtRow(const IntRect& src, const IntRect& dst, int canvasY, int& left1, int& width1, int& left2, int& width2)
{
    ASSERT_WITH_SECURITY_IMPLICATION(canvasY >= src.y() && canvasY < src.maxY());
    left1 = -1;
    width1 = 0;
    left2 = -1;
    width2 = 0;

    if (canvasY < dst.y() || canvasY >= dst.maxY() || src.x() >= dst.maxX() || src.maxX() <= dst.x()) {
        left1 = src.x();
        width1 = src.width();
        return;
    }

    if (src.x() < dst.x()) {
        left1 = src.x();
        width1 = dst.x() - src.x();
    }

    if (src.maxX() > dst.maxX()) {
        left2 = dst.maxX();
        width2 = src.maxX() - dst.maxX();
    }
}

void alphaBlendPremultiplied(ImageFrame& src, ImageFrame& dst, int canvasY, int left, int width)
{
    for (int x = 0; x < width; ++x) {
        int canvasX = left + x;
        ImageFrame::PixelData& pixel = *src.getAddr(canvasX, canvasY);
        if (getAFromColor(pixel) != 0xff) {
            ImageFrame::PixelData prevPixel = *dst.getAddr(canvasX, canvasY);
            pixel = blendSrcOverDstPremultiplied(pixel, prevPixel);
        }
    }
}

void alphaBlendNonPremultiplied(ImageFrame& src, ImageFrame& dst, int canvasY, int left, int width)
{
    for (int x = 0; x < width; ++x) {
        int canvasX = left + x;
        ImageFrame::PixelData& pixel = *src.getAddr(canvasX, canvasY);
        if (getAFromColor(pixel) != 0xff) {
            ImageFrame::PixelData prevPixel = *dst.getAddr(canvasX, canvasY);
            pixel = blendSrcOverDstNonPremultiplied(pixel, prevPixel);
        }
    }
}
#endif

WEBPImageDecoder::WEBPImageDecoder(ImageSource::AlphaOption alphaOption,
                                   ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : ImageDecoder(alphaOption, gammaAndColorProfileOption)
    , m_decoder(0)
#if ENABLE(WKC_BLINK_AWEBP)
    , m_frameBackgroundHasAlpha(false)
#ifdef QCMS_WEBP_COLOR_CORRECTION
    , m_hasColorProfile(false)
#endif
    , m_demux(0)
    , m_demuxState(WEBP_DEMUX_PARSING_HEADER)
    , m_repetitionCount(cAnimationLoopOnce)
    , m_decodedHeight(0)
#else
    , m_hasAlpha(false)
#endif
    , m_formatFlags(0)
#ifdef QCMS_WEBP_COLOR_CORRECTION
    , m_transform(0)
#if !ENABLE(WKC_BLINK_AWEBP)
    , m_haveReadProfile(false)
    , m_decodedHeight(0)
#endif
#endif
{
#if ENABLE(WKC_BLINK_AWEBP)
    m_blendFunction = (alphaOption == ImageSource::AlphaPremultiplied) ? alphaBlendPremultiplied : alphaBlendNonPremultiplied;
#endif
}

WEBPImageDecoder::~WEBPImageDecoder()
{
    clear();
}

void WEBPImageDecoder::clear()
{
#ifdef QCMS_WEBP_COLOR_CORRECTION
    clearColorTransform();
#endif
#if ENABLE(WKC_BLINK_AWEBP)
    if (m_demux)
        WebPDemuxDelete(m_demux);
    m_demux = 0;
    clearDecoder();
#else
    if (m_decoder)
        WebPIDelete(m_decoder);
    m_decoder = 0;
#endif
}

#if ENABLE(WKC_BLINK_AWEBP)
void WEBPImageDecoder::clearDecoder()
{
    if (m_decoder)
        WebPIDelete(m_decoder);
    m_decoder = 0;
    m_decodedHeight = 0;
    m_frameBackgroundHasAlpha = false;
}

void WEBPImageDecoder::setData(SharedBuffer* data, bool allDataReceived)
{
    if (failed())
        return;

    ImageDecoder::setData(data, allDataReceived);
    updateDemuxer();
}

bool WEBPImageDecoder::setSize(unsigned width, unsigned height)
{
    if (ImageDecoder::isSizeAvailable() && size() == IntSize(width, height))
        return true;

    if (!ImageDecoder::setSize(width, height))
        return false;

    prepareScaleDataIfNecessary();
    return true;
}

size_t WEBPImageDecoder::frameCount()
{
    const size_t oldSize = m_frameBufferCache.size();
    const size_t newSize = decodeFrameCount();
    if (oldSize != newSize) {
        m_frameBufferCache.resize(newSize);
        for (size_t i = oldSize; i < newSize; ++i) {
            m_frameBufferCache[i].setPremultiplyAlpha(m_premultiplyAlpha);
            initializeNewFrame(i);
        }
    }
    return newSize;
}

int WEBPImageDecoder::repetitionCount() const
{
    return failed() ? cAnimationLoopOnce : m_repetitionCount;
}

void WEBPImageDecoder::clearFrameBufferCache(size_t clearBeforeFrame)
{
    if (m_frameBufferCache.isEmpty())
        return;

    // See GIFImageDecoder for full explanation.
    clearBeforeFrame = std::min(clearBeforeFrame, m_frameBufferCache.size() - 1);
    const Vector<ImageFrame>::iterator end(m_frameBufferCache.begin() + clearBeforeFrame);

    Vector<ImageFrame>::iterator i(end);
    for (; (i != m_frameBufferCache.begin()) && ((i->status() == ImageFrame::FrameEmpty) || (i->disposalMethod() == ImageFrame::DisposeOverwritePrevious)); --i) {
        if ((i->status() == ImageFrame::FrameComplete) && (i != end))
            i->clearPixelData();
    }

    // Now |i| holds the last frame we need to preserve; clear prior frames.
    for (Vector<ImageFrame>::iterator j(m_frameBufferCache.begin()); j != i; ++j) {
        ASSERT(j->status() != ImageFrame::FramePartial);
        if (j->status() != ImageFrame::FrameEmpty)
            j->clearPixelData();
    }
}
#else
bool WEBPImageDecoder::isSizeAvailable()
{
    if (!ImageDecoder::isSizeAvailable())
         decode(true);

    return ImageDecoder::isSizeAvailable();
}
#endif

ImageFrame* WEBPImageDecoder::frameBufferAtIndex(size_t index)
{
#if ENABLE(WKC_BLINK_AWEBP)
    if (index >= frameCount())
#else
    if (index)
#endif
        return 0;

    if (m_frameBufferCache.isEmpty()) {
        m_frameBufferCache.resize(1);
        m_frameBufferCache[0].setPremultiplyAlpha(m_premultiplyAlpha);
    }

#if ENABLE(WKC_BLINK_AWEBP)
    ImageFrame& frame = m_frameBufferCache[index];
    if (frame.status() != ImageFrame::FrameComplete)
        decode(index);
#else
    ImageFrame& frame = m_frameBufferCache[0];
    if (frame.status() != ImageFrame::FrameComplete)
        decode(false);
#endif
    return &frame;
}

#ifdef QCMS_WEBP_COLOR_CORRECTION

#if ENABLE(WKC_BLINK_AWEBP)
bool WEBPImageDecoder::createColorTransform(const char* data, size_t size)
#else
void WEBPImageDecoder::createColorTransform(const char* data, size_t size)
#endif
{
    clearColorTransform();

    qcms_profile* deviceProfile = ImageDecoder::qcmsOutputDeviceProfile();
    if (!deviceProfile)
#if ENABLE(WKC_BLINK_AWEBP)
        return false;
#else
        return;
#endif
    qcms_profile* inputProfile = qcms_profile_from_memory(data, size);
    if (!inputProfile)
#if ENABLE(WKC_BLINK_AWEBP)
        return false;
#else
        return;
#endif

    // We currently only support color profiles for RGB profiled images.
    ASSERT(icSigRgbData == qcms_profile_get_color_space(inputProfile));
    // The input image pixels are RGBA format.
    qcms_data_type format = QCMS_DATA_RGBA_8;
    // FIXME: Don't force perceptual intent if the image profile contains an intent.
    m_transform = qcms_transform_create(inputProfile, format, deviceProfile, QCMS_DATA_RGBA_8, QCMS_INTENT_PERCEPTUAL);

    qcms_profile_release(inputProfile);
#if !ENABLE(WKC_BLINK_AWEBP)
    return !!m_transform;
#endif
}

#if ENABLE(WKC_BLINK_AWEBP)
void WEBPImageDecoder::readColorProfile()
#else
void WEBPImageDecoder::readColorProfile(const uint8_t* data, size_t size)
#endif
{
    WebPChunkIterator chunkIterator;
#if ENABLE(WKC_BLINK_AWEBP)
    if (!WebPDemuxGetChunk(m_demux, "ICCP", 1, &chunkIterator)) {
#else
    WebPData inputData = { data, size };
    WebPDemuxState state;

    WebPDemuxer* demuxer = WebPDemuxPartial(&inputData, &state);
    if (!WebPDemuxGetChunk(demuxer, "ICCP", 1, &chunkIterator)) {
#endif
        WebPDemuxReleaseChunkIterator(&chunkIterator);
#if !ENABLE(WKC_BLINK_AWEBP)
        WebPDemuxDelete(demuxer);
#endif
        return;
    }

    const char* profileData = reinterpret_cast<const char*>(chunkIterator.chunk.bytes);
    size_t profileSize = chunkIterator.chunk.size;

    // Only accept RGB color profiles from input class devices.
    bool ignoreProfile = false;
    if (profileSize < ImageDecoder::iccColorProfileHeaderLength)
        ignoreProfile = true;
    else if (!ImageDecoder::rgbColorProfile(profileData, profileSize))
        ignoreProfile = true;
    else if (!ImageDecoder::inputDeviceColorProfile(profileData, profileSize))
        ignoreProfile = true;

    if (!ignoreProfile)
#if ENABLE(WKC_BLINK_AWEBP)
        m_hasColorProfile = createColorTransform(profileData, profileSize);
#else
        createColorTransform(profileData, profileSize);
#endif

    WebPDemuxReleaseChunkIterator(&chunkIterator);
#if !ENABLE(WKC_BLINK_AWEBP)
    WebPDemuxDelete(demuxer);
#endif
}

#if !ENABLE(WKC_BLINK_AWEBP)
void WEBPImageDecoder::applyColorProfile(const uint8_t* data, size_t size, ImageFrame& buffer)
{
    int width;
    int decodedHeight;
    if (!WebPIDecGetRGB(m_decoder, &decodedHeight, &width, 0, 0))
        return; // See also https://bugs.webkit.org/show_bug.cgi?id=74062
    if (decodedHeight <= 0)
        return;

    if (!m_haveReadProfile) {
        readColorProfile(data, size);
        m_haveReadProfile = true;
    }

    ASSERT(width == scaledSize().width());
    ASSERT(decodedHeight <= scaledSize().height());

    for (int y = m_decodedHeight; y < decodedHeight; ++y) {
        uint8_t* row = reinterpret_cast<uint8_t*>(buffer.getAddr(0, y));
        if (qcms_transform* transform = colorTransform())
            qcms_transform_data_type(transform, row, row, width, QCMS_OUTPUT_RGBX);
        uint8_t* pixel = row;
        for (int x = 0; x < width; ++x, pixel += 4)
            buffer.setRGBA(x, y, pixel[0], pixel[1], pixel[2], pixel[3]);
    }

    m_decodedHeight = decodedHeight;
}
#endif

void WEBPImageDecoder::clearColorTransform()
{
    if (m_transform)
        qcms_transform_release(m_transform);
    m_transform = 0;
}

#endif // QCMS_WEBP_COLOR_CORRECTION

#if ENABLE(WKC_BLINK_AWEBP)
size_t WEBPImageDecoder::decodeFrameCount()
{
    // If failed() true, return the existing number of frames.  This way
    // if we get halfway through the image before decoding fails, we won't
    // suddenly start reporting that the image has zero frames.
    return failed() ? m_frameBufferCache.size() : WebPDemuxGetI(m_demux, WEBP_FF_FRAME_COUNT);
}

size_t WEBPImageDecoder::findRequiredPreviousFrame(size_t frameIndex, bool frameRectIsOpaque)
{
    ASSERT(frameIndex <= m_frameBufferCache.size());
    if (!frameIndex) {
        // The first frame doesn't rely on any previous data.
        return -1;
    }

    const ImageFrame* currBuffer = &m_frameBufferCache[frameIndex];
    if ((frameRectIsOpaque || currBuffer->alphaBlendSource() == ImageFrame::BlendAtopBgcolor)
        && currBuffer->originalFrameRect().contains(IntRect(IntPoint(), size())))
        return -1;

    // The starting state for this frame depends on the previous frame's
    // disposal method.
    size_t prevFrame = frameIndex - 1;
    const ImageFrame* prevBuffer = &m_frameBufferCache[prevFrame];

    switch (prevBuffer->disposalMethod()) {
    case ImageFrame::DisposeNotSpecified:
    case ImageFrame::DisposeKeep:
        // prevFrame will be used as the starting state for this frame.
        // FIXME: Be even smarter by checking the frame sizes and/or alpha-containing regions.
        return prevFrame;
    case ImageFrame::DisposeOverwritePrevious:
        // Frames that use the DisposeOverwritePrevious method are effectively
        // no-ops in terms of changing the starting state of a frame compared to
        // the starting state of the previous frame, so skip over them and
        // return the required previous frame of it.
        return prevBuffer->requiredPreviousFrameIndex();
    case ImageFrame::DisposeOverwriteBgcolor:
        // If the previous frame fills the whole image, then the current frame
        // can be decoded alone. Likewise, if the previous frame could be
        // decoded without reference to any prior frame, the starting state for
        // this frame is a blank frame, so it can again be decoded alone.
        // Otherwise, the previous frame contributes to this frame.
        return (prevBuffer->originalFrameRect().contains(IntRect(IntPoint(), size()))
            || (prevBuffer->requiredPreviousFrameIndex() == -1)) ? -1 : prevFrame;
    default:
        ASSERT_NOT_REACHED();
        return -1;
    }
}

void WEBPImageDecoder::initializeNewFrame(size_t index)
{
    if (!(m_formatFlags & ANIMATION_FLAG)) {
        ASSERT(!index);
        return;
    }
    WebPIterator animatedFrame;
    WebPDemuxGetFrame(m_demux, index + 1, &animatedFrame);
    ASSERT(animatedFrame.complete == 1);
    ImageFrame* buffer = &m_frameBufferCache[index];
    IntRect frameRect(animatedFrame.x_offset, animatedFrame.y_offset, animatedFrame.width, animatedFrame.height);
    buffer->setOriginalFrameRect(intersection(frameRect, IntRect(IntPoint(), size())));
    buffer->setDuration(animatedFrame.duration);
    buffer->setDisposalMethod(animatedFrame.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND ? ImageFrame::DisposeOverwriteBgcolor : ImageFrame::DisposeKeep);
    buffer->setAlphaBlendSource(animatedFrame.blend_method == WEBP_MUX_BLEND ? ImageFrame::BlendAtopPreviousFrame : ImageFrame::BlendAtopBgcolor);
    buffer->setRequiredPreviousFrameIndex(findRequiredPreviousFrame(index, !animatedFrame.has_alpha));
    WebPDemuxReleaseIterator(&animatedFrame);
}

void WEBPImageDecoder::decode(size_t index)
{
    if (failed())
        return;

    Vector<size_t> framesToDecode;
    size_t frameToDecode = index;
    do {
        framesToDecode.append(frameToDecode);
        frameToDecode = m_frameBufferCache[frameToDecode].requiredPreviousFrameIndex();
    } while (frameToDecode != -1 && m_frameBufferCache[frameToDecode].status() != ImageFrame::FrameComplete);

    ASSERT(m_demux);
    for (auto i = framesToDecode.rbegin(); i != framesToDecode.rend(); ++i) {
        if ((m_formatFlags & ANIMATION_FLAG) && !initFrameBuffer(*i))
            return;
        WebPIterator webpFrame;
        if (!WebPDemuxGetFrame(m_demux, *i + 1, &webpFrame)) {
            setFailed();
        } else {
            decodeSingleFrame(webpFrame.fragment.bytes, webpFrame.fragment.size, *i);
            WebPDemuxReleaseIterator(&webpFrame);
        }
        if (failed())
            return;

        // We need more data to continue decoding.
        if (m_frameBufferCache[*i].status() != ImageFrame::FrameComplete)
            break;
    }

    // It is also a fatal error if all data is received and we have decoded all
    // frames available but the file is truncated.
    if (index >= m_frameBufferCache.size() - 1 && isAllDataReceived() && m_demux && m_demuxState != WEBP_DEMUX_DONE)
        setFailed();
}

bool WEBPImageDecoder::decodeSingleFrame(const uint8_t* dataBytes, size_t dataSize, size_t frameIndex)
{
    if (failed())
        return false;

    ASSERT(isSizeAvailable());

    ASSERT(m_frameBufferCache.size() > frameIndex);
    ImageFrame& buffer = m_frameBufferCache[frameIndex];
    ASSERT(buffer.status() != ImageFrame::FrameComplete);

    if (buffer.status() == ImageFrame::FrameEmpty) {
        if (!buffer.setSize(size().width(), size().height()))
            return setFailed();
        buffer.setStatus(ImageFrame::FramePartial);
        // The buffer is transparent outside the decoded area while the image is loading.
        // The correct value of 'hasAlpha' for the frame will be set when it is fully decoded.
        buffer.setHasAlpha(true);
        buffer.setOriginalFrameRect(IntRect(IntPoint(), size()));
    }

    const IntRect& frameRect = buffer.originalFrameRect();
    if (!m_decoder) {
        WEBP_CSP_MODE mode = outputMode(m_formatFlags & ALPHA_FLAG);
        if (!m_premultiplyAlpha)
            mode = outputMode(false);
#ifdef QCMS_WEBP_COLOR_CORRECTION
        if (colorTransform())
            mode = MODE_RGBA; // Decode to RGBA for input to libqcms.
#endif
        WebPInitDecBuffer(&m_decoderBuffer);
        m_decoderBuffer.colorspace = mode;
        m_decoderBuffer.u.RGBA.stride = size().width() * sizeof(ImageFrame::PixelData);
        m_decoderBuffer.u.RGBA.size = m_decoderBuffer.u.RGBA.stride * frameRect.height();
        m_decoderBuffer.is_external_memory = 1;
        m_decoder = WebPINewDecoder(&m_decoderBuffer);
        if (!m_decoder)
            return setFailed();
    }

    m_decoderBuffer.u.RGBA.rgba = reinterpret_cast<uint8_t*>(buffer.getAddr(frameRect.x(), frameRect.y()));

    switch (WebPIUpdate(m_decoder, dataBytes, dataSize)) {
    case VP8_STATUS_OK:
        applyPostProcessing(frameIndex);
        buffer.setHasAlpha((m_formatFlags & ALPHA_FLAG) || m_frameBackgroundHasAlpha);
        buffer.setStatus(ImageFrame::FrameComplete);
        clearDecoder();
        return true;
    case VP8_STATUS_SUSPENDED:
        if (!isAllDataReceived() && !frameIsCompleteAtIndex(frameIndex)) {
            applyPostProcessing(frameIndex);
            return false;
        }
        // FALLTHROUGH
    default:
        clear();
        return setFailed();
    }
}

bool WEBPImageDecoder::frameIsCompleteAtIndex(size_t index) const
{
    if (!m_demux || m_demuxState <= WEBP_DEMUX_PARSING_HEADER)
        return false;
    return (index < m_frameBufferCache.size()) &&
        (m_frameBufferCache[index].status() == ImageFrame::FrameComplete);
}

bool WEBPImageDecoder::updateDemuxer()
{
    if (failed())
        return false;

    const unsigned webpHeaderSize = 30;
    if (m_data->size() < webpHeaderSize)
        return false; // Await VP8X header so WebPDemuxPartial succeeds.

    WebPDemuxDelete(m_demux);
    WebPData inputData = { reinterpret_cast<const uint8_t*>(m_data->data()), m_data->size() };
    m_demux = WebPDemuxPartial(&inputData, &m_demuxState);
    if (!m_demux || (isAllDataReceived() && m_demuxState != WEBP_DEMUX_DONE))
        return setFailed();

    ASSERT(m_demuxState > WEBP_DEMUX_PARSING_HEADER);
    if (!WebPDemuxGetI(m_demux, WEBP_FF_FRAME_COUNT))
        return false; // Wait until the encoded image frame data arrives.

    if (!isSizeAvailable()) {
        int width = WebPDemuxGetI(m_demux, WEBP_FF_CANVAS_WIDTH);
        int height = WebPDemuxGetI(m_demux, WEBP_FF_CANVAS_HEIGHT);
        if (!setSize(width, height))
            return setFailed();

        m_formatFlags = WebPDemuxGetI(m_demux, WEBP_FF_FORMAT_FLAGS);
        if (!(m_formatFlags & ANIMATION_FLAG)) {
            m_repetitionCount = cAnimationNone;
        } else {
            // Since we have parsed at least one frame, even if partially,
            // the global animation (ANIM) properties have been read since
            // an ANIM chunk must precede the ANMF frame chunks.
            m_repetitionCount = WebPDemuxGetI(m_demux, WEBP_FF_LOOP_COUNT);
            // Repetition count is always <= 16 bits.
            ASSERT(m_repetitionCount == (m_repetitionCount & 0xffff));
            // Repetition count is the number of animation cycles to show,
            // where 0 means "infinite". But ImageSource::repetitionCount()
            // returns -1 for "infinite", and 0 and up for "show the image
            // animation one cycle more than the value". Subtract one here
            // to correctly handle the finite and infinite cases.
            --m_repetitionCount;
            // FIXME: Implement ICC profile support for animated images.
            m_formatFlags &= ~ICCP_FLAG;
        }

#ifdef QCMS_WEBP_COLOR_CORRECTION
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile())
            readColorProfile();
#endif
    }

    ASSERT(isSizeAvailable());
    return true;
}

bool WEBPImageDecoder::initFrameBuffer(size_t frameIndex)
{
    ImageFrame& buffer = m_frameBufferCache[frameIndex];
    if (buffer.status() != ImageFrame::FrameEmpty) // Already initialized.
        return true;

    const size_t requiredPreviousFrameIndex = buffer.requiredPreviousFrameIndex();
    if (requiredPreviousFrameIndex == -1) {
        // This frame doesn't rely on any previous data.
        if (!buffer.setSize(size().width(), size().height()))
            return setFailed();
        m_frameBackgroundHasAlpha = !buffer.originalFrameRect().contains(IntRect(IntPoint(), size()));
    } else {
        const ImageFrame& prevBuffer = m_frameBufferCache[requiredPreviousFrameIndex];
        ASSERT(prevBuffer.status() == ImageFrame::FrameComplete);

        // Preserve the last frame as the starting state for this frame.
        if (!buffer.copyBitmapData(prevBuffer))
            return setFailed();

        if (prevBuffer.disposalMethod() == ImageFrame::DisposeOverwriteBgcolor) {
            // We want to clear the previous frame to transparent, without
            // affecting pixels in the image outside of the frame.
            const IntRect& prevRect = prevBuffer.originalFrameRect();
            ASSERT(!prevRect.contains(IntRect(IntPoint(), size())));
            buffer.zeroFillFrameRect(prevRect);
        }

        m_frameBackgroundHasAlpha = prevBuffer.hasAlpha() || (prevBuffer.disposalMethod() == ImageFrame::DisposeOverwriteBgcolor);
    }

    buffer.setStatus(ImageFrame::FramePartial);
    // The buffer is transparent outside the decoded area while the image is loading.
    // The correct value of 'hasAlpha' for the frame will be set when it is fully decoded.
    buffer.setHasAlpha(true);
    return true;
}

void WEBPImageDecoder::applyPostProcessing(size_t frameIndex)
{
    ImageFrame& buffer = m_frameBufferCache[frameIndex];
    int width;
    int decodedHeight;
    if (!WebPIDecGetRGB(m_decoder, &decodedHeight, &width, 0, 0))
        return; // See also https://bugs.webkit.org/show_bug.cgi?id=74062
    if (decodedHeight <= 0)
        return;

    const IntRect& frameRect = buffer.originalFrameRect();
    ASSERT_WITH_SECURITY_IMPLICATION(width == frameRect.width());
    ASSERT_WITH_SECURITY_IMPLICATION(decodedHeight <= frameRect.height());
    const int left = frameRect.x();
    const int top = frameRect.y();

#ifdef QCMS_WEBP_COLOR_CORRECTION
    if (qcms_transform* transform = colorTransform()) {
        for (int y = m_decodedHeight; y < decodedHeight; ++y) {
            const int canvasY = top + y;
            uint8_t* row = reinterpret_cast<uint8_t*>(buffer.getAddr(left, canvasY));
            qcms_transform_data_type(transform, row, row, width, QCMS_OUTPUT_RGBX);
            uint8_t* pixel = row;
            for (int x = 0; x < width; ++x, pixel += 4) {
                const int canvasX = left + x;
                buffer.setRGBA(canvasX, canvasY, pixel[0], pixel[1], pixel[2], pixel[3]);
            }
        }
    }
#endif // QCMS_WEBP_COLOR_CORRECTION

    // During the decoding of current frame, we may have set some pixels to be transparent (i.e. alpha < 255).
    // However, the value of each of these pixels should have been determined by blending it against the value
    // of that pixel in the previous frame if alpha blend source was 'BlendAtopPreviousFrame'. So, we correct these
    // pixels based on disposal method of the previous frame and the previous frame buffer.
    // FIXME: This could be avoided if libwebp decoder had an API that used the previous required frame
    // to do the alpha-blending by itself.
    if ((m_formatFlags & ANIMATION_FLAG) && frameIndex && buffer.alphaBlendSource() == ImageFrame::BlendAtopPreviousFrame && buffer.requiredPreviousFrameIndex() != -1) {
        ImageFrame& prevBuffer = m_frameBufferCache[frameIndex - 1];
        ASSERT(prevBuffer.status() == ImageFrame::FrameComplete);
        ImageFrame::FrameDisposalMethod prevDisposalMethod = prevBuffer.disposalMethod();
        if (prevDisposalMethod == ImageFrame::DisposeKeep) { // Blend transparent pixels with pixels in previous canvas.
            for (int y = m_decodedHeight; y < decodedHeight; ++y) {
                m_blendFunction(buffer, prevBuffer, top + y, left, width);
            }
        } else if (prevDisposalMethod == ImageFrame::DisposeOverwriteBgcolor) {
            const IntRect& prevRect = prevBuffer.originalFrameRect();
            // We need to blend a transparent pixel with its value just after initFrame() call. That is:
            //   * Blend with fully transparent pixel if it belongs to prevRect <-- This is a no-op.
            //   * Blend with the pixel in the previous canvas otherwise <-- Needs alpha-blending.
            for (int y = m_decodedHeight; y < decodedHeight; ++y) {
                int canvasY = top + y;
                int left1, width1, left2, width2;
                findBlendRangeAtRow(frameRect, prevRect, canvasY, left1, width1, left2, width2);
                if (width1 > 0)
                    m_blendFunction(buffer, prevBuffer, canvasY, left1, width1);
                if (width2 > 0)
                    m_blendFunction(buffer, prevBuffer, canvasY, left2, width2);
            }
        }
    }

    m_decodedHeight = decodedHeight;
}
#else
bool WEBPImageDecoder::decode(bool onlySize)
{
    if (failed())
        return false;

    const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(m_data->data());
    const size_t dataSize = m_data->size();

    if (!ImageDecoder::isSizeAvailable()) {
        static const size_t imageHeaderSize = 30;
        if (dataSize < imageHeaderSize)
            return false;
        int width, height;
#ifdef QCMS_WEBP_COLOR_CORRECTION
        WebPData inputData = { dataBytes, dataSize };
        WebPDemuxState state;
        WebPDemuxer* demuxer = WebPDemuxPartial(&inputData, &state);
        if (!demuxer)
            return setFailed();

        width = WebPDemuxGetI(demuxer, WEBP_FF_CANVAS_WIDTH);
        height = WebPDemuxGetI(demuxer, WEBP_FF_CANVAS_HEIGHT);
        m_formatFlags = WebPDemuxGetI(demuxer, WEBP_FF_FORMAT_FLAGS);
        m_hasAlpha = !!(m_formatFlags & ALPHA_FLAG);

        WebPDemuxDelete(demuxer);
        if (state <= WEBP_DEMUX_PARSING_HEADER)
            return false;
#elif (WEBP_DECODER_ABI_VERSION >= 0x0163)
        WebPBitstreamFeatures features;
        if (WebPGetFeatures(dataBytes, dataSize, &features) != VP8_STATUS_OK)
            return setFailed();
        width = features.width;
        height = features.height;
        m_hasAlpha = features.has_alpha;
#else
        // Earlier version won't be able to display WebP files with alpha.
        if (!WebPGetInfo(dataBytes, dataSize, &width, &height))
            return setFailed();
        m_hasAlpha = false;
#endif
        if (!setSize(width, height))
            return setFailed();
    }

    ASSERT(ImageDecoder::isSizeAvailable());
    if (onlySize)
        return true;

    ASSERT(!m_frameBufferCache.isEmpty());
    ImageFrame& buffer = m_frameBufferCache[0];
    ASSERT(buffer.status() != ImageFrame::FrameComplete);

    if (buffer.status() == ImageFrame::FrameEmpty) {
        if (!buffer.setSize(size().width(), size().height()))
            return setFailed();
        buffer.setStatus(ImageFrame::FramePartial);
        buffer.setHasAlpha(m_hasAlpha);
        buffer.setOriginalFrameRect(IntRect(IntPoint(), size()));
    }

    if (!m_decoder) {
        WEBP_CSP_MODE mode = outputMode(m_hasAlpha);
        if (!m_premultiplyAlpha)
            mode = outputMode(false);
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile())
            mode = MODE_RGBA; // Decode to RGBA for input to libqcms.
        int rowStride = size().width() * sizeof(ImageFrame::PixelData);
        uint8_t* output = reinterpret_cast<uint8_t*>(buffer.getAddr(0, 0));
        int outputSize = size().height() * rowStride;
        m_decoder = WebPINewRGB(mode, output, outputSize, rowStride);
        if (!m_decoder)
            return setFailed();
    }

    switch (WebPIUpdate(m_decoder, dataBytes, dataSize)) {
    case VP8_STATUS_OK:
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile()) 
            applyColorProfile(dataBytes, dataSize, buffer);
        buffer.setStatus(ImageFrame::FrameComplete);
        clear();
        return true;
    case VP8_STATUS_SUSPENDED:
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile()) 
            applyColorProfile(dataBytes, dataSize, buffer);
        return false;
    default:
        clear();                         
        return setFailed();
    }
}
#endif

} // namespace WebCore

#endif
