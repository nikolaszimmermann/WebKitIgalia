/*
        WebNetscapePluginStream.h
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <Foundation/Foundation.h>

#import <WebKit/npapi.h>
#import <WebKit/WebBaseNetscapePluginStream.h>

@class WebNetscapePluginEmbeddedView;
@class NSURLConnection;
@class NSURLRequest;


@interface WebNetscapePluginStream : WebBaseNetscapePluginStream 
{
    WebNetscapePluginEmbeddedView *view;
    NSMutableData *resourceData;
    NSURLRequest *_startingRequest;
}

- (id)initWithRequest:(NSURLRequest *)theRequest
        pluginPointer:(NPP)thePluginPointer
           notifyData:(void *)theNotifyData;

- (void)start;

- (void)stop;

@end
