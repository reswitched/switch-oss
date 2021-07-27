/*
  WKCWebViewEPUB.cpp

  Copyright (c) 2014-2019 ACCESS CO., LTD. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
  Boston, MA  02110-1301, USA.
*/

#include "config.h"
#include "WKCWebView.h"
#include "WKCWebFrame.h"

#include "CString.h"
#include "HashMap.h"
#include "StringHash.h"
#include "Vector.h"
#include "WTFString.h"

#include "helpers/WKCString.h"

#include <wkc/wkcpeer.h> // wkcI18NDecodePeer()
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <zip.h>
#include <string.h>

#ifdef __clang__
#include <sys/stat.h>
#endif

#ifdef _WIN32
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define snprintf _snprintf
#endif
#define strcasecmp stricmp
#endif

#define EPUB_CONTAINER_PATH "META-INF/container.xml"

typedef struct OpenedEPUB_ {
    WTF_MAKE_FAST_ALLOCATED;
public:
    OpenedEPUB_() { m_zip = 0; m_path[0] = 0; }
    ~OpenedEPUB_() {}
public:
    char m_path[256];
    struct zip* m_zip;
} OpenedEPUB;
WKC_DEFINE_GLOBAL_TYPE_ZERO(WTF::Vector<OpenedEPUB*>*, gOpenedEPUBs);

typedef struct OpenedZIPItem_ {
    WTF_MAKE_FAST_ALLOCATED;
public:
    OpenedZIPItem_()
        : m_zip(0)
        , m_fd(0)
        , m_size(0)
        , m_pos(0)
    {
        m_name[0] = 0;
    }
    ~OpenedZIPItem_() {}

public:
    struct zip* m_zip;
    struct zip_file* m_fd;
    char m_name[256];
    zip_uint64_t m_size;
    zip_uint64_t m_pos;
} OpenedZIPItem;
WKC_DEFINE_GLOBAL_TYPE_ZERO(WTF::Vector<OpenedZIPItem*>*, gOpenedZIPItems);

static WKC::FileSystemProcs gOrigFileProcs = {0};
static WKC::FileSystemProcs gFileProcs = {0};

