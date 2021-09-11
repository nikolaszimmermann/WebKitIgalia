/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "config.h"
#include "SVGPathData.h"

#include "FloatLine.h"
#include "FloatRoundedRect.h"
#include "Path.h"
#include "RenderElement.h"
#include "RenderStyle.h"
#include "SVGCircleElement.h"
#include "SVGElementTypeHelpers.h"
#include "SVGEllipseElement.h"
#include "SVGLineElement.h"
#include "SVGNames.h"
#include "SVGPathElement.h"
#include "SVGPathUtilities.h"
#include "SVGPoint.h"
#include "SVGPointList.h"
#include "SVGPolygonElement.h"
#include "SVGPolylineElement.h"
#include "SVGRectElement.h"
#include <wtf/HashMap.h>
#include <wtf/TinyLRUCache.h>

namespace WebCore {

const Path& sharedClipAllPath()
{
    static LazyNeverDestroyed<Path> clipAllPath;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        clipAllPath.construct();
        clipAllPath.get().addRect(FloatRect());
    });
    return clipAllPath.get();
}

const Path& sharedEmptyPath()
{
    static NeverDestroyed<Path> emptyPath;
    return emptyPath;
}

struct SVGEllipsePathPolicy : public TinyLRUCachePolicy<FloatRect, Path> {
public:
    static bool isKeyNull(const FloatRect& rect) { return rect.isEmpty(); }

    static Path createValueForKey(const FloatRect& rect)
    {
        Path path;
        path.addEllipse(rect);
        return path;
    }
};

static const Path& cachedSVGEllipsePath(const FloatRect& rect)
{
    static NeverDestroyed<TinyLRUCache<FloatRect, Path, 4, SVGEllipsePathPolicy>> cache;
    return cache.get().get(rect);
}

static const Path& pathFromCircleElement(const SVGElement& element)
{
    ASSERT(is<SVGCircleElement>(element));

    auto* renderer = element.renderer();
    if (!renderer)
        return sharedEmptyPath();

    auto& style = renderer->style();
    const auto& lengthContext = element.lengthContext();
    float r = lengthContext.valueForLength(style.svgStyle().r());
    if (r > 0) {
        float cx = lengthContext.valueForLength(style.svgStyle().cx(), SVGLengthMode::Width);
        float cy = lengthContext.valueForLength(style.svgStyle().cy(), SVGLengthMode::Height);
        return cachedSVGEllipsePath(FloatRect(cx - r, cy - r, r * 2, r * 2));
    }
    return sharedEmptyPath();
}

static const Path& pathFromEllipseElement(const SVGElement& element)
{
    ASSERT(is<SVGEllipseElement>(element));

    auto* renderer = element.renderer();
    if (!renderer)
        return sharedEmptyPath();

    auto& style = renderer->style();
    const auto& lengthContext = element.lengthContext();
    float rx = lengthContext.valueForLength(style.svgStyle().rx(), SVGLengthMode::Width);
    if (rx <= 0)
        return sharedEmptyPath();

    float ry = lengthContext.valueForLength(style.svgStyle().ry(), SVGLengthMode::Height);
    if (ry <= 0)
        return sharedEmptyPath();

    float cx = lengthContext.valueForLength(style.svgStyle().cx(), SVGLengthMode::Width);
    float cy = lengthContext.valueForLength(style.svgStyle().cy(), SVGLengthMode::Height);
    return cachedSVGEllipsePath(FloatRect(cx - rx, cy - ry, rx * 2, ry * 2));
}

struct SVGLinePathPolicy : public TinyLRUCachePolicy<FloatLine, Path> {
public:
    static bool isKeyNull(const FloatLine& line) { return line.start().isZero() && line.end().isZero(); }

    static Path createValueForKey(const FloatLine& line)
    {
        Path path;
        path.moveTo(line.start());
        path.addLineTo(line.end());
        return path;
    }
};

static const Path& cachedSVGLinePath(const FloatLine& line)
{
    static NeverDestroyed<TinyLRUCache<FloatLine, Path, 4, SVGLinePathPolicy>> cache;
    return cache.get().get(line);
}

static const Path& pathFromLineElement(const SVGElement& element)
{
    ASSERT(is<SVGLineElement>(element));

    const auto& line = downcast<SVGLineElement>(element);

    const auto& lengthContext = line.lengthContext();
    FloatPoint start(line.x1().value(lengthContext), line.y1().value(lengthContext));
    FloatPoint end(line.x2().value(lengthContext), line.y2().value(lengthContext));
    return cachedSVGLinePath(FloatLine(start, end));
}

struct SVGPathByteStreamPolicy : TinyLRUCachePolicy<SVGPathByteStream, Path> {
public:
    static bool isKeyNull(const SVGPathByteStream& stream) { return stream.isEmpty(); }

    static Path createValueForKey(const SVGPathByteStream& stream) { return buildPathFromByteStream(stream); }
};

static const Path& pathFromPathElement(const SVGElement& element)
{
    ASSERT(is<SVGPathElement>(element));

    static NeverDestroyed<TinyLRUCache<SVGPathByteStream, Path, 4, SVGPathByteStreamPolicy>> cache;
    return cache.get().get(downcast<SVGPathElement>(element).pathByteStream());
}

template<bool closeSubpath>
struct SVGPolygonPathPolicy : TinyLRUCachePolicy<Vector<FloatPoint>, Path> {
public:
    static bool isKeyNull(const Vector<FloatPoint>& points) { return !points.size(); }

