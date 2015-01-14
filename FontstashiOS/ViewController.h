//
//  ViewController.h
//  FontstashiOS
//
//  Created by Karim Naaji on 07/10/2014.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

#define FONS_USE_FREETYPE
#define GLFONTSTASH_IMPLEMENTATION
#define FONS_USE_HARFBUZZ
#import "glfontstash.h"
#import "shaders.h"
#import <vector>

#define TEXT_NUMBER 5

@interface ViewController : GLKViewController {
    NSMutableDictionary* bufferByTextId;
    FONScontext* fs;

    int amiri;
    int dejavu;
    int han;
    int hindi;

    fsuint texts[TEXT_NUMBER];
    fsuint buffer1;
    fsuint buffer2;
    int owner;

    GLuint shaderProgram;
    GLuint atlas;
    GLuint vbo1;
    GLuint vbo2;
    NSMutableDictionary* transformTextures;
}

- (void) updateAtlas:(const unsigned int*)pixels xoff:(unsigned int)xoff
                yoff:(unsigned int)yoff width:(unsigned int)width height:(unsigned int)height;
- (void) updateTransforms:(const unsigned int*)pixels xoff:(unsigned int)xoff
                yoff:(unsigned int)yoff width:(unsigned int)width height:(unsigned int)height owner:(int)ownerId;
- (void) createAtlasWithWidth:(unsigned int)width height:(unsigned int)height;
- (void) createTextureTransformsWithWidth:(unsigned int)width height:(unsigned int)height;

@end

#pragma mark glfontstash callbacks

void errorCallback(void* usrPtr, fsuint buffer, GLFONSError error)
{
    NSLog(@"Error callback");

    // callback on error
    switch(error) {
        case GLFONSError::ID_OVERFLOW:
            NSLog(@"ID overflow");
            break;
        default:
            NSLog(@"Unknown error");
    }
}

void createTexTransforms(void* userPtr, unsigned int width, unsigned int height)
{
    ViewController* view = (__bridge ViewController*) userPtr;
    [view createTextureTransformsWithWidth:width height:height];
}

void createAtlas(void* userPtr, unsigned int width, unsigned int height)
{
    ViewController* view = (__bridge ViewController*) userPtr;
    [view createAtlasWithWidth:width height:height];
}

void updateTransforms(void* userPtr, unsigned int xoff, unsigned int yoff,
                      unsigned int width, unsigned int height, const unsigned int* pixels, void* ownerPtr)
{
    ViewController* view = (__bridge ViewController*) userPtr;
    int* owner = (int*)ownerPtr;
    [view updateTransforms:pixels xoff:xoff yoff:yoff width:width height:height owner:*owner];
}

void updateAtlas(void* userPtr, unsigned int xoff, unsigned int yoff,
                  unsigned int width, unsigned int height, const unsigned int* pixels)
{
    ViewController* view = (__bridge ViewController*) userPtr;
    [view updateAtlas:pixels xoff:xoff yoff:yoff width:width height:height];
}