namespace WKC {

bool
WKCWebView::EPUB::initialize()
{
    gOpenedEPUBs = new WTF::Vector<OpenedEPUB*>();
    gOpenedZIPItems = new WTF::Vector<OpenedZIPItem*>();

    return true;
}

void
WKCWebView::EPUB::finalize()
{
    delete gOpenedEPUBs;
    delete gOpenedZIPItems;
    gOpenedEPUBs = 0;
    gOpenedZIPItems = 0;
    memset(&gOrigFileProcs, 0, sizeof(WKC::FileSystemProcs));
    memset(&gFileProcs, 0, sizeof(WKC::FileSystemProcs));
}

void
WKCWebView::EPUB::forceTerminate()
{
    gOpenedEPUBs = 0;
    gOpenedZIPItems = 0;
    memset(&gOrigFileProcs, 0, sizeof(WKC::FileSystemProcs));
    memset(&gFileProcs, 0, sizeof(WKC::FileSystemProcs));
}

WKCWebView::EPUB::EPUB(WKCWebView* parent)
    :m_parent(parent)
    , m_zip(0)
{
    m_path = 0;

    m_EPUBData.fTitle.fItems = 0;
    m_EPUBData.fTitle.fStrings = 0;
    m_EPUBData.fCreator.fItems = 0;
    m_EPUBData.fCreator.fStrings = 0;
    m_EPUBData.fContributor.fItems = 0;
    m_EPUBData.fContributor.fStrings = 0;
    m_EPUBData.fLanguage.fItems = 0;
    m_EPUBData.fLanguage.fStrings = 0;
    m_EPUBData.fRights.fItems = 0;
    m_EPUBData.fRights.fStrings = 0;
    m_EPUBData.fPublisher.fItems = 0;
    m_EPUBData.fPublisher.fStrings = 0;

    m_EPUBData.fDate = 0;
    m_EPUBData.fModified = 0;
    m_EPUBData.fNavUri = 0;

    m_EPUBData.fPageProgressionDirection = WKC::EPageProgressDirectionDefault;

    m_EPUBData.fSpineItems = 0;
    m_EPUBData.fSpines = 0;
    
    m_EPUBData.fNavItems = 0;
    m_EPUBData.fNavs = 0;
}

static void
clearNav(WKCEPUBDataNav* self)
{
    if (!self)
        return;

    size_t count = self->fTocItems;
    for (size_t i=0; i<count; i++) {
        WKCEPUBDataTocItem* item = self->fItems[i];
        if (item) {
            if (item->fToc)
                WTF::fastFree((void *)item->fToc);
            if (item->fTitle)
                WTF::fastFree((void *)item->fTitle);
            if (item->fUri)
                WTF::fastFree((void *)item->fUri);
            if (item->fType)
                WTF::fastFree((void *)item->fType);
            WTF::fastFree(item);
        }
    }
    if (self->fItems)
        WTF::fastFree(self->fItems);

    count = self->fChildItems;
    for (size_t i=0; i<count; i++) {
        clearNav(self->fChildren[i]);
    }
    if (self->fChildren)
        WTF::fastFree(self->fChildren);

    if (self->fType)
        WTF::fastFree((void *)self->fType);

    WTF::fastFree(self);
}

WKCWebView::EPUB::~EPUB()
{
    if (m_zip)
        zip_discard((struct zip *)m_zip);

    if (m_path)
        WTF::fastFree(m_path);
    if (m_EPUBData.fSpines) {
        for(int i=0; i<m_EPUBData.fSpineItems; i++) {
            if (m_EPUBData.fSpines[i].fSpine)
                WTF::fastFree((void *)m_EPUBData.fSpines[i].fSpine);
            if (m_EPUBData.fSpines[i].fProperties)
                WTF::fastFree((void *)m_EPUBData.fSpines[i].fProperties);
        }
        WTF::fastFree(m_EPUBData.fSpines);
    }
    m_EPUBData.fSpines = 0;
    m_EPUBData.fSpineItems = 0;

    if (m_EPUBData.fNavItems) {
        for (int i=0; i<m_EPUBData.fNavItems; i++)
            clearNav(m_EPUBData.fNavs[i]);
        WTF::fastFree(m_EPUBData.fNavs);
    }
    m_EPUBData.fNavs = 0;

    if (m_EPUBData.fTitle.fItems)
        if (m_EPUBData.fTitle.fStrings) {
            for (int i=0; i<m_EPUBData.fTitle.fItems; i++)
                if (m_EPUBData.fTitle.fStrings[i])
                    WTF::fastFree((void *)m_EPUBData.fTitle.fStrings[i]);
            WTF::fastFree((void *)m_EPUBData.fTitle.fStrings);
        }
    if (m_EPUBData.fCreator.fItems)
        if (m_EPUBData.fCreator.fStrings) {
            for (int i=0; i<m_EPUBData.fCreator.fItems; i++)
                if (m_EPUBData.fCreator.fStrings[i])
                    WTF::fastFree((void *)m_EPUBData.fCreator.fStrings[i]);
            WTF::fastFree((void *)m_EPUBData.fCreator.fStrings);
        }
    if (m_EPUBData.fContributor.fItems)
        if (m_EPUBData.fContributor.fStrings) {
            for (int i=0; i<m_EPUBData.fContributor.fItems; i++)
                if (m_EPUBData.fContributor.fStrings[i])
                    WTF::fastFree((void *)m_EPUBData.fContributor.fStrings[i]);
            WTF::fastFree((void *)m_EPUBData.fContributor.fStrings);
        }
    if (m_EPUBData.fLanguage.fItems)
        if (m_EPUBData.fLanguage.fStrings) {
            for (int i=0; i<m_EPUBData.fLanguage.fItems; i++)
                if (m_EPUBData.fLanguage.fStrings[i])
                    WTF::fastFree((void *)m_EPUBData.fLanguage.fStrings[i]);
            WTF::fastFree((void *)m_EPUBData.fLanguage.fStrings);
        }
    if (m_EPUBData.fRights.fItems)
        if (m_EPUBData.fRights.fStrings) {
            for (int i=0; i<m_EPUBData.fRights.fItems; i++)
                if (m_EPUBData.fRights.fStrings[i])
                    WTF::fastFree((void *)m_EPUBData.fRights.fStrings[i]);
            WTF::fastFree((void *)m_EPUBData.fRights.fStrings);
        }
    if (m_EPUBData.fPublisher.fItems)
        if (m_EPUBData.fPublisher.fStrings) {
            for (int i=0; i<m_EPUBData.fPublisher.fItems; i++)
                if (m_EPUBData.fPublisher.fStrings[i])
                    WTF::fastFree((void *)m_EPUBData.fPublisher.fStrings[i]);
            WTF::fastFree(m_EPUBData.fPublisher.fStrings);
        }

    if (m_EPUBData.fDate)
        WTF::fastFree((void *)m_EPUBData.fDate);
    if (m_EPUBData.fModified)
        WTF::fastFree((void *)m_EPUBData.fModified);
    if (m_EPUBData.fNavUri)
        WTF::fastFree((void *)m_EPUBData.fNavUri);

    for (int i=0; i<gOpenedEPUBs->size(); i++) {
        OpenedEPUB* item = gOpenedEPUBs->at(i);
        if (item->m_zip==m_zip) {
            gOpenedEPUBs->remove(i);
            delete item;
            break;
        }
    }
}

bool
WKCWebView::EPUB::openEPUB(const char* in_path, bool parseTOC)
{
    if (!in_path)
        return false;

    OpenedEPUB* item = 0;

    m_path = WTF::fastStrDup(in_path);
    if (!m_path)
        goto error_end;

    if (!parseMetaInfo(parseTOC))
        goto error_end;

    item = new OpenedEPUB();
    strncpy(item->m_path, in_path, 255);
    item->m_zip = (struct zip *)m_zip;
    gOpenedEPUBs->append(item);

    return true;

error_end:
    if (m_path)
        WTF::fastFree(m_path);
    m_path = 0;
    return false;
}

void
WKCWebView::EPUB::closeEPUB()
{
    for (int i=0; i<gOpenedEPUBs->size(); i++) {
        OpenedEPUB* item = gOpenedEPUBs->at(i);
        if (item->m_zip == (struct zip *)m_zip) {
            gOpenedEPUBs->remove(i);
            delete item;
            break;
        }
    }
    if (m_zip)
        zip_discard((struct zip *)m_zip);
    m_zip = 0;
    if (m_EPUBData.fSpines)
        WTF::fastFree(m_EPUBData.fSpines);
    m_EPUBData.fSpines = 0;
    m_EPUBData.fSpineItems = 0;
}

bool
WKCWebView::EPUB::loadSpineItem(unsigned int in_index)
{
    if (in_index>=m_EPUBData.fSpineItems)
        return false;
    m_parent->mainFrame()->loadURI((const char *)m_EPUBData.fSpines[in_index].fSpine);
    return true;
}

// xml relateds
static void
getAllContent(xmlNodePtr in_node, WTF::String& out_str, bool addtag=false)
{
    for (xmlNodePtr t = in_node; t; t = t->next) {
        if ((t->type == XML_TEXT_NODE || t->type == XML_CDATA_SECTION_NODE) && t->content) {
            out_str.append(WTF::String::fromUTF8((const char *)t->content));
        }
        if (t->children) {
            if (t->name && addtag)
                out_str.append("<" + WTF::String::fromUTF8((const char *)t->name) + ">");
            getAllContent(t->children, out_str, addtag);
            if (t->name && addtag)
                out_str.append("</" + WTF::String::fromUTF8((const char *)t->name) + ">");
        }
    }
}

static const xmlChar*
getContent(xmlNodePtr in_node)
{
    for (xmlNodePtr t = in_node; t; t = t->next) {
        if ((t->type == XML_TEXT_NODE || t->type == XML_CDATA_SECTION_NODE) && t->content) {
            return t->content;
        }
    }
    return 0;
}

static const xmlChar*
getText(xmlNodePtr in_node)
{
    for (xmlNodePtr t = in_node; t; t = t->next) {
        if (t->type == XML_TEXT_NODE && t->content) {
            return t->content;
        }
    }
    return 0;
}

static const xmlChar*
getAttribute(xmlAttrPtr in_attr, const char* in_name)
{
    for (xmlAttrPtr t = in_attr; t; t = t->next) {
        if (t->type == XML_ATTRIBUTE_NODE) {
            if (t->name && xmlStrcmp(t->name, (const xmlChar*)in_name) == 0) {
                return getText(t->children);
            }
        }
    }
    return 0;
}

static bool
isTargetElement(xmlNodePtr in_node, const char* in_ns, const char* in_name)
{
    if (in_node->type == XML_ELEMENT_NODE) {
        if (in_node->name && xmlStrcmp(in_node->name, (const xmlChar*)in_name) == 0) {
            if (!in_ns || (in_node->ns && in_node->ns->prefix && xmlStrcmp(in_node->ns->prefix, (const xmlChar*)in_ns) == 0)) {
                return true;
            }
        }
    }
    return false;
}

static void parseToc(void* zipa, WKCEPUBData& data, const char* tocpath, const char* rootpath);

class SpineItem 
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    SpineItem(const WTF::String& href, const WTF::String& properties)
        : m_href(href)
        , m_properties(properties)
    {}
    ~SpineItem()
    {}
    SpineItem() {}

