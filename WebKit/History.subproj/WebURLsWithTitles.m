//
//  IFURLsWithTitles.m
//  WebBrowser
//
//  Created by John Sullivan on Wed Jun 12 2002.
//  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
//

#import "IFURLsWithTitles.h"
#import <WebKit/WebKitDebug.h>

@implementation IFURLsWithTitles

+ (NSArray *)arrayWithIFURLsWithTitlesPboardType
{
    // Make a canned array so we don't construct it on the fly over and over.
    static NSArray *cannedArray = nil;

    if (cannedArray == nil) {
        cannedArray = [[NSArray arrayWithObject:IFURLsWithTitlesPboardType] retain];
    }

    return cannedArray;
}

+(void)writeURLs:(NSArray *)URLs andTitles:(NSArray *)titles toPasteboard:(NSPasteboard *)pasteboard
{
    NSMutableArray *URLStrings;
    NSMutableArray *titlesOrEmptyStrings;
    unsigned index, count;

    count = [URLs count];
    if (count == 0) {
        return;
    }

    if ([pasteboard availableTypeFromArray:[self arrayWithIFURLsWithTitlesPboardType]] == nil) {
        return;
    }

    if (count != [titles count]) {
        titles = nil;
    }

    URLStrings = [NSMutableArray arrayWithCapacity:count];
    titlesOrEmptyStrings = [NSMutableArray arrayWithCapacity:count];
    for (index = 0; index < count; ++index) {
        [URLStrings addObject:[[URLs objectAtIndex:index] absoluteString]];
        [titlesOrEmptyStrings addObject:(titles == nil) ? @"" : [titles objectAtIndex:index]];
    }

    [pasteboard setPropertyList:[NSArray arrayWithObjects:URLStrings, titlesOrEmptyStrings, nil]
                        forType:IFURLsWithTitlesPboardType];
}

+(NSArray *)titlesFromPasteboard:(NSPasteboard *)pasteboard
{
    if ([pasteboard availableTypeFromArray:[self arrayWithIFURLsWithTitlesPboardType]] == nil) {
        return nil;
    }

    return [[pasteboard propertyListForType:IFURLsWithTitlesPboardType] objectAtIndex:1];
}

+(NSArray *)URLsFromPasteboard:(NSPasteboard *)pasteboard
{
    NSArray *URLStrings;
    NSMutableArray *URLs;
    unsigned index, count;
    
    if ([pasteboard availableTypeFromArray:[self arrayWithIFURLsWithTitlesPboardType]] == nil) {
        return nil;
    }

    URLStrings = [[pasteboard propertyListForType:IFURLsWithTitlesPboardType] objectAtIndex:0];
    count = [URLStrings count];
    URLs = [NSMutableArray arrayWithCapacity:count];
    for (index = 0; index < count; ++index) {
        [URLs addObject:[NSURL URLWithString:[URLStrings objectAtIndex:index]]];
    }

    return URLs;
}

@end
