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

#pragma once

#include "LayerFragment.h"
#include "SVGLogger.h"

namespace WebCore {

class RenderObject;
class RenderElement;

// Provides useful logging for all layout() operations in SVG
class SVGLayoutLogger : protected SVGLogger {
    WTF_MAKE_NONCOPYABLE(SVGLayoutLogger);
public:
    SVGLayoutLogger(WTFLogLevel = WTFLogLevel::Info);

    void dump(const RenderElement&, const RenderObject* rendererToMark = nullptr);
    using SVGLogger::loggingDisabled;

private:
    struct RendererScope {
        RendererScope(SVGLayoutLogger& _logger, const RenderElement& renderer)
            : logger(_logger)
        {
            logger.addRenderer(renderer);
        }

        ~RendererScope()
        {
            logger.popLinePrefix();
        }

        SVGLayoutLogger& logger;
    };

    void visitRenderTree(const RenderElement&, const RenderObject* rendererToMark = nullptr);

    OptionSet<SVGLogger::BoxOptions> boxOptionsFromRenderer(const RenderElement&);
    void addRenderer(const RenderElement&);
    void addSVGGeometryInformation(const RenderElement&);
    void addCSSGeometryInformation(const RenderElement&);
    void addSVGCSSGeometryInformation(const RenderElement&);
    void addLayerInformation(const RenderElement&);
};

// TODO: Eventually move to LayerFragment.h
TextStream& operator<<(TextStream&, const LayerFragment&);

} // namespace WebCore
