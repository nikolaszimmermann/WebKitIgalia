/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "RenderSVGResourceFilter.h"

#include "ElementChildIterator.h"
#include "FilterEffect.h"
#include "FloatPoint.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "ImageData.h"
#include "IntRect.h"
#include "Logging.h"
#include "RenderSVGResourceFilterInlines.h"
#include "RenderSVGResourceFilterPrimitive.h"
#include "RenderView.h"
#include "SVGElementTypeHelpers.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGNames.h"
#include "SVGRenderingContext.h"
#include "Settings.h"
#include "SourceGraphic.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/text/TextStream.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderSVGResourceFilter);

RenderSVGResourceFilter::RenderSVGResourceFilter(SVGFilterElement& element, RenderStyle&& style)
    : RenderSVGResourceContainer(element, WTFMove(style))
{
}

RenderSVGResourceFilter::~RenderSVGResourceFilter() = default;

void RenderSVGResourceFilter::removeAllClientsFromCache(bool markForInvalidation)
{
    LOG(Filters, "RenderSVGResourceFilter %p removeAllClientsFromCache", this);

    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceFilter::removeClientFromCache(RenderElement& client, bool markForInvalidation)
{
    LOG(Filters, "RenderSVGResourceFilter %p removing client %p", this, &client);
    
    if (client.hasLayer() && !client.renderTreeBeingDestroyed())
        downcast<RenderLayerModelObject>(client).layer()->styleChanged(StyleDifference::Repaint, &client.style());

    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

bool RenderSVGResourceFilter::applyResource(RenderElement&, const RenderStyle&, GraphicsContext*&, OptionSet<RenderSVGResourceMode>)
{
    ASSERT_NOT_REACHED();
    return false;
}

FloatRect RenderSVGResourceFilter::resourceBoundingBox(const RenderObject& object)
{
    return SVGLengthContext::resolveRectangle<SVGFilterElement>(filterElement(), filterElement().filterUnits(), object.objectBoundingBox());
}

void RenderSVGResourceFilter::primitiveAttributeChanged(RenderObject*, const QualifiedName&)
{
    // FIXME: RenderLayerFilters does not cache the SVGFilterBuilder.
    // Therefore we have no way to map the 'object' to a certain FilterEffect.
    // --> For now we have to rebuilt the entire filter chain.
    markAllClientsForInvalidation(LayoutAndBoundariesInvalidation);
    markAllClientLayersForInvalidation();
}

} // namespace WebCore
