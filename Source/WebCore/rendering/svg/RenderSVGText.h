/*
 * Copyright (C) 2006 Apple Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010-2012. All rights reserved.
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

#include "AffineTransform.h"
#include "RenderSVGBlock.h"
#include "SVGBoundingBoxComputation.h"
#include "SVGRenderSupport.h"
#include "SVGTextLayoutAttributesBuilder.h"

namespace WebCore {

class RenderSVGInlineText;
class SVGTextElement;
class RenderSVGInlineText;

class RenderSVGText final : public RenderSVGBlock {
    WTF_MAKE_ISO_ALLOCATED(RenderSVGText);
public:
    RenderSVGText(SVGTextElement&, RenderStyle&&);
    virtual ~RenderSVGText();

    SVGTextElement& textElement() const;

    bool isChildAllowed(const RenderObject&, const RenderStyle&) const override;

    void setNeedsPositioningValuesUpdate() { m_needsPositioningValuesUpdate = true; }
    void setNeedsTextMetricsUpdate() { m_needsTextMetricsUpdate = true; }

    static RenderSVGText* locateRenderSVGTextAncestor(RenderObject&);
    static const RenderSVGText* locateRenderSVGTextAncestor(const RenderObject&);

    bool needsReordering() const { return m_needsReordering; }
    Vector<SVGTextLayoutAttributes*>& layoutAttributes() { return m_layoutAttributes; }

    void subtreeChildWasAdded(RenderObject*);
    void subtreeChildWillBeRemoved(RenderObject*, Vector<SVGTextLayoutAttributes*, 2>& affectedAttributes);
    void subtreeChildWasRemoved(const Vector<SVGTextLayoutAttributes*, 2>& affectedAttributes);
    void subtreeStyleDidChange(RenderSVGInlineText*);
    void subtreeTextDidChange(RenderSVGInlineText*);

    FloatRect objectBoundingBox() const final { return m_objectBoundingBox; }
    FloatRect strokeBoundingBox() const final;
    FloatRect repaintBoundingBox() const final { return SVGBoundingBoxComputation::computeRepaintBoundingBox(*this); }

    void updatePositionAndOverflow(const FloatRect&);

    LayoutRect visualOverflowRectEquivalent() const { return SVGBoundingBoxComputation::computeVisualOverflowRect(*this); }

private:
    void graphicsElement() const = delete;

    const char* renderName() const override { return "RenderSVGText"; }
    bool isSVGText() const override { return true; }

    LayoutPoint paintingLocation() const { return toLayoutPoint(location() - flooredLayoutPoint(m_objectBoundingBox.minXMinYCorner())); }

    void paint(PaintInfo&, const LayoutPoint&) override;
    bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) override;
    VisiblePosition positionForPoint(const LayoutPoint&, const RenderFragmentContainer*) override;
    void addFocusRingRects(Vector<LayoutRect>&, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer = 0) override;

    void layout() override;

    void willBeDestroyed() override;

    RenderBlock* firstLineBlock() const override;

    bool shouldHandleSubtreeMutations() const;

    bool m_needsReordering { false };
    bool m_needsPositioningValuesUpdate { false };
    bool m_needsTextMetricsUpdate { false };
    SVGTextLayoutAttributesBuilder m_layoutAttributesBuilder;
    Vector<SVGTextLayoutAttributes*> m_layoutAttributes;

    FloatRect m_objectBoundingBox;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderSVGText, isSVGText())
