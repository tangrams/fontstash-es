//
//  ViewController.h
//  FontstashiOS
//
//  Created by Karim Naaji on 07/10/2014.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#define GLFONS_DEBUG
#define GLFONTSTASH_IMPLEMENTATION
#import "glfontstash.h"

#define NB_TEXT 5

@interface ViewController : GLKViewController {
    FONScontext* fs;

    int amiri;
    int dejavu;
    int han;
    int hindi;

    int width;
    int height;
    float dpiRatio;
    
    fsuint textBuffer, textIds[NB_TEXT];
}

@end

int nextPowerOf2(int value) {
    return pow(2, ceil(log(value) / log(2)));
}