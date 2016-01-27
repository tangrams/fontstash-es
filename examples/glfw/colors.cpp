#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <cmath>

#define GLFONS_DEBUG
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"
#include <random>
#include <iostream>

GLFWwindow* window;
float width = 800, height = 600, dpiRatio = 1;

#define NB_TEXT 3500
#define NB_BUFFER 5

FONScontext* ftCtx;
fsuint textBuffers[NB_BUFFER];

int main() {
    std::default_random_engine generator;
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
    ftCtx = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT | FONS_NORMALIZE_TEX_COORDS, params, nullptr);
    //fonsAddFont(ftCtx, "Arial", "/Library/Fonts/Arial.ttf");
    //fonsAddFont(ftCtx, "Arial", "/usr/share/fonts/TTF/DejaVuSans.ttf");
    fonsAddFont(ftCtx, "Arial", "amiri-regular.ttf");

    // set the screen size for font context transformations
    glfonsScreenSize(ftCtx, width * dpiRatio, height * dpiRatio);

    std::uniform_int_distribution<int> colors(0, 255);

    // create and bind buffer
    for (int i = 0; i < NB_BUFFER; ++i) {
        glfonsBufferCreate(ftCtx, &textBuffers[i]);
        int col = glfonsRGBA(colors(generator), colors(generator), colors(generator), 255);
        glfonsSetColor(ftCtx, col);
    }

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

    std::uniform_real_distribution<double> xDistribution(30.0 * dpiRatio, width * dpiRatio - 30.0 * dpiRatio);
    std::uniform_real_distribution<double> yDistribution(30.0 * dpiRatio, height * dpiRatio - 30.0 * dpiRatio);
    std::uniform_int_distribution<int> alphabet('!', '~');

    for (int i = 0; i < NB_TEXT; ++i) {
        fsuint buffer = floor(i / (NB_TEXT / NB_BUFFER)) + 1;
        glfonsBindBuffer(ftCtx, buffer);
        std::string str = std::string("") + (char)alphabet(generator);
        glfonsRasterize(ftCtx, texts.id[i], str.c_str());
        texts.x[i] = xDistribution(generator);
        texts.y[i] = yDistribution(generator);
        glfonsTransform(ftCtx, texts.id[i], texts.x[i], texts.y[i], 0.0, 0.0);
    }

    while (!glfwWindowShouldClose(window)) {
        double t = glfwGetTime();
        glViewport(0, 0, width * dpiRatio, height * dpiRatio);
        glfonsScreenSize(ftCtx, width * dpiRatio, height * dpiRatio);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        clock_t begin = clock();
        for (int i = 0; i < NB_TEXT; ++i) {
            fsuint buffer = floor(i / (NB_TEXT / NB_BUFFER)) + 1;
            glfonsBindBuffer(ftCtx, buffer);
            glfonsTransform(ftCtx, texts.id[i], texts.x[i], texts.y[i] + cos(t + i), cos(t + i), cos(t + i) * 0.5 + 0.5);
        }

        for (int i = 0; i < NB_BUFFER; ++i) {
            glfonsBindBuffer(ftCtx, textBuffers[i]);
            glfonsUpdateBuffer(ftCtx);
        }

        // render the text
        glfonsDraw(ftCtx);
        clock_t end = clock();
        glFinish();
        std::cout << "Frame time: " << float(end - begin) / CLOCKS_PER_SEC * 1000 << "ms" << std::endl;


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release font resources
    glfonsDelete(ftCtx);

    return 0;
}
