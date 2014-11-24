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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    float xnorm = (sin(x) + 1.0) * 0.5;
    
    glfonsSetColor(fs, 255, 255, 255, 255);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 50.0, 150.0);
        glfonsSetOutlineColor(fs, 0, 0, 0, 255);
        glfonsSetSDFProperties(fs, 0.2, 0.3, 0.45, 0.5, 0.7);
        glfonsDrawText(fs, textar1);
    glfonsPopMatrix(fs);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 50.0, 350.0);
        glfonsDrawText(fs, textar2);
        glfonsTranslate(fs, 0.0, 250.0 - 20 * xnorm);
        glfonsDrawText(fs, textfr1);
    glfonsPopMatrix(fs);
    
    glfonsPushMatrix(fs);
        glfonsTranslate(fs, 200.0, 1200.0);
        glfonsDrawText(fs, textjp1);
    glfonsPopMatrix(fs);
    
    glDisable(GL_BLEND);
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
    
    resourcePath = (char*)[[bundle pathForResource:@"DroidSansJapanese"
                                            ofType:@"ttf"] UTF8String];
    
    droidsans = fonsAddFont(fs, "droid-sans-japanese", resourcePath);
    
    if(droidsans == FONS_INVALID) {
        NSLog(@"Could not add font droid sans japanese");
    }
}

- (void)createFontContext
{
    fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
    
    if (fs == NULL) {
        NSLog(@"Could not create font context");
    }
    
    [self loadFonts];
    
    fonsSetFont(fs, droidsans);
    fonsSetSize(fs, 100.0);
    fonsSetShaping(fs, "japanese", "TTB", "jp");
    glfonsBufferText(fs, "ヂョ郎", &textjp1, FONS_EFFECT_NONE);
    
    fonsSetFont(fs, amiri);
    
    fonsSetSize(fs, 200.0);
    fonsSetShaping(fs, "arabic", "RTL", "ar");
    fonsSetBlur(fs, 8.0);
    fonsSetBlurType(fs, FONS_EFFECT_DISTANCE_FIELD);
    glfonsBufferText(fs, "سنالى ما شاسعة وق", &textar1, FONS_EFFECT_DISTANCE_FIELD);
    
    fonsSetBlurType(fs, FONS_EFFECT_NONE);
    fonsSetSize(fs, 100.0);
    glfonsBufferText(fs, "الأمريكي، ما سقط, مع وعلى الحربي،", &textar2, FONS_EFFECT_NONE);
    
    fonsSetFont(fs, dejavu);
    
    fonsSetSize(fs, 200.0);
    fonsSetShaping(fs, "french", "left-to-right", "fr");
    glfonsBufferText(fs, "ffffi", &textfr1, FONS_EFFECT_NONE);
}

- (void)deleteFontContext
{
    glfonsUnbufferText(fs, textar1);
    glfonsUnbufferText(fs, textar2);
    glfonsUnbufferText(fs, textfr1);
    glfonsUnbufferText(fs, textjp1);
    
    glfonsDelete(fs);
}

@end
