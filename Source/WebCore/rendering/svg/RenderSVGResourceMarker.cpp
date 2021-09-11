/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
#include "RenderSVGResourceMarker.h"

#include "GraphicsContext.h"
#include "RenderSVGResourceMarkerInlines.h"
#include "RenderSVGRoot.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/StackStats.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGResourceMarker);

RenderSVGResourceMarker::RenderSVGResourceMarker(SVGMarkerElement& element, RenderStyle&& style)
    : RenderSVGResourceContainer(element, WTFMove(style))
{
}

RenderSVGResourceMarker::~RenderSVGResourceMarker() = default;

LayoutRect RenderSVGResourceMarker::overflowClipRect(const LayoutPoint& location, RenderFragmentContainer*, OverlayScrollbarSizeRelevancy, PaintPhase) const
{
    auto viewportRect = m_viewportDimension;
    if (!m_supplementalLocalToParentTransform.isIdentity())
        viewportRect = m_supplementalLocalToParentTransform.inverse().value_or(AffineTransform()).mapRect(viewportRect);

    auto clipRect = enclosingLayoutRect(viewportRect);
    clipRect.moveBy(location);
    return clipRect;
}

void RenderSVGResourceMarker::updateFromStyle()
{
    RenderSVGResourceContainer::updateFromStyle();
    setHasSVGTransform();

    if (SVGRenderSupport::isOverflowHidden(*this))
        setHasNonVisibleOverflow();
}

void RenderSVGResourceMarker::updateLayerInformation()
{
    if (SVGRenderSupport::isRenderingDisabledDueToEmptySVGViewBox(*this))
        layer()->dirtyAncestorChainVisibleDescendantStatus();
}

void RenderSVGResourceMarker::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;

    // Invalidate all resources if our layout changed.
    if (selfNeedsClientInvalidation())
        RenderSVGRoot::addResourceForClientInvalidation(this);

    RenderSVGContainer::layout();
}

void RenderSVGResourceMarker::removeAllClientsFromCache(bool markForInvalidation)
{
    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceMarker::removeClientFromCache(RenderElement& client, bool markForInvalidation)
{
    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

FloatRect RenderSVGResourceMarker::computeMarkerBoundingBox(const SVGBoundingBoxComputation::DecorationOptions& options, const AffineTransform& markerTransformation) const
{
    SVGBoundingBoxComputation boundingBoxComputation(*this);
    auto boundingBox = boundingBoxComputation.computeDecoratedBoundingBox(options);

    // Map repaint rect into parent coordinate space, in which the marker boundaries have to be evaluated
    return markerTransformation.mapRect(m_supplementalLocalToParentTransform.mapRect(boundingBox));
}

FloatPoint RenderSVGResourceMarker::referencePoint() const
{
    const auto& lengthContext = markerElement().lengthContext();
    return FloatPoint(markerElement().refX().value(lengthContext), markerElement().refY().value(lengthContext));
}

float RenderSVGResourceMarker::angle() const
{
    float angle = -1;
    if (markerElement().orientType() == SVGMarkerOrientAngle)
        angle = markerElement().orientAngle().value();
    return angle;
}

AffineTransform RenderSVGResourceMarker::markerTransformation(const FloatPoint& origin, float autoAngle, float strokeWidth) const
{
    AffineTransform transform;
    transform.translate(origin);
    transform.rotate(markerElement().orientType() == SVGMarkerOrientAngle ? angle() : autoAngle);

    // The 'referencePoint()' coordinate maps to SVGs refX/refY, given in coordinates relative to the viewport established by the marker
    auto mappedOrigin = m_supplementalLocalToParentTransform.mapPoint(referencePoint());

    if (markerElement().markerUnits() == SVGMarkerUnitsStrokeWidth)
        transform.scaleNonUniform(strokeWidth, strokeWidth);

    transform.translate(-mappedOrigin);
    return transform;
}

void RenderSVGResourceMarker::calculateViewport()
{
    RenderSVGResourceContainer::calculateViewport();

    const auto& lengthContext = markerElement().lengthContext();
    m_viewportDimension = { 0, 0, markerElement().markerWidth().value(lengthContext), markerElement().markerHeight().value(lengthContext) };

    auto newTransform = markerElement().viewBoxToViewTransform(m_viewportDimension.width(), m_viewportDimension.height());
    if (newTransform != m_supplementalLocalToParentTransform)
        m_supplementalLocalToParentTransform = newTransform;
}

void RenderSVGResourceMarker::applyTransform(TransformationMatrix& transform, const RenderStyle& style, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption> options) const
{
    SVGRenderSupport::applyTransform(*this, transform, style, boundingBox, m_supplementalLocalToParentTransform, std::nullopt, options);
}

}
