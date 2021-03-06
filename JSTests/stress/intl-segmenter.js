
function shouldBe(actual, expected) {
    if (actual !== expected)
        throw new Error('bad value: ' + actual);
}

function shouldNotThrow(func) {
    func();
}

{
    shouldBe(Intl.Segmenter instanceof Function, true);
    shouldBe(Intl.Segmenter.length, 0);
    shouldBe(Object.getOwnPropertyDescriptor(Intl.Segmenter, 'prototype').writable, false);
    shouldBe(Object.getOwnPropertyDescriptor(Intl.Segmenter, 'prototype').enumerable, false);
    shouldBe(Object.getOwnPropertyDescriptor(Intl.Segmenter, 'prototype').configurable, false);
    let segmenter = new Intl.Segmenter("fr");
    shouldBe(JSON.stringify(segmenter.resolvedOptions()), `{"locale":"fr","granularity":"grapheme"}`);
    shouldBe(segmenter.toString(), `[object Intl.Segmenter]`);
    shouldNotThrow(() => new Intl.Segmenter());
}

{
    let segmenter = new Intl.Segmenter("fr", {granularity: "word"});

    let input = "Moi?  N'est-ce pas.";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 3, "Moi", true ],
        [ 3, 4, "?", false ],
        [ 4, 6, "  ", false ],
        [ 6, 11, "N'est", true ],
        [ 11, 12, "-", false ],
        [ 12, 14, "ce", true ],
        [ 14, 15, " ", false ],
        [ 15, 18, "pas", true ],
        [ 18, 19, ".", false ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }

    shouldBe(JSON.stringify(Intl.Segmenter.supportedLocalesOf('fr')), `["fr"]`);
    shouldBe(JSON.stringify(segmenter.resolvedOptions()), `{"locale":"fr","granularity":"word"}`);
}

{
    let segmenter = new Intl.Segmenter("fr", {granularity: "grapheme"});

    let input = "Moi?  N'est-ce pas.";
    let segments = segmenter.segment(input);

    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let current = cursor++;
        shouldBe(segment, input[current]);
        shouldBe(index, current);
        shouldBe(isWordLike, undefined);
    }

    shouldBe(JSON.stringify(segmenter.resolvedOptions()), `{"locale":"fr","granularity":"grapheme"}`);
}

{
    let segmenter = new Intl.Segmenter("en", {granularity: "sentence"});

    let input = "Performance is a top priority for WebKit. We adhere to a simple directive for all work we do on WebKit: The way to make a program faster is to never let it get slower.";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 42, "Performance is a top priority for WebKit. ", undefined ],
        [ 42, 167, "We adhere to a simple directive for all work we do on WebKit: The way to make a program faster is to never let it get slower.", undefined ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }

    shouldBe(JSON.stringify(segmenter.resolvedOptions()), `{"locale":"en","granularity":"sentence"}`);
}

// languages without spaces.
{
    let segmenter = new Intl.Segmenter("ja", {granularity: "word"});

    // https://en.wikipedia.org/wiki/I_Am_a_Cat
    let input = "???????????????????????????????????????????????????????????????????????????????????????????????????";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 2, "??????", true ],
        [ 2, 3, "???", true ],
        [ 3, 4, "???", true ],
        [ 4, 5, "???", true ],
        [ 5, 7, "??????", true ],
        [ 7, 8, "???", false ],
        [ 8, 10, "??????", true ],
        [ 10, 11, "???", true ],
        [ 11, 13, "??????", true ],
        [ 13, 15, "??????", true ],
        [ 15, 16, "???", false ],
        [ 16, 18, "??????", true ],
        [ 18, 19, "???", true ],
        [ 19, 21, "??????", true ],
        [ 21, 23, "??????", true ],
        [ 23, 26, "?????????", true ],
        [ 26, 28, "??????", true ],
        [ 28, 29, "???", true ],
        [ 29, 30, "???", true ],
        [ 30, 31, "???", true ],
        [ 31, 32, "???", true ],
        [ 32, 33, "???", false ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }
}

{
    let segmenter = new Intl.Segmenter("ja", {granularity: "grapheme"});

    // https://en.wikipedia.org/wiki/I_Am_a_Cat
    let input = "???????????????????????????????????????????????????????????????????????????????????????????????????";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 1, "???", undefined ],
        [ 1, 2, "???", undefined ],
        [ 2, 3, "???", undefined ],
        [ 3, 4, "???", undefined ],
        [ 4, 5, "???", undefined ],
        [ 5, 6, "???", undefined ],
        [ 6, 7, "???", undefined ],
        [ 7, 8, "???", undefined ],
        [ 8, 9, "???", undefined ],
        [ 9, 10, "???", undefined ],
        [ 10, 11, "???", undefined ],
        [ 11, 12, "???", undefined ],
        [ 12, 13, "???", undefined ],
        [ 13, 14, "???", undefined ],
        [ 14, 15, "???", undefined ],
        [ 15, 16, "???", undefined ],
        [ 16, 17, "???", undefined ],
        [ 17, 18, "???", undefined ],
        [ 18, 19, "???", undefined ],
        [ 19, 20, "???", undefined ],
        [ 20, 21, "???", undefined ],
        [ 21, 22, "???", undefined ],
        [ 22, 23, "???", undefined ],
        [ 23, 24, "???", undefined ],
        [ 24, 25, "???", undefined ],
        [ 25, 26, "???", undefined ],
        [ 26, 27, "???", undefined ],
        [ 27, 28, "???", undefined ],
        [ 28, 29, "???", undefined ],
        [ 29, 30, "???", undefined ],
        [ 30, 31, "???", undefined ],
        [ 31, 32, "???", undefined ],
        [ 32, 33, "???", undefined ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }
}

