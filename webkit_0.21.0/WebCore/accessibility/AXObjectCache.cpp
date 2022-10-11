/*
 * Copyright (C) 2008, 2009, 2010, 2015 Apple Inc. All rights reserved.
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

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"

#include "AccessibilityARIAGrid.h"
#include "AccessibilityARIAGridCell.h"
#include "AccessibilityARIAGridRow.h"
#include "AccessibilityImageMapLink.h"
#include "AccessibilityList.h"
#include "AccessibilityListBox.h"
#include "AccessibilityListBoxOption.h"
#include "AccessibilityMediaControls.h"
#include "AccessibilityMenuList.h"
#include "AccessibilityMenuListOption.h"
#include "AccessibilityMenuListPopup.h"
#include "AccessibilityProgressIndicator.h"
#include "AccessibilityRenderObject.h"
#include "AccessibilitySVGRoot.h"
#include "AccessibilityScrollView.h"
#include "AccessibilityScrollbar.h"
#include "AccessibilitySlider.h"
#include "AccessibilitySpinButton.h"
#include "AccessibilityTable.h"
#include "AccessibilityTableCell.h"
#include "AccessibilityTableColumn.h"
#include "AccessibilityTableHeaderContainer.h"
#include "AccessibilityTableRow.h"
#include "Document.h"
#include "Editor.h"
#include "ElementIterator.h"
#include "FocusController.h"
#include "Frame.h"
#include "HTMLAreaElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLLabelElement.h"
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RenderListBox.h"
#include "RenderMenuList.h"
#include "RenderMeter.h"
#include "RenderProgress.h"
#include "RenderSVGRoot.h"
#include "RenderSlider.h"
#include "RenderTable.h"
#include "RenderTableCell.h"
#include "RenderTableRow.h"
#include "RenderView.h"
#include "ScrollView.h"
#include <wtf/PassRefPtr.h>
#include <wtf/SetForScope.h>

#if ENABLE(VIDEO)
#include "MediaControlElements.h"
#endif

namespace WebCore {

using namespace HTMLNames;

// Post value change notifications for password fields or elements contained in password fields at a 40hz interval to thwart analysis of typing cadence
static double AccessibilityPasswordValueChangeNotificationInterval = 0.025;
static double AccessibilityLiveRegionChangedNotificationInterval = 0.020;

AccessibilityObjectInclusion AXComputedObjectAttributeCache::getIgnored(AXID id) const
{
    HashMap<AXID, CachedAXObjectAttributes>::const_iterator it = m_idMapping.find(id);
    return it != m_idMapping.end() ? it->value.ignored : DefaultBehavior;
}

void AXComputedObjectAttributeCache::setIgnored(AXID id, AccessibilityObjectInclusion inclusion)
{
    HashMap<AXID, CachedAXObjectAttributes>::iterator it = m_idMapping.find(id);
    if (it != m_idMapping.end())
        it->value.ignored = inclusion;
    else {
        CachedAXObjectAttributes attributes;
        attributes.ignored = inclusion;
        m_idMapping.set(id, attributes);
    }
}
    
bool AXObjectCache::gAccessibilityEnabled = false;
bool AXObjectCache::gAccessibilityEnhancedUserInterfaceEnabled = false;

void AXObjectCache::enableAccessibility()
{
    gAccessibilityEnabled = true;
}

void AXObjectCache::disableAccessibility()
{
    gAccessibilityEnabled = false;
}

void AXObjectCache::setEnhancedUserInterfaceAccessibility(bool flag)
{
    gAccessibilityEnhancedUserInterfaceEnabled = flag;
}

AXObjectCache::AXObjectCache(Document& document)
    : m_document(document)
    , m_notificationPostTimer(*this, &AXObjectCache::notificationPostTimerFired)
    , m_passwordNotificationPostTimer(*this, &AXObjectCache::passwordNotificationPostTimerFired)
    , m_liveRegionChangedPostTimer(*this, &AXObjectCache::liveRegionChangedNotificationPostTimerFired)
{
}

AXObjectCache::~AXObjectCache()
{
    m_notificationPostTimer.stop();
    m_liveRegionChangedPostTimer.stop();

    for (const auto& object : m_objects.values()) {
        detachWrapper(object.get(), CacheDestroyed);
        object->detach(CacheDestroyed);
        removeAXID(object.get());
    }
}

AccessibilityObject* AXObjectCache::focusedImageMapUIElement(HTMLAreaElement* areaElement)
{
    // Find the corresponding accessibility object for the HTMLAreaElement. This should be
    // in the list of children for its corresponding image.
    if (!areaElement)
        return nullptr;
    
    HTMLImageElement* imageElement = areaElement->imageElement();
    if (!imageElement)
        return nullptr;
    
    AccessibilityObject* axRenderImage = areaElement->document().axObjectCache()->getOrCreate(imageElement);
    if (!axRenderImage)
        return nullptr;
    
    for (const auto& child : axRenderImage->children()) {
        if (!is<AccessibilityImageMapLink>(*child))
            continue;
        
        if (downcast<AccessibilityImageMapLink>(*child).areaElement() == areaElement)
            return child.get();
    }    
    
    return nullptr;
}
    
AccessibilityObject* AXObjectCache::focusedUIElementForPage(const Page* page)
{
    if (!gAccessibilityEnabled)
        return nullptr;

    // get the focused node in the page
    Document* focusedDocument = page->focusController().focusedOrMainFrame().document();
    Element* focusedElement = focusedDocument->focusedElement();
    if (is<HTMLAreaElement>(focusedElement))
        return focusedImageMapUIElement(downcast<HTMLAreaElement>(focusedElement));

    AccessibilityObject* obj = focusedDocument->axObjectCache()->getOrCreate(focusedElement ? static_cast<Node*>(focusedElement) : focusedDocument);
    if (!obj)
        return nullptr;

    if (obj->shouldFocusActiveDescendant()) {
        if (AccessibilityObject* descendant = obj->activeDescendant())
            obj = descendant;
    }

    // the HTML element, for example, is focusable but has an AX object that is ignored
    if (obj->accessibilityIsIgnored())
        obj = obj->parentObjectUnignored();

    return obj;
}

AccessibilityObject* AXObjectCache::get(Widget* widget)
{
    if (!widget)
        return nullptr;
        
    AXID axID = m_widgetObjectMapping.get(widget);
    ASSERT(!HashTraits<AXID>::isDeletedValue(axID));
    if (!axID)
        return nullptr;
    
    return m_objects.get(axID);    
}
    
AccessibilityObject* AXObjectCache::get(RenderObject* renderer)
{
    if (!renderer)
        return nullptr;
    
    AXID axID = m_renderObjectMapping.get(renderer);
    ASSERT(!HashTraits<AXID>::isDeletedValue(axID));
    if (!axID)
        return nullptr;

    return m_objects.get(axID);    
}

AccessibilityObject* AXObjectCache::get(Node* node)
{
    if (!node)
        return nullptr;

    AXID renderID = node->renderer() ? m_renderObjectMapping.get(node->renderer()) : 0;
    ASSERT(!HashTraits<AXID>::isDeletedValue(renderID));

    AXID nodeID = m_nodeObjectMapping.get(node);
    ASSERT(!HashTraits<AXID>::isDeletedValue(nodeID));

    if (node->renderer() && nodeID && !renderID) {
        // This can happen if an AccessibilityNodeObject is created for a node that's not
        // rendered, but later something changes and it gets a renderer (like if it's
        // reparented).
        remove(nodeID);
        return nullptr;
    }

    if (renderID)
        return m_objects.get(renderID);

    if (!nodeID)
        return nullptr;

    return m_objects.get(nodeID);
}

// FIXME: This probably belongs on Node.
// FIXME: This should take a const char*, but one caller passes nullAtom.
bool nodeHasRole(Node* node, const String& role)
{
    if (!node || !is<Element>(node))
        return false;

    auto& roleValue = downcast<Element>(*node).fastGetAttribute(roleAttr);
    if (role.isNull())
        return roleValue.isEmpty();
    if (roleValue.isEmpty())
        return false;

    return SpaceSplitString(roleValue, true).contains(role);
}

static Ref<AccessibilityObject> createFromRenderer(RenderObject* renderer)
{
    // FIXME: How could renderer->node() ever not be an Element?
    Node* node = renderer->node();

    // If the node is aria role="list" or the aria role is empty and its a
    // ul/ol/dl type (it shouldn't be a list if aria says otherwise).
    if (node && ((nodeHasRole(node, "list") || nodeHasRole(node, "directory"))
                      || (nodeHasRole(node, nullAtom) && (node->hasTagName(ulTag) || node->hasTagName(olTag) || node->hasTagName(dlTag)))))
        return AccessibilityList::create(renderer);

    // aria tables
    if (nodeHasRole(node, "grid") || nodeHasRole(node, "treegrid"))
        return AccessibilityARIAGrid::create(renderer);
    if (nodeHasRole(node, "row"))
        return AccessibilityARIAGridRow::create(renderer);
    if (nodeHasRole(node, "gridcell") || nodeHasRole(node, "columnheader") || nodeHasRole(node, "rowheader"))
        return AccessibilityARIAGridCell::create(renderer);

#if ENABLE(VIDEO)
    // media controls
    if (node && node->isMediaControlElement())
        return AccessibilityMediaControl::create(renderer);
#endif

    if (is<RenderSVGRoot>(*renderer))
        return AccessibilitySVGRoot::create(renderer);
    
    if (is<RenderBoxModelObject>(*renderer)) {
        RenderBoxModelObject& cssBox = downcast<RenderBoxModelObject>(*renderer);
        if (is<RenderListBox>(cssBox))
            return AccessibilityListBox::create(&downcast<RenderListBox>(cssBox));
        if (is<RenderMenuList>(cssBox))
            return AccessibilityMenuList::create(&downcast<RenderMenuList>(cssBox));

        // standard tables
        if (is<RenderTable>(cssBox))
            return AccessibilityTable::create(&downcast<RenderTable>(cssBox));
        if (is<RenderTableRow>(cssBox))
            return AccessibilityTableRow::create(&downcast<RenderTableRow>(cssBox));
        if (is<RenderTableCell>(cssBox))
            return AccessibilityTableCell::create(&downcast<RenderTableCell>(cssBox));

        // progress bar
        if (is<RenderProgress>(cssBox))
            return AccessibilityProgressIndicator::create(&downcast<RenderProgress>(cssBox));

#if ENABLE(METER_ELEMENT)
        if (is<RenderMeter>(cssBox))
            return AccessibilityProgressIndicator::create(&downcast<RenderMeter>(cssBox));
#endif

        // input type=range
        if (is<RenderSlider>(cssBox))
            return AccessibilitySlider::create(&downcast<RenderSlider>(cssBox));
    }

    return AccessibilityRenderObject::create(renderer);
}

static Ref<AccessibilityObject> createFromNode(Node* node)
{
    return AccessibilityNodeObject::create(node);
}

AccessibilityObject* AXObjectCache::getOrCreate(Widget* widget)
{
    if (!widget)
        return nullptr;

    if (AccessibilityObject* obj = get(widget))
        return obj;
    
    RefPtr<AccessibilityObject> newObj;
    if (is<ScrollView>(*widget))
        newObj = AccessibilityScrollView::create(downcast<ScrollView>(widget));
    else if (is<Scrollbar>(*widget))
        newObj = AccessibilityScrollbar::create(downcast<Scrollbar>(widget));

    // Will crash later if we have two objects for the same widget.
    ASSERT(!get(widget));

    // Catch the case if an (unsupported) widget type is used. Only FrameView and ScrollBar are supported now.
    ASSERT(newObj);
    if (!newObj)
        return nullptr;

    getAXID(newObj.get());
    
    m_widgetObjectMapping.set(widget, newObj->axObjectID());
    m_objects.set(newObj->axObjectID(), newObj);    
    newObj->init();
    attachWrapper(newObj.get());
    return newObj.get();
}

AccessibilityObject* AXObjectCache::getOrCreate(Node* node)
{
    if (!node)
        return nullptr;

    if (AccessibilityObject* obj = get(node))
        return obj;

    if (node->renderer())
        return getOrCreate(node->renderer());

    if (!node->parentElement())
        return nullptr;
    
    // It's only allowed to create an AccessibilityObject from a Node if it's in a canvas subtree.
    // Or if it's a hidden element, but we still want to expose it because of other ARIA attributes.
    bool inCanvasSubtree = lineageOfType<HTMLCanvasElement>(*node->parentElement()).first();
    bool isHidden = isNodeAriaVisible(node);

    bool insideMeterElement = false;
#if ENABLE(METER_ELEMENT)
    insideMeterElement = is<HTMLMeterElement>(*node->parentElement());
#endif
    
    if (!inCanvasSubtree && !isHidden && !insideMeterElement)
        return nullptr;

    // Fallback content is only focusable as long as the canvas is displayed and visible.
    // Update the style before Element::isFocusable() gets called.
    if (inCanvasSubtree)
        node->document().updateStyleIfNeeded();

    RefPtr<AccessibilityObject> newObj = createFromNode(node);

    // Will crash later if we have two objects for the same node.
    ASSERT(!get(node));

    getAXID(newObj.get());

    m_nodeObjectMapping.set(node, newObj->axObjectID());
    m_objects.set(newObj->axObjectID(), newObj);
    newObj->init();
    attachWrapper(newObj.get());
    newObj->setLastKnownIsIgnoredValue(newObj->accessibilityIsIgnored());
    // Sometimes asking accessibilityIsIgnored() will cause the newObject to be deallocated, and then
    // it will disappear when this function is finished, leading to a use-after-free.
    if (newObj->isDetached())
        return nullptr;
    
    return newObj.get();
}

AccessibilityObject* AXObjectCache::getOrCreate(RenderObject* renderer)
{
    if (!renderer)
        return nullptr;

    if (AccessibilityObject* obj = get(renderer))
        return obj;

    RefPtr<AccessibilityObject> newObj = createFromRenderer(renderer);

    // Will crash later if we have two objects for the same renderer.
    ASSERT(!get(renderer));

    getAXID(newObj.get());

    m_renderObjectMapping.set(renderer, newObj->axObjectID());
    m_objects.set(newObj->axObjectID(), newObj);
    newObj->init();
    attachWrapper(newObj.get());
    newObj->setLastKnownIsIgnoredValue(newObj->accessibilityIsIgnored());
    // Sometimes asking accessibilityIsIgnored() will cause the newObject to be deallocated, and then
    // it will disappear when this function is finished, leading to a use-after-free.
    if (newObj->isDetached())
        return nullptr;
    
    return newObj.get();
}
    
AccessibilityObject* AXObjectCache::rootObject()
{
    if (!gAccessibilityEnabled)
        return nullptr;

    return getOrCreate(m_document.view());
}

AccessibilityObject* AXObjectCache::rootObjectForFrame(Frame* frame)
{
    if (!gAccessibilityEnabled)
        return nullptr;

    if (!frame)
        return nullptr;
    return getOrCreate(frame->view());
}    
    
AccessibilityObject* AXObjectCache::getOrCreate(AccessibilityRole role)
{
    RefPtr<AccessibilityObject> obj = nullptr;
    
    // will be filled in...
    switch (role) {
    case ListBoxOptionRole:
        obj = AccessibilityListBoxOption::create();
        break;
    case ImageMapLinkRole:
        obj = AccessibilityImageMapLink::create();
        break;
    case ColumnRole:
        obj = AccessibilityTableColumn::create();
        break;            
    case TableHeaderContainerRole:
        obj = AccessibilityTableHeaderContainer::create();
        break;   
    case SliderThumbRole:
        obj = AccessibilitySliderThumb::create();
        break;
    case MenuListPopupRole:
        obj = AccessibilityMenuListPopup::create();
        break;
    case MenuListOptionRole:
        obj = AccessibilityMenuListOption::create();
        break;
    case SpinButtonRole:
        obj = AccessibilitySpinButton::create();
        break;
    case SpinButtonPartRole:
        obj = AccessibilitySpinButtonPart::create();
        break;
    default:
        obj = nullptr;
    }
    
    if (obj)
        getAXID(obj.get());
    else
        return nullptr;

    m_objects.set(obj->axObjectID(), obj);    
    obj->init();
    attachWrapper(obj.get());
    return obj.get();
}

void AXObjectCache::remove(AXID axID)
{
    if (!axID)
        return;
    
    // first fetch object to operate some cleanup functions on it 
    AccessibilityObject* obj = m_objects.get(axID);
    if (!obj)
        return;
    
    detachWrapper(obj, ElementDestroyed);
    obj->detach(ElementDestroyed, this);
    removeAXID(obj);
    
    // finally remove the object
    if (!m_objects.take(axID))
        return;
    
    ASSERT(m_objects.size() >= m_idsInUse.size());    
}
    
void AXObjectCache::remove(RenderObject* renderer)
{
    if (!renderer)
        return;
    
    AXID axID = m_renderObjectMapping.get(renderer);
    remove(axID);
    m_renderObjectMapping.remove(renderer);
}

void AXObjectCache::remove(Node* node)
{
    if (!node)
        return;

    if (is<Element>(*node))
        m_deferredRecomputeIsIgnoredList.remove(downcast<Element>(node));
    m_deferredTextChangedList.remove(node);
    removeNodeForUse(node);

    // This is all safe even if we didn't have a mapping.
    AXID axID = m_nodeObjectMapping.get(node);
    remove(axID);
    m_nodeObjectMapping.remove(node);

    if (node->renderer()) {
        remove(node->renderer());
        return;
    }
}

void AXObjectCache::remove(Widget* view)
{
    if (!view)
        return;
        
    AXID axID = m_widgetObjectMapping.get(view);
    remove(axID);
    m_widgetObjectMapping.remove(view);
}
    
    
#if !PLATFORM(WIN)
AXID AXObjectCache::platformGenerateAXID() const
{
    static AXID lastUsedID = 0;

    // Generate a new ID.
    AXID objID = lastUsedID;
    do {
        ++objID;
    } while (!objID || HashTraits<AXID>::isDeletedValue(objID) || m_idsInUse.contains(objID));

    lastUsedID = objID;

    return objID;
}
#endif

AXID AXObjectCache::getAXID(AccessibilityObject* obj)
{
    // check for already-assigned ID
    AXID objID = obj->axObjectID();
    if (objID) {
        ASSERT(m_idsInUse.contains(objID));
        return objID;
    }

    objID = platformGenerateAXID();

    m_idsInUse.add(objID);
    obj->setAXObjectID(objID);
    
    return objID;
}

void AXObjectCache::removeAXID(AccessibilityObject* object)
{
    if (!object)
        return;
    
    AXID objID = object->axObjectID();
    if (!objID)
        return;
    ASSERT(!HashTraits<AXID>::isDeletedValue(objID));
    ASSERT(m_idsInUse.contains(objID));
    object->setAXObjectID(0);
    m_idsInUse.remove(objID);
}

void AXObjectCache::textChanged(Node* node)
{
    textChanged(getOrCreate(node));
}

void AXObjectCache::textChanged(RenderObject* renderer)
{
    textChanged(getOrCreate(renderer));
}

void AXObjectCache::textChanged(AccessibilityObject* obj)
{
    if (!obj)
        return;

    bool parentAlreadyExists = obj->parentObjectIfExists();
    obj->textChanged();
    postNotification(obj, obj->document(), AXObjectCache::AXTextChanged);
    if (parentAlreadyExists)
        obj->notifyIfIgnoredValueChanged();
}

void AXObjectCache::updateCacheAfterNodeIsAttached(Node* node)
{
    // Calling get() will update the AX object if we had an AccessibilityNodeObject but now we need
    // an AccessibilityRenderObject, because it was reparented to a location outside of a canvas.
    get(node);
}

void AXObjectCache::handleMenuOpened(Node* node)
{
    if (!node || !node->renderer() || !nodeHasRole(node, "menu"))
        return;
    
    postNotification(getOrCreate(node), &document(), AXMenuOpened);
}
    
void AXObjectCache::handleLiveRegionCreated(Node* node)
{
    if (!is<Element>(node) || !node->renderer())
        return;
    
    Element* element = downcast<Element>(node);
    String liveRegionStatus = element->fastGetAttribute(aria_liveAttr);
    if (liveRegionStatus.isEmpty()) {
        const AtomicString& ariaRole = element->fastGetAttribute(roleAttr);
        if (!ariaRole.isEmpty())
            liveRegionStatus = AccessibilityObject::defaultLiveRegionStatusForRole(AccessibilityObject::ariaRoleToWebCoreRole(ariaRole));
    }
    
    if (AccessibilityObject::liveRegionStatusIsEnabled(liveRegionStatus))
        postNotification(getOrCreate(node), &document(), AXLiveRegionCreated);
}
    
void AXObjectCache::childrenChanged(Node* node, Node* newChild)
{
    if (newChild) {
        handleMenuOpened(newChild);
        handleLiveRegionCreated(newChild);
    }
    
    childrenChanged(get(node));
}

void AXObjectCache::childrenChanged(RenderObject* renderer, RenderObject* newChild)
{
    if (!renderer)
        return;
    
    if (newChild) {
        handleMenuOpened(newChild->node());
        handleLiveRegionCreated(newChild->node());
    }
    
    childrenChanged(get(renderer));
}

void AXObjectCache::childrenChanged(AccessibilityObject* obj)
{
    if (!obj)
        return;

    obj->childrenChanged();
}
    
void AXObjectCache::notificationPostTimerFired()
{
    Ref<Document> protectorForCacheOwner(m_document);
    m_notificationPostTimer.stop();
    
    // In tests, posting notifications has a tendency to immediately queue up other notifications, which can lead to unexpected behavior
    // when the notification list is cleared at the end. Instead copy this list at the start.
    auto notifications = WTF::move(m_notificationsToPost);
    
    for (const auto& note : notifications) {
        AccessibilityObject* obj = note.first.get();
        if (!obj->axObjectID())
            continue;

        if (!obj->axObjectCache())
            continue;
        
#ifndef NDEBUG
        // Make sure none of the render views are in the process of being layed out.
        // Notifications should only be sent after the renderer has finished
        if (is<AccessibilityRenderObject>(*obj)) {
            if (auto* renderer = downcast<AccessibilityRenderObject>(*obj).renderer())
                ASSERT(!renderer->view().layoutState());
        }
#endif

        AXNotification notification = note.second;
        
        // Ensure that this menu really is a menu. We do this check here so that we don't have to create
        // the axChildren when the menu is marked as opening.
        if (notification == AXMenuOpened) {
            obj->updateChildrenIfNecessary();
            if (obj->roleValue() != MenuRole)
                continue;
        }
        
        postPlatformNotification(obj, notification);

        if (notification == AXChildrenChanged && obj->parentObjectIfExists() && obj->lastKnownIsIgnoredValue() != obj->accessibilityIsIgnored())
            childrenChanged(obj->parentObject());
    }
}

void AXObjectCache::passwordNotificationPostTimerFired()
{
#if PLATFORM(COCOA)
    m_passwordNotificationPostTimer.stop();

    // In tests, posting notifications has a tendency to immediately queue up other notifications, which can lead to unexpected behavior
    // when the notification list is cleared at the end. Instead copy this list at the start.
    auto notifications = WTF::move(m_passwordNotificationsToPost);

    for (auto& notification : notifications)
        postTextStateChangePlatformNotification(notification.get(), AXTextEditTypeInsert, " ", VisiblePosition());
#endif
}
    
void AXObjectCache::postNotification(RenderObject* renderer, AXNotification notification, PostTarget postTarget, PostType postType)
{
    if (!renderer)
        return;
    
    stopCachingComputedObjectAttributes();

    // Get an accessibility object that already exists. One should not be created here
    // because a render update may be in progress and creating an AX object can re-trigger a layout
    RefPtr<AccessibilityObject> object = get(renderer);
    while (!object && renderer) {
        renderer = renderer->parent();
        object = get(renderer); 
    }
    
    if (!renderer)
        return;
    
    postNotification(object.get(), &renderer->document(), notification, postTarget, postType);
}

void AXObjectCache::postNotification(Node* node, AXNotification notification, PostTarget postTarget, PostType postType)
{
    if (!node)
        return;
    
    stopCachingComputedObjectAttributes();

    // Get an accessibility object that already exists. One should not be created here
    // because a render update may be in progress and creating an AX object can re-trigger a layout
    RefPtr<AccessibilityObject> object = get(node);
    while (!object && node) {
        node = node->parentNode();
        object = get(node);
    }
    
    if (!node)
        return;
    
    postNotification(object.get(), &node->document(), notification, postTarget, postType);
}

void AXObjectCache::postNotification(AccessibilityObject* object, Document* document, AXNotification notification, PostTarget postTarget, PostType postType)
{
    stopCachingComputedObjectAttributes();

    if (object && postTarget == TargetObservableParent)
        object = object->observableObject();

    if (!object && document)
        object = get(document->renderView());

    if (!object)
        return;

    if (postType == PostAsynchronously) {
        m_notificationsToPost.append(std::make_pair(object, notification));
        if (!m_notificationPostTimer.isActive())
            m_notificationPostTimer.startOneShot(0);
    } else
        postPlatformNotification(object, notification);
}

void AXObjectCache::checkedStateChanged(Node* node)
{
    postNotification(node, AXObjectCache::AXCheckedStateChanged);
}

void AXObjectCache::handleMenuItemSelected(Node* node)
{
    if (!node)
        return;
    
    if (!nodeHasRole(node, "menuitem") && !nodeHasRole(node, "menuitemradio") && !nodeHasRole(node, "menuitemcheckbox"))
        return;
    
    if (!downcast<Element>(*node).focused() && !equalIgnoringCase(downcast<Element>(*node).fastGetAttribute(aria_selectedAttr), "true"))
        return;
    
    postNotification(getOrCreate(node), &document(), AXMenuListItemSelected);
}
    
void AXObjectCache::handleFocusedUIElementChanged(Node* oldNode, Node* newNode)
{
    handleMenuItemSelected(newNode);
    platformHandleFocusedUIElementChanged(oldNode, newNode);
}
    
void AXObjectCache::selectedChildrenChanged(Node* node)
{
    handleMenuItemSelected(node);
    
    // postTarget is TargetObservableParent so that you can pass in any child of an element and it will go up the parent tree
    // to find the container which should send out the notification.
    postNotification(node, AXSelectedChildrenChanged, TargetObservableParent);
}

void AXObjectCache::selectedChildrenChanged(RenderObject* renderer)
{
    if (renderer)
        handleMenuItemSelected(renderer->node());

    // postTarget is TargetObservableParent so that you can pass in any child of an element and it will go up the parent tree
    // to find the container which should send out the notification.
    postNotification(renderer, AXSelectedChildrenChanged, TargetObservableParent);
}

#ifndef NDEBUG
void AXObjectCache::showIntent(const AXTextStateChangeIntent &intent)
{
    switch (intent.type) {
    case AXTextStateChangeTypeUnknown:
        dataLog("Unknown");
        break;
    case AXTextStateChangeTypeEdit:
        dataLog("Edit::");
        break;
    case AXTextStateChangeTypeSelectionMove:
        dataLog("Move::");
        break;
    case AXTextStateChangeTypeSelectionExtend:
        dataLog("Extend::");
        break;
    }
    switch (intent.type) {
    case AXTextStateChangeTypeUnknown:
        break;
    case AXTextStateChangeTypeEdit:
        switch (intent.change) {
        case AXTextEditTypeUnknown:
            dataLog("Unknown");
            break;
        case AXTextEditTypeDelete:
            dataLog("Delete");
            break;
        case AXTextEditTypeInsert:
            dataLog("Insert");
            break;
        case AXTextEditTypeDictation:
            dataLog("DictationInsert");
            break;
        case AXTextEditTypeTyping:
            dataLog("TypingInsert");
            break;
        case AXTextEditTypeCut:
            dataLog("Cut");
            break;
        case AXTextEditTypePaste:
            dataLog("Paste");
            break;
        case AXTextEditTypeAttributesChange:
            dataLog("AttributesChange");
            break;
        }
        break;
    case AXTextStateChangeTypeSelectionMove:
    case AXTextStateChangeTypeSelectionExtend:
        switch (intent.selection.direction) {
        case AXTextSelectionDirectionUnknown:
            dataLog("Unknown::");
            break;
        case AXTextSelectionDirectionBeginning:
            dataLog("Beginning::");
            break;
        case AXTextSelectionDirectionEnd:
            dataLog("End::");
            break;
        case AXTextSelectionDirectionPrevious:
            dataLog("Previous::");
            break;
        case AXTextSelectionDirectionNext:
            dataLog("Next::");
            break;
        case AXTextSelectionDirectionDiscontiguous:
            dataLog("Discontiguous::");
            break;
        }
        switch (intent.selection.direction) {
        case AXTextSelectionDirectionUnknown:
        case AXTextSelectionDirectionBeginning:
        case AXTextSelectionDirectionEnd:
        case AXTextSelectionDirectionPrevious:
        case AXTextSelectionDirectionNext:
            switch (intent.selection.granularity) {
            case AXTextSelectionGranularityUnknown:
                dataLog("Unknown");
                break;
            case AXTextSelectionGranularityCharacter:
                dataLog("Character");
                break;
            case AXTextSelectionGranularityWord:
                dataLog("Word");
                break;
            case AXTextSelectionGranularityLine:
                dataLog("Line");
                break;
            case AXTextSelectionGranularitySentence:
                dataLog("Sentence");
                break;
            case AXTextSelectionGranularityParagraph:
                dataLog("Paragraph");
                break;
            case AXTextSelectionGranularityPage:
                dataLog("Page");
                break;
            case AXTextSelectionGranularityDocument:
                dataLog("Document");
                break;
            case AXTextSelectionGranularityAll:
                dataLog("All");
                break;
            }
            break;
        case AXTextSelectionDirectionDiscontiguous:
            break;
        }
        break;
    }
    dataLog("\n");
}
#endif

void AXObjectCache::setTextSelectionIntent(const AXTextStateChangeIntent& intent)
{
    m_textSelectionIntent = intent;
}
    
void AXObjectCache::setIsSynchronizingSelection(bool isSynchronizing)
{
    m_isSynchronizingSelection = isSynchronizing;
}

static bool isPasswordFieldOrContainedByPasswordField(AccessibilityObject* object)
{
    return object && (object->isPasswordField() || object->isContainedByPasswordField());
}

void AXObjectCache::postTextStateChangeNotification(Node* node, const AXTextStateChangeIntent& intent, const VisibleSelection& selection)
{
    if (!node)
        return;

#if PLATFORM(COCOA)
    stopCachingComputedObjectAttributes();

    postTextStateChangeNotification(getOrCreate(node), intent, selection);
#else
    postNotification(node->renderer(), AXObjectCache::AXSelectedTextChanged, TargetObservableParent);
    UNUSED_PARAM(intent);
    UNUSED_PARAM(selection);
#endif
}

void AXObjectCache::postTextStateChangeNotification(const Position& position, const AXTextStateChangeIntent& intent, const VisibleSelection& selection)
{
    Node* node = position.deprecatedNode();
    if (!node)
        return;

    stopCachingComputedObjectAttributes();

#if PLATFORM(COCOA)
    AccessibilityObject* object = getOrCreate(node);
    if (object && object->accessibilityIsIgnored()) {
        if (position.atLastEditingPositionForNode()) {
            if (AccessibilityObject* nextSibling = object->nextSiblingUnignored(1))
                object = nextSibling;
        } else if (position.atFirstEditingPositionForNode()) {
            if (AccessibilityObject* previousSibling = object->previousSiblingUnignored(1))
                object = previousSibling;
        }
    }

    postTextStateChangeNotification(object, intent, selection);
#else
    postTextStateChangeNotification(node, intent, selection);
#endif
}

void AXObjectCache::postTextStateChangeNotification(AccessibilityObject* object, const AXTextStateChangeIntent& intent, const VisibleSelection& selection)
{
    stopCachingComputedObjectAttributes();

#if PLATFORM(COCOA)
    if (object) {
        if (isPasswordFieldOrContainedByPasswordField(object))
            return;

        if (auto observableObject = object->observableObject())
            object = observableObject;
    }

    const AXTextStateChangeIntent& newIntent = (intent.type == AXTextStateChangeTypeUnknown || (m_isSynchronizingSelection && m_textSelectionIntent.type != AXTextStateChangeTypeUnknown)) ? m_textSelectionIntent : intent;
    postTextStateChangePlatformNotification(object, newIntent, selection);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(intent);
    UNUSED_PARAM(selection);
#endif

    setTextSelectionIntent(AXTextStateChangeIntent());
    setIsSynchronizingSelection(false);
}

void AXObjectCache::postTextStateChangeNotification(Node* node, AXTextEditType type, const String& text, const VisiblePosition& position)
{
    if (!node)
        return;
    ASSERT(type != AXTextEditTypeUnknown);

    stopCachingComputedObjectAttributes();

    AccessibilityObject* object = getOrCreate(node);
#if PLATFORM(COCOA)
    if (object) {
        if (enqueuePasswordValueChangeNotification(object))
            return;
        object = object->observableObject();
    }

    postTextStateChangePlatformNotification(object, type, text, position);
#else
    nodeTextChangePlatformNotification(object, textChangeForEditType(type), position.deepEquivalent().deprecatedEditingOffset(), text);
#endif
}

void AXObjectCache::postTextReplacementNotification(Node* node, AXTextEditType deletionType, const String& deletedText, AXTextEditType insertionType, const String& insertedText, const VisiblePosition& position)
{
    if (!node)
        return;
    ASSERT(deletionType == AXTextEditTypeDelete);
    ASSERT(insertionType == AXTextEditTypeInsert || insertionType == AXTextEditTypeTyping || insertionType == AXTextEditTypeDictation || insertionType == AXTextEditTypePaste);

    stopCachingComputedObjectAttributes();

    AccessibilityObject* object = getOrCreate(node);
#if PLATFORM(COCOA)
    if (object) {
        if (enqueuePasswordValueChangeNotification(object))
            return;
        object = object->observableObject();
    }

    postTextReplacementPlatformNotification(object, deletionType, deletedText, insertionType, insertedText, position);
#else
    nodeTextChangePlatformNotification(object, textChangeForEditType(deletionType), position.deepEquivalent().deprecatedEditingOffset(), deletedText);
    nodeTextChangePlatformNotification(object, textChangeForEditType(insertionType), position.deepEquivalent().deprecatedEditingOffset(), insertedText);
#endif
}

bool AXObjectCache::enqueuePasswordValueChangeNotification(AccessibilityObject* object)
{
    if (!isPasswordFieldOrContainedByPasswordField(object))
        return false;

    AccessibilityObject* observableObject = object->observableObject();
    if (!observableObject) {
        ASSERT_NOT_REACHED();
        // return true even though the enqueue didn't happen because this is a password field and caller shouldn't post a notification
        return true;
    }

    m_passwordNotificationsToPost.add(observableObject);
    if (!m_passwordNotificationPostTimer.isActive())
        m_passwordNotificationPostTimer.startOneShot(AccessibilityPasswordValueChangeNotificationInterval);

    return true;
}

void AXObjectCache::frameLoadingEventNotification(Frame* frame, AXLoadingEvent loadingEvent)
{
    if (!frame)
        return;

    // Delegate on the right platform
    RenderView* contentRenderer = frame->contentRenderer();
    if (!contentRenderer)
        return;

    AccessibilityObject* obj = getOrCreate(contentRenderer);
    frameLoadingEventPlatformNotification(obj, loadingEvent);
}

void AXObjectCache::postLiveRegionChangeNotification(AccessibilityObject* object)
{
    if (m_liveRegionChangedPostTimer.isActive())
        m_liveRegionChangedPostTimer.stop();

    if (!m_liveRegionObjectsSet.contains(object))
        m_liveRegionObjectsSet.add(object);

    m_liveRegionChangedPostTimer.startOneShot(AccessibilityLiveRegionChangedNotificationInterval);
}

void AXObjectCache::liveRegionChangedNotificationPostTimerFired()
{
    m_liveRegionChangedPostTimer.stop();

    if (m_liveRegionObjectsSet.isEmpty())
        return;

    for (auto& object : m_liveRegionObjectsSet)
        postNotification(object.get(), object->document(), AXObjectCache::AXLiveRegionChanged);
    m_liveRegionObjectsSet.clear();
}

void AXObjectCache::handleScrollbarUpdate(ScrollView* view)
{
    if (!view)
        return;
    
    // We don't want to create a scroll view from this method, only update an existing one.
    if (AccessibilityObject* scrollViewObject = get(view)) {
        stopCachingComputedObjectAttributes();
        scrollViewObject->updateChildrenIfNecessary();
    }
}
    
void AXObjectCache::handleAriaExpandedChange(Node* node)
{
    if (AccessibilityObject* obj = getOrCreate(node))
        obj->handleAriaExpandedChanged();
}
    
void AXObjectCache::handleActiveDescendantChanged(Node* node)
{
    if (AccessibilityObject* obj = getOrCreate(node))
        obj->handleActiveDescendantChanged();
}

void AXObjectCache::handleAriaRoleChanged(Node* node)
{
    stopCachingComputedObjectAttributes();

    if (AccessibilityObject* obj = getOrCreate(node)) {
        obj->updateAccessibilityRole();
        obj->notifyIfIgnoredValueChanged();
    }
}

void AXObjectCache::handleAttributeChanged(const QualifiedName& attrName, Element* element)
{
    if (attrName == roleAttr)
        handleAriaRoleChanged(element);
    else if (attrName == altAttr || attrName == titleAttr)
        deferTextChangedIfNeeded(element);
    else if (attrName == forAttr && is<HTMLLabelElement>(*element))
        labelChanged(element);

    if (!attrName.localName().string().startsWith("aria-"))
        return;

    if (attrName == aria_activedescendantAttr)
        handleActiveDescendantChanged(element);
    else if (attrName == aria_busyAttr)
        postNotification(element, AXObjectCache::AXElementBusyChanged);
    else if (attrName == aria_valuenowAttr || attrName == aria_valuetextAttr)
        postNotification(element, AXObjectCache::AXValueChanged);
    else if (attrName == aria_labelAttr || attrName == aria_labeledbyAttr || attrName == aria_labelledbyAttr)
        deferTextChangedIfNeeded(element);
    else if (attrName == aria_checkedAttr)
        checkedStateChanged(element);
    else if (attrName == aria_selectedAttr)
        selectedChildrenChanged(element);
    else if (attrName == aria_expandedAttr)
        handleAriaExpandedChange(element);
    else if (attrName == aria_hiddenAttr)
        childrenChanged(element->parentNode(), element);
    else if (attrName == aria_invalidAttr)
        postNotification(element, AXObjectCache::AXInvalidStatusChanged);
    else
        postNotification(element, AXObjectCache::AXAriaAttributeChanged);
}

void AXObjectCache::labelChanged(Element* element)
{
    ASSERT(is<HTMLLabelElement>(*element));
    HTMLElement* correspondingControl = downcast<HTMLLabelElement>(*element).control();
    deferTextChangedIfNeeded(correspondingControl);
}

void AXObjectCache::recomputeIsIgnored(RenderObject* renderer)
{
    if (AccessibilityObject* obj = get(renderer))
        obj->notifyIfIgnoredValueChanged();
}

void AXObjectCache::startCachingComputedObjectAttributesUntilTreeMutates()
{
    if (!m_computedObjectAttributeCache)
        m_computedObjectAttributeCache = std::make_unique<AXComputedObjectAttributeCache>();
}

void AXObjectCache::stopCachingComputedObjectAttributes()
{
    m_computedObjectAttributeCache = nullptr;
}

VisiblePosition AXObjectCache::visiblePositionForTextMarkerData(TextMarkerData& textMarkerData)
{
    if (!isNodeInUse(textMarkerData.node))
        return VisiblePosition();
    
    // FIXME: Accessability should make it clear these are DOM-compliant offsets or store Position objects.
    VisiblePosition visiblePos = VisiblePosition(createLegacyEditingPosition(textMarkerData.node, textMarkerData.offset), textMarkerData.affinity);
    Position deepPos = visiblePos.deepEquivalent();
    if (deepPos.isNull())
        return VisiblePosition();
    
    RenderObject* renderer = deepPos.deprecatedNode()->renderer();
    if (!renderer)
        return VisiblePosition();
    
    AXObjectCache* cache = renderer->document().axObjectCache();
    if (!cache->isIDinUse(textMarkerData.axID))
        return VisiblePosition();
    
    if (deepPos.deprecatedNode() != textMarkerData.node || deepPos.deprecatedEditingOffset() != textMarkerData.offset)
        return VisiblePosition();
    
    return visiblePos;
}

void AXObjectCache::textMarkerDataForVisiblePosition(TextMarkerData& textMarkerData, const VisiblePosition& visiblePos)
{
    // This memory must be bzero'd so instances of TextMarkerData can be tested for byte-equivalence.
    // This also allows callers to check for failure by looking at textMarkerData upon return.
    memset(&textMarkerData, 0, sizeof(TextMarkerData));
    
    if (visiblePos.isNull())
        return;
    
    Position deepPos = visiblePos.deepEquivalent();
    Node* domNode = deepPos.deprecatedNode();
    ASSERT(domNode);
    if (!domNode)
        return;
    
    if (domNode->isHTMLElement()) {
        HTMLInputElement* inputElement = domNode->toInputElement();
        if (inputElement && inputElement->isPasswordField())
            return;
    }
    
    // find or create an accessibility object for this node
    AXObjectCache* cache = domNode->document().axObjectCache();
    RefPtr<AccessibilityObject> obj = cache->getOrCreate(domNode);
    
    textMarkerData.axID = obj.get()->axObjectID();
    textMarkerData.node = domNode;
    textMarkerData.offset = deepPos.deprecatedEditingOffset();
    textMarkerData.affinity = visiblePos.affinity();   
    
    cache->setNodeInUse(domNode);
}

const Element* AXObjectCache::rootAXEditableElement(const Node* node)
{
    const Element* result = node->rootEditableElement();
    const Element* element = is<Element>(*node) ? downcast<Element>(node) : node->parentElement();

    for (; element; element = element->parentElement()) {
        if (nodeIsTextControl(element))
            result = element;
    }

    return result;
}

void AXObjectCache::clearTextMarkerNodesInUse(Document* document)
{
    if (!document)
        return;
    
    // Check each node to see if it's inside the document being deleted, of if it no longer belongs to a document.
    HashSet<Node*> nodesToDelete;
    for (const auto& node : m_textMarkerNodes) {
        if (!node->inDocument() || &(node)->document() == document)
            nodesToDelete.add(node);
    }
    
    for (const auto& node : nodesToDelete)
        m_textMarkerNodes.remove(node);
}
    
bool AXObjectCache::nodeIsTextControl(const Node* node)
{
    if (!node)
        return false;

    const AccessibilityObject* axObject = getOrCreate(const_cast<Node*>(node));
    return axObject && axObject->isTextControl();
}
    
void AXObjectCache::performDeferredCacheUpdate()
{
    if (m_performingDeferredCacheUpdate)
        return;

    SetForScope<bool> performingDeferredCacheUpdate(m_performingDeferredCacheUpdate, true);
    for (auto* node : m_deferredTextChangedList)
        textChanged(node);
    m_deferredTextChangedList.clear();

    for (auto* element : m_deferredRecomputeIsIgnoredList) {
        if (auto* renderer = element->renderer())
            recomputeIsIgnored(renderer);
    }
    m_deferredRecomputeIsIgnoredList.clear();
}

void AXObjectCache::deferRecomputeIsIgnored(Element* element)
{
    if (!element)
        return;

    if (element->renderer() && element->renderer()->beingDestroyed())
        return;

    m_deferredRecomputeIsIgnoredList.add(element);
}

void AXObjectCache::deferTextChangedIfNeeded(Node* node)
{
    if (!node)
        return;

    if (node->renderer() && node->renderer()->beingDestroyed())
        return;

    auto& document = node->document();
    // FIXME: We should just defer all text changes.
    if (document.needsStyleRecalc() || document.inRenderTreeUpdate() || (document.view() && document.view()->isInRenderTreeLayout())) {
        m_deferredTextChangedList.add(node);
        return;
    }
    textChanged(node);
}

bool isNodeAriaVisible(Node* node)
{
    if (!node)
        return false;

    // ARIA Node visibility is controlled by aria-hidden
    //  1) if aria-hidden=true, the whole subtree is hidden
    //  2) if aria-hidden=false, and the object is rendered, there's no effect
    //  3) if aria-hidden=false, and the object is NOT rendered, then it must have
    //     aria-hidden=false on each parent until it gets to a rendered object
    //  3b) a text node inherits a parents aria-hidden value
    bool requiresAriaHiddenFalse = !node->renderer();
    bool ariaHiddenFalsePresent = false;
    for (Node* testNode = node; testNode; testNode = testNode->parentNode()) {
        if (is<Element>(*testNode)) {
            const AtomicString& ariaHiddenValue = downcast<Element>(*testNode).fastGetAttribute(aria_hiddenAttr);
            if (equalIgnoringCase(ariaHiddenValue, "true"))
                return false;
            
            bool ariaHiddenFalse = equalIgnoringCase(ariaHiddenValue, "false");
            if (!testNode->renderer() && !ariaHiddenFalse)
                return false;
            if (!ariaHiddenFalsePresent && ariaHiddenFalse)
                ariaHiddenFalsePresent = true;
        }
    }
    
    return !requiresAriaHiddenFalse || ariaHiddenFalsePresent;
}

AccessibilityObject* AXObjectCache::rootWebArea()
{
    AccessibilityObject* rootObject = this->rootObject();
    if (!rootObject || !rootObject->isAccessibilityScrollView())
        return nullptr;
    return downcast<AccessibilityScrollView>(*rootObject).webAreaObject();
}

AXAttributeCacheEnabler::AXAttributeCacheEnabler(AXObjectCache* cache)
    : m_cache(cache)
{
    if (m_cache)
        m_cache->startCachingComputedObjectAttributesUntilTreeMutates();
}
    
AXAttributeCacheEnabler::~AXAttributeCacheEnabler()
{
    if (m_cache)
        m_cache->stopCachingComputedObjectAttributes();
}

#if !PLATFORM(COCOA)
AXTextChange AXObjectCache::textChangeForEditType(AXTextEditType type)
{
    switch (type) {
    case AXTextEditTypeCut:
    case AXTextEditTypeDelete:
        return AXTextDeleted;
    case AXTextEditTypeInsert:
    case AXTextEditTypeDictation:
    case AXTextEditTypeTyping:
    case AXTextEditTypePaste:
        return AXTextInserted;
    case AXTextEditTypeAttributesChange:
        return AXTextAttributesChanged;
    case AXTextEditTypeUnknown:
        break;
    }
    ASSERT_NOT_REACHED();
    return AXTextInserted;
}
#endif
    
} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
