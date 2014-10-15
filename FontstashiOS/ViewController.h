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

#define GLFONTSTASH_IMPLEMENTATION
#import "glfontstash.h"

@interface ViewController : GLKViewController {
    FONScontext* fs;
    int fontNormal;
    int fontItalic;
    fsuint text1;
}
@end
