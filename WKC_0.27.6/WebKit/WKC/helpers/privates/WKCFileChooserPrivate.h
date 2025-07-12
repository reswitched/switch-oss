/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_FILECHOOSER_H_
#define _WKC_HELPERS_PRIVATE_FILECHOOSER_H_

#include "helpers/WKCFileChooser.h"
#include "helpers/WKCString.h"

#include "Vector.h"
#include "WTFString.h"

namespace WebCore {
class FileChooser;
} // namespace

namespace WKC {

class FileChooserWrap : public FileChooser {
WTF_MAKE_FAST_ALLOCATED;
public:
    FileChooserWrap(FileChooserPrivate& parent) : FileChooser(parent) {}
    ~FileChooserWrap() {}
};

class FileChooserPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    FileChooserPrivate(WebCore::FileChooser*);
    ~FileChooserPrivate();

    WebCore::FileChooser* webcore() const { return m_webcore; }
    FileChooser& wkc() { return m_wkc; }

    bool allowsMultipleFiles() const;
    void chooseFile(const String&);
    void chooseFiles(const String*, int);

public:
    static const int cMaxPath;

private:
    WebCore::FileChooser* m_webcore;
    FileChooserWrap m_wkc;

};

class StringVectorWrap : public StringVector {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StringVectorWrap(StringVectorPrivate* parent) : StringVector(parent) {}
    ~StringVectorWrap() {}
};

class StringVectorPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StringVectorPrivate(const WTF::Vector<WTF::String>&);
    ~StringVectorPrivate();

    size_t size() const;
    String& at(size_t index);

private:
    WTF::Vector<String> m_vector;
};


class FileChooserSettingsWrap : public FileChooserSettings {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FileChooserSettingsWrap(FileChooserSettingsPrivate* parent) : FileChooserSettings(parent) {}
    ~FileChooserSettingsWrap() {}
};

class FileChooserSettingsPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FileChooserSettingsPrivate(FileChooserPrivate*);
    ~FileChooserSettingsPrivate();

    bool allowsMultipleFiles() const { return m_allowsMultipleFiles; }
    StringVector& acceptMIMETypes() { return m_acceptMIMETypes; }

private:
    bool m_allowsMultipleFiles;
    StringVectorWrap m_acceptMIMETypes;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_FILECHOOSER_H_

