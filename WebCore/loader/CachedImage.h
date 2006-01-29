/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef KHTML_CachedImage_h
#define KHTML_CachedImage_h

#include "CachedObject.h"
#include <khtml_settings.h>

namespace WebCore
{

class DocLoader;
class Cache;
class Image;

class CachedImage : public CachedObject
{
public:
    CachedImage(DocLoader*, const DOMString &url, KIO::CacheControl cachePolicy, time_t expireDate);
    virtual ~CachedImage();

    const Image& image() const;

    IntSize imageSize() const;    // returns the size of the complete image
    
    // This method indicates that the decoded frame of the image is fully available and that the image
    // is not the error image.
    bool isDecoded() const { return !isErrorImage() && imageSize() == decodedRect().size(); }
    IntRect decodedRect() const;  // returns the rectangle that represents the portion of the image that has been decoded already

    virtual void ref(CachedObjectClient*);
    virtual void deref(CachedObjectClient*);

    virtual void data(QBuffer&, bool atEnd);
    virtual void error(int code, const char* message);

    bool isErrorImage() const { return m_errorOccurred; }

    void setShowAnimations(KHTMLSettings::KAnimationAdvice);

    virtual bool schedule() const { return true; }

    void checkNotify();
    
    virtual bool isImage() const { return true; }

    void clear();
    
private:
    void notifyObservers(const IntRect&);

    Image* m_image;
    int m_dataSize;
    
    bool m_errorOccurred : 1;
    KHTMLSettings::KAnimationAdvice m_showAnimations : 2;

    friend class Cache;
};

}
#endif
