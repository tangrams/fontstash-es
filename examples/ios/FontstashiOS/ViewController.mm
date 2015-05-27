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
- (void)deleteFontContext;

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
    glClearColor(1.f, 1.f, 1.f, 1.f);

    [self createFontContext];
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    [self deleteFontContext];
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    dpiRatio = [[UIScreen mainScreen] scale];
    width = self.view.bounds.size.width * dpiRatio;
    height = self.view.bounds.size.height * dpiRatio;

    glfonsScreenSize(fs, width, height);
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    static double t;
    t += 0.01;
    
    glViewport(0, 0, width * dpiRatio, height * dpiRatio);
    glfonsScreenSize(fs, width * dpiRatio, height * dpiRatio);
    
    glfonsTransform(fs, textIds[0], (width / 2.0) * dpiRatio, (height / 2.0) * dpiRatio, cos(t) * 0.5, cos(t) * 0.5 + 0.5);
    glfonsTransform(fs, textIds[4], (width / 2.0) * dpiRatio, (height / 2.0 - 200.0 + cos(t) * 20.0) * dpiRatio, 0.0, 1.0);
    // push transforms to gpu
    glfonsUpdateTransforms(fs);
    // render the text
    glfonsDraw(fs);
}

#pragma mark - Fontstash

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
    GLFONSparams params;
    params.useGLBackend = true; // if not set to true, you must provide your own gl backend
    fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, nullptr);
    
    if (fs == NULL) {
        NSLog(@"Could not create font context");
    }
    
    [self loadFonts];
    
    fonsSetFont(fs, amiri);
    
    // set the screen size for font context transformations
    glfonsScreenSize(fs, width * dpiRatio, height * dpiRatio);
    
    // create and bind buffer
    glfonsBufferCreate(fs, nextPowerOf2(NB_TEXT), &textBuffer);
    
    // generate text ids for the currently bound text buffer
    glfonsGenText(fs, NB_TEXT, textIds);
    
    // rasterize some text
    fonsSetBlur(fs, 2.5);
    fonsSetBlurType(fs, FONS_EFFECT_DISTANCE_FIELD);
    fonsSetSize(fs, 20.0 * dpiRatio);
    
    glfonsRasterize(fs, textIds[0], "the quick brown fox");
    fonsSetSize(fs, 40.0 * dpiRatio);
    glfonsRasterize(fs, textIds[1], "jumps over the lazy dog");
    fonsSetSize(fs, 60.0 * dpiRatio);
    glfonsRasterize(fs, textIds[2], "the quick brown fox jumps over the lazy dog");
    glfonsRasterize(fs, textIds[3], "конец konéts");
    glfonsRasterize(fs, textIds[4], "fontstash-es");
    
    for(int i = 0; i < NB_TEXT; ++i) {
        glfonsTransform(fs, textIds[i], (100.0 + i * 10.0) * dpiRatio, (100.0 + i * 50.0) * dpiRatio, 0.0, 0.6 + (NB_TEXT / 0.5) * i);
    }
    
    // push transforms of currently bound buffer buffer to gpu
    glfonsUpdateTransforms(fs);
    
    // upload rasterized data of currently bound buffer to gpu
    glfonsUpload(fs);
}

- (void)deleteFontContext
{
    glfonsDelete(fs);
}

@end
