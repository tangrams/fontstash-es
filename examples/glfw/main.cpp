#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <cmath>

//#define GLFONS_DEBUG
#define GLFONTSTASH_IMPLEMENTATION
#import "glfontstash.h"

GLFWwindow* window;
float width = 800, height = 600, dpiRatio = 1;

#define NB_TEXT 5

FONScontext* ftCtx;
fsuint textBuffer, textIds[NB_TEXT];

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
    glfonsGenText(ftCtx, NB_TEXT, textIds);

    // rasterize some text
    fonsSetBlur(ftCtx, 2.5);
    fonsSetBlurType(ftCtx, FONS_EFFECT_DISTANCE_FIELD);
    fonsSetSize(ftCtx, 20.0 * dpiRatio);
    glfonsRasterize(ftCtx, textIds[0], "the quick brown fox");
    fonsSetSize(ftCtx, 40.0 * dpiRatio);
    glfonsRasterize(ftCtx, textIds[1], "jumps over the lazy dog");
    fonsSetSize(ftCtx, 60.0 * dpiRatio);
    glfonsRasterize(ftCtx, textIds[2], "the quick brown fox jumps over the lazy dog");
    glfonsRasterize(ftCtx, textIds[3], "конец konéts");
    glfonsRasterize(ftCtx, textIds[4], "fontstash-es");

    for(int i = 0; i < NB_TEXT; ++i) {
        glfonsTransform(ftCtx, textIds[i], (100.0 + i * 10.0) * dpiRatio, (100.0 + i * 50.0) * dpiRatio, 0.0, 0.6 + (NB_TEXT / 0.5) * i);
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

        glfonsTransform(ftCtx, textIds[0], (width / 2.0) * dpiRatio, (height / 2.0) * dpiRatio, cos(t) * 0.5, cos(t) * 0.5 + 0.5);
        glfonsTransform(ftCtx, textIds[4], (width / 2.0) * dpiRatio, (height / 2.0 - 200.0 + cos(t) * 20.0) * dpiRatio, 0.0, 1.0);
        // push transforms to gpu
        glfonsUpdateTransforms(ftCtx);

        glfonsSetColor(ftCtx, 0x000000);
        // render the text
        glfonsDraw(ftCtx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release font resources
    glfonsDelete(ftCtx);

    return 0;
}
