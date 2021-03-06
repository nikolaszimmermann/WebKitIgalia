if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
}

description("This test checks basic funtionalities of FileSystemSyncAccessHandle.");

var rootHandle, fileHandle, accessHandle1, accessHandle2, createError;

function finishTest(error)
{
    if (error) {
        testFailed(error);
    }
    finishJSTest();
}

function getDirectory() {
    navigator.storage.getDirectory().then((handle) => {
        rootHandle = handle;
        createFileHandle(rootHandle);
    }).catch((error) => {
        finishTest(error);
    });
}

function createFileHandle(fromHandle) {
    fromHandle.getFileHandle("sync-access-handle.txt", { "create" : true }).then((handle) => {
        fileHandle = handle;
        shouldBeEqualToString("fileHandle.name", "sync-access-handle.txt");
        shouldBeEqualToString("fileHandle.kind", "file");
        createSyncAccessHandle(fileHandle);
    }).catch((error) => {
        finishTest(error);
    });
}

function createSyncAccessHandle(fromHandle) {
    fromHandle.createSyncAccessHandle().then((handle) => {
        accessHandle1 = handle;
        debug("accessHandle1 is created");
        getSize(accessHandle1);
    }).catch((error) => {
        finishTest(error);
    });
}

function getSize(accessHandle) {
    accessHandle.getSize().then((result) => {
        size = result;
        shouldBe("size", "0");
        createSecondSyncAcessHandle(fileHandle);
    }).catch((error) => {
        finishTest(error);
    });
}

function createSecondSyncAcessHandle(fromHandle) {
    fromHandle.createSyncAccessHandle().then((handle) => {
        accessHandle2 = handle;
        // Second creation should succeed.
        debug("accessHandle2 is created");
        finishTest();
    }).catch((error) => {
        // First creation should fail as there is an active handle.
        createError = error;
        debug("accessHandle2 cannot be created");
        shouldBeEqualToString("createError.toString()", "InvalidStateError: The object is in an invalid state.");
        close();
    });
}

function close() {
    accessHandle1.close().then((result) => {
        debug("accessHandle1 is closed");
        createSecondSyncAcessHandle(fileHandle);
    }).catch((error) => {
        finishTest(error);
    });
}

getDirectory();