{
    let segmenter = new Intl.Segmenter("ja", {granularity: "sentence"});

    // https://en.wikipedia.org/wiki/I_Am_a_Cat
    let input = "???????????????????????????????????????????????????????????????????????????????????????????????????";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 8, "????????????????????????", undefined ],
        [ 8, 16, "????????????????????????", undefined ],
        [ 16, 33, "???????????????????????????????????????????????????", undefined ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }
}

// Surrogate pairs.
{
    let segmenter = new Intl.Segmenter("ja", {granularity: "grapheme"});
    let input = "??????????";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 2, "????", undefined ],
        [ 2, 3, "???", undefined ],
        [ 3, 4, "???", undefined ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }
}

{
    let segmenter = new Intl.Segmenter("ja", {granularity: "word"});
    let input = "??????????";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 2, "????", true ],
        [ 2, 4, "??????", true ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }
}

{
    let segmenter = new Intl.Segmenter("ja", {granularity: "sentence"});
    let input = "??????????";
    let segments = segmenter.segment(input);

    let results = [
        [ 0, 4, "??????????", undefined ],
    ];
    let cursor = 0;
    for (let {segment, index, isWordLike} of segments) {
        let result = results[cursor++];
        shouldBe(result[0], index);
        shouldBe(result[1], index + segment.length);
        shouldBe(result[2], segment);
        shouldBe(result[3], isWordLike);
    }
}

{
    let segmenter = new Intl.Segmenter("ja", {granularity: "grapheme"});
    let input = "??????????";
    let segments = segmenter.segment(input);

    shouldBe(JSON.stringify(segments.containing(0)), `{"segment":"????","index":0,"input":"??????????"}`);
    shouldBe(JSON.stringify(segments.containing(1)), `{"segment":"????","index":0,"input":"??????????"}`);
    shouldBe(JSON.stringify(segments.containing(2)), `{"segment":"???","index":2,"input":"??????????"}`);
    shouldBe(JSON.stringify(segments.containing(3)), `{"segment":"???","index":3,"input":"??????????"}`);
    shouldBe(JSON.stringify(segments.containing(4)), undefined);
}

{
    // ???0 1 2 3 4 5???6???7???8???9
    // ???A l l o n s???-???y???!???
    let input = "Allons-y!";

    let segmenter = new Intl.Segmenter("fr", {granularity: "word"});
    let segments = segmenter.segment(input);
    let current = undefined;

    current = segments.containing(0);
    shouldBe(JSON.stringify(current), `{"segment":"Allons","index":0,"input":"Allons-y!","isWordLike":true}`);

    current = segments.containing(5);
    shouldBe(JSON.stringify(current), `{"segment":"Allons","index":0,"input":"Allons-y!","isWordLike":true}`);

    current = segments.containing(6);
    shouldBe(JSON.stringify(current), `{"segment":"-","index":6,"input":"Allons-y!","isWordLike":false}`);

    current = segments.containing(current.index + current.segment.length);
    shouldBe(JSON.stringify(current), `{"segment":"y","index":7,"input":"Allons-y!","isWordLike":true}`);

    current = segments.containing(current.index + current.segment.length);
    shouldBe(JSON.stringify(current), `{"segment":"!","index":8,"input":"Allons-y!","isWordLike":false}`);

    current = segments.containing(current.index + current.segment.length);
    shouldBe(JSON.stringify(current), undefined);
    // ??? undefined
}
{
    let input = "";

    let segmenter = new Intl.Segmenter("fr", {granularity: "word"});
    let segments = segmenter.segment(input);

    shouldBe(JSON.stringify(segments.containing(0)), undefined);
    let results = Array.from(segments[Symbol.iterator]());
    shouldBe(results.length, 0);
}
{
    let input = " ";

    let segmenter = new Intl.Segmenter("fr", {granularity: "word"});
    let segments = segmenter.segment(input);

    shouldBe(JSON.stringify(segments.containing(0)), `{"segment":" ","index":0,"input":" ","isWordLike":false}`);
    shouldBe(JSON.stringify(segments.containing(1)), undefined);
    shouldBe(JSON.stringify(segments.containing(2)), undefined);
    let results = Array.from(segments[Symbol.iterator]());
    shouldBe(results.length, 1);
    let {segment, index, isWordLike} = results[0];
    shouldBe(0, index);
    shouldBe(1, index + segment.length);
    shouldBe(" ", segment);
    shouldBe(false, isWordLike);
}
