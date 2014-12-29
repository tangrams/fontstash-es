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

    fsuint textar1;
    fsuint textar2;
    fsuint textfr1;
    fsuint textch1;
    fsuint texthi1;

    std::vector<fsuint> texts;
}

@end
