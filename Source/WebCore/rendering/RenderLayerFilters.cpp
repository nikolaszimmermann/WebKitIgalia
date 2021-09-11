/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "RenderLayerFilters.h"

#include "CSSFilter.h"
#include "CachedSVGDocument.h"
#include "CachedSVGDocumentReference.h"
#include "Logging.h"
#include "RenderSVGResourceFilter.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

RenderLayerFilters::RenderLayerFilters(RenderLayer& layer)
    : m_layer(layer)
{
}

RenderLayerFilters::~RenderLayerFilters()
{
    removeReferenceFilterClients();
}

void RenderLayerFilters::setFilter(RefPtr<CSSFilter>&& filter)
{
    m_filter = WTFMove(filter);
}

bool RenderLayerFilters::hasFilterThatMovesPixels() const
{
    return m_filter && m_filter->hasFilterThatMovesPixels();
}

bool RenderLayerFilters::hasFilterThatShouldBeRestrictedBySecurityOrigin() const
{
    return m_filter && m_filter->hasFilterThatShouldBeRestrictedBySecurityOrigin();
}

void RenderLayerFilters::notifyFinished(CachedResource&, const NetworkLoadMetrics&)
{
    // FIXME: This really shouldn't have to invalidate layer composition,
    // but tests like css3/filters/effect-reference-delete.html fail if that doesn't happen.
    if (auto* enclosingElement = m_layer.enclosingElement())
        enclosingElement->invalidateStyleAndLayerComposition();
    m_layer.renderer().repaint();
}

void RenderLayerFilters::updateReferenceFilterClients(const FilterOperations& operations)
{
    removeReferenceFilterClients();

    for (auto& operation : operations.operations()) {
        if (!is<ReferenceFilterOperation>(*operation))
            continue;

        auto& referenceOperation = downcast<ReferenceFilterOperation>(*operation);
        auto* documentReference = referenceOperation.cachedSVGDocumentReference();
        if (auto* cachedSVGDocument = documentReference ? documentReference->document() : nullptr) {
            // Reference is external; wait for notifyFinished().
            cachedSVGDocument->addClient(*this);
            m_externalSVGReferences.append(cachedSVGDocument);
        } else {
            // Reference is internal; add layer as a client so we can trigger filter repaint on SVG attribute change.
            auto* filterElement = m_layer.renderer().document().getElementById(referenceOperation.fragment());
            if (!filterElement)
                continue;
            auto* renderer = filterElement->renderer();
            if (!is<RenderSVGResourceFilter>(renderer))
                continue;
            downcast<RenderSVGResourceFilter>(*renderer).addClientRenderLayer(&m_layer);
            m_internalSVGReferences.append(filterElement);
        }
    }
}

void RenderLayerFilters::removeReferenceFilterClients()
{
    for (auto& resourceHandle : m_externalSVGReferences)
        resourceHandle->removeClient(*this);

    m_externalSVGReferences.clear();

    for (auto& filterElement : m_internalSVGReferences) {
        if (auto* renderer = filterElement->renderer())
            downcast<RenderSVGResourceContainer>(*renderer).removeClientRenderLayer(&m_layer);
    }
    m_internalSVGReferences.clear();
}

void RenderLayerFilters::buildFilter(RenderLayerModelObject& renderer, float scaleFactor, RenderingMode renderingMode)
{
    bool isSVGRenderer = renderer.isSVGLayerAwareRenderer();
    if (isSVGRenderer)
        scaleFactor = 1.0f; // Page scaling is handled via setAbsoluteTransform() in beginFilterEffect() for SVG filters.

    if (!m_filter) {
        m_filter = CSSFilter::create();
        m_filter->setFilterScale(scaleFactor);
        m_filter->setRenderingMode(renderingMode);
    } else {
        // FIXME: For SVG we only want to reset the intermediate results if the absoluteTransform changes.
        // However we do not have access to the CTM of the current graphics context yet at this point.
        bool clearResults = isSVGRenderer;
        if (m_filter->filterScale() != scaleFactor) {
            m_filter->setFilterScale(scaleFactor);
            clearResults = true;
        }

        if (clearResults)
            m_filter->clearIntermediateResults();
    }

    // If the filter fails to build, remove it from the layer. It will still attempt to
    // go through regular processing (e.g. compositing), but never apply anything.
    // FIXME: this rebuilds the entire effects chain even if the filter style didn't change.
    if (!m_filter->build(renderer, renderer.style().filter(), isSVGRenderer ? FilterConsumer::SVGFilterFunction : FilterConsumer::FilterProperty)) {
        // SVG: A filter element without effect is still "valid" -- the target element will be hidden, instead of ignoring the filter.
        if (!isSVGRenderer)
            m_filter = nullptr;
    }
}

