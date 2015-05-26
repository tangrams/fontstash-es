#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <random>
#include <stack>
#include <GLFW/glfw3.h>

#define GLFONS_DEBUG
#define GLFONTSTASH_IMPLEMENTATION
#import "glfontstash.h"

GLFWwindow* window;
float width = 800, height = 600, dpiRatio = 1;

#define NB_TEXT 5
#define TEXT_TRANSFORM_RESOLUTION 32

FONScontext* ftCtx;
fsuint textBuffer, textIds[NB_TEXT];

int main() {
    int fbWidth, fbHeight;
    
    // context init
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 2);
    window = glfwCreateWindow(width, height, "fontstash-es", NULL, NULL);
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    dpiRatio = fbWidth / width;
    glfwMakeContextCurrent(window);
    
    GLFONSparams params;
    params.useGLBackend = true;
    ftCtx = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, nullptr);
    
    // init font
    fonsSetSize(ftCtx, 20.0);
    fonsAddFont(ftCtx, "Arial", "/Library/Fonts/Arial.ttf");
    
    glfonsBufferCreate(ftCtx, TEXT_TRANSFORM_RESOLUTION, &textBuffer);
    glfonsBindBuffer(ftCtx, textBuffer);
    glfonsGenText(ftCtx, NB_TEXT, textIds);
    
    for(int i = 0; i < NB_TEXT; ++i) {
        glfonsRasterize(ftCtx, textIds[i], "Some text");
    }
    
    glfonsUpload(ftCtx);
    glfonsBindBuffer(ftCtx, 0);

    while (!glfwWindowShouldClose(window)) {
        glViewport(0, 0, width * dpiRatio, height * dpiRatio);
        glfonsScreenSize(ftCtx, width * dpiRatio, height * dpiRatio);
        glClearColor(0.18f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // render text
        glfonsDraw(ftCtx);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // release font resources
    glfonsDelete(ftCtx);
    
    return 0;
}

