/**
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#pragma once

#include "PaintInfo.h"
#include "RenderObject.h"

namespace WebCore {

class FloatPoint;
class FloatRect;
class ImageBuffer;
class LayoutRect;
class RenderBoxModelObject;
class RenderElement;
class RenderGeometryMap;
class RenderLayerModelObject;
class RenderStyle;
class RenderSVGResourceClipper;
class RenderSVGRoot;
class SVGElement;
class TransformState;

// SVGRendererSupport is a helper class sharing code between all SVG renderers.
class SVGRenderSupport {
public:
    // Helper function determining wheter overflow is hidden
    static bool isOverflowHidden(const RenderElement&);

    // Determines whether a container needs to be laid out because it's filtered and a child is being laid out.
    static bool filtersForceContainerLayout(const RenderElement&);

    // Determines whether the passed point lies in a clipping area
    static bool pointInClippingArea(const RenderLayerModelObject&, const LayoutPoint&);

    static void mapLocalToContainer(const RenderElement& container, const RenderLayerModelObject* ancestorContainer, TransformState&, OptionSet<MapCoordinatesMode>, bool* wasFixed);

    // Shared between SVG renderers and resources.
    static void applyStrokeStyleToContext(GraphicsContext&, const RenderStyle&, const RenderElement&);

    static void styleChanged(RenderElement&, const RenderStyle*);
    
    static FloatRect transformReferenceBox(const RenderElement&, const SVGElement&, const RenderStyle&);

#if ENABLE(CSS_COMPOSITING)
    static bool isolatesBlending(const RenderStyle&);
    static void updateMaskedAncestorShouldIsolateBlending(const RenderElement&);
#endif

    static RenderSVGRoot* findTreeRootObject(RenderElement&);
    static const RenderSVGRoot* findTreeRootObject(const RenderElement&);

    static bool shouldPaintHiddenRenderer(const RenderLayerModelObject&);

    static WindRule clipRuleForRenderer(const RenderElement&);
    static void paintSVGClippingMask(const RenderLayerModelObject&, PaintInfo&, RenderSVGResourceClipper*, const FloatRect& objectBoundingBox);
    static void paintSVGClippingMask(const RenderLayerModelObject&, PaintInfo&);

    static void paintSVGMask(const RenderLayerModelObject&, PaintInfo&, const LayoutPoint& adjustedPaintOffset);

    static void updateLayerTransform(const RenderLayerModelObject&);
    static bool isRenderingDisabledDueToEmptySVGViewBox(const RenderLayerModelObject&);

    static std::optional<LayoutRect> computeVisibleRectInContainer(const RenderElement&, const LayoutRect&, const RenderLayerModelObject* container, RenderObject::VisibleRectContext);

    static void applyTransform(const RenderElement&, TransformationMatrix&, const RenderStyle&, const FloatRect& boundingBox, const std::optional<AffineTransform>& preApplySVGTransformMatrix, const std::optional<AffineTransform>& postApplySVGTransformMatrix, OptionSet<RenderStyle::TransformOperationOption>);

private:
    // This class is not constructable.
    SVGRenderSupport();
    ~SVGRenderSupport();
};

class SVGHitTestCycleDetectionScope {
    WTF_MAKE_NONCOPYABLE(SVGHitTestCycleDetectionScope);
public:
    explicit SVGHitTestCycleDetectionScope(const RenderElement&);
    ~SVGHitTestCycleDetectionScope();
    static bool isEmpty();
    static bool isVisiting(const RenderElement&);

private:
    static WeakHashSet<RenderElement>& visitedElements();
    WeakPtr<RenderElement> m_element;
};

class SVGLayerTransformUpdater {
    WTF_MAKE_NONCOPYABLE(SVGLayerTransformUpdater);
public:
    SVGLayerTransformUpdater(RenderLayerModelObject&);
    ~SVGLayerTransformUpdater();

private:
    RenderLayerModelObject& m_renderer;
    FloatRect m_transformReferenceBox;
};

} // namespace WebCore
