/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2011 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGResourceClipper.h"

#include "ElementIterator.h"
#include "Frame.h"
#include "FrameView.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "IntRect.h"
#include "Logging.h"
#include "RenderSVGResourceClipperInlines.h"
#include "RenderSVGText.h"
#include "RenderStyle.h"
#include "RenderView.h"
#include "SVGClipPathElement.h"
#include "SVGContainerLayout.h"
#include "SVGElementTypeHelpers.h"
#include "SVGNames.h"
#include "SVGPathData.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGUseElement.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/SetForScope.h>
#include <wtf/text/TextStream.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGResourceClipper);

RenderSVGResourceClipper::RenderSVGResourceClipper(SVGClipPathElement& element, RenderStyle&& style)
    : RenderSVGResourceContainer(element, WTFMove(style))
{
}

RenderSVGResourceClipper::~RenderSVGResourceClipper() = default;

void RenderSVGResourceClipper::removeAllClientsFromCache(bool markForInvalidation)
{
    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceClipper::removeClientFromCache(RenderElement& client, bool markForInvalidation)
{
    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourceClipper::applyResource(RenderElement&, const RenderStyle&, GraphicsContext*&, OptionSet<RenderSVGResourceMode>)
{
    ASSERT_NOT_REACHED();
    return false;
}

SVGGraphicsElement* RenderSVGResourceClipper::shouldApplyPathClipping() const
{
    // If the current clip-path gets clipped itself, we have to fall back to masking.
    if (style().clipPath())
        return nullptr;

    SVGGraphicsElement* useGraphicsElement = nullptr;

    // If clip-path only contains one visible shape or path, we can use path-based clipping. Invisible
    // shapes don't affect the clipping and can be ignored. If clip-path contains more than one
    // visible shape, the additive clipping may not work, caused by the clipRule. EvenOdd
    // as well as NonZero can cause self-clipping of the elements.
    // See also http://www.w3.org/TR/SVG/painting.html#FillRuleProperty
    for (Node* childNode = clipPathElement().firstChild(); childNode; childNode = childNode->nextSibling()) {
        auto* renderer = childNode->renderer();
        if (!renderer)
            continue;
        // Only shapes or paths are supported for direct clipping. We need to fall back to masking for texts.
        if (is<RenderSVGText>(renderer))
            return nullptr;
        if (!is<SVGGraphicsElement>(*childNode))
            continue;
        auto& style = renderer->style();
        if (style.display() == DisplayType::None || style.visibility() != Visibility::Visible)
            continue;
        // Current shape in clip-path gets clipped too. Fall back to masking.
        if (style.clipPath())
            return nullptr;
        // Fallback to masking, if there is more than one clipping path.
        if (useGraphicsElement)
            return nullptr;
        useGraphicsElement = downcast<SVGGraphicsElement>(childNode);
    }

    return useGraphicsElement;
}

enum class ClippingMode {
    NoClipping,
    PathClipping,
    MaskClipping
};

static ClippingMode& currentClippingMode()
{
    static ClippingMode s_clippingMode { ClippingMode::NoClipping };
    return s_clippingMode;
}

void RenderSVGResourceClipper::applyPathClipping(GraphicsContext& context, const FloatRect& objectBoundingBox, SVGGraphicsElement& graphicsElement)
{
    ASSERT(hasLayer());
    ASSERT(layer()->isSelfPaintingLayer());

    ASSERT(currentClippingMode() == ClippingMode::NoClipping || currentClippingMode() == ClippingMode::MaskClipping);
    SetForScope<ClippingMode> switchClippingMode(currentClippingMode(), ClippingMode::PathClipping);

    auto* clipRendererPtr = graphicsElement.renderer();
    ASSERT(clipRendererPtr);
    ASSERT(clipRendererPtr->hasLayer());
    ASSERT(is<RenderSVGModelObject>(clipRendererPtr));
    auto& clipRenderer = downcast<RenderSVGModelObject>(*clipRendererPtr);

    AffineTransform clipPathTransform;
    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        clipPathTransform.translate(objectBoundingBox.location());
        clipPathTransform.scale(objectBoundingBox.size());
    }

    if (layer()->hasTransform())
        clipPathTransform.multiply(layer()->transform()->toAffineTransform());

    const auto& clipPath = clipRenderer.computeClipPath(clipPathTransform);
    auto windRule = clipRenderer.style().svgStyle().clipRule();

    // The SVG specification wants us to clip everything, if clip-path doesn't have a child.
    if (clipPath.isEmpty())
        context.clipPath(sharedClipAllPath(), windRule);
    else {
        auto ctm = context.getCTM();
        context.concatCTM(clipPathTransform);
        context.clipPath(clipPath, windRule);
        context.setCTM(ctm);
    }
}

void RenderSVGResourceClipper::applyMaskClipping(PaintInfo& paintInfo, const RenderLayerModelObject& targetRenderer, const FloatRect& objectBoundingBox)
{
    ASSERT(hasLayer());
    ASSERT(layer()->isSelfPaintingLayer());
    ASSERT(targetRenderer.hasLayer());

    ASSERT(currentClippingMode() == ClippingMode::NoClipping || currentClippingMode() == ClippingMode::MaskClipping);
    SetForScope<ClippingMode> switchClippingMode(currentClippingMode(), ClippingMode::MaskClipping);

    auto& context = paintInfo.context();
    GraphicsContextStateSaver stateSaver(context);

    if (style().clipPath()) {
        auto* resources = SVGResourcesCache::cachedResourcesForRenderer(*this);
        if (auto* clipper = resources ? resources->clipper() : nullptr)
            clipper->applyMaskClipping(paintInfo, targetRenderer, objectBoundingBox);
    }

    AffineTransform contentTransform;

    auto& clipPathElement = this->clipPathElement();
    if (clipPathElement.clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        contentTransform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        contentTransform.scale(objectBoundingBox.width(), objectBoundingBox.height());
    } else if (!targetRenderer.isSVGLayerAwareRenderer()) {
        contentTransform.translate(objectBoundingBox.x(), objectBoundingBox.y());
        contentTransform.scale(style().effectiveZoom());
    }

    // Figure out if we need to push a transparency layer to render our mask.
    bool pushTransparencyLayer = false;
    bool compositedMask = targetRenderer.hasLayer() && targetRenderer.layer()->hasCompositedMask();
    bool flattenCompositingLayers = paintInfo.paintBehavior.contains(PaintBehavior::FlattenCompositingLayers);

    // Switch to a paint behavior where all children of the <clipPath> are rendered using special constraints:
    // - fill-opacity/stroke-opacity/opacity set to 1
    // - masker/filter not applied when rendering the children
    // - fill is set to the initial fill paint server (solid, black)
    // - stroke is set to the initial stroke paint server (none)
    auto& frameView = view().frameView();
    auto oldBehavior = frameView.paintBehavior();
    frameView.setPaintBehavior(oldBehavior | PaintBehavior::RenderingSVGClipOrMask);

    if (!compositedMask || flattenCompositingLayers) {
        pushTransparencyLayer = true;

        context.setCompositeOperation(CompositeOperator::DestinationIn);
        context.beginTransparencyLayer(1);
        context.setCompositeOperation(CompositeOperator::SourceOver);
    }

    layer()->paintSVGResourceLayer(context, LayoutRect::infiniteRect(), contentTransform);

    if (pushTransparencyLayer)
        context.endTransparencyLayer();
    frameView.setPaintBehavior(oldBehavior);
}

bool RenderSVGResourceClipper::hitTestClipContent(const FloatRect& objectBoundingBox, const LayoutPoint& nodeAtPoint)
{
    auto point = nodeAtPoint;
    if (!SVGRenderSupport::pointInClippingArea(*this, point))
        return false;

    SVGHitTestCycleDetectionScope hitTestScope(*this);

    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform applyTransform;
        applyTransform.translate(objectBoundingBox.location());
        applyTransform.scale(objectBoundingBox.size());
        point = LayoutPoint(applyTransform.inverse().value_or(AffineTransform()).mapPoint(point));
    }

    HitTestResult result(toLayoutPoint(point - flooredLayoutPoint(this->objectBoundingBox().minXMinYCorner())));
    constexpr OptionSet<HitTestRequest::Type> hitType { HitTestRequest::Type::SVGClipContent, HitTestRequest::Type::DisallowUserAgentShadowContent };
    return layer()->hitTest(hitType, result.hitTestLocation(), result, nullptr);
}

FloatRect RenderSVGResourceClipper::resourceBoundingBox(const RenderObject& object)
{
    FloatRect targetBoundingBox = object.objectBoundingBox();

    // Resource was not layouted yet. Give back the boundingBox of the object.
    if (selfNeedsLayout())
        return targetBoundingBox;

    FloatRect clipRect = m_strokeBoundingBox;

    if (layer()->hasTransform())
        clipRect = layer()->currentTransform(RenderStyle::individualTransformOperations).mapRect(clipRect);

    if (clipPathElement().clipPathUnits() == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        AffineTransform contentTransform;
        contentTransform.translate(targetBoundingBox.location());
        contentTransform.scale(targetBoundingBox.size());
        clipRect = contentTransform.mapRect(clipRect);
    }

    return clipRect;
}

}
