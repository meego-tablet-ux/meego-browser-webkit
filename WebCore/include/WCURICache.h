/*	WCURICache.h
	Copyright 2001, Apple, Inc. All rights reserved.
*/

#import <Foundation/Foundation.h>

@protocol WCURICache 

-(void)requestWithURL:(NSURL *)url requestor:(id)requestor userData:(void *)userData;
-(void)requestWithString:(NSString *)uriString requestor:(id)requestor userData:(void *)userData;

-(void)cancelRequestWithURL:(NSURL *)url requestor:(id)requestor;
-(void)cancelRequestWithString:(NSString *)uriString requestor:(id)requestor;
-(void)cancelAllRequestsWithURL:(NSURL *)url;
-(void)cancelAllRequestsWithString:(NSString *)uriString;

@end

#if defined(__cplusplus)
extern "C" {
#endif

// *** Function to access WCURICache singleton

id <WCURICache> WCGetDefaultURICache(void); 

#if defined(__cplusplus)
} // extern "C"
#endif
