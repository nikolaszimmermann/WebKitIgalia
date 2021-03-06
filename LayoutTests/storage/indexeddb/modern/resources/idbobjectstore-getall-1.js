description("Test IDBObjectStore.getAll()");

indexedDBTest(prepareDatabase);

function done()
{
    finishJSTest();
}

function log(message)
{
    debug(message);
}

var testGenerator;

function continueWithEvent(event)
{
    testGenerator.next(event);
}

function asyncContinue()
{
    setTimeout("testGenerator.next();", 0);
}

function idbRequest(request)
{
    request.onerror = continueWithEvent;
    request.onsuccess = continueWithEvent;
	return request;
}

var db;

function prepareDatabase(event)
{
    debug("Initial upgrade needed: Old version - " + event.oldVersion + " New version - " + event.newVersion);

    db = event.target.result;
    os = db.createObjectStore("foo");
	os.add(false, 1);
	os.add(-10, 2);
	os.add(10, 3);
	os.add("hello", 4);
	os.add("hellothere", 5);

    event.target.transaction.oncomplete = function() {
        testGenerator = testSteps();
        testGenerator.next();
    };
}

function dumpArray(array)
{
	if (!array) {
		debug("Undefined array");
		return;
	}

	var string = "[ ";
	for (var i = 0; i < array.length; ++i)
		string += "'" + array[i] + "' ";
	string += "]";
	
	debug(string);
}

function is(a, b, message) {
    result = a == b ? "true" : "false";
    debug(message + ": " + result);
}

function* testSteps()
{
    objectStore = db.transaction("foo").objectStore("foo");
 
	// Get everything
    req = idbRequest(objectStore.getAll());
    event = yield;    
	debug("getAll() result is:");
	dumpArray(req.result);

	// Get everything, limit to 4
    req = idbRequest(objectStore.getAll(undefined, 4));
    event = yield;    
	debug("getAll(undefined, 4) result is:");
	dumpArray(req.result);
	
	// Non-existent key
    req = idbRequest(objectStore.getAll(6));
    event = yield;    
	debug("getAll(6) result is:");
	dumpArray(req.result);

	// Existent key
    req = idbRequest(objectStore.getAll(3));
    event = yield;    
	debug("getAll(3) result is:");
	dumpArray(req.result);
	
	// Key range only
    req = idbRequest(objectStore.getAll(IDBKeyRange.only(5)));
    event = yield;    
	debug("getAll(IDBKeyRange.only(5)) result is:");
	dumpArray(req.result);

	// Key range lower bound
    req = idbRequest(objectStore.getAll(IDBKeyRange.lowerBound(2)));
    event = yield;    
	debug("getAll(IDBKeyRange.lowerBound(2)) result is:");
	dumpArray(req.result);

	// Key range upper bound
    req = idbRequest(objectStore.getAll(IDBKeyRange.upperBound(2)));
    event = yield;    
	debug("getAll(IDBKeyRange.upperBound(2)) result is:");
	dumpArray(req.result);

	// Key range bound
    req = idbRequest(objectStore.getAll(IDBKeyRange.bound(2, 4)));
    event = yield;    
	debug("getAll(IDBKeyRange.bound(2, 4)) result is:");
	dumpArray(req.result);
    
    finishJSTest();
 }
 