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

    char * resourcePath = (char*)[[[NSBundle mainBundle] pathForResource:@"amiri-regular" ofType:@"ttf"] UTF8String];
    
    fontNormal = fonsAddFont(fs, "sans", resourcePath);
    
    if (fontNormal == FONS_INVALID) {
        printf("Could not add font normal.\n");
    }
    
    fonsSetFont(fs, fontNormal);
    fonsSetSize(fs, 200.0);
    fonsSetShaping(fs, "arabic", "RTL", "ar");
    
    fonsSetBlur(fs, 5.0);
    fonsSetBlurType(fs, FONS_EFFECT_DISTANCE_FIELD);
    glfonsBufferText(fs, "سنالى ما شاسعة وق", &text1, FONS_EFFECT_DISTANCE_FIELD);
    
    fonsSetBlurType(fs, FONS_EFFECT_NONE);
    glfonsBufferText(fs, "سنالى ما شاسعة وق", &text2, FONS_EFFECT_NONE);
    
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
    
    glfonsSetColor(fs, 255, 255, 255, 200);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 50.0, 350.0);
        glfonsSetSDFProperties(fs, 0.2, 0.3, 0.45, 0.5, 0.7);
        glfonsDrawText(fs, text1);
    glfonsPopMatrix(fs);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 50.0, 150.0);
        glfonsDrawText(fs, text2);
    glfonsPopMatrix(fs);
}

@end
