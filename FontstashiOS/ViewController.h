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

#import <stdio.h>
#import <string.h>
#import <vector>

#define FONS_USE_FREETYPE
#define GLFONTSTASH_IMPLEMENTATION
#define FONS_USE_HARFBUZZ
#import "glfontstash.h"

@interface ViewController : GLKViewController {
    FONScontext* fs;

    int amiri;
    int dejavu;
    int han;
    int hindi;

    fsuint texts[5];
    fsuint buffer1;
    fsuint buffer2;
}

@end


void errorCallback(void* userPtr, GLFONSbuffer* buffer, GLFONSError fonsError) {
    NSLog(@"Error callback");

    // callback on error
    switch(fonsError) {
        case GLFONSError::ID_OVERFLOW:
            NSLog(@"ID overflow");
            break;
        default:
            NSLog(@"Unknown error");
    }
}

void createTexTransforms(void* userPtr, unsigned int width, unsigned int height) {
    NSLog(@"Create texture transforms");

    // create a texture for the texture transforms data of width * heigth
}

void createAtlas(void* userPtr, unsigned int width, unsigned int height) {
    NSLog(@"Create texture atlas");

    // create a texture for the atlas of width * height
}

void updateTransforms(void* userPtr, unsigned int xoff, unsigned int yoff,
                      unsigned int width, unsigned int height, const unsigned int* pixels) {
    NSLog(@"Update transform %d %d %d %d", xoff, yoff, width, height);

    // update the transform texture
}

void updateAtlas(void* userPtr, unsigned int xoff, unsigned int yoff,
                  unsigned int width, unsigned int height, const unsigned int* pixels) {
    NSLog(@"Update atlas %d %d %d %d", xoff, yoff, width, height);

    // update the atlas texture
}

void vertexData(void* userPtr, unsigned int nVerts, const float* data) {
    NSLog(@"Upload vertex data to GPU, %d", nVerts);

    // creates a vertex buffer object, and send data to gpu
}

