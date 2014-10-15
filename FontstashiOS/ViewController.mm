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
    
    fonsSetSize(fs, 54.0f);
    fonsSetFont(fs, fontNormal);
    
    glfonsBufferText(fs, "Fontstash", &text1);
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
    int i = 0;
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 100.0, 300.0);
        glfonsSetColor(fs, 255, 255, 255, 150);
    
        glfonsDrawText(fs, text1);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, ++i * 40.0);
            for(int i = 0; i < 9; ++i) {
                glfonsPushMatrix(fs);
                glfonsSetColor(fs, 255, 255, 255, (255 - 150)/9 * (9-i));
                glfonsDrawText(fs, text1, i, i);
                glfonsPopMatrix(fs);
            }
        glfonsPopMatrix(fs);
    
        glfonsSetColor(fs, 255, 255, 255, 150);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, ++i * 40.0);
            for(int i = 0; i < 9; ++i) {
                glfonsTranslate(fs, 0.0, sin((x + i * 3.0) * 4.0));
                glfonsPushMatrix(fs);
                glfonsDrawText(fs, text1, i, i);
                glfonsPopMatrix(fs);
            }
        glfonsPopMatrix(fs);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, ++i * 40.0);
            for(int i = 0; i < 9; ++i) {
                glfonsPushMatrix(fs);
                glfonsTranslate(fs, xnorm * glfonsGetGlyphOffset(fs, text1, i), 0.0);
                glfonsDrawText(fs, text1, i, i);
                glfonsPopMatrix(fs);
            }
        glfonsPopMatrix(fs);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, ++i * 40.0);
            for(int i = 0; i < 9; ++i) {
                glfonsPushMatrix(fs);
                glfonsScale(fs, 1.0 + i * 0.2 * xnorm, 1.0 + i * 0.2 * xnorm);
                glfonsTranslate(fs, xnorm * glfonsGetGlyphOffset(fs, text1, i), 0.0);
                glfonsDrawText(fs, text1, i, i);
            glfonsPopMatrix(fs);
            }
        glfonsPopMatrix(fs);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, ++i * 40.0);
            for(int i = 0; i < 9; ++i) {
                glfonsPushMatrix(fs);
                glfonsTranslate(fs, xnorm * glfonsGetGlyphOffset(fs, text1, i), 0.0);
                glfonsRotate(fs, 360 / 9 * i * xnorm);
                glfonsDrawText(fs, text1, i, i);
                glfonsPopMatrix(fs);
            }
        glfonsPopMatrix(fs);
    
        glfonsPushMatrix(fs);
            glfonsTranslate(fs, 0.0, ++i * 40.0);
            for(int i = 0; i < 9; ++i) {
                glfonsPushMatrix(fs);
                glfonsTranslate(fs, xnorm * -glfonsGetGlyphOffset(fs, text1, i), 0.0);
                glfonsDrawText(fs, text1, i, i);
                glfonsPopMatrix(fs);
            }
        glfonsPopMatrix(fs);
    glfonsPopMatrix(fs);
}

@end
