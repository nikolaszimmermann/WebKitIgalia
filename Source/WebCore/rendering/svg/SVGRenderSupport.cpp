/*
 * Copyright (C) 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2018 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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
#include "SVGRenderSupport.h"

#include "ElementAncestorIterator.h"
#include "NodeRenderStyle.h"
#include "RenderChildIterator.h"
#include "RenderElement.h"
#include "RenderGeometryMap.h"
#include "RenderIterator.h"
#include "RenderLayer.h"
#include "RenderSVGForeignObject.h"
#include "RenderSVGImage.h"
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMarker.h"
#include "RenderSVGResourceMasker.h"
#include "RenderSVGRoot.h"
#include "RenderSVGShape.h"
#include "RenderSVGText.h"
#include "RenderSVGTransformableContainer.h"
#include "RenderSVGViewportContainer.h"
#include "RenderView.h"
#include "SVGElementTypeHelpers.h"
#include "SVGGeometryElement.h"
#include "SVGMarkerElement.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGSVGElement.h"
#include "SVGUseElement.h"
#include "TransformOperations.h"
#include "TransformState.h"

namespace WebCore {

void SVGRenderSupport::mapLocalToContainer(const RenderElement& renderer, const RenderLayerModelObject* ancestorContainer, TransformState& transformState, OptionSet<MapCoordinatesMode> mode, bool* wasFixed)
{
    ASSERT(renderer.style().position() == PositionType::Static);

    if (ancestorContainer == &renderer)
        return;

    ASSERT(!renderer.view().frameView().layoutContext().isPaintOffsetCacheEnabled());

    bool ancestorSkipped;
    auto* container = renderer.container(ancestorContainer, ancestorSkipped);
    if (!container)
        return;

    ASSERT_UNUSED(ancestorSkipped, !ancestorSkipped);

    // If this box has a transform, it acts as a fixed position container for fixed descendants,
    // and may itself also be fixed position. So propagate 'fixed' up only if this box is fixed position.
    if (renderer.hasTransform())
        mode.remove(IsFixed);

    if (wasFixed)
        *wasFixed = mode.contains(IsFixed);

    auto containerOffset = renderer.offsetFromContainer(*container, LayoutPoint(transformState.mappedPoint()));

    bool preserve3D = mode & UseTransforms && (container->style().preserves3D() || renderer.style().preserves3D());
    if (mode & UseTransforms && renderer.shouldUseTransformFromContainer(container)) {
        TransformationMatrix t;
        renderer.getTransformFromContainer(container, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(containerOffset.width(), containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);

    mode.remove(ApplyContainerFlip);

    container->mapLocalToContainer(ancestorContainer, transformState, mode, wasFixed);
}

RenderSVGRoot* SVGRenderSupport::findTreeRootObject(RenderElement& start)
{
    return lineageOfType<RenderSVGRoot>(start).first();
}

const RenderSVGRoot* SVGRenderSupport::findTreeRootObject(const RenderElement& start)
{
    return lineageOfType<RenderSVGRoot>(start).first();
}

bool SVGRenderSupport::isOverflowHidden(const RenderElement& renderer)
{
    // RenderSVGRoot should never query for overflow state - it should always clip itself to the initial viewport size.
    ASSERT(!renderer.isDocumentElementRenderer());

    return renderer.style().overflowX() == Overflow::Hidden || renderer.style().overflowX() == Overflow::Scroll;
}

bool SVGRenderSupport::filtersForceContainerLayout(const RenderElement& renderer)
{
    // If any of this container's children need to be laid out, and a filter is applied
    // to the container, we need to repaint the entire container.
    if (!renderer.normalChildNeedsLayout())
        return false;

    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(renderer);
    if (!resources || !resources->filter())
        return false;

    return true;
}

FloatRect SVGRenderSupport::transformReferenceBox(const RenderElement& renderer, const SVGElement& element, const RenderStyle& style)
{
    switch (style.transformBox()) {
    case TransformBox::BorderBox:
        // For SVG elements without an associated CSS layout box, the used value for border-box is stroke-box.
    case TransformBox::StrokeBox:
        return renderer.strokeBoundingBox();
    case TransformBox::ContentBox:
        // For SVG elements without an associated CSS layout box, the used value for content-box is fill-box.
    case TransformBox::FillBox:
        return renderer.objectBoundingBox();
    case TransformBox::ViewBox:
        return FloatRect { { }, element.lengthContext().viewportSize() };
    }
    return { };
}

inline bool isPointInCSSClippingArea(const RenderLayerModelObject& renderer, const LayoutPoint& point)
{
    ASSERT(renderer.hasLayer());
    if (!renderer.hasLayer())
        return false;

    auto& layer = *renderer.layer();

    auto* clipPathOperation = renderer.style().clipPath();
    if (is<ShapeClipPathOperation>(clipPathOperation)) {
        auto& clipPath = downcast<ShapeClipPathOperation>(*clipPathOperation);
        auto referenceBox = layer.transformReferenceBox(clipPath.referenceBox(), LayoutSize(), LayoutRect());
        if (!referenceBox.contains(point))
            return false;
        return clipPath.pathForReferenceRect(referenceBox).contains(point, clipPath.windRule());
    }
    if (is<BoxClipPathOperation>(clipPathOperation)) {
        auto& clipPath = downcast<BoxClipPathOperation>(*clipPathOperation);
        auto referenceBox = layer.transformReferenceBox(clipPath.referenceBox(), LayoutSize(), LayoutRect());
        if (!referenceBox.contains(point))
            return false;
        return clipPath.pathForReferenceRect(FloatRoundedRect {referenceBox}).contains(point);
    }

    return true;
}

bool SVGRenderSupport::pointInClippingArea(const RenderLayerModelObject& renderer, const LayoutPoint& point)
{
    if (SVGHitTestCycleDetectionScope::isVisiting(renderer))
        return false;

    auto* clipPathOperation = renderer.style().clipPath();
    if (is<ShapeClipPathOperation>(clipPathOperation) || is<BoxClipPathOperation>(clipPathOperation))
        return isPointInCSSClippingArea(renderer, point);

    // We just take clippers into account to determine if a point is on the node. The Specification may
    // change later and we also need to check maskers.
    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(renderer);
    if (!resources)
        return true;

    if (auto* clipper = resources->clipper())
        return clipper->hitTestClipContent(renderer.objectBoundingBox(), point);

    return true;
}

void SVGRenderSupport::applyStrokeStyleToContext(GraphicsContext& context, const RenderStyle& style, const RenderElement& renderer)
{
    Element* element = renderer.element();
    if (!is<SVGElement>(element)) {
        ASSERT_NOT_REACHED();
        return;
    }

    const SVGRenderStyle& svgStyle = style.svgStyle();

    const auto& lengthContext = downcast<SVGElement>(renderer.element())->lengthContext();
    context.setStrokeThickness(lengthContext.valueForLength(style.strokeWidth()));
    context.setLineCap(style.capStyle());
    context.setLineJoin(style.joinStyle());
    if (style.joinStyle() == LineJoin::Miter)
        context.setMiterLimit(style.strokeMiterLimit());

    const Vector<SVGLengthValue>& dashes = svgStyle.strokeDashArray();
    if (dashes.isEmpty())
        context.setStrokeStyle(SolidStroke);
    else {
        DashArray dashArray;
        dashArray.reserveInitialCapacity(dashes.size());
        bool canSetLineDash = false;
        float scaleFactor = 1;

        if (is<SVGGeometryElement>(element)) {
            ASSERT(renderer.isSVGShape());
            // FIXME: A value of zero is valid. Need to differentiate this case from being unspecified.
            if (float pathLength = downcast<SVGGeometryElement>(element)->pathLength())
                scaleFactor = downcast<RenderSVGShape>(renderer).getTotalLength() / pathLength;
        }
        
        for (auto& dash : dashes) {
            dashArray.uncheckedAppend(dash.value(lengthContext) * scaleFactor);
            if (dashArray.last() > 0)
                canSetLineDash = true;
        }

        if (canSetLineDash)
            context.setLineDash(dashArray, lengthContext.valueForLength(svgStyle.strokeDashOffset()) * scaleFactor);
        else
            context.setStrokeStyle(SolidStroke);
    }
}

void SVGRenderSupport::styleChanged(RenderElement& renderer, const RenderStyle* oldStyle)
{
#if ENABLE(CSS_COMPOSITING)
    if (renderer.element() && renderer.element()->isSVGElement() && (!oldStyle || renderer.style().hasBlendMode() != oldStyle->hasBlendMode()))
        SVGRenderSupport::updateMaskedAncestorShouldIsolateBlending(renderer);
#else
    UNUSED_PARAM(renderer);
    UNUSED_PARAM(oldStyle);
#endif
}

#if ENABLE(CSS_COMPOSITING)

bool SVGRenderSupport::isolatesBlending(const RenderStyle& style)
{
    return style.svgStyle().isolatesBlending() || style.hasFilter() || style.hasBlendMode() || style.opacity() < 1.0f;
}

void SVGRenderSupport::updateMaskedAncestorShouldIsolateBlending(const RenderElement& renderer)
{
    ASSERT(renderer.element());
    ASSERT(renderer.element()->isSVGElement());

    for (auto& ancestor : ancestorsOfType<SVGGraphicsElement>(*renderer.element())) {
        auto* style = ancestor.computedStyle();
        if (!style || !isolatesBlending(*style))
            continue;
        if (style->svgStyle().hasMasker())
            ancestor.setShouldIsolateBlending(renderer.style().hasBlendMode());
        return;
    }
}

#endif

SVGHitTestCycleDetectionScope::SVGHitTestCycleDetectionScope(const RenderElement& element)
{
    m_element = element;
    auto result = visitedElements().add(*m_element);
    ASSERT_UNUSED(result, result.isNewEntry);
}

SVGHitTestCycleDetectionScope::~SVGHitTestCycleDetectionScope()
{
    bool result = visitedElements().remove(*m_element);
    ASSERT_UNUSED(result, result);
}

WeakHashSet<RenderElement>& SVGHitTestCycleDetectionScope::visitedElements()
{
    static NeverDestroyed<WeakHashSet<RenderElement>> s_visitedElements;
    return s_visitedElements;
}

bool SVGHitTestCycleDetectionScope::isEmpty()
{
    return visitedElements().computesEmpty();
}

bool SVGHitTestCycleDetectionScope::isVisiting(const RenderElement& element)
{
    return visitedElements().contains(element);
}

WindRule SVGRenderSupport::clipRuleForRenderer(const RenderElement& renderer)
{
    ASSERT(renderer.element());
    auto& element = *renderer.element();
    auto& style = renderer.style();

    WindRule clipRule = style.svgStyle().clipRule();
    if (element.hasTagName(SVGNames::useTag)) {
        auto& useElement = downcast<SVGUseElement>(element);
        auto* rendererClipChild = useElement.rendererClipChild();
        if (rendererClipChild && !useElement.hasAttributeWithoutSynchronization(SVGNames::clip_ruleAttr))
            clipRule = rendererClipChild->style().svgStyle().clipRule();
    }

    return clipRule;
}

bool SVGRenderSupport::shouldPaintHiddenRenderer(const RenderLayerModelObject& renderer)
{
    if (!renderer.hasLayer())
        return false;

    // SVG resource layers are only painted indirectly, via paintSVGResourceLayer().
    // Check if we're the child of a RenderSVGResourceContainer (<clipPath>, <mask>, ...).
    if (auto* resourceContainer = renderer.layer()->enclosingSVGResourceContainer()) {
        ASSERT(resourceContainer->hasLayer());
        if (!resourceContainer->layer()->isPaintingSVGResourceLayer())
            return false;

        // Only shapes, paths and texts are allowed for clipping.
        if (resourceContainer->isSVGResourceClipper()) {
            for (auto& childRenderer : lineageOfType<RenderElement>(renderer)) {
                // Stop checking the ancestor chain once we reached the enclosing RenderSVGResourceContainer.
                if (&childRenderer == resourceContainer)
                    break;

                auto* checkRenderer = &childRenderer;

                // If we encounter a <use> element check the referenced renderer.
                auto& childRendererElement = *childRenderer.element();
                if (childRendererElement.hasTagName(SVGNames::useTag)) {
                    if (!(checkRenderer = downcast<SVGUseElement>(childRendererElement).rendererClipChild()))
                        return false;
                }

                if (!is<RenderSVGShape>(*checkRenderer) && !is<RenderSVGText>(*checkRenderer))
                    return false;
            }
        }

        return true;
    }

    // Children of <defs> not associated with a RenderSVGResourceContainer (<clipPath>, <mask>, ...) are never allowed to paint.
    if (renderer.layer()->enclosingSVGHiddenContainer()) {
        // One exception is e.g. <feImage> referencing a <path> in a <defs> section. The <path> does not know that it is referenced
        // by the <feImage>, and we would normally return false here. If SVGFEImage::platformApplySoftware() calls paintSVGResourceLayer()
        // the currentSVGResourceLayer() points to the enclosing <filter> element. If that condition is fulfilled, we allow painting hidden
        // renderers.
        if (auto* currentSVGResourceLayer = RenderLayer::currentSVGResourceFilterLayer()) {
            ASSERT_UNUSED(currentSVGResourceLayer, is<RenderSVGResourceFilter>(currentSVGResourceLayer->renderer()));
            return true;
        }
        return false;
    }

    return true;
}

void SVGRenderSupport::paintSVGClippingMask(const RenderLayerModelObject& renderer, PaintInfo& paintInfo, RenderSVGResourceClipper* clipper, const FloatRect& objectBoundingBox)
{
    auto& style = renderer.style();
    auto& context = paintInfo.context();
    if (!paintInfo.shouldPaintWithinRoot(renderer) || style.visibility() != Visibility::Visible || paintInfo.phase != PaintPhase::ClippingMask || context.paintingDisabled())
        return;

    ASSERT(clipper);
    ASSERT(!renderer.isSVGLayerAwareRenderer());
    clipper->applyMaskClipping(paintInfo, renderer, objectBoundingBox);
}

void SVGRenderSupport::paintSVGClippingMask(const RenderLayerModelObject& renderer, PaintInfo& paintInfo)
{
    auto& style = renderer.style();
    auto& context = paintInfo.context();
    if (!paintInfo.shouldPaintWithinRoot(renderer) || style.visibility() != Visibility::Visible || paintInfo.phase != PaintPhase::ClippingMask || context.paintingDisabled())
        return;

    ASSERT(renderer.isSVGLayerAwareRenderer());
    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(renderer);
    if (auto clipper = resources ? resources->clipper() : nullptr)
        clipper->applyMaskClipping(paintInfo, renderer, renderer.objectBoundingBox());
}

void SVGRenderSupport::paintSVGMask(const RenderLayerModelObject& renderer, PaintInfo& paintInfo, const LayoutPoint& adjustedPaintOffset)
{
    auto& context = paintInfo.context();
    if (!paintInfo.shouldPaintWithinRoot(renderer) || paintInfo.phase != PaintPhase::Mask || context.paintingDisabled())
        return;

    auto* resources = SVGResourcesCache::cachedResourcesForRenderer(renderer);
    auto* masker = resources ? resources->masker() : nullptr;
    if (!masker)
        return;

    masker->applyMask(paintInfo, renderer, adjustedPaintOffset);
}

void SVGRenderSupport::updateLayerTransform(const RenderLayerModelObject& renderer)
{
    // Transform-origin depends on box size, so we need to update the layer transform after layout.
    if (renderer.hasLayer())
        renderer.layer()->updateTransform();
}

bool SVGRenderSupport::isRenderingDisabledDueToEmptySVGViewBox(const RenderLayerModelObject& renderer)
{
    // SVG: An empty viewBox disables rendering.
    if (!renderer.parent())
        return false;

    if (is<RenderSVGRoot>(renderer)) {
        auto& svgRoot = downcast<RenderSVGRoot>(renderer);
        return svgRoot.svgSVGElement().hasAttribute(SVGNames::viewBoxAttr) && svgRoot.svgSVGElement().hasEmptyViewBox();
    }

    if (is<RenderSVGViewportContainer>(renderer)) {
        auto& viewportContainer = downcast<RenderSVGViewportContainer>(renderer);
        return viewportContainer.svgSVGElement().hasAttribute(SVGNames::viewBoxAttr) && viewportContainer.svgSVGElement().hasEmptyViewBox();
    }

    if (is<RenderSVGResourceMarker>(renderer)) {
        auto& marker = downcast<RenderSVGResourceMarker>(renderer);
        return marker.markerElement().hasAttribute(SVGNames::viewBoxAttr) && marker.markerElement().hasEmptyViewBox();
    }

    return false;
}

std::optional<LayoutRect> SVGRenderSupport::computeVisibleRectInContainer(const RenderElement& renderer, const LayoutRect& rect, const RenderLayerModelObject* container, RenderObject::VisibleRectContext context)
{
    ASSERT(is<RenderSVGModelObject>(renderer) || is<RenderSVGBlock>(renderer));
    ASSERT(!renderer.style().hasInFlowPosition());

    ASSERT(!renderer.view().frameView().layoutContext().isPaintOffsetCacheEnabled());

    if (container == &renderer)
        return rect;

    bool containerIsSkipped;
    auto* localContainer = renderer.container(container, containerIsSkipped);
    if (!localContainer)
        return rect;

    ASSERT_UNUSED(containerIsSkipped, !containerIsSkipped);

    LayoutRect adjustedRect = rect;

    // Move to origin of local coordinate system, if this is the first call to computeVisibleRectInContainer() originating
    // from a SVG renderer (RenderSVGModelObject / RenderSVGBlock) or if we cross the boundary from HTML -> SVG via RenderSVGForeignObject.
    bool moveToOrigin = is<RenderSVGForeignObject>(renderer);
    if (context.options.contains(RenderObject::VisibleRectContextOption::TranslateToSVGRendererOrigin)) {
        context.options.remove(RenderObject::VisibleRectContextOption::TranslateToSVGRendererOrigin);
        moveToOrigin = true;
    }
    if (moveToOrigin)
        adjustedRect.moveBy(flooredLayoutPoint(renderer.objectBoundingBox().minXMinYCorner()));

    if (auto* transform = renderer.hasLayer() ? downcast<RenderLayerModelObject>(renderer).layer()->transform() : nullptr)
        adjustedRect = transform->mapRect(adjustedRect);

    return localContainer->computeVisibleRectInContainer(adjustedRect, container, context);
}

static inline void applySVGTransform(TransformationMatrix& transform, const AffineTransform& svgTransform)
{
    if (svgTransform.isIdentity())
        return;

    if (svgTransform.isIdentityOrTranslation()) {
        transform.translate(svgTransform.e(), svgTransform.f());
        return;
    }

    transform.multiply(TransformationMatrix(svgTransform));
}

void SVGRenderSupport::applyTransform(const RenderElement& renderer, TransformationMatrix& transform, const RenderStyle& style, const FloatRect& boundingBox, const std::optional<AffineTransform>& preApplySVGTransformMatrix, const std::optional<AffineTransform>& postApplySVGTransformMatrix, OptionSet<RenderStyle::TransformOperationOption> options)
{
    style.applyTransform(transform, boundingBox,
        [&](const TransformOperations& transformOperations) {
            if (preApplySVGTransformMatrix.has_value())
                applySVGTransform(transform, preApplySVGTransformMatrix.value());

            // CSS Transforms take precedence.
            if (transformOperations.size()) {
                for (const auto& operation : transformOperations.operations())
                    operation->apply(transform, boundingBox.size());
            } else if (is<SVGGraphicsElement>(renderer.element()))
                applySVGTransform(transform, downcast<SVGGraphicsElement>(*renderer.element()).animatedLocalTransform());

            if (postApplySVGTransformMatrix.has_value())
                applySVGTransform(transform, postApplySVGTransformMatrix.value());
        }, options);
}

SVGLayerTransformUpdater::SVGLayerTransformUpdater(RenderLayerModelObject& renderer)
    : m_renderer(renderer)
{
    if (!renderer.hasLayer())
        return;

    m_transformReferenceBox = renderer.layer()->transformReferenceBox();
    SVGRenderSupport::updateLayerTransform(m_renderer);
}

SVGLayerTransformUpdater::~SVGLayerTransformUpdater()
{
    if (!m_renderer.hasLayer())
        return;
    if (m_renderer.layer()->transformReferenceBox() == m_transformReferenceBox)
        return;

    SVGRenderSupport::updateLayerTransform(m_renderer);
}

}
