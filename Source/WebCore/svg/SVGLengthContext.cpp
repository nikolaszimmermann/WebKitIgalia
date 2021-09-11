/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007-2019 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2014 Adobe Systems Incorporated. All rights reserved.
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
#include "SVGLengthContext.h"

#include "CSSHelper.h"
#include "FontMetrics.h"
#include "Frame.h"
#include "LengthFunctions.h"
#include "RenderSVGRoot.h"
#include "RenderSVGViewportContainer.h"
#include "RenderView.h"
#include "SVGElementTypeHelpers.h"
#include "SVGSVGElement.h"
#include "ShadowRoot.h"
#include <wtf/MathExtras.h>

namespace WebCore {

SVGLengthContext::SVGLengthContext(const SVGElement& context)
    : m_contextModeAndElement(&context, ResolveAgainstDefaultViewport)
{
}

SVGLengthContext::SVGLengthContext(const SVGElement& context, const FloatRect& viewport)
    : m_contextModeAndElement(&context, ResolveAgainstOverridenViewport)
    , m_viewportSize(viewport.size())
{
}

FloatRect SVGLengthContext::resolveRectangle(const SVGElement& context, SVGUnitTypes::SVGUnitType type, const FloatRect& viewport, const SVGLengthValue& x, const SVGLengthValue& y, const SVGLengthValue& width, const SVGLengthValue& height)
{
    ASSERT(type != SVGUnitTypes::SVG_UNIT_TYPE_UNKNOWN);
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        const auto& lengthContext = context.lengthContext();
        return FloatRect(x.value(lengthContext), y.value(lengthContext), width.value(lengthContext), height.value(lengthContext));
    }

    SVGLengthContext lengthContext(context, viewport);
    return FloatRect(x.value(lengthContext) + viewport.x(),
                     y.value(lengthContext) + viewport.y(),
                     width.value(lengthContext),
                     height.value(lengthContext));
}

FloatPoint SVGLengthContext::resolvePoint(const SVGElement& context, SVGUnitTypes::SVGUnitType type, const SVGLengthValue& x, const SVGLengthValue& y)
{
    ASSERT(type != SVGUnitTypes::SVG_UNIT_TYPE_UNKNOWN);
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        const auto& lengthContext = context.lengthContext();
        return FloatPoint(x.value(lengthContext), y.value(lengthContext));
    }

    // FIXME: valueAsPercentage() won't be correct for eg. cm units. They need to be resolved in user space and then be considered in objectBoundingBox space.
    return FloatPoint(x.valueAsPercentage(), y.valueAsPercentage());
}

float SVGLengthContext::resolveLength(const SVGElement& context, SVGUnitTypes::SVGUnitType type, const SVGLengthValue& x)
{
    ASSERT(type != SVGUnitTypes::SVG_UNIT_TYPE_UNKNOWN);
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        const auto& lengthContext = context.lengthContext();
        return x.value(lengthContext);
    }

    // FIXME: valueAsPercentage() won't be correct for eg. cm units. They need to be resolved in user space and then be considered in objectBoundingBox space.
    return x.valueAsPercentage();
}

float SVGLengthContext::valueForLength(const Length& length, SVGLengthMode lengthMode) const
{
    if (length.isPercent()) {
        auto result = convertValueFromPercentageToUserUnits(length.value() / 100, lengthMode);
        if (result.hasException())
            return 0;
        return result.releaseReturnValue();
    }
    if (length.isAuto() || !length.isSpecified())
        return 0;

    auto viewportSize = m_viewportSize.value_or(FloatSize());

    switch (lengthMode) {
    case SVGLengthMode::Width:
        return floatValueForLength(length, viewportSize.width());
    case SVGLengthMode::Height:
        return floatValueForLength(length, viewportSize.height());
    case SVGLengthMode::Other:
        return floatValueForLength(length, viewportSize.diagonalLength() / sqrtOfTwoFloat);
    };

    ASSERT_NOT_REACHED();
    return 0;
}

