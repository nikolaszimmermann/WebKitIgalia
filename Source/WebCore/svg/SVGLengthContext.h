/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#include "ExceptionOr.h"
#include "FloatRect.h"
#include "SVGLengthValue.h"
#include "SVGUnitTypes.h"
#include <wtf/CompactPointerTuple.h>

namespace WebCore {

class SVGElement;

struct Length;

class SVGLengthContext {
public:
    template<typename T>
    static FloatRect resolveRectangle(const T& context, SVGUnitTypes::SVGUnitType type, const FloatRect& viewport)
    {
        return SVGLengthContext::resolveRectangle(context, type, viewport, context.x(), context.y(), context.width(), context.height());
    }

    static FloatRect resolveRectangle(const SVGElement&, SVGUnitTypes::SVGUnitType, const FloatRect& viewport, const SVGLengthValue& x, const SVGLengthValue& y, const SVGLengthValue& width, const SVGLengthValue& height);
    static FloatPoint resolvePoint(const SVGElement&, SVGUnitTypes::SVGUnitType, const SVGLengthValue& x, const SVGLengthValue& y);
    static float resolveLength(const SVGElement&, SVGUnitTypes::SVGUnitType, const SVGLengthValue&);

    float valueForLength(const Length&, SVGLengthMode = SVGLengthMode::Other) const;
    ExceptionOr<float> convertValueToUserUnits(float, SVGLengthType, SVGLengthMode) const;
    ExceptionOr<float> convertValueFromUserUnits(float, SVGLengthType, SVGLengthMode) const;

    void updateViewport();
    FloatSize viewportSize() const { return m_viewportSize.value_or(FloatSize()); }

private:
    friend class SVGElement;
    friend NeverDestroyed<SVGLengthContext>;

    enum ContextMode : uint8_t {
        ResolveAgainstDefaultViewport,
        ResolveAgainstOverridenViewport
    };

    using ContextModeAndElement = CompactPointerTuple<const SVGElement*, ContextMode>;
    const SVGElement* contextElement() const { return m_contextModeAndElement.pointer(); }

    SVGLengthContext() = default;
    SVGLengthContext(const SVGElement&);
    SVGLengthContext(const SVGElement&, const FloatRect& viewport);

    ExceptionOr<float> convertValueFromUserUnitsToPercentage(float value, SVGLengthMode) const;
    ExceptionOr<float> convertValueFromPercentageToUserUnits(float value, SVGLengthMode) const;

    ExceptionOr<float> convertValueFromUserUnitsToEMS(float value) const;
    ExceptionOr<float> convertValueFromEMSToUserUnits(float value) const;

    ExceptionOr<float> convertValueFromUserUnitsToEXS(float value) const;
    ExceptionOr<float> convertValueFromEXSToUserUnits(float value) const;

    ContextModeAndElement m_contextModeAndElement { nullptr, ResolveAgainstDefaultViewport };
    std::optional<FloatSize> m_viewportSize;
};

const SVGLengthContext& lengthContextFromElement(const SVGElement*);
const SVGLengthContext& emptyLengthContext();

} // namespace WebCore