    static Path createValueForKey(const Vector<FloatPoint>& points)
    {
        Path path;
        path.moveTo(points.first());

        unsigned size = points.size();
        for (unsigned i = 1; i < size; ++i)
            path.addLineTo(points[i]);

        if (closeSubpath)
            path.closeSubpath();
        return path;
    }
};

template<bool closeSubpath>
static const Path& cachedSVGPolygonPath(const Vector<FloatPoint>& points)
{
    static NeverDestroyed<TinyLRUCache<Vector<FloatPoint>, Path, 4, SVGPolygonPathPolicy<closeSubpath>>> cache;
    return cache.get().get(points);
}

static const Path& pathFromPolygonElement(const SVGElement& element)
{
    ASSERT(is<SVGPolygonElement>(element));

    const Vector<FloatPoint>& points = downcast<SVGPolygonElement>(element).points();
    if (points.isEmpty())
        return sharedEmptyPath();

    return cachedSVGPolygonPath<true>(points);
}

static const Path& pathFromPolylineElement(const SVGElement& element)
{
    ASSERT(is<SVGPolylineElement>(element));

    const Vector<FloatPoint>& points = downcast<SVGPolylineElement>(element).points();
    if (points.isEmpty())
        return sharedEmptyPath();

    return cachedSVGPolygonPath<false>(points);
}

struct SVGRectPathPolicy : public TinyLRUCachePolicy<FloatRect, Path> {
public:
    static bool isKeyNull(const FloatRect& rect) { return rect.isEmpty(); }

    static Path createValueForKey(const FloatRect& rect)
    {
        Path path;
        path.addRect(rect);
        return path;
    }
};

static const Path& cachedSVGRectPath(const FloatRect& rect)
{
    static NeverDestroyed<TinyLRUCache<FloatRect, Path, 4, SVGRectPathPolicy>> cache;
    return cache.get().get(rect);
}

struct SVGRoundedSVGRectPathPolicy : public TinyLRUCachePolicy<FloatRoundedRect, Path> {
public:
    static bool isKeyNull(const FloatRoundedRect& rect) { return rect.isEmpty(); }

    static Path createValueForKey(const FloatRoundedRect& rect)
    {
        Path path;
        // FIXME: We currently enforce using beziers here, as at least on CoreGraphics/Lion, as
        // the native method uses a different line dash origin, causing svg/custom/dashOrigin.svg to fail.
        // See bug https://bugs.webkit.org/show_bug.cgi?id=79932 which tracks this issue.
        path.addRoundedRect(rect.rect(), FloatSize(rect.radii().topLeft().width(), rect.radii().topLeft().height()), Path::RoundedRectStrategy::PreferBezier);
        return path;
    }
};

static const Path& cachedSVGRoundedRectPath(const FloatRoundedRect& rect)
{
    static NeverDestroyed<TinyLRUCache<FloatRoundedRect, Path, 4, SVGRoundedSVGRectPathPolicy>> cache;
    return cache.get().get(rect);
}

static const Path& pathFromRectElement(const SVGElement& element)
{
    ASSERT(is<SVGRectElement>(element));

    auto* renderer = element.renderer();
    if (!renderer)
        return sharedEmptyPath();

    auto& style = renderer->style();
    const auto& lengthContext = element.lengthContext();
    float width = lengthContext.valueForLength(style.width(), SVGLengthMode::Width);
    if (width <= 0)
        return sharedEmptyPath();

    float height = lengthContext.valueForLength(style.height(), SVGLengthMode::Height);
    if (height <= 0)
        return sharedEmptyPath();

    float x = lengthContext.valueForLength(style.svgStyle().x(), SVGLengthMode::Width);
    float y = lengthContext.valueForLength(style.svgStyle().y(), SVGLengthMode::Height);
    float rx = lengthContext.valueForLength(style.svgStyle().rx(), SVGLengthMode::Width);
    float ry = lengthContext.valueForLength(style.svgStyle().ry(), SVGLengthMode::Height);
    bool hasRx = rx > 0;
    bool hasRy = ry > 0;
    if (hasRx || hasRy) {
        if (!hasRx)
            rx = ry;
        else if (!hasRy)
            ry = rx;
        return cachedSVGRoundedRectPath(FloatRoundedRect(FloatRect(x, y, width, height), FloatRoundedRect::Radii(rx, ry)));
    }

    return cachedSVGRectPath(FloatRect(x, y, width, height));
}

const Path& pathFromGraphicsElement(const SVGElement* element)
{
    ASSERT(element);

    if (!element)
        return sharedEmptyPath();

    typedef const Path& (*PathFromFunction)(const SVGElement&);
    static HashMap<AtomStringImpl*, PathFromFunction>* map = 0;
    if (!map) {
        map = new HashMap<AtomStringImpl*, PathFromFunction>;
        map->set(SVGNames::circleTag->localName().impl(), pathFromCircleElement);
        map->set(SVGNames::ellipseTag->localName().impl(), pathFromEllipseElement);
        map->set(SVGNames::lineTag->localName().impl(), pathFromLineElement);
        map->set(SVGNames::pathTag->localName().impl(), pathFromPathElement);
        map->set(SVGNames::polygonTag->localName().impl(), pathFromPolygonElement);
        map->set(SVGNames::polylineTag->localName().impl(), pathFromPolylineElement);
        map->set(SVGNames::rectTag->localName().impl(), pathFromRectElement);
    }

    if (auto pathFromFunction = map->get(element->localName().impl()))
        return (*pathFromFunction)(*element);

    return sharedEmptyPath();
}

} // namespace WebCore