ExceptionOr<float> SVGLengthContext::convertValueToUserUnits(float value, SVGLengthType lengthType, SVGLengthMode lengthMode) const
{
    // If the SVGLengthContext carries a custom viewport, force resolving against it.
    if (m_contextModeAndElement.type() == ResolveAgainstOverridenViewport) {
        // 100% = 100.0 instead of 1.0 for historical reasons, this could eventually be changed
        if (lengthType == SVGLengthType::Percentage)
            value /= 100;
        return convertValueFromPercentageToUserUnits(value, lengthMode);
    }

    switch (lengthType) {
    case SVGLengthType::Unknown:
        return Exception { NotSupportedError };
    case SVGLengthType::Number:
        return value;
    case SVGLengthType::Pixels:
        return value;
    case SVGLengthType::Percentage:
        return convertValueFromPercentageToUserUnits(value / 100, lengthMode);
    case SVGLengthType::Ems:
        return convertValueFromEMSToUserUnits(value);
    case SVGLengthType::Exs:
        return convertValueFromEXSToUserUnits(value);
    case SVGLengthType::Centimeters:
        return value * cssPixelsPerInch / 2.54f;
    case SVGLengthType::Millimeters:
        return value * cssPixelsPerInch / 25.4f;
    case SVGLengthType::Inches:
        return value * cssPixelsPerInch;
    case SVGLengthType::Points:
        return value * cssPixelsPerInch / 72;
    case SVGLengthType::Picas:
        return value * cssPixelsPerInch / 6;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

ExceptionOr<float> SVGLengthContext::convertValueFromUserUnits(float value, SVGLengthType lengthType, SVGLengthMode lengthMode) const
{
    switch (lengthType) {
    case SVGLengthType::Unknown:
        return Exception { NotSupportedError };
    case SVGLengthType::Number:
        return value;
    case SVGLengthType::Percentage:
        return convertValueFromUserUnitsToPercentage(value * 100, lengthMode);
    case SVGLengthType::Ems:
        return convertValueFromUserUnitsToEMS(value);
    case SVGLengthType::Exs:
        return convertValueFromUserUnitsToEXS(value);
    case SVGLengthType::Pixels:
        return value;
    case SVGLengthType::Centimeters:
        return value * 2.54f / cssPixelsPerInch;
    case SVGLengthType::Millimeters:
        return value * 25.4f / cssPixelsPerInch;
    case SVGLengthType::Inches:
        return value / cssPixelsPerInch;
    case SVGLengthType::Points:
        return value * 72 / cssPixelsPerInch;
    case SVGLengthType::Picas:
        return value * 6 / cssPixelsPerInch;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

ExceptionOr<float> SVGLengthContext::convertValueFromUserUnitsToPercentage(float value, SVGLengthMode lengthMode) const
{
    if (!m_viewportSize)
        return Exception { NotSupportedError };

    auto viewportSize = m_viewportSize.value();

    switch (lengthMode) {
    case SVGLengthMode::Width:
        return value / viewportSize.width() * 100;
    case SVGLengthMode::Height:
        return value / viewportSize.height() * 100;
    case SVGLengthMode::Other:
        return value / (viewportSize.diagonalLength() / sqrtOfTwoFloat) * 100;
    };

    ASSERT_NOT_REACHED();
    return 0;
}

ExceptionOr<float> SVGLengthContext::convertValueFromPercentageToUserUnits(float value, SVGLengthMode lengthMode) const
{
    if (!m_viewportSize)
        return Exception { NotSupportedError };

    auto viewportSize = m_viewportSize.value();

    switch (lengthMode) {
    case SVGLengthMode::Width:
        return value * viewportSize.width();
    case SVGLengthMode::Height:
        return value * viewportSize.height();
    case SVGLengthMode::Other:
        return value * viewportSize.diagonalLength() / sqrtOfTwoFloat;
    };

    ASSERT_NOT_REACHED();
    return 0;
}

static inline const RenderStyle* renderStyleForLengthResolving(const SVGElement* context)
{
    if (!context)
        return nullptr;

    const ContainerNode* currentContext = context;
    do {
        if (currentContext->renderer())
            return &currentContext->renderer()->style();
        currentContext = currentContext->parentNode();
    } while (currentContext);

    // There must be at least a RenderSVGRoot renderer, carrying a style.
    ASSERT_NOT_REACHED();
    return nullptr;
}

ExceptionOr<float> SVGLengthContext::convertValueFromUserUnitsToEMS(float value) const
{
    auto* style = renderStyleForLengthResolving(contextElement());
    if (!style)
        return Exception { NotSupportedError };

    float fontSize = style->computedFontPixelSize();
    if (!fontSize)
        return Exception { NotSupportedError };

    return value / fontSize;
}

ExceptionOr<float> SVGLengthContext::convertValueFromEMSToUserUnits(float value) const
{
    auto* style = renderStyleForLengthResolving(contextElement());
    if (!style)
        return Exception { NotSupportedError };

    return value * style->computedFontPixelSize();
}

ExceptionOr<float> SVGLengthContext::convertValueFromUserUnitsToEXS(float value) const
{
    auto* style = renderStyleForLengthResolving(contextElement());
    if (!style)
        return Exception { NotSupportedError };

    // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
    // if this causes problems in real world cases maybe it would be best to remove this
    float xHeight = std::ceil(style->fontMetrics().xHeight());
    if (!xHeight)
        return Exception { NotSupportedError };

    return value / xHeight;
}

ExceptionOr<float> SVGLengthContext::convertValueFromEXSToUserUnits(float value) const
{
    auto* style = renderStyleForLengthResolving(contextElement());
    if (!style)
        return Exception { NotSupportedError };

    // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
    // if this causes problems in real world cases maybe it would be best to remove this
    return value * std::ceil(style->fontMetrics().xHeight());
}

void SVGLengthContext::updateViewport()
{
    ASSERT(m_contextModeAndElement.type() == ResolveAgainstDefaultViewport);

    m_viewportSize.reset();
    if (!contextElement())
        return;

    // Root <svg> element lengths are resolved against the top level viewport.
    if (contextElement()->isOutermostSVGSVGElement()) {
        m_viewportSize = downcast<SVGSVGElement>(*contextElement()).currentViewportSize();
        return;
    }

    auto* ancestor = contextElement()->parentOrShadowHostNode();
    while (ancestor) {
        if (is<SVGSVGElement>(ancestor)) {
            const auto& svgSVGElement = downcast<SVGSVGElement>(*ancestor);

            m_viewportSize = svgSVGElement.currentViewBoxRect().size();
            if (m_viewportSize->isEmpty()) {
                m_viewportSize = svgSVGElement.currentViewportSize();
                if (svgSVGElement.isOutermostSVGSVGElement()) {
                    const auto& style = *renderStyleForLengthResolving(&svgSVGElement);
                    auto zoom = style.effectiveZoom();
                    if (zoom != 1)
                        m_viewportSize->scale(1.0 / zoom);
                }
            }

            return;
        }

        ancestor = ancestor->parentOrShadowHostNode();
    }
}

const SVGLengthContext& lengthContextFromElement(const SVGElement* contextElement)
{
    return contextElement ? contextElement->lengthContext() : emptyLengthContext();
}

const SVGLengthContext& emptyLengthContext()
{
    static NeverDestroyed<SVGLengthContext> emptyContext;
    return emptyContext;
}

}
