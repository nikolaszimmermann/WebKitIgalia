// This file was procedurally generated from the following sources:
// - src/async-generators/yield-spread-arr-multiple.case
// - src/async-generators/default/async-declaration.template
/*---
description: Use yield value in a array spread position (Async generator Function declaration)
esid: prod-AsyncGeneratorDeclaration
features: [async-iteration]
flags: [generated, async]
includes: [compareArray.js]
info: |
    Async Generator Function Definitions

    AsyncGeneratorDeclaration:
      async [no LineTerminator here] function * BindingIdentifier ( FormalParameters ) {
        AsyncGeneratorBody }


    Array Initializer

    SpreadElement[Yield, Await]:
      ...AssignmentExpression[+In, ?Yield, ?Await]

---*/
var arr = ['a', 'b', 'c'];
var item;


var callCount = 0;

async function *gen() {
  callCount += 1;
  yield [...yield yield];
}

var iter = gen();

iter.next(false);
item = iter.next(['a', 'b', 'c']);

item.then(({ done, value }) => {
  item = iter.next(value);

  item.then(({ done, value }) => {
    assert.compareArray(value, arr);
    assert.sameValue(done, false);
  }).then($DONE, $DONE);
}).catch($DONE);

assert.sameValue(callCount, 1);
