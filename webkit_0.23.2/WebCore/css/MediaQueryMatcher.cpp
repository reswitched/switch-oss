/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "MediaQueryMatcher.h"

#include "Document.h"
#include "Element.h"
#include "Frame.h"
#include "FrameView.h"
#include "MediaList.h"
#include "MediaQueryEvaluator.h"
#include "MediaQueryList.h"
#include "MediaQueryListListener.h"
#include "NodeRenderStyle.h"
#include "StyleResolver.h"

namespace WebCore {

MediaQueryMatcher::Listener::Listener(PassRefPtr<MediaQueryListListener> listener, PassRefPtr<MediaQueryList> query)
    : m_listener(listener)
    , m_query(query)
{
}

MediaQueryMatcher::Listener::~Listener()
{
}

void MediaQueryMatcher::Listener::evaluate(MediaQueryEvaluator* evaluator)
{
    bool notify;
    m_query->evaluate(evaluator, notify);
    if (notify)
        m_listener->queryChanged(m_query.get());
}

MediaQueryMatcher::MediaQueryMatcher(Document* document)
    : m_document(document)
    , m_evaluationRound(1)
{
    ASSERT(m_document);
}

MediaQueryMatcher::~MediaQueryMatcher()
{
}

void MediaQueryMatcher::documentDestroyed()
{
    m_listeners.clear();
    m_document = 0;
}

String MediaQueryMatcher::mediaType() const
{
    if (!m_document || !m_document->frame() || !m_document->frame()->view())
        return String();

    return m_document->frame()->view()->mediaType();
}

std::unique_ptr<MediaQueryEvaluator> MediaQueryMatcher::prepareEvaluator() const
{
    if (!m_document || !m_document->frame())
        return nullptr;

    Element* documentElement = m_document->documentElement();
    if (!documentElement)
        return nullptr;

    RefPtr<RenderStyle> rootStyle = m_document->ensureStyleResolver().styleForElement(documentElement, m_document->renderStyle(), DisallowStyleSharing, MatchOnlyUserAgentRules);

    return std::make_unique<MediaQueryEvaluator>(mediaType(), m_document->frame(), rootStyle.get());
}

bool MediaQueryMatcher::evaluate(const MediaQuerySet* media)
{
    if (!media)
        return false;

    std::unique_ptr<MediaQueryEvaluator> evaluator = prepareEvaluator();
    return evaluator && evaluator->eval(media);
}

PassRefPtr<MediaQueryList> MediaQueryMatcher::matchMedia(const String& query)
{
    if (!m_document)
        return 0;

    RefPtr<MediaQuerySet> media = MediaQuerySet::create(query);
#if ENABLE(RESOLUTION_MEDIA_QUERY)
    // Add warning message to inspector whenever dpi/dpcm values are used for "screen" media.
    reportMediaQueryWarningIfNeeded(m_document, media.get());
#endif
    return MediaQueryList::create(this, media, evaluate(media.get()));
}

void MediaQueryMatcher::addListener(PassRefPtr<MediaQueryListListener> listener, PassRefPtr<MediaQueryList> query)
{
    if (!m_document)
        return;

    for (size_t i = 0; i < m_listeners.size(); ++i) {
        if (*m_listeners[i]->listener() == *listener && m_listeners[i]->query() == query)
            return;
    }

    m_listeners.append(std::make_unique<Listener>(listener, query));
}

void MediaQueryMatcher::removeListener(MediaQueryListListener* listener, MediaQueryList* query)
{
    if (!m_document)
        return;

    m_listeners.removeFirstMatching([listener, query] (const std::unique_ptr<Listener>& current) {
        return *current->listener() == *listener && current->query() == query;
    });
}

void MediaQueryMatcher::styleResolverChanged()
{
    ASSERT(m_document);

    ++m_evaluationRound;
    std::unique_ptr<MediaQueryEvaluator> evaluator = prepareEvaluator();
    if (!evaluator)
        return;

    for (size_t i = 0; i < m_listeners.size(); ++i)
        m_listeners[i]->evaluate(evaluator.get());
}

} // namespace WebCore
