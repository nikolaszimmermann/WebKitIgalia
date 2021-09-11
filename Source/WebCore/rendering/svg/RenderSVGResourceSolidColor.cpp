/*
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
#include "RenderSVGResourceSolidColor.h"

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "RenderSVGShape.h"
#include "RenderStyle.h"
#include "RenderView.h"
#include "SVGRenderSupport.h"

namespace WebCore {

RenderSVGResourceSolidColor::RenderSVGResourceSolidColor() = default;

RenderSVGResourceSolidColor::~RenderSVGResourceSolidColor() = default;

bool RenderSVGResourceSolidColor::applyResource(RenderElement& renderer, const RenderStyle& style, GraphicsContext*& context, OptionSet<RenderSVGResourceMode> resourceMode)
{
    ASSERT(context);
    ASSERT(!resourceMode.isEmpty());

    const SVGRenderStyle& svgStyle = style.svgStyle();

    bool isRenderingClipOrMask = renderer.view().frameView().paintBehavior().contains(PaintBehavior::RenderingSVGClipOrMask);

    if (resourceMode.contains(RenderSVGResourceMode::ApplyToFill)) {
        if (isRenderingClipOrMask) {
            context->setAlpha(1);
            context->setFillRule(SVGRenderSupport::clipRuleForRenderer(renderer));
        } else {
            context->setAlpha(svgStyle.fillOpacity());
            context->setFillRule(svgStyle.fillRule());
        }

        context->setFillColor(style.colorByApplyingColorFilter(m_color));

        if (resourceMode.contains(RenderSVGResourceMode::ApplyToText))
            context->setTextDrawingMode(TextDrawingMode::Fill);
    } else if (resourceMode.contains(RenderSVGResourceMode::ApplyToStroke)) {
        // When rendering the mask for a RenderSVGResourceClipper, the stroke code path is never hit.
        ASSERT(!isRenderingClipOrMask);
        context->setAlpha(svgStyle.strokeOpacity());
        context->setStrokeColor(style.colorByApplyingColorFilter(m_color));

        SVGRenderSupport::applyStrokeStyleToContext(*context, style, renderer);

        if (resourceMode.contains(RenderSVGResourceMode::ApplyToText))
            context->setTextDrawingMode(TextDrawingMode::Stroke);
    }

    return true;
}

void RenderSVGResourceSolidColor::postApplyResource(RenderElement&, GraphicsContext*& context, OptionSet<RenderSVGResourceMode> resourceMode, const Path* path, const RenderSVGShape* shape)
{
    ASSERT(context);
    ASSERT(!resourceMode.isEmpty());

    if (resourceMode.contains(RenderSVGResourceMode::ApplyToFill)) {
        if (path)
            context->fillPath(*path);
        else if (shape)
            shape->fillShape(*context);
    }
    if (resourceMode.contains(RenderSVGResourceMode::ApplyToStroke)) {
        if (path)
            context->strokePath(*path);
        else if (shape)
            shape->strokeShape(*context);
    }
}

}
