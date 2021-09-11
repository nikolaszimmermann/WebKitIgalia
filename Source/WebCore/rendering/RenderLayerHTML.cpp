/*
 * Copyright (c) 2019-2021 Igalia S.L.
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

#include "config.h"
#include "RenderLayerHTML.h"

#include "RenderGeometryMap.h"
#include "RenderLayerBacking.h"

namespace WebCore {

DEFINE_ALLOCATOR_WITH_HEAP_IDENTIFIER(RenderLayerHTML);

RenderLayerHTML::RenderLayerHTML(RenderLayerModelObject& renderer)
    : RenderLayer(renderer)
{
}

RenderLayerHTML::~RenderLayerHTML()
{
}

LayoutPoint RenderLayerHTML::rendererLocation() const
{
    if (is<RenderBox>(renderer()))
        return downcast<RenderBox>(renderer()).location();
    return LayoutPoint();
}

RoundedRect RenderLayerHTML::rendererRoundedBorderBoxRect() const
{
    if (is<RenderBox>(renderer()))
        return downcast<RenderBox>(renderer()).roundedBorderBoxRect();
    return RoundedRect(LayoutRect());
}

LayoutRect RenderLayerHTML::rendererBorderBoxRect() const
{
    if (is<RenderBox>(renderer()))
        return downcast<RenderBox>(renderer()).borderBoxRect();
    return LayoutRect();
}

LayoutRect RenderLayerHTML::rendererBorderBoxRectInFragment(RenderFragmentContainer* fragment, RenderBox::RenderBoxFragmentInfoFlags flags) const
{
    ASSERT(is<RenderBox>(renderer()));
    return downcast<RenderBox>(renderer()).borderBoxRectInFragment(fragment, flags);
}

LayoutRect RenderLayerHTML::rendererOverflowClipRect(const LayoutPoint& offset, RenderFragmentContainer* fragment, OverlayScrollbarSizeRelevancy overlayScrollbarSizeRelevancy, PaintPhase paintPhase) const
{
    ASSERT(is<RenderBox>(renderer()));
    return downcast<RenderBox>(renderer()).overflowClipRect(offset, fragment, overlayScrollbarSizeRelevancy, paintPhase);
}

LayoutRect RenderLayerHTML::rendererOverflowClipRectForChildLayers(const LayoutPoint& offset, RenderFragmentContainer* fragment, OverlayScrollbarSizeRelevancy overlayScrollbarSizeRelevancy) const
{
    ASSERT(is<RenderBox>(renderer()));
    return downcast<RenderBox>(renderer()).overflowClipRectForChildLayers(offset, fragment, overlayScrollbarSizeRelevancy);
}

LayoutRect RenderLayerHTML::rendererClipRect(const LayoutPoint& offset, RenderFragmentContainer* fragment) const
{
    ASSERT(is<RenderBox>(renderer()));
    return downcast<RenderBox>(renderer()).clipRect(offset, fragment);
}

} // namespace WebCore
