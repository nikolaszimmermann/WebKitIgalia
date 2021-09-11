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
#include "SVGGraphicsElement.h"
#include "SVGUnitTypes.h"

#include <wtf/HashMap.h>

namespace WebCore {

class GraphicsContext;
class SVGClipPathElement;

class RenderSVGResourceClipper final : public RenderSVGResourceContainer {
    WTF_MAKE_ISO_ALLOCATED(RenderSVGResourceClipper);
public:
    RenderSVGResourceClipper(SVGClipPathElement&, RenderStyle&&);
    virtual ~RenderSVGResourceClipper();

    inline SVGClipPathElement& clipPathElement() const;

    SVGGraphicsElement* shouldApplyPathClipping() const;
    void applyPathClipping(GraphicsContext&, const FloatRect& objectBoundingBox, SVGGraphicsElement&);
    void applyMaskClipping(PaintInfo&, const RenderLayerModelObject& targetRenderer, const FloatRect& objectBoundingBox);

    void removeAllClientsFromCache(bool markForInvalidation = true) override;
    void removeClientFromCache(RenderElement&, bool markForInvalidation = true) override;

    bool applyResource(RenderElement&, const RenderStyle&, GraphicsContext*&, OptionSet<RenderSVGResourceMode>) override;
    FloatRect resourceBoundingBox(const RenderObject&) override;

    RenderSVGResourceType resourceType() const override { return ClipperResourceType; }
    
    bool hitTestClipContent(const FloatRect&, const LayoutPoint&);

    inline SVGUnitTypes::SVGUnitType clipPathUnits() const;

private:
    void element() const = delete;

    const char* renderName() const override { return "RenderSVGResourceClipper"; }
    bool isSVGResourceClipper() const override { return true; }
};

}

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::RenderSVGResourceClipper)
static bool isType(const WebCore::RenderObject& renderer) { return renderer.isSVGResourceClipper(); }
static bool isType(const WebCore::RenderSVGResource& resource) { return resource.resourceType() == WebCore::ClipperResourceType; }
SPECIALIZE_TYPE_TRAITS_END()
