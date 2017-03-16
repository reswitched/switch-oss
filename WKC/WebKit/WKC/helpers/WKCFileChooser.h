/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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
 *
 */

#ifndef _WKC_HELPER_WKC_FILECHOOSER_H_
#define _WKC_HELPER_WKC_FILECHOOSER_H_

namespace WKC {
class String;
class FileChooserPrivate;
class FileChooserSettingsPrivate;
class StringVectorPrivate;
 
class WKC_API FileChooser {
public:
    bool allowsMultipleFiles() const;
    void chooseFile(const String&);
    void chooseFiles(const String*, int);

    FileChooserPrivate& priv() const { return m_private;} 

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    FileChooser(FileChooserPrivate&);
    ~FileChooser();

private:
    FileChooser(const FileChooser&);
    FileChooser& operator=(const FileChooser&);

public:
    static const int cMaxPath;

private:
    FileChooserPrivate& m_private;
};


class WKC_API StringVector {
public:
    size_t size() const;
    String& at(size_t index) const;

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    StringVector(StringVectorPrivate*);
    virtual ~StringVector();

private:
    StringVectorPrivate* m_private;
};

class WKC_API FileChooserSettings {
public:
    static FileChooserSettings* create(FileChooser* fileChooser);
    static void destroy(FileChooserSettings* self);

    bool allowsMultipleFiles() const;
    StringVector& acceptMIMETypes() const;

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    // Use create()/destroy() instead.
    FileChooserSettings(FileChooserSettingsPrivate*);
    virtual ~FileChooserSettings();

private:
    FileChooserSettingsPrivate* m_private;
};

} // namespace

#endif // _WKC_HELPER_WKC_FILECHOOSER_H_