inline bool isTotalNumberOfEffectInputsSane(FilterEffect& effect)
{
    static const unsigned maxTotalOfEffectInputs = 100;
    return effect.totalNumberOfEffectInputs() <= maxTotalOfEffectInputs;
}

GraphicsContext* RenderLayerFilters::beginFilterEffect(GraphicsContext& destinationContext, const FloatRect& filterBoxRect, const FloatRect& filterTargetRect, const FloatRect& dirtyRect, const FloatRect& layerRepaintRect)
{
    if (!m_filter || !m_filter->hasEffects())
        return nullptr;

    if (!isTotalNumberOfEffectInputsSane(*m_filter->lastEffect()))
        return nullptr;

    bool isSVGRenderer = m_layer.renderer().isSVGLayerAwareRenderer();

    auto& filter = *m_filter;
    auto filterSourceRect = filter.computeSourceImageRectForDirtyRect(filterTargetRect, dirtyRect);
    // For SVG we need to continue processing with an empty filterTargetRect to support e.g. a <feTile> filter applied on a child-less <g>.
    if (isSVGRenderer) {
        bool hasEmptyBoundingBox = m_layer.renderer().objectBoundingBox().isEmpty();
        bool hasIntrinsicDimensions = !m_layer.renderer().isSVGTransformableContainer();
        if (hasEmptyBoundingBox && hasIntrinsicDimensions)
            return nullptr;
    } else if (filterSourceRect.isEmpty())
        return nullptr;

    AffineTransform absoluteTransform;
    FloatRect absoluteClampedFilterSourceRect = filterSourceRect;

    if (isSVGRenderer) {
        auto ctm = destinationContext.getCTM(GraphicsContext::DefinitelyIncludeDeviceScale);
        absoluteTransform = AffineTransform().scale(ctm.xScale(), ctm.yScale());

        ASSERT(filter.filterScale() == 1.0f);
        absoluteClampedFilterSourceRect = enclosingIntRect(absoluteTransform.mapRect(filterSourceRect));

        FloatSize filterResolution(1, 1);
        ImageBuffer::clampedSize(absoluteClampedFilterSourceRect.size(), filterResolution);
        absoluteClampedFilterSourceRect.scale(filterResolution);

        // Eventually we'll end up with a 4097px rect here due to enclosingIntRect() after the clamping, correct for that, otherwise the filter is not painted, as it exceeds the 4096 px limit.
        FloatSize scale(1, 1);
        if (ImageBuffer::sizeNeedsClamping(enclosingIntRect(absoluteClampedFilterSourceRect).size(), scale))
            filterResolution.scale(scale.width(), scale.height());

        bool clearResults = false;
        if (filter.filterResolution() != filterResolution) {
            filter.setFilterResolution(filterResolution);
            clearResults = true;
        }

        if (filter.absoluteTransform() != absoluteTransform) {
            filter.setAbsoluteTransform(absoluteTransform);
            clearResults = true;
        }

        if (clearResults)
            filter.clearIntermediateResults();
    }

    bool hasUpdatedBackingStore = false;

    if (isSVGRenderer) {
        hasUpdatedBackingStore = filter.updateBackingStoreRect(filterSourceRect);
        filter.setFilterRegion(filterBoxRect);
    } else if (!ImageBuffer::sizeNeedsClamping(filterSourceRect.size())) {
        hasUpdatedBackingStore = filter.updateBackingStoreRect(filterSourceRect);
        filter.setFilterRegion(filterSourceRect);
    }

    if (!filter.hasFilterThatMovesPixels())
        m_repaintRect = dirtyRect;
    else {
        if (hasUpdatedBackingStore)
            m_repaintRect = filterSourceRect;
        else {
            m_repaintRect = dirtyRect;
            m_repaintRect.unite(layerRepaintRect);
            m_repaintRect.intersect(filterSourceRect);
        }
    }
    m_paintOffset = absoluteClampedFilterSourceRect.location();
    resetDirtySourceRect();

    filter.determineFilterPrimitiveSubregion();

    // SVG: Do not early exit above if the filterSourceRect is empty, we might render an empty container which is filtered (e.g. using feTile).
    // In these cases we need to ensure that the filer primitive subregion is calculated, as in such a case, we still draw content, even
    // though the filterSourceRect is empty.
    if (isSVGRenderer && filterSourceRect.isEmpty())
        return nullptr;

#if ENABLE(DESTINATION_COLOR_SPACE_LINEAR_SRGB)
    auto colorSpace = isSVGRenderer ? DestinationColorSpace::LinearSRGB() : DestinationColorSpace::SRGB();
#else
    auto colorSpace = DestinationColorSpace::SRGB();
#endif

    filter.allocateBackingStoreIfNeeded(destinationContext, absoluteClampedFilterSourceRect.size(), colorSpace);

    auto* sourceGraphicsContext = filter.inputContext();
    if (!sourceGraphicsContext || filter.filterRegion().isEmpty())
        return nullptr;

    // Translate the context so that the contents of the layer is captured in the offscreen memory buffer.
    sourceGraphicsContext->save();
    sourceGraphicsContext->translate(-m_paintOffset);

    if (isSVGRenderer) {
        auto filterResolution = filter.filterResolution();
        if (filterResolution != FloatSize(1, 1))
            sourceGraphicsContext->scale(filterResolution);
        if (!absoluteTransform.isIdentity())
            sourceGraphicsContext->concatCTM(absoluteTransform);
    }

    sourceGraphicsContext->clearRect(m_repaintRect);
    if (!isSVGRenderer)
        sourceGraphicsContext->clip(m_repaintRect);

    return sourceGraphicsContext;
}

