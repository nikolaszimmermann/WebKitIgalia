/*
 * Copyright (C) 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2018 Adobe Systems Incorporated. All rights reserved.
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
#include "SVGRenderingContext.h"

#include "BasicShapes.h"
#include "Frame.h"
#include "FrameView.h"
#include "RenderLayer.h"
#include "RenderSVGImage.h"
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMasker.h"
#include "RenderSVGText.h"
#include "RenderView.h"
#include "SVGElementTypeHelpers.h"
#include "SVGGraphicsElement.h"
#include "SVGLengthContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "TransformState.h"
#include <wtf/MathExtras.h>

namespace WebCore {

float SVGRenderingContext::calculateScreenFontSizeScalingFactor(const RenderObject& renderer)
{
    // Walk up the render tree, accumulating transforms
    RenderLayer* layer = nullptr;
    if (renderer.hasLayer())
        layer = downcast<const RenderLayerModelObject>(renderer).layer();
    else
        layer = renderer.enclosingLayer();

    RenderLayer* stopAtLayer = nullptr;
    while (layer) {
        // We can stop at compositing layers, to match the backing resolution.
        if (layer->isComposited()) {
            stopAtLayer = layer->parent();
            break;
        }

        layer = layer->parent();
    }

    auto ctm = SVGRenderingContext::calculateAbsoluteTransformForRenderer(renderer, stopAtLayer ? &stopAtLayer->renderer() : nullptr);
    ctm.scale(renderer.document().deviceScaleFactor());
    return narrowPrecisionToFloat(std::hypot(ctm.xScale(), ctm.yScale()) / sqrtOfTwoDouble);
}

AffineTransform SVGRenderingContext::calculateAbsoluteTransformForRenderer(const RenderObject& renderer, const RenderLayerModelObject* stopAtRenderer, bool includeDeviceScaleFactor)
{
    TransformState transformState(TransformState::ApplyTransformDirection, FloatPoint());
    transformState.setTransformMatrixTracking(includeDeviceScaleFactor ? TransformState::TrackSVGScreenCTMMatrix : TransformState::TrackSVGCTMMatrix);

    const RenderLayerModelObject* repaintContainer = nullptr;
    if (stopAtRenderer && stopAtRenderer->parent()) {
        if (auto* enclosingLayer = stopAtRenderer->parent()->enclosingLayer())
            repaintContainer = &enclosingLayer->renderer();
    }

    renderer.mapLocalToContainer(repaintContainer, transformState, { UseTransforms, ApplyContainerFlip });
    transformState.flatten();

    auto transform = transformState.releaseTrackedTransform();
    if (!transform)
        return AffineTransform();

    return transform->toAffineTransform();
}

RefPtr<ImageBuffer> SVGRenderingContext::createImageBuffer(const FloatRect& targetRect, const AffineTransform& absoluteTransform, const DestinationColorSpace& colorSpace, RenderingMode renderingMode, const GraphicsContext* context)
{
    IntRect paintRect = enclosingIntRect(absoluteTransform.mapRect(targetRect));
    // Don't create empty ImageBuffers.
    if (paintRect.isEmpty())
        return nullptr;

    FloatSize scale;
    FloatSize clampedSize = ImageBuffer::clampedSize(paintRect.size(), scale);

#if USE(DIRECT2D)
    auto imageBuffer = ImageBuffer::create(clampedSize, renderingMode, context, 1, colorSpace, PixelFormat::BGRA8);
#else
    UNUSED_PARAM(context);
    auto imageBuffer = ImageBuffer::create(clampedSize, renderingMode, 1, colorSpace, PixelFormat::BGRA8);
#endif
    if (!imageBuffer)
        return nullptr;

    AffineTransform transform;
    transform.scale(scale).translate(-paintRect.location()).multiply(absoluteTransform);

    GraphicsContext& imageContext = imageBuffer->context();
    imageContext.concatCTM(transform);

    return imageBuffer;
}

RefPtr<ImageBuffer> SVGRenderingContext::createImageBuffer(const FloatRect& targetRect, const FloatRect& clampedRect, const DestinationColorSpace& colorSpace, RenderingMode renderingMode, const GraphicsContext* context)
{
    IntSize clampedSize = roundedIntSize(clampedRect.size());
    FloatSize unclampedSize = roundedIntSize(targetRect.size());

    // Don't create empty ImageBuffers.
    if (clampedSize.isEmpty())
        return nullptr;

#if USE(DIRECT2D)
    auto imageBuffer = ImageBuffer::create(clampedSize, renderingMode, context, 1, colorSpace, PixelFormat::BGRA8);
#else
    UNUSED_PARAM(context);
    auto imageBuffer = ImageBuffer::create(clampedSize, renderingMode, 1, colorSpace, PixelFormat::BGRA8);
#endif
    if (!imageBuffer)
        return nullptr;

    GraphicsContext& imageContext = imageBuffer->context();

    // Compensate rounding effects, as the absolute target rect is using floating-point numbers and the image buffer size is integer.
    imageContext.scale(unclampedSize / targetRect.size());

    return imageBuffer;
}

void SVGRenderingContext::clipToImageBuffer(GraphicsContext& context, const AffineTransform& absoluteTransform, const FloatRect& targetRect, RefPtr<ImageBuffer>& imageBuffer, bool safeToClear)
{
    if (!imageBuffer)
        return;

    FloatRect absoluteTargetRect = enclosingIntRect(absoluteTransform.mapRect(targetRect));

    // The mask image has been created in the absolute coordinate space, as the image should not be scaled.
    // So the actual masking process has to be done in the absolute coordinate space as well.
    context.concatCTM(absoluteTransform.inverse().value_or(AffineTransform()));
    context.clipToImageBuffer(*imageBuffer, absoluteTargetRect);
    context.concatCTM(absoluteTransform);

    // When nesting resources, with objectBoundingBox as content unit types, there's no use in caching the
    // resulting image buffer as the parent resource already caches the result.
    if (safeToClear)
        imageBuffer = nullptr;
}

void SVGRenderingContext::clear2DRotation(AffineTransform& transform)
{
    AffineTransform::DecomposedType decomposition;
    transform.decompose(decomposition);
    decomposition.angle = 0;
    transform.recompose(decomposition);
}

}