    WTF::String m_href;
    WTF::String m_properties;
};

static bool
allocEPUBStringArray(const WTF::Vector<WTF::String> items, WKCEPUBStringArray& dest)
{
    if (items.size()==0)
        return true;
    dest.fItems = items.size();
    dest.fStrings = (const unsigned char **)WTF::fastMalloc(sizeof(const unsigned char *)*dest.fItems);
    if (!dest.fStrings)
        return false;

    for (int i=0; i<dest.fItems; i++) {
        dest.fStrings[i] = (const unsigned char *)WTF::fastStrDup(items[i].utf8().data());
        if (!dest.fStrings[i])
            return false;
    }
    return true;
}

bool
WKCWebView::EPUB::parseMetaInfo(bool parseTOC)
{
    int err = 0;
    m_zip = zip_open(m_path, 0, &err);
    if (!m_zip)
        return false;

    void* buf = 0;
    int len = 0;
    struct zip_stat st;
    zip_file* fd = 0;
    char* contentpath = 0;
    char* rootpath = 0;
    char* basepath = 0;
    char* navuri = 0;
    xmlParserCtxtPtr xmlparser = 0;
    WTF::Vector<WTF::String> spines;
    WTF::Vector<bool> spinesislinear;

    WTF::Vector<WTF::String> titles;
    WTF::Vector<WTF::String> creators;
    WTF::Vector<WTF::String> contributors;
    WTF::Vector<WTF::String> publishers;
    WTF::Vector<WTF::String> rights;
    WTF::Vector<WTF::String> languages;

    WTF::HashMap<WTF::String, SpineItem> items;

    items.clear();

    // create parser
    xmlparser = xmlCreatePushParserCtxt((xmlSAXHandlerPtr)0, (void*)0, (const char*)0, 0, (const char*)0);
    if (!xmlparser)
        goto error_end;

    // read container
    zip_stat_init(&st);
    if (zip_stat((struct zip *)m_zip, EPUB_CONTAINER_PATH, 0, &st)<0)
        goto error_end;
    len = st.size;
    if (len<=0)
        goto error_end;
    buf = WTF::fastMalloc(len);
    if (!buf)
        goto error_end;
    fd = zip_fopen((struct zip *)m_zip, EPUB_CONTAINER_PATH, 0);
    if (!fd)
        goto error_end;
    if (zip_fread(fd, buf, len)!=len)
        goto error_end;
    zip_fclose(fd);
    fd = 0;

    // parse container
    err = xmlParseChunk(xmlparser, (const char *)buf, len, true);
    if (err)
        goto error_end;
    if (!xmlparser->myDoc)
        goto error_end;
    for (xmlNodePtr node = xmlparser->myDoc->children; node; node = node->next) {
        if (isTargetElement(node, 0, "container")) {
            for (xmlNodePtr n2 = node->children; n2; n2 = n2->next) {
                if (isTargetElement(n2, 0, "rootfiles")) {
                    for (xmlNodePtr n3 = n2->children; n3; n3 = n3->next) {
                        if (isTargetElement(n3, 0, "rootfile")) {
                            const char* fullpath = (const char *)getAttribute(n3->properties, "full-path");
                            if (fullpath) {
                                contentpath = WTF::fastStrDup(fullpath);
                                goto found;
                            }
                        }
                    }
                }
            }
        }
    }
found:
    xmlFreeDoc(xmlparser->myDoc);
    xmlparser->myDoc = 0;
    xmlFreeParserCtxt(xmlparser);
    xmlparser = 0;
    WTF::fastFree(buf);
    buf = 0;
    if (!contentpath)
        goto error_end;

    // read opf
    if (strchr(contentpath, '/') || strchr(contentpath, '\\')) {
        basepath = WTF::fastStrDup(contentpath);
        if (!basepath)
            goto error_end;
        {
            char* p = strrchr(basepath, '/');
            if (p)
                *p = 0;
        }
    } else {
        basepath = WTF::fastStrDup("");
        if (!basepath)
            goto error_end;
    }
    rootpath = (char *)WTF::fastMalloc(1 + strlen(m_path) + 1 + strlen(basepath) + 1);
    if (!rootpath)
        goto error_end;
    rootpath[0] = 0;
    if (m_path[0]!='/' && m_path[0]!='\\')
        strcpy(rootpath, "/");
    strcat(rootpath, m_path);
    for (int i=0; i<strlen(rootpath); i++) {
        if (rootpath[i]=='\\')
            rootpath[i] = '/';
    }
    if (basepath[0]) {
        strcat(rootpath, "/");
        strcat(rootpath, basepath);
    }

    xmlparser = xmlCreatePushParserCtxt((xmlSAXHandlerPtr)0, (void*)0, (const char*)0, 0, (const char*)0);
    if (!xmlparser)
        goto error_end;

    zip_stat_init(&st);
    if (zip_stat((struct zip *)m_zip, (const char *)contentpath, 0, &st)<0)
        goto error_end;
    len = st.size;
    if (len<=0)
        goto error_end;
    buf = WTF::fastMalloc(len);
    if (!buf)
        goto error_end;
    fd = zip_fopen((struct zip *)m_zip, (const char *)contentpath, 0);
    if (!fd)
        goto error_end;
    if (zip_fread(fd, buf, len)!=len)
        goto error_end;
    zip_fclose(fd);
    fd = 0;

    // parse opf
    err = xmlParseChunk(xmlparser, (const char *)buf, len, true);
    if (err)
        goto error_end;
    if (!xmlparser->myDoc)
        goto error_end;
    for (xmlNodePtr node = xmlparser->myDoc->children; node; node = node->next) {
        const xmlChar* item = 0;
        if (isTargetElement(node, 0, "package")) {
            for (xmlNodePtr n2 = node->children; n2; n2 = n2->next) {
                if (isTargetElement(n2, 0, "metadata")) {
                    for (xmlNodePtr n3 = n2->children; n3; n3 = n3->next) {
                        if (isTargetElement(n3, 0, "date")) {
                            if (item = getContent(n3->children)) {
                                if (m_EPUBData.fDate)
                                    WTF::fastFree((void *)m_EPUBData.fDate);
                                m_EPUBData.fDate = (const unsigned char *)WTF::fastStrDup((char *)item);
                            }
                        } else if (isTargetElement(n3, 0, "title")) {
                            if (item = getContent(n3->children)) {
                                titles.append(WTF::String::fromUTF8(item));
                            }
                        } else if (isTargetElement(n3, 0, "creator")) {
                            if (item = getContent(n3->children)) {
                                creators.append(WTF::String::fromUTF8(item));
                            }
                        } else if (isTargetElement(n3, 0, "contributor")) {
                            if (item = getContent(n3->children)) {
                                contributors.append(WTF::String::fromUTF8(item));
                            }
                        } else if (isTargetElement(n3, 0, "publisher")) {
                            if (item = getContent(n3->children)) {
                                publishers.append(WTF::String::fromUTF8(item));
                            }
                        } else if (isTargetElement(n3, 0, "rights")) {
                            if (item = getContent(n3->children)) {
                                rights.append(WTF::String::fromUTF8(item));
                            }
                        } else if (isTargetElement(n3, 0, "language")) {
                            if (item = getContent(n3->children)) {
                                languages.append(WTF::String::fromUTF8(item));
                            }
                        } else if (isTargetElement(n3, 0, "meta")) {
                            const xmlChar* prop = getAttribute(n3->properties, "property");
                            if (prop && strcasecmp((const char *)prop, "dcterms:modified")==0) {
                                if (item = getContent(n3->children)) {
                                    if (m_EPUBData.fModified)
                                        WTF::fastFree((void *)m_EPUBData.fModified);
                                    m_EPUBData.fModified  = (const unsigned char *)WTF::fastStrDup((char *)item);
                                }
                            }
                        }
                    }
                } else if (isTargetElement(n2, 0, "manifest")) {
                    for (xmlNodePtr n3 = n2->children; n3; n3 = n3->next) {
                        if (isTargetElement(n3, 0, "item")) {
                            const xmlChar* id = getAttribute(n3->properties, "id");
                            const xmlChar* href = getAttribute(n3->properties, "href");
                            const xmlChar* properties = getAttribute(n3->properties, "properties");
                            if (href) {
                                if (id) {
                                    SpineItem sitem(href, properties);
                                    items.add(WTF::String(id), sitem);
                                }
                                if (properties && strcasecmp((const char *)properties, "nav")==0) {
                                    if (navuri)
                                        WTF::fastFree((void *)navuri);
                                    navuri = WTF::fastStrDup((const char *)href);
                                }
                            }
                        }
                    }
                } else if (isTargetElement(n2, 0, "spine")) {
                    item = getAttribute(n2->properties, "page-progression-direction");
                    if (item) {
                        if (strcasecmp((const char *)item, "ltr")==0) {
                            m_EPUBData.fPageProgressionDirection = WKC::EPageProgressDirectionLtR;
                        } else if (strcasecmp((const char *)item, "rtl")==0) {
                            m_EPUBData.fPageProgressionDirection = WKC::EPageProgressDirectionRtL;
                        } else if (strcasecmp((const char *)item, "default")==0) {
                            m_EPUBData.fPageProgressionDirection = WKC::EPageProgressDirectionDefault;
                        }
                    }
                    for (xmlNodePtr n3 = n2->children; n3; n3 = n3->next) {
                        if (isTargetElement(n3, 0, "itemref")) {
                            item = getAttribute(n3->properties, "idref");
                            const xmlChar* linear = getAttribute(n3->properties, "linear");
                            if (item) {
                                spines.append(WTF::String(item));
                                if (linear && strcasecmp((const char *)linear, "no") == 0)
                                    spinesislinear.append(false);
                                else
                                    spinesislinear.append(true);
                            }
                        }
                    }
                }
            }
        }
    }

    if (spines.size()==0)
        goto error_end;

    if (!allocEPUBStringArray(titles, m_EPUBData.fTitle))
        goto error_end;
    if (!allocEPUBStringArray(creators, m_EPUBData.fCreator))
        goto error_end;
    if (!allocEPUBStringArray(contributors, m_EPUBData.fContributor))
        goto error_end;
    if (!allocEPUBStringArray(publishers, m_EPUBData.fPublisher))
        goto error_end;
    if (!allocEPUBStringArray(rights, m_EPUBData.fRights))
        goto error_end;
    if (!allocEPUBStringArray(languages, m_EPUBData.fLanguage))
        goto error_end;

    for (int i=0; i<spines.size(); i++) {
        WTF::String spinepath = basepath;
        if (spinepath.length())
            spinepath.append("/");
        spinepath.append(items.get(spines[i]).m_href);
        zip_stat_init(&st);
        if (zip_stat((struct zip *)m_zip, (const char *)spinepath.utf8().data(), 0, &st)<0)
            goto error_end;
    }

    m_EPUBData.fSpines = (WKCEPUBDataSpine *)WTF::fastMalloc(sizeof(WKCEPUBDataSpine)*spines.size());
    if (!m_EPUBData.fSpines)
        goto error_end;
    for (int i=0; i<spines.size(); i++) {
        WTF::String idref = spines[i];
        WTF::String href = items.get(idref).m_href;
        WTF::String properties = items.get(idref).m_properties;
        bool islinear = spinesislinear[i];
        if (href.length()) {
            WTF::String s = WTF::String::format("file://%s/%s", rootpath, href.utf8().data());
            m_EPUBData.fSpines[i].fSpine = (unsigned char *)WTF::fastStrDup(s.utf8().data());
            if (!m_EPUBData.fSpines[i].fSpine)
                goto error_end;
            if (properties.length()) {
                m_EPUBData.fSpines[i].fProperties = (unsigned char *)WTF::fastStrDup(properties.utf8().data());
                if (!m_EPUBData.fSpines[i].fProperties)
                    goto error_end;
            }
            else
                m_EPUBData.fSpines[i].fProperties = 0;
            m_EPUBData.fSpines[i].fLinear = islinear ? 1 : 0;
            m_EPUBData.fSpineItems++;
        }
    }

    xmlFreeDoc(xmlparser->myDoc);
    xmlparser->myDoc = 0;
    xmlFreeParserCtxt(xmlparser);
    WTF::fastFree(buf);

    if (navuri && parseTOC) {
        WTF::String path = WTF::String::format("%s/%s", basepath, navuri);
        char* tocpath = WTF::fastStrDup(rootpath);
        if (!tocpath)
            goto error_end;
        const char* p = strchr(navuri, '/');
        if (p) {
            WTF::fastFree(tocpath);
            WTF::String rp(rootpath);
            WTF::String np(navuri, p-navuri);
            WTF::String tp = rp + '/' + np;
            tocpath = WTF::fastStrDup(tp.utf8().data());
            if (!tocpath)
                goto error_end;
        }
        parseToc(m_zip, m_EPUBData, path.utf8().data(), tocpath);
        WTF::String s = WTF::String::format("file://%s/%s", rootpath, navuri);
        m_EPUBData.fNavUri = (unsigned char *)WTF::fastStrDup(s.utf8().data());
        WTF::fastFree(tocpath);
    }

    if (navuri)
        WTF::fastFree(navuri);
    WTF::fastFree(contentpath);
    WTF::fastFree(rootpath);
    WTF::fastFree(basepath);

    return true;

error_end:
    if (fd)
        zip_fclose(fd);
    if (m_zip) {
        zip_discard((struct zip *)m_zip);
        m_zip = 0;
    }
    if (buf)
        WTF::fastFree(buf);
    if (contentpath)
        WTF::fastFree(contentpath);
    if (rootpath)
        WTF::fastFree(rootpath);
    if (basepath)
        WTF::fastFree(basepath);
    if (navuri)
        WTF::fastFree(navuri);
    if (xmlparser) {
        if (xmlparser->myDoc) {
            xmlFreeDoc(xmlparser->myDoc);
        }
        xmlFreeParserCtxt(xmlparser);
    }

    return false;
}

