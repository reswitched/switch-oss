/*
 *  Copyright (C) 2010, 2015 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DOMObjectCache.h"

#include "DOMWindowProperty.h"
#include "Document.h"
#include "Frame.h"
#include "FrameDestructionObserver.h"
#include "Node.h"
#include <glib-object.h>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/Vector.h>
#include <wtf/glib/GRefPtr.h>

namespace WebKit {

struct DOMObjectCacheData {
    DOMObjectCacheData(GObject* wrapper)
        : object(wrapper)
        , cacheReferences(1)
    {
    }

    void clearObject()
    {
        ASSERT(object);
        ASSERT(cacheReferences >= 1);
        ASSERT(object->ref_count >= 1);

        // Make sure we don't unref more than the references the object actually has. It can happen that user
        // unreffed a reference owned by the cache.
        cacheReferences = std::min(static_cast<unsigned>(object->ref_count), cacheReferences);
        GRefPtr<GObject> protect(object);
        do {
            g_object_unref(object);
        } while (--cacheReferences);
        object = nullptr;
    }

    void* refObject()
    {
        ASSERT(object);

        cacheReferences++;
        return g_object_ref(object);
    }

    GObject* object;
    unsigned cacheReferences;
};

class DOMObjectCacheFrameObserver;
typedef HashMap<WebCore::Frame*, std::unique_ptr<DOMObjectCacheFrameObserver>> DOMObjectCacheFrameObserverMap;

static DOMObjectCacheFrameObserverMap& domObjectCacheFrameObservers()
{
    static NeverDestroyed<DOMObjectCacheFrameObserverMap> map;
    return map;
}

static DOMObjectCacheFrameObserver& getOrCreateDOMObjectCacheFrameObserver(WebCore::Frame& frame)
{
    DOMObjectCacheFrameObserverMap::AddResult result = domObjectCacheFrameObservers().add(&frame, nullptr);
    if (result.isNewEntry)
        result.iterator->value = std::make_unique<DOMObjectCacheFrameObserver>(frame);
    return *result.iterator->value;
}

class DOMObjectCacheFrameObserver final: public WebCore::FrameDestructionObserver {
public:
    DOMObjectCacheFrameObserver(WebCore::Frame& frame)
        : FrameDestructionObserver(&frame)
    {
    }

    ~DOMObjectCacheFrameObserver()
    {
        ASSERT(m_objects.isEmpty());
    }

    void addObjectCacheData(DOMObjectCacheData& data)
    {
        ASSERT(!m_objects.contains(&data));

        WebCore::DOMWindow* domWindow = m_frame->document()->domWindow();
        if (domWindow && (!m_domWindowObserver || m_domWindowObserver->domWindow() != domWindow)) {
            // New DOMWindow, clear the cache and create a new DOMWindowObserver.
            clear();
            m_domWindowObserver = std::make_unique<DOMWindowObserver>(*m_frame, *this, domWindow);
        }

        m_objects.append(&data);
        g_object_weak_ref(data.object, DOMObjectCacheFrameObserver::objectFinalizedCallback, this);
    }

private:
    class DOMWindowObserver final: public WebCore::DOMWindowProperty {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        DOMWindowObserver(WebCore::Frame& frame, DOMObjectCacheFrameObserver& frameObserver, WebCore::DOMWindow* window)
            : DOMWindowProperty(&frame)
            , m_frameObserver(frameObserver)
            , m_domWindow(window)
        {
            ASSERT(m_domWindow);
        }

        virtual ~DOMWindowObserver()
        {
        }

        WebCore::DOMWindow* domWindow() const { return m_domWindow; }

    private:
        virtual void willDetachGlobalObjectFromFrame() override
        {
            // Clear the DOMWindowProperty first, and then notify the Frame observer.
            DOMWindowProperty::willDetachGlobalObjectFromFrame();
            m_frameObserver.willDetachGlobalObjectFromFrame();
        }

        DOMObjectCacheFrameObserver& m_frameObserver;
        WebCore::DOMWindow* m_domWindow;
    };

    static void objectFinalizedCallback(gpointer userData, GObject* finalizedObject)
    {
        DOMObjectCacheFrameObserver* observer = static_cast<DOMObjectCacheFrameObserver*>(userData);
        observer->m_objects.removeFirstMatching([finalizedObject](DOMObjectCacheData* data) {
            return data->object == finalizedObject;
        });
    }

    void clear()
    {
        if (m_objects.isEmpty())
            return;

        auto objects = WTF::move(m_objects);
        for (auto* data : objects) {
            g_object_weak_unref(data->object, DOMObjectCacheFrameObserver::objectFinalizedCallback, this);
            data->clearObject();
        }
    }

    virtual void willDetachPage() override
    {
        clear();
    }

    virtual void frameDestroyed() override
    {
        clear();
        WebCore::Frame* frame = m_frame;
        FrameDestructionObserver::frameDestroyed();
        domObjectCacheFrameObservers().remove(frame);
    }

    void willDetachGlobalObjectFromFrame()
    {
        clear();
        m_domWindowObserver = nullptr;
    }

    Vector<DOMObjectCacheData*, 8> m_objects;
    std::unique_ptr<DOMWindowObserver> m_domWindowObserver;
};

typedef HashMap<void*, std::unique_ptr<DOMObjectCacheData>> DOMObjectMap;

static DOMObjectMap& domObjects()
{
    static NeverDestroyed<DOMObjectMap> staticDOMObjects;
    return staticDOMObjects;
}

void DOMObjectCache::forget(void* objectHandle)
{
    ASSERT(domObjects().contains(objectHandle));
    domObjects().remove(objectHandle);
}

void* DOMObjectCache::get(void* objectHandle)
{
    DOMObjectCacheData* data = domObjects().get(objectHandle);
    return data ? data->refObject() : nullptr;
}

void DOMObjectCache::put(void* objectHandle, void* wrapper)
{
    DOMObjectMap::AddResult result = domObjects().add(objectHandle, nullptr);
    if (result.isNewEntry)
        result.iterator->value = std::make_unique<DOMObjectCacheData>(G_OBJECT(wrapper));
}

void DOMObjectCache::put(WebCore::Node* objectHandle, void* wrapper)
{
    DOMObjectMap::AddResult result = domObjects().add(objectHandle, nullptr);
    if (!result.isNewEntry)
        return;

    result.iterator->value = std::make_unique<DOMObjectCacheData>(G_OBJECT(wrapper));
    if (WebCore::Frame* frame = objectHandle->document().frame())
        getOrCreateDOMObjectCacheFrameObserver(*frame).addObjectCacheData(*result.iterator->value);
}

}
