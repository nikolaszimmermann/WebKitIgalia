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

#pragma once

#include "RenderSVGResourceContainer.h"

namespace WebCore {

class AffineTransform;
class RenderObject;
class SVGMarkerElement;

class RenderSVGResourceMarker final : public RenderSVGResourceContainer {
    WTF_MAKE_ISO_ALLOCATED(RenderSVGResourceMarker);
public:
    RenderSVGResourceMarker(SVGMarkerElement&, RenderStyle&&);
    virtual ~RenderSVGResourceMarker();

    inline SVGMarkerElement& markerElement() const;

    void removeAllClientsFromCache(bool markForInvalidation = true) override;
    void removeClientFromCache(RenderElement&, bool markForInvalidation = true) override;

    // Calculates marker boundaries, mapped to the target element's coordinate space
    FloatRect computeMarkerBoundingBox(const SVGBoundingBoxComputation::DecorationOptions&, const AffineTransform& markerTransformation) const;

    void layout() override;
    void calculateViewport() override;

    AffineTransform markerTransformation(const FloatPoint& origin, float angle, float strokeWidth) const;

    bool applyResource(RenderElement&, const RenderStyle&, GraphicsContext*&, OptionSet<RenderSVGResourceMode>) override { return false; }
    FloatRect resourceBoundingBox(const RenderObject&) override { return FloatRect(); }

    FloatPoint referencePoint() const;
    float angle() const;
    inline SVGMarkerUnitsType markerUnits() const;

    RenderSVGResourceType resourceType() const override { return MarkerResourceType; }
    bool isSVGResourceMarker() const override { return true; }

private:
    void element() const = delete;

    const char* renderName() const override { return "RenderSVGResourceMarker"; }

    void updateFromStyle() override;
    void updateLayerInformation() final;
    LayoutRect overflowClipRect(const LayoutPoint& location, RenderFragmentContainer* = nullptr, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, PaintPhase = PaintPhase::BlockBackground) const final;
    void applyTransform(TransformationMatrix&, const RenderStyle&, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption>) const final;

    FloatRect m_viewportDimension;
    AffineTransform m_supplementalLocalToParentTransform;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::RenderSVGResourceMarker)
static bool isType(const WebCore::RenderObject& renderer) { return renderer.isSVGResourceMarker(); }
static bool isType(const WebCore::RenderSVGResource& resource) { return resource.resourceType() == WebCore::MarkerResourceType; }
SPECIALIZE_TYPE_TRAITS_END()
