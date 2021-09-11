/*
 * Copyright (C) 2006 Apple Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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

#include "config.h"
#include "RenderSVGBlock.h"

#include "RenderLayer.h"
#include "RenderSVGResource.h"
#include "RenderView.h"
#include "SVGGraphicsElement.h"
#include "SVGRenderSupport.h"
#include "SVGResourcesCache.h"
#include "StyleInheritedData.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGBlock);

RenderSVGBlock::RenderSVGBlock(SVGGraphicsElement& element, RenderStyle&& style)
    : RenderBlockFlow(element, WTFMove(style))
{
}

void RenderSVGBlock::updateFromStyle()
{
    RenderBlockFlow::updateFromStyle();

    auto transform = graphicsElement().animatedLocalTransform();
    if (!transform.isIdentity())
        setHasSVGTransform();
}

void RenderSVGBlock::willBeDestroyed()
{
    SVGResourcesCache::clientDestroyed(*this);
    RenderBlockFlow::willBeDestroyed();
}

void RenderSVGBlock::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlockFlow::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(*this, diff, style());
}

void RenderSVGBlock::applyTransform(TransformationMatrix& transform, const RenderStyle& style, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption> options) const
{
    SVGRenderSupport::applyTransform(*this, transform, style, boundingBox, std::nullopt, std::nullopt, options);
}

void RenderSVGBlock::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, OptionSet<MapCoordinatesMode> mode, bool* wasFixed) const
{
    SVGRenderSupport::mapLocalToContainer(*this, repaintContainer, transformState, mode, wasFixed);
}

LayoutSize RenderSVGBlock::offsetFromContainer(RenderElement& container, const LayoutPoint&, bool*) const
{
    ASSERT_UNUSED(container, &container == this->container());
    ASSERT(!isInFlowPositioned());
    ASSERT(!isAbsolutelyPositioned());
    ASSERT(!isInline());
    return LayoutSize();
}

std::optional<LayoutRect> RenderSVGBlock::computeVisibleRectInContainer(const LayoutRect& rect, const RenderLayerModelObject* container, VisibleRectContext context) const
{
    return SVGRenderSupport::computeVisibleRectInContainer(*this, rect, container, context);
}

void RenderSVGBlock::absoluteRects(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset) const
{
    rects.append(snappedIntRect(LayoutRect(accumulatedOffset + location(), size())));
}

void RenderSVGBlock::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
    quads.append(localToAbsoluteQuad(FloatRect(objectBoundingBox()), UseTransforms, wasFixed));
}

LayoutRect RenderSVGBlock::clippedOverflowRect(const RenderLayerModelObject* repaintContainer, VisibleRectContext context) const
{
    if (style().visibility() != Visibility::Visible && !enclosingLayer()->hasVisibleContent())
        return LayoutRect();

    ASSERT(!view().frameView().layoutContext().isPaintOffsetCacheEnabled());
    return computeRect(visualOverflowRect(), repaintContainer, context);
}

}