class EPUBDataTocItem
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    EPUBDataTocItem()
        : index(-1)
    {}
    ~EPUBDataTocItem() {}

    unsigned int index;
    WTF::String href;
    WTF::String toc;
    WTF::String title;
    WTF::String type;
};
class EPUBDataNav
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    EPUBDataNav() {}
    ~EPUBDataNav() {}

    Vector<EPUBDataTocItem> items;
    Vector<EPUBDataNav> children;
};

static EPUBDataNav*
parseNav(xmlNodePtr node, int& index)
{
    EPUBDataNav* navs = new EPUBDataNav();
    bool havenav = false;

    for (xmlNodePtr n1 = node->children; n1; n1 = n1->next) {
        if (isTargetElement(n1, 0, "li")) {
            for (xmlNodePtr n2 = n1->children; n2; n2 = n2->next) {
                if (isTargetElement(n2, 0, "a")) {
                    const xmlChar* href = getAttribute(n2->properties, "href");
                    const xmlChar* title = getAttribute(n2->properties, "title");
                    const xmlChar* type = getAttribute(n2->properties, "type");
                    if (!type)
                        type = getAttribute(n2->properties, "epub:type");
                    WTF::String toc;
                    getAllContent(n2->children, toc, true);
                    if (href && toc.length()) {
                        EPUBDataTocItem item;
                        item.index = index++;
                        item.href = WTF::String::fromUTF8(href);
                        item.toc = toc;
                        item.title = title ? WTF::String::fromUTF8(title) : toc;
                        if (type)
                            item.type = WTF::String::fromUTF8(type);
                        navs->items.append(item);
                        havenav = true;
                    }
                } else if (isTargetElement(n2, 0, "ol") || isTargetElement(n2, 0, "ul")) {
                    EPUBDataNav* child = parseNav(n2, index);
                    if (child) {
                        navs->children.append(*child);
                        havenav = true;
                        delete child;
                    }
                }
            }
        } else if (isTargetElement(n1, 0, "ol") || isTargetElement(n1, 0, "ul")) {
            EPUBDataNav* child = parseNav(n1, index);
            if (child) {
                navs->children.append(*child);
                havenav = true;
                delete child;
            }
        }
    }

    if (havenav)
        return navs;

    delete navs;
    return 0;
}

