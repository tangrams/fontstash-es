//
//  ViewController.m
//  FontstashiOS
//
//  Created by Karim Naaji on 07/10/2014.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import "ViewController.h"

@interface ViewController () {

}
@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    [self setupGL];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    int width = self.view.bounds.size.width;
    int height = self.view.bounds.size.height;
    
    glViewport(0, 0, width, height);
    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    
    fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
    if (fs == NULL) {
        printf("Could not create stash.\n");
    }

    char * resourcePath = (char*)[[[NSBundle mainBundle] pathForResource:@"DroidSerif-Regular" ofType:@"ttf"] UTF8String];
    
    fontNormal = fonsAddFont(fs, "sans", resourcePath);
    
    if (fontNormal == FONS_INVALID) {
        printf("Could not add font normal.\n");
    }
    
    resourcePath = (char*)[[[NSBundle mainBundle] pathForResource:@"DroidSerif-Italic" ofType:@"ttf"] UTF8String];
    
    fontItalic = fonsAddFont(fs, "sans-italic", resourcePath);
    
    fonsSetSize(fs, 48.0f);
    fonsSetFont(fs, fontNormal);
    
    glfonsBufferText(fs, "Fontstash", &text1);
    
    fonsSetSize(fs, 100.0f);
    fonsSetFont(fs, fontItalic);
    
    glfonsBufferText(fs, "A1234 &$é", &text2);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    glfonsDelete(fs);
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    
    static float x;
    x += .05f;
    float xnorm = (sin(x) + 1.0) * 0.5;
    
    glfonsSetColor(fs, 255, 255, 255, 255);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 100.0, 100.0);
        glfonsDrawText(fs, text2);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, 100.0 + xnorm * 10);
            glfonsDrawText(fs, text2);
    
            glfonsPushMatrix(fs);
                glfonsTranslate(fs, 0.0, 150.0);
                glfonsSetColor(fs, 130, 80, 95, xnorm * 255);
                glfonsDrawText(fs, text2);
            glfonsPopMatrix(fs);
    
        glfonsPopMatrix(fs);
    glfonsPopMatrix(fs);

    glfonsSetColor(fs, 120, 120, 100, 30);
    
    glfonsPushMatrix(fs);
        glfonsScale(fs, 4.0 * xnorm + 1.0, 4.0 * xnorm + 1.0);
        glfonsTranslate(fs, 100.0, 300.0);
        glfonsDrawText(fs, text1);
    
        for(int i = 0; i < 4; ++i) {
            glfonsSetColor(fs, 120, 120, 100, (i + 1) * 50);
            glfonsTranslate(fs, 20.0, 0.0);
            glfonsRotate(fs, 20.0);
            glfonsDrawText(fs, text1);
        }
    glfonsPopMatrix(fs);
    
    glfonsSetColor(fs, 120, 140, 140, 200);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 80.0, 300.0);
        glfonsDrawText(fs, text1);
    
        glfonsPushMatrix(fs);
            glfonsRotate(fs, xnorm * 360);
            glfonsTranslate(fs, 0.0, 10.0);
            glfonsDrawText(fs, text1);
    
            glfonsPushMatrix(fs);
                glfonsTranslate(fs, 0.0, 10.0);
                glfonsDrawText(fs, text1);
            glfonsPopMatrix(fs);
        glfonsPopMatrix(fs);
    
        // TODO : fix this (last transforms are missing)
        glfonsTranslate(fs, 0.0, 10.0);
        glfonsDrawText(fs, text1);
    glfonsPopMatrix(fs);
}

@end
