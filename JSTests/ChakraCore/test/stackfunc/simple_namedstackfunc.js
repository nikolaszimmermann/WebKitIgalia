//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var o = {}
function test()
{
    var i = 0;
    function simple_stackfunc() // this can be stack allocated
    {
        if (i == 0)
        {
            i++;
            return simple_stackfunc();
        }
        return i;
    }
    return simple_stackfunc();
}

WScript.Echo(test());
WScript.Echo(test());

