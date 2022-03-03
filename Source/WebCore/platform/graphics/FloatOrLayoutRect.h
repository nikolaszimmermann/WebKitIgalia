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
#include <type_traits>
#include <variant>

namespace WebCore {

class FloatOrLayoutRect final {
    struct UndefinedRect { };

    // Convenience functions to access the variants elements.
    template<typename T>
    constexpr bool is() const { return std::holds_alternative<T>(m_data); }

    template<typename T>
    constexpr const T& as() const { return std::get<T>(m_data); }

    template<typename T>
    constexpr T& as() { return std::get<T>(m_data); }

public:
    constexpr FloatOrLayoutRect(FloatRect&& rect)
        : m_data(std::in_place_type_t<FloatRect>(), WTFMove(rect))
    {
    }

    constexpr FloatOrLayoutRect(LayoutRect&& rect)
        : m_data(std::in_place_type_t<LayoutRect>(), WTFMove(rect))
    {
    }

    constexpr FloatOrLayoutRect()
        : m_data(UndefinedRect { })
    {
    }

    operator FloatRect() const
    {
        if (is<FloatRect>())
            return as<FloatRect>();
        if (is<LayoutRect>())
            return as<LayoutRect>();
        ASSERT(is<UndefinedRect>());
        return { };
    }

    FloatRect floatRectForPainting(float deviceScaleFactor) const
    {
        if (is<FloatRect>())
            return as<FloatRect>();
        if (is<LayoutRect>())
            return snapRectToDevicePixels(as<LayoutRect>(), deviceScaleFactor);
        ASSERT(is<UndefinedRect>());
        return { };
    }

    // Convenience function: Using move() the location of the rect can be altered, regardless of its type.
    void move(const LayoutSize& size)
    {
        if (is<FloatRect>()) {
            as<FloatRect>().move(size);
            return;
        }

        if (is<LayoutRect>()) {
            as<LayoutRect>().move(size);
            return;
        }

        ASSERT(is<UndefinedRect>());
        RELEASE_ASSERT_NOT_REACHED();
    }

private:
    std::variant<FloatRect, LayoutRect, UndefinedRect> m_data;
};

static_assert(std::is_trivially_copyable<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially coypable.");
static_assert(!std::is_polymorphic<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must not be polymporphic.");
static_assert(!std::has_virtual_destructor<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must not have a virtual destructor.");

static_assert(std::is_constructible<FloatOrLayoutRect, FloatRect>::value, "'FloatOrLayoutRect' must be trivially constructible from 'FloatRect'.");
static_assert(!std::is_trivially_constructible<FloatOrLayoutRect, FloatRect>::value, "'FloatOrLayoutRect' cannot be trivially constructible from 'FloatRect'.");
static_assert(!std::is_trivially_constructible<FloatOrLayoutRect, LayoutRect>::value, "'FloatOrLayoutRect' cannot be trivially constructible from 'LayoutRect'.");

static_assert(std::is_default_constructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially default-constructible.");
static_assert(!std::is_trivially_default_constructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' cannot be trivially default-constructible.");

static_assert(std::is_copy_constructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially copy-constructible.");
static_assert(std::is_trivially_copy_constructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially copy-constructible.");

static_assert(std::is_move_constructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially move-constructible.");
static_assert(std::is_trivially_move_constructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially move-constructible.");

static_assert(std::is_assignable<FloatOrLayoutRect, FloatRect>::value, "'FloatOrLayoutRect' must be trivially assignable from 'FloatRect'.");
static_assert(std::is_assignable<FloatOrLayoutRect, LayoutRect>::value, "'FloatOrLayoutRect' must be trivially assignable from 'LayoutRect'.");
static_assert(!std::is_trivially_assignable<FloatOrLayoutRect, FloatRect>::value, "'FloatOrLayoutRect' cannot be trivially assignable from 'FloatRect'.");
static_assert(!std::is_trivially_assignable<FloatOrLayoutRect, LayoutRect>::value, "'FloatOrLayoutRect' cannot be trivially assignable from 'LayoutRect'.");

static_assert(std::is_copy_assignable<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially copy-assignable.");
static_assert(std::is_trivially_copy_assignable<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially copy-assignable.");

static_assert(std::is_move_assignable<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially move-assignable.");
static_assert(std::is_trivially_move_assignable<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially move-assignable.");

static_assert(std::is_destructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially destructible.");
static_assert(std::is_trivially_destructible<FloatOrLayoutRect>::value, "'FloatOrLayoutRect' must be trivially destructible.");

} // namespace WebCore
