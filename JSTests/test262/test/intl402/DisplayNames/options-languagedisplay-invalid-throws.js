// Copyright (C) 2021 the V8 project authors. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-Intl.DisplayNames
description: >
  Return abrupt completion from an invalid fallback option
info: |
  Intl.DisplayNames ( locales , options )

  1. If NewTarget is undefined, throw a TypeError exception.
  2. Let displayNames be ? OrdinaryCreateFromConstructor(NewTarget, "%DisplayNamesPrototype%",
    « [[InitializedDisplayNames]], [[Locale]], [[Style]], [[Type]], [[Fallback]], [[Fields]] »).
  ...
  4. Let options be ? ToObject(options).
  ...
  8. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
  ...
  12. Let type be ? GetOption(options, "type", "string", « "language", "region", "script", "currency" », "language").
  13. If type is undefined, throw a TypeError exception.
  ...
  24. Let languageDisplay be ? GetOption(options, "languageDisplay", "string", « "dialect", "standard" », "dialect").
  ...

  GetOption ( options, property, type, values, fallback )

  1. Let value be ? Get(options, property).
  2. If value is not undefined, then
    ...
    c. If type is "string", then
      i. Let value be ? ToString(value).
    d. If values is not undefined, then
      i. If values does not contain an element equal to value, throw a RangeError exception.
  ...
features: [Intl.DisplayNames-v2]
locale: [en]
---*/

assert.throws(RangeError, () => {
  new Intl.DisplayNames('en', {languageDisplay: 'err', type: 'language'});
}, 'err');

assert.throws(RangeError, () => {
  new Intl.DisplayNames('en', {languageDisplay: 'standar', type: 'language'});
}, 'standar, not standard');

assert.throws(RangeError, () => {
  new Intl.DisplayNames('en', {languageDisplay: null, type: 'language'});
}, 'null');

assert.throws(RangeError, () => {
  new Intl.DisplayNames('en', {languageDisplay: '', type: 'language'});
}, 'the empty string');

assert.throws(RangeError, () => {
  new Intl.DisplayNames('en', {languageDisplay: ['dialect', 'standard'], type: 'language'});
}, 'an array with the valid options is not necessarily valid');
