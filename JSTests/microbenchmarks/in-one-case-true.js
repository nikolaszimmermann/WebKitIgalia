//@ skip if $model == "Apple Watch Series 3" # added by mark-jsc-stress-test.py
function foo(a) {
    var result = 0;
    for (var i = 0; i < a.length; ++i) {
        result <<= 1;
        result ^= "foo" in a[i];
    }
    return result;
}

function bar() {
    var array = [{foo:42}, {foo:42}];
    var result = 0;
    for (var i = 0; i < 1000000; ++i)
        result += foo(array);
    return result;
}

var result = bar();
if (result != 3000000)
    throw "Bad result: " + result;