static WKCEPUBDataNav*
createNavTree(const EPUBDataNav* nav, const char* rootpath)
{
    if (!nav)
        return 0;

    WKCEPUBDataNav* navs = (WKCEPUBDataNav *)WTF::fastMalloc(sizeof(WKCEPUBDataNav));
    if (!navs)
        return 0;
    memset(navs, 0, sizeof(WKCEPUBDataNav));

    size_t count = nav->items.size();
    if (count) {
        navs->fTocItems = count;
        navs->fItems = (WKCEPUBDataTocItem **)WTF::fastMalloc(sizeof(WKCEPUBDataTocItem)*count);
        if (!navs->fItems)
            goto error_end;
        for (size_t i=0; i<count; i++) {
            WKCEPUBDataTocItem* item = (WKCEPUBDataTocItem *)WTF::fastMalloc(sizeof(WKCEPUBDataTocItem));
            if (!item)
                goto error_end;
            item->fIndex = nav->items[i].index;
            item->fToc = (const unsigned char *)WTF::fastStrDup(nav->items[i].toc.utf8().data());
            item->fTitle = (const unsigned char *)WTF::fastStrDup(nav->items[i].title.utf8().data());
            item->fType = (const unsigned char *)WTF::fastStrDup(nav->items[i].type.utf8().data());
            WTF::String href = WTF::String::format("file://%s/%s", rootpath, nav->items[i].href.utf8().data());
            item->fUri = (const unsigned char *)WTF::fastStrDup(href.utf8().data());
            if (!item->fToc || !item->fUri || !item->fTitle || !item->fType) {
                if (item->fToc)
                    WTF::fastFree((void *)item->fToc);
                if (item->fUri)
                    WTF::fastFree((void *)item->fUri);
                if (item->fTitle)
                    WTF::fastFree((void *)item->fTitle);
                if (item->fType)
                    WTF::fastFree((void *)item->fType);
                WTF::fastFree(item);
                goto error_end;
            }
            navs->fItems[i] = item;
        }
    }

    count = nav->children.size();
    if (count) {
        navs->fChildItems = count;
        navs->fChildren = (WKCEPUBDataNav**)WTF::fastMalloc(sizeof(WKCEPUBDataNav*)*count);
        if (!navs->fChildren)
            goto error_end;
        for (size_t i=0; i<count; i++) {
            navs->fChildren[i] = createNavTree(&nav->children[i], rootpath);
            if (!navs->fChildren[i])
                goto error_end;
        }
    }

    return navs;

error_end:
    if (navs)
        clearNav(navs);
    return 0;
}

