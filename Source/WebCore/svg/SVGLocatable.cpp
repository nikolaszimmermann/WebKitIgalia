/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
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
#include "SVGLocatable.h"

#include "RenderElement.h"
#include "RenderSVGForeignObject.h"
#include "RenderSVGHiddenContainer.h"
#include "RenderSVGImage.h"
#include "RenderSVGRoot.h"
#include "RenderSVGViewportContainer.h"
#include "SVGElementTypeHelpers.h"
#include "SVGGraphicsElement.h"
#include "SVGImageElement.h"
#include "SVGMatrix.h"
#include "SVGNames.h"
#include "SVGRenderingContext.h"

namespace WebCore {

static bool isViewportElement(Node* node)
{
    return (node->hasTagName(SVGNames::svgTag)
        || node->hasTagName(SVGNames::symbolTag)
        || node->hasTagName(SVGNames::foreignObjectTag)
        || is<SVGImageElement>(*node));
}

SVGElement* SVGLocatable::nearestViewportElement(const SVGElement* element)
{
    ASSERT(element);
    for (Element* current = element->parentOrShadowHostElement(); current; current = current->parentOrShadowHostElement()) {
        if (isViewportElement(current))
            return downcast<SVGElement>(current);
    }

    return nullptr;
}

SVGElement* SVGLocatable::farthestViewportElement(const SVGElement* element)
{
    ASSERT(element);
    SVGElement* farthest = nullptr;
    for (Element* current = element->parentOrShadowHostElement(); current; current = current->parentOrShadowHostElement()) {
        if (isViewportElement(current))
            farthest = downcast<SVGElement>(current);
    }
    return farthest;
}

FloatRect SVGLocatable::getBBox(SVGElement* element, StyleUpdateStrategy styleUpdateStrategy)
{
    ASSERT(element);
    if (styleUpdateStrategy == AllowStyleUpdate)
        element->document().updateLayoutIgnorePendingStylesheets();

    // FIXME: Eventually we should support getBBox for detached elements.
    if (!element->renderer())
        return FloatRect();

    return element->renderer()->objectBoundingBox();
}

AffineTransform SVGLocatable::computeCTM(SVGElement* element, CTMScope mode, StyleUpdateStrategy styleUpdateStrategy)
{
    ASSERT(element);
    if (styleUpdateStrategy == AllowStyleUpdate)
        element->document().updateLayoutIgnorePendingStylesheets();

    auto* renderer = element->renderer();
    if (!renderer)
        return AffineTransform();

    SVGElement* stopAtElement = mode == NearestViewportScope ? nearestViewportElement(element) : nullptr;
    RenderLayerModelObject* stopAtRenderer = nullptr;
    if (stopAtElement) {
        // Handle all renderers here that isViewportElement() can return.
        ASSERT(stopAtElement->renderer());
        if (is<RenderSVGRoot>(*stopAtElement->renderer()))
            stopAtRenderer = &downcast<RenderSVGRoot>(*stopAtElement->renderer());
        else if (is<RenderSVGViewportContainer>(*stopAtElement->renderer()))
            stopAtRenderer = &downcast<RenderSVGViewportContainer>(*stopAtElement->renderer());
        else if (is<RenderSVGForeignObject>(*stopAtElement->renderer()))
            stopAtRenderer = &downcast<RenderSVGForeignObject>(*stopAtElement->renderer());
        else if (is<RenderSVGImage>(*stopAtElement->renderer()))
            stopAtRenderer = &downcast<RenderSVGImage>(*stopAtElement->renderer());
        else if (is<RenderSVGHiddenContainer>(*stopAtElement->renderer()))
            stopAtRenderer = &downcast<RenderSVGHiddenContainer>(*stopAtElement->renderer());
        else
            ASSERT_NOT_REACHED();
    } else if (is<RenderSVGRoot>(*renderer)) {
        auto& svgRoot = downcast<RenderSVGRoot>(*renderer);

        // For getCTM() do not exit the SVG subtree.
        // For getScreenCTM() stop at the initial containing block (stopAtRenderer = nullptr).
        if (mode == NearestViewportScope)
            stopAtRenderer = &svgRoot;
    }

    bool includeDeviceScaleFactor = mode == SVGLocatable::ScreenScope;
    return SVGRenderingContext::calculateAbsoluteTransformForRenderer(*renderer, stopAtRenderer, includeDeviceScaleFactor);
}

ExceptionOr<Ref<SVGMatrix>> SVGLocatable::getTransformToElement(SVGElement* target, StyleUpdateStrategy styleUpdateStrategy)
{
    AffineTransform ctm = getCTM(styleUpdateStrategy);

    if (is<SVGGraphicsElement>(target)) {
        AffineTransform targetCTM = downcast<SVGGraphicsElement>(*target).getCTM(styleUpdateStrategy);
        if (auto inverse = targetCTM.inverse())
            ctm = inverse.value() * ctm;
        else
            return Exception { InvalidStateError, "Matrix is not invertible"_s };
    }

    return SVGMatrix::create(ctm);
}

}
