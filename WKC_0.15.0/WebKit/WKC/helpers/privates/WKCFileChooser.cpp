/*
 * Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCFileChooser.h"
#include "helpers/privates/WKCFileChooserPrivate.h"

#include "FileChooser.h"

#include "WTFString.h"
#include "helpers/WKCString.h"

namespace WKC {

FileChooserPrivate::FileChooserPrivate(WebCore::FileChooser* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

FileChooserPrivate::~FileChooserPrivate()
{
}
bool
FileChooserPrivate::allowsMultipleFiles() const
{
    return m_webcore->settings().allowsMultipleFiles;
}

void
FileChooserPrivate::chooseFile(const String& str)
{
    m_webcore->chooseFile(str);
}

void
FileChooserPrivate::chooseFiles(const String* str, int items)
{
    WTF::Vector<WTF::String> ws;

    for (int i=0; i<items; i++) {
        ws.append(str[i]);
    }
    m_webcore->chooseFiles(ws);
}

const int FileChooserPrivate::cMaxPath = MAX_PATH;

////////////////////////////////////////////////////////////////////////////////

FileChooserSettingsPrivate::FileChooserSettingsPrivate(FileChooserPrivate* fileChooser)
     : m_allowsMultipleFiles(fileChooser->webcore()->settings().allowsMultipleFiles)
     , m_acceptMIMETypes(new StringVectorPrivate(fileChooser->webcore()->settings().acceptMIMETypes))
{
}

FileChooserSettingsPrivate::~FileChooserSettingsPrivate()
{
}

StringVectorPrivate::StringVectorPrivate(const WTF::Vector<WTF::String>& v)
     : m_vector()
{
    const size_t size = v.size();
    m_vector.grow(size);
    for (int i = 0; i < size; i++) {
        m_vector[i] = v[i];
    }
}

StringVectorPrivate::~StringVectorPrivate()
{
}

size_t
StringVectorPrivate::size() const
{
    return m_vector.size();
}

String&
StringVectorPrivate::at(size_t index)
{
    return m_vector.at(index);
}
 
FileChooser::FileChooser(FileChooserPrivate& parent)
    : m_private(parent)
{
}

FileChooser::~FileChooser()
{
}

bool
FileChooser::allowsMultipleFiles() const
{
    return m_private.allowsMultipleFiles();
}

void
FileChooser::chooseFile(const String& str)
{
    m_private.chooseFile(str);
}

void
FileChooser::chooseFiles(const String* str, int items)
{
    m_private.chooseFiles(str, items);
}

const int FileChooser::cMaxPath = FileChooserPrivate::cMaxPath;

FileChooserSettings*
FileChooserSettings::create(FileChooser* fileChooser)
{
    FileChooserSettingsPrivate* priv = new FileChooserSettingsPrivate(&fileChooser->priv());
    return new FileChooserSettings(priv);
}

void
FileChooserSettings::destroy(FileChooserSettings* self)
{
    delete self;
}

FileChooserSettings::FileChooserSettings(FileChooserSettingsPrivate* parent)
    : m_private(parent)
{
}

FileChooserSettings::~FileChooserSettings()
{
    delete m_private;
}

bool
FileChooserSettings::allowsMultipleFiles() const
{
    return m_private->allowsMultipleFiles();
}

StringVector&
FileChooserSettings::acceptMIMETypes() const
{
    return m_private->acceptMIMETypes();
}

StringVector::StringVector(StringVectorPrivate* parent)
    : m_private(parent)
{
}

StringVector::~StringVector()
{
    delete m_private;
}

size_t
StringVector::size() const
{
    return m_private->size();
}

String&
StringVector::at(size_t index) const
{
    return m_private->at(index);
}

} // namespace
