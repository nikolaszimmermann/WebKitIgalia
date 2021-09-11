/**
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2012 Zoltan Herczeg <zherczeg@webkit.org>.
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

#include "ImageBuffer.h"
#include "PaintInfo.h"

namespace WebCore {

class AffineTransform;
class FloatRect;
class RenderElement;
class RenderObject;
class RenderSVGResourceFilter;

// SVGRenderingContext 
class SVGRenderingContext {
public:
    static RefPtr<ImageBuffer> createImageBuffer(const FloatRect& targetRect, const AffineTransform& absoluteTransform, const DestinationColorSpace&, RenderingMode, const GraphicsContext* = nullptr);
    static RefPtr<ImageBuffer> createImageBuffer(const FloatRect& targetRect, const FloatRect& clampedRect, const DestinationColorSpace&, RenderingMode, const GraphicsContext* = nullptr);

    static void clipToImageBuffer(GraphicsContext&, const AffineTransform& absoluteTransform, const FloatRect& targetRect, RefPtr<ImageBuffer>&, bool safeToClear);

    static float calculateScreenFontSizeScalingFactor(const RenderObject&);
    static AffineTransform calculateAbsoluteTransformForRenderer(const RenderObject& renderer, const RenderLayerModelObject* stopAtRenderer, bool includeDeviceScaleFactor = true);
    static void clear2DRotation(AffineTransform&);
};

} // namespace WebCore
