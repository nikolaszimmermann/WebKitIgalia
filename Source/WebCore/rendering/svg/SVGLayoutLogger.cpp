/**
 * Copyright (C) 2021 Igalia S.L.
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
#include "SVGLayoutLogger.h"

#include "RenderChildIterator.h"
#include "RenderLayer.h"
#include "RenderLayoutState.h"
#include "RenderSVGModelObject.h"
#include "RenderView.h"

namespace WebCore {

constexpr bool shouldDumpLayerFragments = false;

SVGLayoutLogger::SVGLayoutLogger(WTFLogLevel logLevel)
    : SVGLogger(LOG_CHANNEL(SVG), logLevel)
{
}

void SVGLayoutLogger::dump(const RenderElement& renderer, const RenderObject* rendererToMark)
{
    if (loggingDisabled())
        return;

    addNewLineAndPrefix();
    addText("SVG render tree dump:\n");
    visitRenderTree(renderer, rendererToMark);
}

void SVGLayoutLogger::visitRenderTree(const RenderElement& renderer, const RenderObject* rendererToMark)
{
    SVGLayoutLogger::RendererScope renderScope(*this, renderer);
    if (!renderer.isSVGLayerAwareRenderer()) {
        addCSSGeometryInformation(renderer);
        addLayerInformation(renderer);
        addNewLine();

        for (auto& childRenderer : childrenOfType<RenderElement>(renderer))
            visitRenderTree(childRenderer, rendererToMark);

        return;
    }

    // Dump SVG render tree from top-to-bottom in render tree order - process only RenderElement
    // objects, thus skipping pure RenderObject derived renderers such as RenderSVGInlineText).
    addSVGGeometryInformation(renderer);
    addCSSGeometryInformation(renderer);
    addSVGCSSGeometryInformation(renderer);
    addLayerInformation(renderer);
    addNewLine();

    for (auto& childRenderer : childrenOfType<RenderElement>(renderer))
        visitRenderTree(childRenderer, rendererToMark);
}

OptionSet<SVGLogger::BoxOptions> SVGLayoutLogger::boxOptionsFromRenderer(const RenderElement& renderer)
{
    OptionSet<BoxOptions> options;
    if (renderer.parent())
        options.add(BoxOptions::HasParent);
    if (childrenOfType<RenderElement>(renderer).first())
        options.add(BoxOptions::HasChildren);
    if (renderer.nextSibling())
        options.add(BoxOptions::HasNextSibling);
    return options;
}

void SVGLayoutLogger::addRenderer(const RenderElement& renderer)
{
    TextStream stream(TextStream::LineMode::SingleLine);
    stream << "renderer=" << &renderer;
    if (is<RenderLayerModelObject>(renderer))
        stream << ", layer=" << downcast<RenderLayerModelObject>(renderer).layer();

    addBox(renderer.renderName(), stream.release(), boxOptionsFromRenderer(renderer));
}

void SVGLayoutLogger::addSVGGeometryInformation(const RenderElement& renderer)
{
    constexpr unsigned padding = 20;

    SVGLogger::SectionScope section(*this, "SVG geometry:");
    section.addEntry("objectBoundingBox", renderer.objectBoundingBox(), padding);
    section.addEntry("strokeBoundingBox", renderer.strokeBoundingBox(), padding);
    section.addEntry("repaintBoundingBox", renderer.repaintBoundingBox(), padding);
}

void SVGLayoutLogger::addCSSGeometryInformation(const RenderElement& renderer)
{
    if (!is<RenderBox>(renderer))
        return;
    auto& box = downcast<RenderBox>(renderer);

    constexpr unsigned padding = 30;

    SVGLogger::SectionScope section(*this, "CSS geometry:");
    section.addEntry("frameRect", box.frameRect(), padding);
    section.addEntry("borderBoxRect", box.borderBoxRect(), padding);
    section.addEntry("visualOverflowRect", box.visualOverflowRect(), padding);
    section.addEntry("location", box.location(), padding);
}

void SVGLayoutLogger::addSVGCSSGeometryInformation(const RenderElement& renderer)
{
    if (!is<RenderSVGModelObject>(renderer))
        return;
    auto& svg = downcast<RenderSVGModelObject>(renderer);

    constexpr unsigned padding = 30;

    SVGLogger::SectionScope section(*this, "CSS geometry: (equivalents computed from SVG)");
    section.addEntry("frameRectEquivalent", svg.frameRectEquivalent(), padding);
    section.addEntry("borderBoxRectEquivalent", svg.borderBoxRectEquivalent(), padding);
    section.addEntry("visualOverflowRectEquivalent", svg.visualOverflowRectEquivalent(), padding);
    section.addEntry("layoutLocation", svg.layoutLocation(), padding);
}

void SVGLayoutLogger::addLayerInformation(const RenderElement& renderer)
{
    if (!is<RenderLayerModelObject>(renderer) || !renderer.hasLayer())
        return;
    auto& layer = *downcast<RenderLayerModelObject>(renderer).layer();

    constexpr unsigned padding = 15;

    SVGLogger::SectionScope section(*this, "Layer geometry:");
    section.addEntry("location", layer.location(), padding);
    section.addEntry("size", layer.size(), padding);

    section.addNewLine();

    if (auto* transform = layer.transform())
        section.addEntry("transform", *transform, padding);

    section.addNewLine();

    auto* rootLayer = renderer.view().layer();
    auto paintDirtyRect = rootLayer->rect();
    auto offsetFromRoot = layer.offsetFromAncestor(rootLayer);
    section.addEntry("offsetFromRoot", offsetFromRoot, padding);

    LayerFragments layerFragments;
    if (shouldDumpLayerFragments) {
        LayoutStateDisabler layoutStateDisabler(renderer.view().frameView().layoutContext());
        layer.collectFragments(layerFragments, rootLayer, paintDirtyRect, RenderLayer::PaginationInclusionMode::ExcludeCompositedPaginatedLayers, TemporaryClipRects, IgnoreOverlayScrollbarSize, RespectOverflowClip, offsetFromRoot);
    }

    for (unsigned i = 0; i < layerFragments.size(); ++i) {
        const auto& fragment = layerFragments[i];
        section.addEntry(makeString("fragment[", i, ']'), fragment, padding);
    }
}

TextStream& operator<<(TextStream& ts, const LayerFragment& layerFragment)
{
    auto toRightPaddedString = [](auto& object) -> String {
        constexpr unsigned padding = 20;
        TextStream stream;
        stream << object;
        return rightPaddedString(stream.release(), padding);
    };

    ts << "shouldPaintContent=" << layerFragment.shouldPaintContent << " "
    << "layerBounds=" << toRightPaddedString(layerFragment.layerBounds) << " "
    << "boundingBox=" << toRightPaddedString(layerFragment.boundingBox) << " "
    << "backgroundRect=" << toRightPaddedString(layerFragment.backgroundRect) << " "
    << "foregroundRect=" << toRightPaddedString(layerFragment.foregroundRect);

    return ts;
}

}
