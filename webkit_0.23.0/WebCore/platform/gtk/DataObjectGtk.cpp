/*
 * Copyright (C) 2009, Martin Robinson
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
#include "DataObjectGtk.h"

#include <gtk/gtk.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static void replaceNonBreakingSpaceWithSpace(String& str)
{
    static const UChar NonBreakingSpaceCharacter = 0xA0;
    static const UChar SpaceCharacter = ' ';
    str.replace(NonBreakingSpaceCharacter, SpaceCharacter);
}

HashMap<String, String> DataObjectGtk::unknownTypes() const
{
    return m_unknownTypeData;
}

void DataObjectGtk::setText(const String& newText)
{
    m_text = newText;
    replaceNonBreakingSpaceWithSpace(m_text);
}

void DataObjectGtk::setMarkup(const String& newMarkup)
{
    m_markup = newMarkup;
}

void DataObjectGtk::setURIList(const String& uriListString)
{
    m_uriList = uriListString;

    // This code is originally from: platform/chromium/ChromiumDataObject.cpp.
    // FIXME: We should make this code cross-platform eventually.

    // Line separator is \r\n per RFC 2483 - however, for compatibility
    // reasons we also allow just \n here.
    Vector<String> uriList;
    uriListString.split('\n', uriList);

    // Process the input and copy the first valid URL into the url member.
    // In case no URLs can be found, subsequent calls to getData("URL")
    // will get an empty string. This is in line with the HTML5 spec (see
    // "The DragEvent and DataTransfer interfaces"). Also extract all filenames
    // from the URI list.
    bool setURL = false;
    for (size_t i = 0; i < uriList.size(); ++i) {
        String& line = uriList[i];
        line = line.stripWhiteSpace();
        if (line.isEmpty())
            continue;
        if (line[0] == '#')
            continue;

        URL url = URL(URL(), line);
        if (url.isValid()) {
            if (!setURL) {
                m_url = url;
                setURL = true;
            }

            GUniqueOutPtr<GError> error;
            GUniquePtr<gchar> filename(g_filename_from_uri(line.utf8().data(), 0, &error.outPtr()));
            if (!error && filename)
                m_filenames.append(String::fromUTF8(filename.get()));
        }
    }
}

void DataObjectGtk::setURL(const URL& url, const String& label)
{
    m_url = url;
    m_uriList = url;
    setText(url.string());

    String actualLabel(label);
    if (actualLabel.isEmpty())
        actualLabel = url;

    StringBuilder markup;
    markup.append("<a href=\"");
    markup.append(url.string());
    markup.append("\">");
    GUniquePtr<gchar> escaped(g_markup_escape_text(actualLabel.utf8().data(), -1));
    markup.append(String::fromUTF8(escaped.get()));
    markup.append("</a>");
    setMarkup(markup.toString());
}

void DataObjectGtk::clearText()
{
    m_text = emptyString();
}

void DataObjectGtk::clearMarkup()
{
    m_markup = emptyString();
}

String DataObjectGtk::urlLabel() const
{
    if (hasText())
        return text();

    if (hasURL())
        return url();

    return String();
}

void DataObjectGtk::clearAllExceptFilenames()
{
    m_text = emptyString();
    m_markup = emptyString();
    m_uriList = emptyString();
    m_url = URL();
    m_image = nullptr;
    m_unknownTypeData.clear();
}

void DataObjectGtk::clearAll()
{
    clearAllExceptFilenames();
    m_filenames.clear();
}

DataObjectGtk* DataObjectGtk::forClipboard(GtkClipboard* clipboard)
{
    static HashMap<GtkClipboard*, RefPtr<DataObjectGtk> > objectMap;

    if (!objectMap.contains(clipboard)) {
        RefPtr<DataObjectGtk> dataObject = DataObjectGtk::create();
        objectMap.set(clipboard, dataObject);
        return dataObject.get();
    }

    HashMap<GtkClipboard*, RefPtr<DataObjectGtk> >::iterator it = objectMap.find(clipboard);
    return it->value.get();
}

}
