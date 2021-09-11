/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "RenderSVGContainer.h"

namespace WebCore {

// This is used for non-root <svg> elements and <marker> elements, neither of which are SVGTransformable
// thus we inherit from RenderSVGContainer instead of RenderSVGTransformableContainer
class RenderSVGViewportContainer final : public RenderSVGContainer {
    WTF_MAKE_ISO_ALLOCATED(RenderSVGViewportContainer);
public:
    RenderSVGViewportContainer(SVGSVGElement&, RenderStyle&&);

    SVGSVGElement& svgSVGElement() const;

    FloatRect currentViewport() const { return m_viewportDimension; }
    FloatSize currentViewportSize() const { return m_viewportDimension.size(); }

    bool isLayoutSizeChanged() const { return m_isLayoutSizeChanged; }
    bool didTransformToRootUpdate() const { return m_didTransformToRootUpdate; }

private:
    void element() const = delete;

    bool isSVGViewportContainer() const override { return true; }
    const char* renderName() const override { return "RenderSVGViewportContainer"; }

    void layoutChildren() final;
    void updateLayerInformation() final;
    void calculateViewport() final;
    bool pointIsInsideViewportClip(const FloatPoint& pointInParent) final;
    LayoutRect overflowClipRect(const LayoutPoint& location, RenderFragmentContainer* = nullptr, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, PaintPhase = PaintPhase::BlockBackground) const final;
    void applyTransform(TransformationMatrix&, const RenderStyle&, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption>) const final;

    void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override;
    void updateFromStyle() override;

    bool m_didTransformToRootUpdate { false };
    bool m_isLayoutSizeChanged { false };

    FloatRect m_viewportDimension;
    AffineTransform m_supplementalLocalToParentTransform;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderSVGViewportContainer, isSVGViewportContainer())
