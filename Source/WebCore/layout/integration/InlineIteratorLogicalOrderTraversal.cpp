/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InlineIteratorLogicalOrderTraversal.h"

#include "InlineIteratorLine.h"

namespace WebCore {
namespace InlineIterator {

static TextLogicalOrderCache makeTextLogicalOrderCacheIfNeeded(const RenderText& text)
{
    if (!text.containsReversedText())
        return { };

    auto cache = WTF::makeUnique<TextLogicalOrderCacheData>();
    for (auto textBox : textBoxesFor(text))
        cache->boxes.append(textBox);

    if (cache->boxes.isEmpty())
        return nullptr;

    std::sort(cache->boxes.begin(), cache->boxes.end(), [&](auto& a, auto& b) {
        return a->start() < b->start();
    });

    return cache;
}

static void updateTextLogicalOrderCacheIfNeeded(const TextBoxIterator& textBox, TextLogicalOrderCache& cache)
{
    if (!cache && !(cache = makeTextLogicalOrderCacheIfNeeded(textBox->renderer())))
        return;

    if (cache->index < cache->boxes.size() && cache->boxes[cache->index] == textBox)
        return;

    cache->index = cache->boxes.find(textBox);

    if (cache->index == notFound) {
        cache = { };
        updateTextLogicalOrderCacheIfNeeded(textBox, cache);
    }
}

std::pair<TextBoxIterator, TextLogicalOrderCache> firstTextBoxInLogicalOrderFor(const RenderText& text)
{
    if (auto cache = makeTextLogicalOrderCacheIfNeeded(text))
        return { cache->boxes.first(), WTFMove(cache) };

    return { firstTextBoxFor(text), nullptr };
}

TextBoxIterator nextTextBoxInLogicalOrder(const TextBoxIterator& textBox, TextLogicalOrderCache& cache)
{
    updateTextLogicalOrderCacheIfNeeded(textBox, cache);

    if (!cache)
        return textBox->nextTextBox();

    cache->index++;

    if (cache->index < cache->boxes.size())
        return cache->boxes[cache->index];

    return { };
}

static LineLogicalOrderCache makeLineLogicalOrderCache(const LineIterator& line)
{
    auto cache = WTF::makeUnique<LineLogicalOrderCacheData>();

    cache->line = line;
    cache->boxes = leafBoxesInLogicalOrder(line, [](auto first, auto last) {
        std::reverse(first, last);
    });

    return cache;
}

static void updateLineLogicalOrderCacheIfNeeded(const LeafBoxIterator& box, LineLogicalOrderCache& cache)
{
    auto line = box->line();
    if (!cache || cache->line != line)
        cache = makeLineLogicalOrderCache(line);

    if (cache->index < cache->boxes.size() && cache->boxes[cache->index] == box)
        return;

    cache->index = cache->boxes.find(box);

    ASSERT(cache->index != notFound);
}

LeafBoxIterator firstLeafOnLineInLogicalOrder(const LineIterator& line, LineLogicalOrderCache& cache)
{
    cache = makeLineLogicalOrderCache(line);

    if (cache->boxes.isEmpty())
        return { };

    cache->index = 0;
    return cache->boxes.first();
}

LeafBoxIterator lastLeafOnLineInLogicalOrder(const LineIterator& line, LineLogicalOrderCache& cache)
{
    cache = makeLineLogicalOrderCache(line);

    if (cache->boxes.isEmpty())
        return { };

    cache->index = cache->boxes.size() - 1;
    return cache->boxes.last();
}

LeafBoxIterator nextLeafOnLineInLogicalOrder(const LeafBoxIterator& box, LineLogicalOrderCache& cache)
{
    updateLineLogicalOrderCacheIfNeeded(box, cache);

    cache->index++;

    if (cache->index < cache->boxes.size())
        return cache->boxes[cache->index];

    return { };
}

LeafBoxIterator previousLeafOnLineInLogicalOrder(const LeafBoxIterator& box, LineLogicalOrderCache& cache)
{
    updateLineLogicalOrderCacheIfNeeded(box, cache);

    if (!cache->index)
        return { };

    cache->index--;

    return cache->boxes[cache->index];
}

LeafBoxIterator firstLeafOnLineInLogicalOrderWithNode(const LineIterator& line, LineLogicalOrderCache& cache)
{
    auto box = firstLeafOnLineInLogicalOrder(line, cache);
    while (box && !box->renderer().node())
        box = nextLeafOnLineInLogicalOrder(box, cache);
    return box;
}

LeafBoxIterator lastLeafOnLineInLogicalOrderWithNode(const LineIterator& line, LineLogicalOrderCache& cache)
{
    auto box = lastLeafOnLineInLogicalOrder(line, cache);
    while (box && !box->renderer().node())
        box = previousLeafOnLineInLogicalOrder(box, cache);
    return box;
}

}
}
