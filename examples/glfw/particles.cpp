#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <cmath>

//#define GLFONS_DEBUG
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"
#include <random>
#include <iostream>

GLFWwindow* window;
float width = 800, height = 600, dpiRatio = 1;

#define NB_TEXT 8000

FONScontext* ftCtx;
fsuint textBuffer;

int nextPowerOf2(int value) {
    return pow(2, ceil(log(value) / log(2)));
}

int main() {
    int fbWidth, fbHeight;
    
    // GL context init
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 2);
    window = glfwCreateWindow(width, height, "fontstash-es", NULL, NULL);
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    dpiRatio = fbWidth / width;
    glfwMakeContextCurrent(window);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    
    // init font context
    GLFONSparams params;
    params.useGLBackend = true; // if not set to true, you must provide your own gl backend
    ftCtx = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, nullptr);
    fonsAddFont(ftCtx, "Arial", "/Library/Fonts/Arial.ttf");
    
    // set the screen size for font context transformations
    glfonsScreenSize(ftCtx, width * dpiRatio, height * dpiRatio);

    // create and bind buffer
    glfonsBufferCreate(ftCtx, nextPowerOf2(NB_TEXT), &textBuffer);

    // generate text ids for the currently bound text buffer
    
    struct Text {
        fsuint id[NB_TEXT];
        float x[NB_TEXT];
        float y[NB_TEXT];
    } texts;
    
    glfonsGenText(ftCtx, NB_TEXT, texts.id);
    
    // rasterize some text
    fonsSetBlur(ftCtx, 2.5);
    fonsSetBlurType(ftCtx, FONS_EFFECT_DISTANCE_FIELD);
    fonsSetSize(ftCtx, 20.0 * dpiRatio);
    
    std::default_random_engine generator;
    std::uniform_real_distribution<double> xDistribution(30.0, width - 30.0);
    std::uniform_real_distribution<double> yDistribution(30.0, height - 30.0);
    std::uniform_int_distribution<int> alphabet('a', 'z');
    
    for (int i = 0; i < NB_TEXT; ++i) {
        std::string str = std::string("") + (char)alphabet(generator);
        glfonsRasterize(ftCtx, texts.id[i], str.c_str());
        texts.x[i] = xDistribution(generator) + (width / 2.0f) * dpiRatio;
        texts.y[i] = yDistribution(generator) + (height / 2.0f) * dpiRatio;
        glfonsTransform(ftCtx, texts.id[i], texts.x[i] * dpiRatio, texts.y[i] * dpiRatio, 0.0, 1.0);
    }
    
    // push transforms of currently bound buffer buffer to gpu
    glfonsUpdateTransforms(ftCtx);
    
    // upload rasterized data of currently bound buffer to gpu
    glfonsUpload(ftCtx);
    
    while (!glfwWindowShouldClose(window)) {
        double t = glfwGetTime();
        glViewport(0, 0, width * dpiRatio, height * dpiRatio);
        glfonsScreenSize(ftCtx, width * dpiRatio, height * dpiRatio);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        clock_t begin = clock();
        
        // push transforms to gpu
        glfonsUpdateTransforms(ftCtx);
        
        for (int i = 0; i < NB_TEXT; ++i) {
            glfonsTransform(ftCtx, texts.id[i], texts.x[i] * dpiRatio, texts.y[i] * dpiRatio + cos(t + i) * 10.0, cos(t + i), cos(t + i) * 0.5 + 0.5);
        }
        
        // render the text
        glfonsDraw(ftCtx);
        
        clock_t end = clock();
        std::cout << "Frame time: " << float(end - begin) / CLOCKS_PER_SEC * 1000 << "ms" << std::endl;
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // release font resources
    glfonsDelete(ftCtx);
    
    return 0;
}
