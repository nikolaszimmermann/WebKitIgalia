/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <qrect.h>

#define MAX(a,b) ((a) >= (b) ? (a) : (b))


QRect::QRect() : xp(0), yp(0), w(1), h(1)
{
}

QRect::QRect(int x, int y, int width, int height) : xp(x), yp(y), w(width), h(height)
{
}

QRect::QRect(QPoint p, QSize s) : xp(p.x()), yp(p.y()), w(s.width()), h(s.height())
{
}

bool QRect::isNull() const
{
    return w == 0 && h == 0;
}

bool QRect::isValid() const
{
    return w > 0 && h > 0;
}

int QRect::x() const
{
    return xp;
}

int QRect::y() const
{
    return yp;
}

int QRect::left() const
{
    return xp;
}

int QRect::top() const
{
    return yp;
}

int QRect::right() const
{
    return xp + w - 1;
}

int QRect::bottom() const
{
    return yp + h - 1;
}

int QRect::width() const
{
    return w;
}

int QRect::height() const
{
    return h;
}

QPoint QRect::topLeft() const
{
    return QPoint(xp,yp);
}

QSize QRect::size() const
{
    return QSize(w,h);
}

void QRect::setWidth(int width)
{
    w = width;
}

void QRect::setHeight(int height)
{
    h = height;
}

QRect QRect::intersect(const QRect &r) const
{
    return operator&(r);
}

bool QRect::intersects(const QRect &r) const
{
    return (intersect(r)).isValid();
}

QRect QRect::operator&(const QRect &r) const
{
    int nx, ny, nw, nh;

    nx = MAX(xp, r.xp);
    ny = MAX(yp, r.yp);

    if (xp + w <= r.xp + r.w) {
        nw = xp + w - nx;
    } else {
        nw = r.xp + r.w - nx; 
    }

    if (yp + h <= r.yp + r.h) {
        nh = yp + h - ny;
    } else {
        nh = r.yp + r.h - ny; 
    }

    return QRect(nx, ny, nw, nh);
}

bool operator==(const QRect &a, const QRect &b)
{
	return a.xp == b.xp && a.yp == b.yp && a.w == b.w && a.h == b.h;
}

bool operator!=(const QRect &a, const QRect &b)
{
	return a.xp != b.xp || a.yp != b.yp || a.w != b.w || a.h != b.h;
}

#ifdef _KWQ_IOSTREAM_
ostream &operator<<(ostream &o, const QRect &r)
{
    return o << "QRect: [left: " << r.left () << "; top: " << r.top() << "; right: " << r.right() << "; bottom: " << r.bottom() << "]";
}
#endif


