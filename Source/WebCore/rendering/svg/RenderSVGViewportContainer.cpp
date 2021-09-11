/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
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
#include "RenderSVGViewportContainer.h"

#include "GraphicsContext.h"
#include "RenderSVGRoot.h"
#include "RenderView.h"
#include "SVGContainerLayout.h"
#include "SVGElementTypeHelpers.h"
#include "SVGSVGElement.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGViewportContainer);

RenderSVGViewportContainer::RenderSVGViewportContainer(SVGSVGElement& element, RenderStyle&& style)
    : RenderSVGContainer(element, WTFMove(style))
{
}

SVGSVGElement& RenderSVGViewportContainer::svgSVGElement() const
{
    return downcast<SVGSVGElement>(RenderSVGContainer::element());
}

LayoutRect RenderSVGViewportContainer::overflowClipRect(const LayoutPoint& location, RenderFragmentContainer*, OverlayScrollbarSizeRelevancy, PaintPhase) const
{
    auto viewportRect = m_viewportDimension;
    if (!m_supplementalLocalToParentTransform.isIdentity())
        viewportRect = m_supplementalLocalToParentTransform.inverse().value_or(AffineTransform()).mapRect(viewportRect);

    auto clipRect = enclosingLayoutRect(viewportRect);
    clipRect.moveBy(location);
    return clipRect;
}

void RenderSVGViewportContainer::updateFromStyle()
{
    RenderSVGContainer::updateFromStyle();
    setHasSVGTransform();

    if (SVGRenderSupport::isOverflowHidden(*this))
        setHasNonVisibleOverflow();
}

void RenderSVGViewportContainer::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderSVGContainer::styleDidChange(diff, oldStyle);

    if (!hasLayer())
        return;

    // SVG2 only requires a CSS stacking context if the inner <svg> element has overflow == hidden,
    // in order to enforce the viewBox for child layers, we do need an internal stacking context nevertheless.
    if (!SVGRenderSupport::isOverflowHidden(*this))
        layer()->setIsOpportunisticStackingContext(true);
}

void RenderSVGViewportContainer::updateLayerInformation()
{
    if (SVGRenderSupport::isRenderingDisabledDueToEmptySVGViewBox(*this))
        layer()->dirtyAncestorChainVisibleDescendantStatus();
}

void RenderSVGViewportContainer::layoutChildren()
{
    RenderSVGContainer::layoutChildren();
    m_didTransformToRootUpdate = false;
}

void RenderSVGViewportContainer::calculateViewport()
{
    RenderSVGContainer::calculateViewport();

    SVGSVGElement& useSVGSVGElement = svgSVGElement();
    auto previousViewportDimension = m_viewportDimension;

    const auto& lengthContext = useSVGSVGElement.lengthContext();
    auto x = useSVGSVGElement.x().value(lengthContext);
    auto y = useSVGSVGElement.y().value(lengthContext);
    auto width = useSVGSVGElement.width().value(lengthContext);
    auto height = useSVGSVGElement.height().value(lengthContext);
    m_viewportDimension = { x, y, width, height };
    m_isLayoutSizeChanged = useSVGSVGElement.hasRelativeLengths();

    AffineTransform newTransform;
    newTransform.translate(x, y);
    if (!useSVGSVGElement.currentViewBoxRect().isEmpty())
        newTransform.multiply(useSVGSVGElement.viewBoxToViewTransform(m_viewportDimension.width(), m_viewportDimension.height()));

    if (newTransform != m_supplementalLocalToParentTransform) {
        m_supplementalLocalToParentTransform = newTransform;
        m_didTransformToRootUpdate = true;
    } else if (previousViewportDimension != m_viewportDimension)
        m_didTransformToRootUpdate = true;

    if (!m_didTransformToRootUpdate)
        m_didTransformToRootUpdate = SVGContainerLayout::transformToRootChanged(parent());
}

bool RenderSVGViewportContainer::pointIsInsideViewportClip(const FloatPoint& pointInParent)
{
    // Respect the viewport clip (which is in parent coords)
    if (!SVGRenderSupport::isOverflowHidden(*this))
        return true;
    
    return m_viewportDimension.contains(pointInParent);
}

void RenderSVGViewportContainer::applyTransform(TransformationMatrix& transform, const RenderStyle& style, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption> options) const
{
    SVGRenderSupport::applyTransform(*this, transform, style, boundingBox, m_supplementalLocalToParentTransform.isIdentity() ? std::nullopt : std::make_optional(m_supplementalLocalToParentTransform), std::nullopt, options);
}

}