static xmlNodePtr
findNavTag(xmlNodePtr node)
{
    if (!node)
        return 0;

    for (xmlNodePtr n = node; n; n = n->next) {
        if (isTargetElement(n, 0, "nav"))
            return n;
        else {
            xmlNodePtr ret = findNavTag(n->children);
            if (ret)
                return ret;
        }
    }

    return 0;
}

static void
parseToc(void* zipa, WKCEPUBData& data, const char* tocpath, const char* rootpath)
{
    int err = 0;
    char* buf = 0;
    int len = 0;
    struct zip_stat st;
    zip_file* fd = 0;
    xmlParserCtxtPtr xmlparser = 0;
    EPUBDataNav* navs[32] = {0};
    Vector<String> navtypes;
    int navitems = 0;

    zip_stat_init(&st);
    if (zip_stat((struct zip *)zipa, tocpath, 0, &st)<0)
        goto error_end;
    len = st.size;
    if (len<=0)
        goto error_end;
    buf = (char *)WTF::fastMalloc(len);
    if (!buf)
        goto error_end;
    fd = zip_fopen((struct zip *)zipa, tocpath, 0);
    if (!fd)
        goto error_end;
    if (zip_fread(fd, buf, len)!=len)
        goto error_end;
    zip_fclose(fd);
    fd = 0;

    xmlparser = xmlCreatePushParserCtxt((xmlSAXHandlerPtr)0, (void*)0, (const char*)0, 0, (const char*)0);
    if (!xmlparser)
        goto error_end;
    err = xmlParseChunk(xmlparser, (const char *)buf, len, true);
    if (err)
        goto error_end;
    if (!xmlparser->myDoc)
        goto error_end;

    for (xmlNodePtr n0 = xmlparser->myDoc->children; n0; n0 = n0->next) {
        if (isTargetElement(n0, 0, "html")) {
            for (xmlNodePtr n1 = n0->children; n1; n1 = n1->next) {
                if (isTargetElement(n1, 0, "body")) {
                    xmlNodePtr node = n1->children;
                    while (xmlNodePtr navtag = findNavTag(node)) {
                        const xmlChar* navtype = 0;
                        navtype = getAttribute(navtag->properties, "type");
                        if (!navtype)
                            navtype = getAttribute(navtag->properties, "epub:type");
                        if (navtype)
                            navtypes.append((char *)navtype);
                        else
                            navtypes.append(String(""));
                        node = navtag->next;
                        for (xmlNodePtr n2 = navtag->children; n2; n2 = n2->next) {
                            if (isTargetElement(n2, 0, "ol") || isTargetElement(n2, 0, "ul")) {
                                if (navitems<32) {
                                    int index = 0;
                                    navs[navitems] = parseNav(n2, index);
                                    if (navs[navitems])
                                        navitems++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!navitems)
        goto error_end;

    data.fNavItems = navitems;
    data.fNavs = (WKCEPUBDataNav **)WTF::fastMalloc(sizeof(WKCEPUBDataNav*)*navitems);
    if (!data.fNavs)
        goto error_end;
    for (int i=0; i<navitems; i++) {
        data.fNavs[i] = createNavTree(navs[i], rootpath);
        if (navtypes[i].length())
            data.fNavs[i]->fType = (const unsigned char *)WTF::fastStrDup(navtypes[i].utf8().data());
        else
            data.fNavs[i]->fType = 0;
        delete navs[i];
    }

    xmlFreeDoc(xmlparser->myDoc);
    xmlFreeParserCtxt(xmlparser);
    WTF::fastFree(buf);
    return;

error_end:
    if (fd)
        zip_fclose(fd);
    if (buf)
        WTF::fastFree(buf);
    if (xmlparser) {
        if (xmlparser->myDoc) {
            xmlFreeDoc(xmlparser->myDoc);
        }
        xmlFreeParserCtxt(xmlparser);
    }
}

// file callbacks
static bool
isEPUBFD(void* in_fd)
{
    if (!gOpenedZIPItems)
        return false;

    for (int i=0; i<gOpenedZIPItems->size(); i++) {
        OpenedZIPItem* item = gOpenedZIPItems->at(i);
        if (item==in_fd)
            return true;
    }
    return false;
}

static void*
EPUBFOpen(int in_usage, const char* in_path, const char* in_mode)
{
    for (int i=0; i<gOpenedEPUBs->size(); i++) {
        OpenedEPUB* item = gOpenedEPUBs->at(i);
        if (!item->m_path[0])
            continue;
        if ((strlen(in_path) > strlen(item->m_path)) &&
            strncmp(in_path, item->m_path, strlen(item->m_path))==0) {
            char* path = WTF::fastStrDup(in_path + strlen(item->m_path)+1);
            if (!path)
                return 0;
            int len = strlen(path);
            for (int j=0; j<len; j++) {
                if (path[j]=='\\')
                    path[j] = '/';
            }
            struct zip_stat sb;
            if (zip_stat(item->m_zip, path, 0, &sb)!=0) {
                WTF::fastFree(path);
                return 0;
            }
            struct zip_file* z = zip_fopen(item->m_zip, path, ZIP_FL_NOCASE);
            if (!z) {
                WTF::fastFree(path);
                return 0;
            }
            OpenedZIPItem* zi = new OpenedZIPItem();
            zi->m_zip = item->m_zip;
            zi->m_fd = z;
            strncpy(zi->m_name, path, 255);
            zi->m_size = sb.size;
            zi->m_pos = 0;
            gOpenedZIPItems->append(zi);
            WTF::fastFree(path);
            return zi;
        }
    }
    return gOrigFileProcs.fFOpenProc(in_usage, in_path, in_mode);
}


static int
EPUBFClose(void* in_fd)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFCloseProc(in_fd);

    OpenedZIPItem* item = (OpenedZIPItem *)in_fd;
    zip_fclose(item->m_fd);
    for (int i=0; i<gOpenedZIPItems->size(); i++) {
        OpenedZIPItem* entry = gOpenedZIPItems->at(i);
        if (entry == item) {
            gOpenedZIPItems->remove(i);
            delete item;
            break;
        }
    }
    return 0;
}

static wkc_uint64
EPUBFRead(void *out_buffer, wkc_uint64 in_size, wkc_uint64 in_count, void *in_fd)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFReadProc(out_buffer, in_size, in_count, in_fd);
    OpenedZIPItem* item = (OpenedZIPItem *)in_fd;
    int ret = zip_fread(item->m_fd, out_buffer, in_size*in_count);
    if (ret<0)
        return ret;
    item->m_pos+=ret;
    return ret;
}

static int
EPUBFStat(void *in_fd, struct stat *buf)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFStatProc(in_fd, buf);

    OpenedZIPItem* item = (OpenedZIPItem *)in_fd;
    struct zip_stat sb;
    int ret = zip_stat(item->m_zip, item->m_name, 0, &sb);
    if (ret==0)
        buf->st_size = sb.size;
    return ret;
}

static int
EPUBFEof(void* in_fd)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFeofProc(in_fd);
    OpenedZIPItem* item = (OpenedZIPItem *)in_fd;
    if (item->m_pos>=item->m_size)
        return 1;
    return 0;
}

static int
EPUBFSeek(void *in_fd, wkc_int64 in_offset, int in_whence)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFSeekProc(in_fd, in_offset, in_whence);

    OpenedZIPItem* item = (OpenedZIPItem *)in_fd;
    int ret = 0;
    void* buf = 0;
    if (in_whence==1) {
        buf = WTF::fastMalloc(in_offset);
        if (!buf)
            return -1;
        ret = zip_fread(item->m_fd, buf, in_offset);
        WTF::fastFree(buf);
        if (ret<0)
            return -1;
        item->m_pos += ret;
        return item->m_pos;
    }

    zip_fclose(item->m_fd);
    zip_fopen(item->m_zip, item->m_name, ZIP_FL_NOCASE);
    int len = 0;
    if (in_whence==0) {
        len = in_offset;
        if (len>item->m_size)
            len = item->m_size;
    } else {
        if (item->m_size < -in_offset)
            in_offset = item->m_size;
        len = item->m_size + in_offset;
    }
    if (len==0)
        return 0;

    buf = WTF::fastMalloc(len);
    if (!buf)
        return -1;
    ret = zip_fread(item->m_fd, buf, in_offset);
    WTF::fastFree(buf);
    if (ret<0)
        return -1;
    item->m_pos = ret;
    return item->m_pos;
}

static wkc_int64
EPUBFTell(void *in_fd)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFTellProc(in_fd);
    OpenedZIPItem* item = (OpenedZIPItem *)in_fd;
    return item->m_pos;
}

static int
EPUBFError(void* in_fd)
{
    if (!isEPUBFD(in_fd))
        return gOrigFileProcs.fFErrorProc(in_fd);
    return 0;
}

static int
EPUBStat(const char* in_filename, struct stat* out_buf)
{
    for (int i=0; i<gOpenedEPUBs->size(); i++) {
        OpenedEPUB* item = gOpenedEPUBs->at(i);
        if (!item->m_path[0])
            continue;
        if ((strlen(in_filename) > strlen(item->m_path)) &&
            strncmp(in_filename, item->m_path, strlen(item->m_path))==0) {
            char* path = WTF::fastStrDup(in_filename + strlen(item->m_path)+1);
            if (!path)
                return -1;
            int len = strlen(path);
            for (int j=0; j<len; j++) {
                if (path[j]=='\\')
                    path[j] = '/';
            }
            struct zip_stat sb;
            int ret = zip_stat(item->m_zip, path, 0, &sb);
            WTF::fastFree(path);
            if (ret==0)
                out_buf->st_size = sb.size;
            return ret;
        }
    }
    return gOrigFileProcs.fStatProc(in_filename, out_buf);
}

const WKC::FileSystemProcs*
WKCWebView::EPUB::EPUBFileProcsWrapper(const WKC::FileSystemProcs *procs)
{
    if (!procs)
        return 0;

    memcpy(&gOrigFileProcs, procs, sizeof(WKC::FileSystemProcs));
    memcpy(&gFileProcs, procs, sizeof(WKC::FileSystemProcs));

    gFileProcs.fFOpenProc = EPUBFOpen;
    gFileProcs.fFCloseProc = EPUBFClose;
    gFileProcs.fFReadProc = EPUBFRead;
    gFileProcs.fFeofProc = EPUBFEof;
    gFileProcs.fFStatProc = EPUBFStat;
    gFileProcs.fFSeekProc = EPUBFSeek;
    gFileProcs.fFTellProc = EPUBFTell;
    gFileProcs.fFErrorProc = EPUBFError;
    gFileProcs.fStatProc = EPUBStat;

    return &gFileProcs;
}

} // namespace
