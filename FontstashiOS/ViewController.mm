//
//  ViewController.m
//  FontstashiOS
//
//  Created by Karim Naaji on 07/10/2014.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import "ViewController.h"

@interface ViewController () 

@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;
- (void)createFontContext;
- (void)deleteFontContext;
- (void)renderFont;
- (void)loadFonts;

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
    glClearColor(0.25f, 0.25f, 0.28f, 1.0f);

    [self createFontContext];

    glfonsUpdateViewport(fs, 1);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    [self deleteFontContext];
}

#pragma mark - GLKView and GLKViewController delegate methods

static float x;

- (void)update
{
    x += .05f;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    [self renderFont];
}

#pragma mark - Fontstash

static int frames = 0;
static float avg = 0.0f;

- (void)renderFont
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    glfonsSetColor(fs, 255, 255, 255);

    float xnorm = (sin(x) + 1.0) * 0.5;

    NSDate *start = [NSDate date];

    int i = 0;
    for(auto id: texts) {
        i++;
        if(i > 10) {
            glfonsTransform(fs, id, 0.0, i * 25.0 * xnorm, 2.0 * M_PI * i, xnorm);
        } else {
            glfonsTransform(fs, id, 50.0, 150.0 + i * 25.0, 2.0 * M_PI * i, xnorm);
        }
    }

    glfonsDraw(fs);

    NSDate *end = [NSDate date];
    NSTimeInterval exec = [end timeIntervalSinceDate:start];

    frames++;
    avg += 1000.0 * exec;

    if(frames % 50 == 0) {
        NSLog(@"avg = %f ms", avg / frames);
    }

    glDisable(GL_BLEND);
    
    GLenum glError = glGetError();
    if (glError) {
        printf("GL Error %d!!!\n", glError);
        exit(0);
    }
}

- (void)loadFonts
{
    NSBundle* bundle = [NSBundle mainBundle];
    char* resourcePath;
    
    resourcePath = (char*)[[bundle pathForResource:@"DejaVuSerif"
                                            ofType:@"ttf"] UTF8String];
    
    dejavu = fonsAddFont(fs, "droid-serif", resourcePath);
}

- (void)createFontContext
{
    fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
    
    if (fs == NULL) {
        NSLog(@"Could not create font context");
    }
    
    [self loadFonts];

    fonsSetFont(fs, dejavu);

    fonsSetSize(fs, 50.0);

    for(int i = 0; i < 20; i++) {
        fsuint id;
        glfonsBufferText(fs, "text", &id, FONS_EFFECT_NONE);
        texts.push_back(id);
    }

    glfonsUploadVertices(fs);

}

- (void)deleteFontContext
{
    glfonsDelete(fs);
}

@end
