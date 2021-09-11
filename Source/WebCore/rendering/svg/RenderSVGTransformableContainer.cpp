/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
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
#include "RenderSVGTransformableContainer.h"

#include "SVGContainerLayout.h"
#include "SVGElementTypeHelpers.h"
#include "SVGGElement.h"
#include "SVGGraphicsElement.h"
#include "SVGPathData.h"
#include "SVGUseElement.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGTransformableContainer);

RenderSVGTransformableContainer::RenderSVGTransformableContainer(SVGGraphicsElement& element, RenderStyle&& style)
    : RenderSVGContainer(element, WTFMove(style))
{
}

inline SVGUseElement* associatedUseElement(SVGGraphicsElement& element)
{
    // If we're either the renderer for a <use> element, or for any <g> element inside the shadow
    // tree, that was created during the use/symbol/svg expansion in SVGUseElement. These containers
    // need to respect the translations induced by their corresponding use elements x/y attributes.
    if (is<SVGUseElement>(element))
        return &downcast<SVGUseElement>(element);

    if (element.isInShadowTree() && is<SVGGElement>(element)) {
        SVGElement* correspondingElement = element.correspondingElement();
        if (is<SVGUseElement>(correspondingElement))
            return downcast<SVGUseElement>(correspondingElement);
    }

    return nullptr;
}

FloatPoint RenderSVGTransformableContainer::extraContainerTranslation() const
{
    if (auto* useElement = associatedUseElement(graphicsElement())) {
        const auto& lengthContext = useElement->lengthContext();
        return { useElement->x().value(lengthContext), useElement->y().value(lengthContext) };
    }

    return { };
}

void RenderSVGTransformableContainer::calculateViewport()
{
    RenderSVGContainer::calculateViewport();

    if (auto* useElement = associatedUseElement(graphicsElement()))
        useElement->updateLengthContext();

    m_supplementalLocalToParentTransform.makeIdentity();

    auto translation = extraContainerTranslation();
    m_supplementalLocalToParentTransform.translate(translation.x(), translation.y());

    m_didTransformToRootUpdate = m_hadTransformUpdate || SVGContainerLayout::transformToRootChanged(parent());
}

void RenderSVGTransformableContainer::layoutChildren()
{
    RenderSVGContainer::layoutChildren();
    m_didTransformToRootUpdate = false;
}

void RenderSVGTransformableContainer::updateFromStyle()
{
    RenderSVGContainer::updateFromStyle();

    if (associatedUseElement(graphicsElement()))
        setHasSVGTransform();
}

const Path& RenderSVGTransformableContainer::computeClipPath(AffineTransform& transform) const
{
    if (auto* useElement = associatedUseElement(graphicsElement())) {
        auto* rendererClipChildPtr = useElement->rendererClipChild();
        if (rendererClipChildPtr && is<RenderSVGModelObject>(rendererClipChildPtr)) {
            auto& rendererClipChild = downcast<RenderSVGModelObject>(*rendererClipChildPtr);

            auto& rendererUseElement = downcast<RenderSVGTransformableContainer>(*useElement->renderer());
            ASSERT(rendererUseElement.hasLayer());
            transform.multiply(rendererUseElement.layer()->currentTransform(RenderStyle::individualTransformOperations).toAffineTransform());

            return rendererClipChild.computeClipPath(transform);
        }

        return sharedEmptyPath();
    }

    return RenderSVGModelObject::computeClipPath(transform);
}

void RenderSVGTransformableContainer::applyTransform(TransformationMatrix& transform, const RenderStyle& style, const FloatRect& boundingBox, OptionSet<RenderStyle::TransformOperationOption> options) const
{
    SVGRenderSupport::applyTransform(*this, transform, style, boundingBox, std::nullopt, hasSVGTransform() ? std::make_optional(m_supplementalLocalToParentTransform) : std::nullopt, options);
}

void RenderSVGTransformableContainer::styleWillChange(StyleDifference diff, const RenderStyle& newStyle)
{
    const RenderStyle* oldStyle = hasInitializedStyle() ? &style() : nullptr;
    if (oldStyle && oldStyle->transform() != newStyle.transform())
        setHadTransformUpdate();
    RenderSVGContainer::styleWillChange(diff, newStyle);
}

SVGGraphicsElement& RenderSVGTransformableContainer::graphicsElement()
{
    return downcast<SVGGraphicsElement>(RenderSVGContainer::element());
}

}
