/*
 * Copyright (c) 2022 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "FloatRect.h"
#include "LayoutRect.h"
#include <optional>
#include <variant>

namespace WebCore {

class FloatOrLayoutRect {
public:
    FloatOrLayoutRect() = default;

    FloatOrLayoutRect(const FloatRect& rect)
        : m_data { rect }
    {
    }

    FloatOrLayoutRect(const LayoutRect& rect)
        : m_data { rect }
    {
    }

    ~FloatOrLayoutRect() = default;

    operator FloatRect() const
    {
        if (isFloatRect())
            return floatRect();
        ASSERT(isLayoutRect());
        return layoutRect();
    }

    FloatRect rectForPainting(float deviceScaleFactor) const
    {
        if (isFloatRect())
            return floatRect();
        ASSERT(isLayoutRect());
        return snapRectToDevicePixels(layoutRect(), deviceScaleFactor);
    }

    // Convenience functions:
    // Using move() the location of the rect can be altered, regardless of its type.
    void move(const LayoutSize& size)
    {
        if (isFloatRect()) {
            floatRect().move(size);
            return;
        }
        ASSERT(isLayoutRect());
        layoutRect().move(size);
    }

private:
    constexpr bool isFloatRect() const { return std::holds_alternative<FloatRect>(m_data); }
    const FloatRect& floatRect() const { return std::get<FloatRect>(m_data); }
    FloatRect& floatRect() { return std::get<FloatRect>(m_data); }

    constexpr bool isLayoutRect() const { return std::holds_alternative<LayoutRect>(m_data); }
    const LayoutRect& layoutRect() const { return std::get<LayoutRect>(m_data); }
    LayoutRect& layoutRect() { return std::get<LayoutRect>(m_data); }

    std::variant<FloatRect, LayoutRect> m_data;
};

} // namespace WebCore
