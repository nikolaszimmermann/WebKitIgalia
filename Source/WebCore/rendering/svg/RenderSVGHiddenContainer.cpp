/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "RenderSVGHiddenContainer.h"

#include "RenderSVGPath.h"
#include "SVGLogger.h"
#include "SVGResourcesCache.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/StackStats.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGHiddenContainer);

RenderSVGHiddenContainer::RenderSVGHiddenContainer(SVGElement& element, RenderStyle&& style)
    : RenderSVGContainer(element, WTFMove(style))
{
}

void RenderSVGHiddenContainer::layout()
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

    calculateViewport();
    layoutChildren();

    SVGRenderSupport::updateLayerTransform(*this);

    // Invalidate all resources of this client if our layout changed.
    if (everHadLayout() && needsLayout())
        SVGResourcesCache::clientLayoutChanged(*this);

    clearNeedsLayout();
}

std::optional<LayoutRect> RenderSVGHiddenContainer::computeVisibleRectInContainer(const LayoutRect& rect, const RenderLayerModelObject*, VisibleRectContext) const
{
    // This subtree does not take up space or paint
    return rect;
}

void RenderSVGHiddenContainer::absoluteRects(Vector<IntRect>&, const LayoutPoint&) const
{
    // This subtree does not take up space or paint
}

void RenderSVGHiddenContainer::absoluteQuads(Vector<FloatQuad>&, bool*) const
{
    // This subtree does not take up space or paint
}

bool RenderSVGHiddenContainer::nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation&, const LayoutPoint&, HitTestAction)
{
    // This subtree does not take up space or paint
    return false;
}

void RenderSVGHiddenContainer::addFocusRingRects(Vector<LayoutRect>&, const LayoutPoint&, const RenderLayerModelObject*)
{
    // This subtree does not take up space or paint
}

}
