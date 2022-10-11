/*
 * Copyright (C) Research In Motion Limited 2010, 2012. All rights reserved.
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
#include "SVGPathUtilities.h"

#include "Path.h"
#include "PathTraversalState.h"
#include "SVGPathBlender.h"
#include "SVGPathBuilder.h"
#include "SVGPathByteStreamBuilder.h"
#include "SVGPathByteStreamSource.h"
#include "SVGPathElement.h"
#include "SVGPathParser.h"
#include "SVGPathSegListBuilder.h"
#include "SVGPathSegListSource.h"
#include "SVGPathStringBuilder.h"
#include "SVGPathStringSource.h"
#include "SVGPathTraversalStateBuilder.h"

namespace WebCore {

static SVGPathBuilder* globalSVGPathBuilder(Path& result)
{
#if !PLATFORM(WKC)
    static SVGPathBuilder* s_builder = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathBuilder*, s_builder, nullptr);
#endif
    if (!s_builder)
        s_builder = new SVGPathBuilder;

    s_builder->setCurrentPath(&result);
    return s_builder;
}

static SVGPathSegListBuilder* globalSVGPathSegListBuilder(SVGPathElement* element, SVGPathSegRole role, SVGPathSegList& result)
{
#if !PLATFORM(WKC)
    static SVGPathSegListBuilder* s_builder = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathSegListBuilder*, s_builder, nullptr);
#endif
    if (!s_builder)
        s_builder = new SVGPathSegListBuilder;

    s_builder->setCurrentSVGPathElement(element);
    s_builder->setCurrentSVGPathSegList(result);
    s_builder->setCurrentSVGPathSegRole(role);
    return s_builder;
}

static SVGPathByteStreamBuilder* globalSVGPathByteStreamBuilder(SVGPathByteStream* result)
{
#if !PLATFORM(WKC)
    static SVGPathByteStreamBuilder* s_builder = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathByteStreamBuilder*, s_builder, nullptr);
#endif
    if (!s_builder)
        s_builder = new SVGPathByteStreamBuilder;

    s_builder->setCurrentByteStream(result);
    return s_builder;
}

static SVGPathStringBuilder* globalSVGPathStringBuilder()
{
#if !PLATFORM(WKC)
    static SVGPathStringBuilder* s_builder = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathStringBuilder*, s_builder, nullptr)
#endif
    if (!s_builder)
        s_builder = new SVGPathStringBuilder;

    return s_builder;
}

static SVGPathTraversalStateBuilder* globalSVGPathTraversalStateBuilder(PathTraversalState& traversalState, float length)
{
#if !PLATFORM(WKC)
    static SVGPathTraversalStateBuilder* s_builder = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathTraversalStateBuilder*, s_builder, nullptr);
#endif
    if (!s_builder)
        s_builder = new SVGPathTraversalStateBuilder;

    s_builder->setCurrentTraversalState(&traversalState);
    s_builder->setDesiredLength(length);
    return s_builder;
}

static SVGPathParser* globalSVGPathParser(SVGPathSource* source, SVGPathConsumer* consumer)
{
#if !PLATFORM(WKC)
    static SVGPathParser* s_parser = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathParser*, s_parser, nullptr);
#endif
    if (!s_parser)
        s_parser = new SVGPathParser;

    s_parser->setCurrentSource(source);
    s_parser->setCurrentConsumer(consumer);
    return s_parser;
}

static SVGPathBlender* globalSVGPathBlender()
{
#if !PLATFORM(WKC)
    static SVGPathBlender* s_blender = nullptr;
#else
    WKC_DEFINE_STATIC_PTR(SVGPathBlender*, s_blender, nullptr);
#endif
    if (!s_blender)
        s_blender = new SVGPathBlender;

    return s_blender;
}

bool buildPathFromString(const String& d, Path& result)
{
    if (d.isEmpty())
        return true;

    SVGPathBuilder* builder = globalSVGPathBuilder(result);

    auto source = std::make_unique<SVGPathStringSource>(d);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    parser->cleanup();
    return ok;
}

bool buildSVGPathByteStreamFromSVGPathSegList(const SVGPathSegList& list, SVGPathByteStream* result, PathParsingMode parsingMode)
{
    ASSERT(result);
    result->clear();
    if (list.isEmpty())
        return true;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(result);

    auto source = std::make_unique<SVGPathSegListSource>(list);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    parser->cleanup();
    return ok;
}

bool appendSVGPathByteStreamFromSVGPathSeg(PassRefPtr<SVGPathSeg> pathSeg, SVGPathByteStream* result, PathParsingMode parsingMode)
{
    ASSERT(result);
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    ASSERT(parsingMode == UnalteredParsing);

    SVGPathSegList appendedItemList(PathSegUnalteredRole);
    appendedItemList.append(pathSeg);
    auto appendedByteStream = std::make_unique<SVGPathByteStream>();

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(appendedByteStream.get());
    auto source = std::make_unique<SVGPathSegListSource>(appendedItemList);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode, false);
    parser->cleanup();

    if (ok)
        result->append(*appendedByteStream);

    return ok;
}

bool buildPathFromByteStream(SVGPathByteStream* stream, Path& result)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return true;

    SVGPathBuilder* builder = globalSVGPathBuilder(result);

    auto source = std::make_unique<SVGPathByteStreamSource>(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    parser->cleanup();
    return ok;
}

bool buildSVGPathSegListFromByteStream(SVGPathByteStream* stream, SVGPathElement* element, SVGPathSegList& result, PathParsingMode parsingMode)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return true;

    SVGPathSegListBuilder* builder = globalSVGPathSegListBuilder(element, parsingMode == NormalizedParsing ? PathSegNormalizedRole : PathSegUnalteredRole, result);

    auto source = std::make_unique<SVGPathByteStreamSource>(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    parser->cleanup();
    return ok;
}

bool buildStringFromByteStream(SVGPathByteStream* stream, String& result, PathParsingMode parsingMode)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return true;

    SVGPathStringBuilder* builder = globalSVGPathStringBuilder();

    auto source = std::make_unique<SVGPathByteStreamSource>(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    result = builder->result();
    parser->cleanup();
    return ok;
}

bool buildStringFromSVGPathSegList(const SVGPathSegList& list, String& result, PathParsingMode parsingMode)
{
    result = String();
    if (list.isEmpty())
        return true;

    SVGPathStringBuilder* builder = globalSVGPathStringBuilder();

    auto source = std::make_unique<SVGPathSegListSource>(list);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    result = builder->result();
    parser->cleanup();
    return ok;
}

bool buildSVGPathByteStreamFromString(const String& d, SVGPathByteStream* result, PathParsingMode parsingMode)
{
    ASSERT(result);
    result->clear();
    if (d.isEmpty())
        return true;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(result);

    auto source = std::make_unique<SVGPathStringSource>(d);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(parsingMode);
    parser->cleanup();
    return ok;
}

bool buildAnimatedSVGPathByteStream(SVGPathByteStream* fromStream, SVGPathByteStream* toStream, SVGPathByteStream* result, float progress)
{
    ASSERT(fromStream);
    ASSERT(toStream);
    ASSERT(result);
    ASSERT(toStream != result);

    result->clear();
    if (toStream->isEmpty())
        return true;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(result);

    auto fromSource = std::make_unique<SVGPathByteStreamSource>(fromStream);
    auto toSource = std::make_unique<SVGPathByteStreamSource>(toStream);
    SVGPathBlender* blender = globalSVGPathBlender();
    bool ok = blender->blendAnimatedPath(progress, fromSource.get(), toSource.get(), builder);
    blender->cleanup();
    return ok;
}

bool addToSVGPathByteStream(SVGPathByteStream* fromStream, SVGPathByteStream* byStream, unsigned repeatCount)
{
    ASSERT(fromStream);
    ASSERT(byStream);
    if (fromStream->isEmpty() || byStream->isEmpty())
        return true;

    SVGPathByteStreamBuilder* builder = globalSVGPathByteStreamBuilder(fromStream);

    auto fromStreamCopy = fromStream->copy();
    fromStream->clear();

    auto fromSource = std::make_unique<SVGPathByteStreamSource>(fromStreamCopy.get());
    auto bySource = std::make_unique<SVGPathByteStreamSource>(byStream);
    SVGPathBlender* blender = globalSVGPathBlender();
    bool ok = blender->addAnimatedPath(fromSource.get(), bySource.get(), builder, repeatCount);
    blender->cleanup();
    return ok;
}

bool getSVGPathSegAtLengthFromSVGPathByteStream(SVGPathByteStream* stream, float length, unsigned& pathSeg)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::SegmentAtLength);
    SVGPathTraversalStateBuilder* builder = globalSVGPathTraversalStateBuilder(traversalState, length);

    auto source = std::make_unique<SVGPathByteStreamSource>(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    pathSeg = builder->pathSegmentIndex();
    parser->cleanup();
    return ok;
}

bool getTotalLengthOfSVGPathByteStream(SVGPathByteStream* stream, float& totalLength)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::TotalLength);
    SVGPathTraversalStateBuilder* builder = globalSVGPathTraversalStateBuilder(traversalState, 0);

    auto source = std::make_unique<SVGPathByteStreamSource>(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    totalLength = builder->totalLength();
    parser->cleanup();
    return ok;
}

bool getPointAtLengthOfSVGPathByteStream(SVGPathByteStream* stream, float length, SVGPoint& point)
{
    ASSERT(stream);
    if (stream->isEmpty())
        return false;

    PathTraversalState traversalState(PathTraversalState::Action::VectorAtLength);
    SVGPathTraversalStateBuilder* builder = globalSVGPathTraversalStateBuilder(traversalState, length);

    auto source = std::make_unique<SVGPathByteStreamSource>(stream);
    SVGPathParser* parser = globalSVGPathParser(source.get(), builder);
    bool ok = parser->parsePathDataFromSource(NormalizedParsing);
    point = builder->currentPoint();
    parser->cleanup();
    return ok;
}

}
