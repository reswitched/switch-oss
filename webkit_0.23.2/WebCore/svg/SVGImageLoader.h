/*
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
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

#ifndef SVGImageLoader_h
#define SVGImageLoader_h

#include "ImageLoader.h"

namespace WebCore {

class SVGImageElement;

class SVGImageLoader final : public ImageLoader {
public:
    explicit SVGImageLoader(SVGImageElement&);
    virtual ~SVGImageLoader();

private:
    virtual void dispatchLoadEvent() override;
    virtual String sourceURI(const AtomicString&) const override;
};

} // namespace WebCore

#endif // SVGImageLoader_h
