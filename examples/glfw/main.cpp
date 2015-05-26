#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <random>
#include <stack>
#include <GLFW/glfw3.h>

#define GLFONTSTASH_IMPLEMENTATION
#import "glfontstash.h"

GLFWwindow* window;
float width = 800;
float height = 600;
float dpiRatio = 1;

FONScontext* ftCtx;

void update() {
    double time = glfwGetTime();
}

void init() {
    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(width, height, "isect2d", NULL, NULL);

    if (!window) {
        glfwTerminate();
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    dpiRatio = fbWidth / width;

    glfwMakeContextCurrent(window);
}

void render() {
    while (!glfwWindowShouldClose(window)) {
        update();

        glViewport(0, 0, width * dpiRatio, height * dpiRatio);
        
        glfonsScreenSize(ftCtx, width * dpiRatio, height * dpiRatio);
        
        glClearColor(0.18f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }
}

void initFontContext() {
    GLFONSparams params;

    params.useGLBackend = true;
    
    ftCtx = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, nullptr);
    
    fonsSetSize(ftCtx, 20.0);
    fonsAddFont(ftCtx, "Arial", "/Library/Fonts/Arial.ttf");
}

int main() {

    init();
    initFontContext();
    render();
    
    glfonsDelete(ftCtx);

    return 0;
}

