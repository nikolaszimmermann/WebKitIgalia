/*
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
#include "RenderSVGResourceMasker.h"

#include "Element.h"
#include "ElementIterator.h"
#include "FloatPoint.h"
#include "Image.h"
#include "IntRect.h"
#include "RenderSVGResourceMaskerInlines.h"
#include "SVGContainerLayout.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGResourceMasker);

RenderSVGResourceMasker::RenderSVGResourceMasker(SVGMaskElement& element, RenderStyle&& style)
    : RenderSVGResourceContainer(element, WTFMove(style))
{
}

RenderSVGResourceMasker::~RenderSVGResourceMasker() = default;

void RenderSVGResourceMasker::removeAllClientsFromCache(bool markForInvalidation)
{
    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceMasker::removeClientFromCache(RenderElement& client, bool markForInvalidation)
{
    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourceMasker::applyResource(RenderElement&, const RenderStyle&, GraphicsContext*&, OptionSet<RenderSVGResourceMode>)
{
    ASSERT_NOT_REACHED();
    return false;
}

void RenderSVGResourceMasker::applyMask(PaintInfo& paintInfo, const RenderLayerModelObject& targetRenderer, const LayoutPoint& adjustedPaintOffset)
{
    ASSERT(hasLayer());
    ASSERT(layer()->isSelfPaintingLayer());
    ASSERT(targetRenderer.hasLayer());

    auto& context = paintInfo.context();
    GraphicsContextStateSaver stateSaver(context);

    auto objectBoundingBox = targetRenderer.objectBoundingBox();
    auto boundingBoxTopLeftCorner = flooredLayoutPoint(objectBoundingBox.minXMinYCorner());
    auto coordinateSystemOriginTranslation = adjustedPaintOffset - boundingBoxTopLeftCorner;
    if (!coordinateSystemOriginTranslation.isZero())
        context.translate(coordinateSystemOriginTranslation);

    AffineTransform contentTransform;
    auto& maskElement = this->maskElement();
    if (maskElement.maskContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        contentTransform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        contentTransform.scale(objectBoundingBox.width(), objectBoundingBox.height());
    }

    auto repaintBoundingBox = targetRenderer.repaintBoundingBox();
    auto absoluteTransform = context.getCTM(GraphicsContext::DefinitelyIncludeDeviceScale);

    auto maskColorSpace = DestinationColorSpace::SRGB();
    auto drawColorSpace = DestinationColorSpace::SRGB();

    const auto& svgStyle = style().svgStyle();
#if ENABLE(DESTINATION_COLOR_SPACE_LINEAR_SRGB)
    if (svgStyle.colorInterpolation() == ColorInterpolation::LinearRGB) {
#if USE(CG)
        maskColorSpace = DestinationColorSpace::LinearSRGB();
#endif
        drawColorSpace = DestinationColorSpace::LinearSRGB();
    }
#endif

    auto maskImage = SVGRenderingContext::createImageBuffer(repaintBoundingBox, absoluteTransform, maskColorSpace, context.renderingMode());
    if (!maskImage)
        return;

    context.setCompositeOperation(CompositeOperator::DestinationIn);
    context.beginTransparencyLayer(1);

    auto& maskImageContext = maskImage->context();
    layer()->paintSVGResourceLayer(maskImageContext, LayoutRect::infiniteRect(), contentTransform);

#if !USE(CG)
    maskImage->transformToColorSpace(drawColorSpace);
#else
    UNUSED_PARAM(drawColorSpace);
#endif

    if (svgStyle.maskType() == MaskType::Luminance)
        maskImage->convertToLuminanceMask();

    context.setCompositeOperation(CompositeOperator::SourceOver);

    // The mask image has been created in the absolute coordinate space, as the image should not be scaled.
    // So the actual masking process has to be done in the absolute coordinate space as well.
    FloatRect absoluteTargetRect = enclosingIntRect(absoluteTransform.mapRect(repaintBoundingBox));
    context.concatCTM(absoluteTransform.inverse().value_or(AffineTransform()));
    context.drawConsumingImageBuffer(WTFMove(maskImage), absoluteTargetRect);
    context.endTransparencyLayer();
}

FloatRect RenderSVGResourceMasker::resourceBoundingBox(const RenderObject& object)
{
    FloatRect targetBoundingBox = object.objectBoundingBox();

    // Resource was not layouted yet. Give back the boundingBox of the object.
    if (selfNeedsLayout())
        return targetBoundingBox;

    auto& maskElement = this->maskElement();

    FloatRect maskRect = m_strokeBoundingBox;
    if (maskElement.maskContentUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform contentTransform;
        contentTransform.translate(targetBoundingBox.location());
        contentTransform.scale(targetBoundingBox.size());
        maskRect = contentTransform.mapRect(maskRect);
    }

    FloatRect maskBoundaries = SVGLengthContext::resolveRectangle<SVGMaskElement>(maskElement, maskElement.maskUnits(), targetBoundingBox);
    maskRect.intersect(maskBoundaries);
    return maskRect;
}

}