void RenderLayerFilters::applyFilterEffect(GraphicsContext& destinationContext)
{
    LOG_WITH_STREAM(Filters, stream << "\nRenderLayerFilters " << this << " applyFilterEffect");
    if (!m_filter || !m_filter->hasEffects())
        return;

    auto& filter = *m_filter;
    if (!isTotalNumberOfEffectInputsSane(*filter.lastEffect()))
        return;

    // SVG: The inputContext() might be nullptr, if the filterSourceRect is empty (feTile on empty <g/>).
    if (filter.inputContext())
        filter.inputContext()->restore();

    filter.apply();

    bool isSVGRenderer = m_layer.renderer().isSVGLayerAwareRenderer();

    // Get the filtered output and draw it in place.
    FloatRect destRect;
    if (isSVGRenderer)
        destRect = filter.lastEffect()->absolutePaintRect();
    else {
        destRect = filter.outputRect();
        destRect.moveBy(m_paintOffset);
    }

    if (auto* outputBuffer = filter.output()) {
        const auto& absoluteTransform = filter.absoluteTransform();

        if (isSVGRenderer) {
            auto previousTransform = destinationContext.getCTM();
            if (!absoluteTransform.isIdentity())
                destinationContext.concatCTM(absoluteTransform.inverse().value_or(AffineTransform()));

            auto filterResolution = filter.filterResolution();
            if (filterResolution != FloatSize(1, 1)) {
                destinationContext.scale(FloatSize(1 / filterResolution.width(), 1 / filterResolution.height()));

                auto absoluteSourceImageRect = filter.sourceImageRect();
                absoluteSourceImageRect.scale(filterResolution);
                destinationContext.translate(absoluteSourceImageRect.location() - enclosingIntRect(absoluteSourceImageRect).location());
            }

            // LOG_WITH_STREAM(Filters, stream << " filter image:\n" << outputBuffer->toDataURL("image/png") << "\n";);

            destinationContext.drawImageBuffer(*outputBuffer, destRect);
            destinationContext.setCTM(previousTransform);
        } else {
            destinationContext.drawImageBuffer(*outputBuffer, snapRectToDevicePixels(LayoutRect(destRect), m_layer.renderer().document().deviceScaleFactor()));
            ASSERT(absoluteTransform.isIdentity());
        }
    }

    filter.clearIntermediateResults();
    LOG_WITH_STREAM(Filters, stream << "RenderLayerFilters " << this << " applyFilterEffect done\n");
}

} // namespace WebCore
