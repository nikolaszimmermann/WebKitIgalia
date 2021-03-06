// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 15.2.3.7-6-a-93-2
description: >
    Object.defineProperties will update [[Value]] attribute of indexed
    data property 'P' successfully when [[Configurable]] attribute is
    true and [[Writable]] attribute is false but not when both are
    false (8.12.9 - step Note & 10.a.ii.1)
includes: [propertyHelper.js]
---*/


var obj = {};

Object.defineProperty(obj, "0", {
  value: 1001,
  writable: false,
  configurable: true
});

Object.defineProperty(obj, "1", {
  value: 1003,
  writable: false,
  configurable: false
});

try {
  Object.defineProperties(obj, {
    0: {
      value: 1002
    },
    1: {
      value: 1004
    }
  });

  throw new Test262Error("Expected an exception.");
} catch (e) {
  verifyEqualTo(obj, "0", 1002);

  verifyNotWritable(obj, "0");

  verifyNotEnumerable(obj, "0");

  verifyConfigurable(obj, "0");
  verifyEqualTo(obj, "1", 1003);

  verifyNotWritable(obj, "1");

  verifyNotEnumerable(obj, "1");

  verifyNotConfigurable(obj, "1");

  if (!(e instanceof TypeError)) {
    throw new Test262Error("Expected TypeError, got " + e);
  }

}
