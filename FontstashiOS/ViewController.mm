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

    CGSize screen = [UIScreen mainScreen].bounds.size;
    
    glViewport(0, 0, screen.width, screen.height);
    glClearColor(0.25f, 0.25f, 0.28f, 1.0f);

    [self createFontContext];

    glfonsUpdateViewport(fs, screen.width, screen.height);
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

- (void)renderFont
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    glfonsSetColor(fs, 255, 255, 255);

    float xnorm = (sin(x) + 1.0) * 0.5;

    glfonsTransform(fs, textar1, 50, 150, 0, 1.0);
    glfonsTransform(fs, textar2, 50, 350, 0, 1.0);
    glfonsTransform(fs, textch1, 200, 500, 0, 1.0);
    glfonsTransform(fs, texthi1, 50, 420, 0, 1.0);
    glfonsTransform(fs, textfr1, 50 + 20 * xnorm, 550 + 20 * xnorm, xnorm * 2.0 * M_PI, 1.0);

    glfonsDraw(fs);

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

    resourcePath = (char*)[[bundle pathForResource:@"amiri-regular"
                                            ofType:@"ttf"] UTF8String];

    amiri = fonsAddFont(fs, "amiri", resourcePath);

    if (amiri == FONS_INVALID) {
        NSLog(@"Could not add font normal");
    }

    resourcePath = (char*)[[bundle pathForResource:@"DejaVuSerif"
                                            ofType:@"ttf"] UTF8String];

    dejavu = fonsAddFont(fs, "droid-serif", resourcePath);

    if(dejavu == FONS_INVALID) {
        NSLog(@"Could not add font droid serif");
    }

    resourcePath = (char*)[[bundle pathForResource:@"fireflysung"
                                            ofType:@"ttf"] UTF8String];

    han = fonsAddFont(fs, "fireflysung", resourcePath);

    if(han == FONS_INVALID) {
        NSLog(@"Could not add font droid sans japanese");
    }

    resourcePath = (char*)[[bundle pathForResource:@"Sanskrit2003"
                                            ofType:@"ttf"] UTF8String];

    hindi = fonsAddFont(fs, "Sanskrit2003", resourcePath);

    if(hindi == FONS_INVALID) {
        NSLog(@"Could not add font Sanskrit2003");
    }
}

- (void)createFontContext
{
    fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
    
    if (fs == NULL) {
        NSLog(@"Could not create font context");
    }
    
    [self loadFonts];

    fonsSetFont(fs, han);
    fonsSetSize(fs, 100.0);
    fonsSetShaping(fs, "han", "TTB", "ch");
    glfonsBufferText(fs, "緳 踥踕", &textch1, FONS_EFFECT_NONE);

    fonsClearState(fs);

    fonsSetFont(fs, amiri);

    fonsSetSize(fs, 200.0);
    fonsSetShaping(fs, "arabic", "RTL", "ar");
    glfonsBufferText(fs, "سنالى ما شاسعة وق", &textar1, FONS_EFFECT_NONE);

    fonsClearState(fs);

    fonsSetSize(fs, 100.0);
    fonsSetShaping(fs, "arabic", "RTL", "ar");
    glfonsBufferText(fs, "تسجّل يتكلّم", &textar2, FONS_EFFECT_NONE);

    fonsClearState(fs);

    fonsSetFont(fs, dejavu);

    fonsSetSize(fs, 50.0);
    fonsSetShaping(fs, "french", "left-to-right", "fr");
    glfonsBufferText(fs, "ffffi", &textfr1, FONS_EFFECT_NONE);

    fonsClearState(fs);

    fonsSetFont(fs, hindi);
    fonsSetSize(fs, 200.0);
    fonsSetShaping(fs, "devanagari", "LTR", "hi");
    glfonsBufferText(fs, "हालाँकि प्रचलित रूप पूजा", &texthi1, FONS_EFFECT_NONE);

    glfonsUploadVertices(fs);

}

- (void)deleteFontContext
{
    glfonsDelete(fs);
}

@end
