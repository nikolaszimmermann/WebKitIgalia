/*
 * Copyright (C) 2006 Apple Inc.
 * Copyright (C) 2009 Google, Inc.
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
#include "RenderSVGForeignObject.h"

#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "LayoutRepainter.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RenderSVGBlockInlines.h"
#include "RenderSVGResource.h"
#include "RenderView.h"
#include "SVGElementTypeHelpers.h"
#include "SVGForeignObjectElement.h"
#include "SVGLogger.h"
#include "SVGRenderSupport.h"
#include "SVGRenderingContext.h"
#include "SVGResourcesCache.h"
#include "TransformState.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/StackStats.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGForeignObject);

RenderSVGForeignObject::RenderSVGForeignObject(SVGForeignObjectElement& element, RenderStyle&& style)
    : RenderSVGBlock(element, WTFMove(style))
{
}

RenderSVGForeignObject::~RenderSVGForeignObject() = default;

SVGForeignObjectElement& RenderSVGForeignObject::foreignObjectElement() const
{
    return downcast<SVGForeignObjectElement>(RenderSVGBlock::graphicsElement());
}

void RenderSVGForeignObject::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.context().paintingDisabled())
        return;

#if !defined(NDEBUG)
    SVGLogger::DebugScope debugScope(
        [&](TextStream& stream) {
            stream << renderName() << " " << this << " -> begin paint"
            << " (paintOffset=" << paintOffset
            << ", location=" << location()
            << ", objectBoundingBox=" << objectBoundingBox()
            << ", context.getCTM()=" << paintInfo.context().getCTM() << ")";
        },
        [&](TextStream& stream) {
            stream << renderName() << " " << this << " <- end paint";
        });
#endif

    if (!SVGRenderSupport::shouldPaintHiddenRenderer(*this))
        return;

    if (paintInfo.phase == PaintPhase::ClippingMask) {
        SVGRenderSupport::paintSVGClippingMask(*this, paintInfo);
        return;
    }

    auto adjustedPaintOffset = paintOffset + location();
    if (paintInfo.phase == PaintPhase::Mask) {
        SVGRenderSupport::paintSVGMask(*this, paintInfo, adjustedPaintOffset);
        return;
    }

    GraphicsContextStateSaver stateSaver(paintInfo.context());

    auto coordinateSystemOriginTranslation = adjustedPaintOffset - flooredLayoutPoint(objectBoundingBox().location());
    paintInfo.context().translate(coordinateSystemOriginTranslation.width(), coordinateSystemOriginTranslation.height());

    RenderSVGBlock::paint(paintInfo, paintOffset);
}

void RenderSVGForeignObject::updateLogicalWidth()
{
    setWidth(enclosingLayoutRect(m_viewport).width());
}

RenderBox::LogicalExtentComputedValues RenderSVGForeignObject::computeLogicalHeight(LayoutUnit, LayoutUnit logicalTop) const
{
    return { enclosingLayoutRect(m_viewport).height(), logicalTop, ComputedMarginValues() };
}

void RenderSVGForeignObject::layout()
{
#if !defined(NDEBUG)
    SVGLogger::DebugScope debugScope(
        [&](TextStream& stream) {
            stream << renderName() << " " << this << " -> begin layout"
            << " (selfNeedsLayout=" << selfNeedsLayout()
            << ", needsLayout=" << needsLayout() << ")";
        },
        [&](TextStream& stream) {
            stream << renderName() << " " << this << " <- end layout";
        });
#endif

    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());

    foreignObjectElement().updateLengthContext();
    const auto& lengthContext = foreignObjectElement().lengthContext();

    // Cache viewport boundaries
    auto x = foreignObjectElement().x().value(lengthContext);
    auto y = foreignObjectElement().y().value(lengthContext);
    auto width = foreignObjectElement().width().value(lengthContext);
    auto height = foreignObjectElement().height().value(lengthContext);
    m_viewport = { 0, 0, width, height };

    m_supplementalLocalToParentTransform.makeIdentity();
    m_supplementalLocalToParentTransform.translate(x, y);

    bool layoutChanged = everHadLayout() && selfNeedsLayout();
    RenderSVGBlock::layout();
    ASSERT(!needsLayout());

    setLocation(LayoutPoint());
    SVGRenderSupport::updateLayerTransform(*this);

    // Invalidate all resources of this client if our layout changed.
    if (layoutChanged)
        SVGResourcesCache::clientLayoutChanged(*this);

    repainter.repaintAfterLayout();
}

LayoutRect RenderSVGForeignObject::overflowClipRect(const LayoutPoint& location, RenderFragmentContainer*, OverlayScrollbarSizeRelevancy, PaintPhase) const
{
    auto clipRect = enclosingLayoutRect(m_viewport);
    clipRect.moveBy(location);
    return clipRect;
}

void RenderSVGForeignObject::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderSVGBlock::styleDidChange(diff, oldStyle);

    if (!hasLayer())
        return;

    if (!SVGRenderSupport::isOverflowHidden(*this))
        layer()->setIsOpportunisticStackingContext(true);
}

void RenderSVGForeignObject::updateFromStyle()
{
    RenderSVGBlock::updateFromStyle();

    // Enforce <fO> to carry a transform: <fO> should behave as absolutely positioned container
    // for CSS content. Thus it needs to become a rootPaintingLayer during paint() such that
    // fixed position content uses the <fO> as ancestor layer (when computing offsets from the container).
    setHasSVGTransform();

    if (SVGRenderSupport::isOverflowHidden(*this))
        setHasNonVisibleOverflow();
}

void RenderSVGForeignObject::applyTransform(TransformationMatrix& transform, const RenderStyle& style, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption> options) const
{
    SVGRenderSupport::applyTransform(*this, transform, style, boundingBox, std::nullopt, m_supplementalLocalToParentTransform.isIdentity() ? std::nullopt : std::make_optional(m_supplementalLocalToParentTransform), options);
}

}
