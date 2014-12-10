//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef GLFONTSTASH_H
#define GLFONTSTASH_H

#include "glm.hpp"
#include "type_ptr.hpp"
#include "matrix_transform.hpp"

#include <iostream>
#include "fontstash.h"

#include <map>
#include <stack>

typedef unsigned int fsuint;

#define N_GLYPH_VERTS   6

struct GLFONSvbo {
    int nverts;
    unsigned int size;
    GLuint buffer;
};

typedef struct GLFONSvbo GLFONSvbo;

struct GLStash {
    GLFONSvbo* vbo;
    FONSeffectType effect;
    glm::vec4 bbox;
    float* glyphsXOffset;
    float length;
    int nbGlyph;
};

typedef struct GLStash GLStash;

struct GLFONScontext {
    GLuint tex;
    int width, height;
    // the id counter
    fsuint idct;
    
    GLuint sdfShaderProgram;
    GLuint defaultShaderProgram;
    
    GLuint posAttrib;
    GLuint texCoordAttrib;
    GLuint colorAttrib;
    
    std::map<fsuint, GLStash*>* stashes;
    
    std::stack<glm::mat4> matrixStack;
    glm::mat4 projectionMatrix;
    
    glm::vec4 color;
    glm::vec4 outlineColor;
    
    glm::vec4 sdfProperties;
    float sdfMixFactor;
    
    glm::mat4 transform;
};

typedef struct GLFONScontext GLFONScontext;

static const GLchar* vertexShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#endif
attribute vec4 a_position;
attribute vec2 a_texCoord;

uniform mat4 u_mvp;
varying vec2 f_uv;

void main() {
    gl_Position = u_mvp * a_position;
    f_uv = a_texCoord;
}
)END";

static const GLchar* sdfFragShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D u_tex;
uniform vec4 u_color;
uniform vec4 u_outlineColor;
uniform float u_mixFactor;
uniform float u_minOutlineD;
uniform float u_maxOutlineD;
uniform float u_minInsideD;
uniform float u_maxInsideD;

varying vec2 f_uv;

void main(void) {
    float distance = texture2D(u_tex, f_uv).a;
    vec4 inside = smoothstep(u_minInsideD, u_maxInsideD, distance) * u_color;
    vec4 outline = smoothstep(u_minOutlineD, u_maxOutlineD, distance) * u_outlineColor;
    gl_FragColor = mix(outline, inside, u_mixFactor);
}
)END";

static const GLchar* defaultFragShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D u_tex;
uniform vec4 u_color;

varying vec2 f_uv;

void main(void) {
    vec4 texColor = texture2D(u_tex, f_uv);
    gl_FragColor = vec4(u_color.rgb, u_color.a * texColor.a);
}
)END";

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);

void glfonsBufferText(FONScontext* ctx, const char* s, fsuint* id, FONSeffectType effect);
void glfonsUnbufferText(FONScontext* ctx, fsuint id);

void glfonsRotate(FONScontext* ctx, float angle);
void glfonsTranslate(FONScontext* ctx, float x, float y);
void glfonsScale(FONScontext* ctx, float x, float y);

void glfonsDrawText(FONScontext* ctx, fsuint id);
void glfonsDrawText(FONScontext* ctx, fsuint id, unsigned int from, unsigned int to);
void glfonsSetColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void glfonsSetOutlineColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void glfonsSetSDFProperties(FONScontext* ctx, float minOutlineD, float maxOutlineD, float minInsideD, float maxInsideD, float mixFactor);

void glfonsUpdateViewport(FONScontext* ctx);

void glfonsPushMatrix(FONScontext* ctx);
void glfonsPopMatrix(FONScontext* ctx);

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1);
float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i);
float glfonsGetLength(FONScontext* ctx, fsuint id);

int glfonsGetGlyphCount(FONScontext* ctx, fsuint id);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

static void printShaderInfoLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if(length > 1) {
        char* log = (char*)malloc(sizeof(char) * length);
        glGetShaderInfoLog(shader, length, NULL, log);
        printf("Log: %s\n", log);
        free(log);
    }
}

static GLuint compileShader(const GLchar* src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    
    if(!status) {
        printShaderInfoLog(shader);
        glDeleteShader(shader);
        exit(-1);
    }
    return shader;
}

static GLuint linkShaderProgram(const GLchar* vertexSrc, const GLchar* fragmentSrc) {
    GLuint program = glCreateProgram();
    GLuint vertex = compileShader(vertexSrc, GL_VERTEX_SHADER);
    GLuint fragment = compileShader(fragmentSrc, GL_FRAGMENT_SHADER);
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    
    if(!linkStatus) {
        printShaderInfoLog(program);
        glDeleteProgram(program);
        exit(-1);
    }
    return program;
}

static int glfons__renderCreate(void* userPtr, int width, int height) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    // Create may be called multiple times, delete existing texture.
    if (gl->tex != 0) {
        glDeleteTextures(1, &gl->tex);
        gl->tex = 0;
    }
    gl->idct = 0;
    gl->stashes = new std::map<fsuint, GLStash*>();
    
    // create texture
    glGenTextures(1, &gl->tex);
    if (!gl->tex) return 0;
    gl->width = width;
    gl->height = height;
    
    glBindTexture(GL_TEXTURE_2D, gl->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gl->projectionMatrix = glm::ortho(0.0, (double)viewport[2] * 2, (double)viewport[3] * 2, 0.0, -1.0, 1.0);
    
    // create shader
    gl->defaultShaderProgram = linkShaderProgram(vertexShaderSrc, defaultFragShaderSrc);
    gl->sdfShaderProgram = linkShaderProgram(vertexShaderSrc, sdfFragShaderSrc);
    
    gl->posAttrib = glGetAttribLocation(gl->defaultShaderProgram, "a_position");
    gl->texCoordAttrib = glGetAttribLocation(gl->defaultShaderProgram, "a_texCoord");
    
    gl->posAttrib = glGetAttribLocation(gl->sdfShaderProgram, "a_position");
    gl->texCoordAttrib = glGetAttribLocation(gl->sdfShaderProgram, "a_texCoord");
    
    gl->matrixStack.push(glm::mat4(1.0));
    gl->color = glm::vec4(1.0);
    gl->transform = glm::mat4(1.0);
    
    return 1;
}

static int glfons__renderResize(void* userPtr, int width, int height) {
    // Reuse create to resize too.
    return glfons__renderCreate(userPtr, width, height);
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    if (gl->tex == 0) return;
    
    int h = rect[3] - rect[1];
    
    glBindTexture(GL_TEXTURE_2D, gl->tex);
    const unsigned char* subdata = data + rect[1] * gl->width;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect[1], gl->width, h, GL_ALPHA, GL_UNSIGNED_BYTE, subdata);

    glBindTexture(GL_TEXTURE_2D, 0);
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
    // called by fontstash, but has nothing to do
}

static void glfons__renderDelete(void* userPtr) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    
    if (gl->tex != 0) {
        glDeleteTextures(1, &gl->tex);
    }
    
    for(auto& elmt : *gl->stashes) {
        GLStash* stash = elmt.second;
        if(stash != NULL) {
            glDeleteBuffers(1, &stash->vbo->buffer);
            delete stash->vbo;
            delete[] stash->glyphsXOffset;
            delete stash;
        }
    }
    
    glDeleteProgram(gl->defaultShaderProgram);
    glDeleteProgram(gl->sdfShaderProgram);
    
    delete gl->stashes;
    delete gl;
}

void glfonsUpdateViewport(FONScontext* ctx) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gl->projectionMatrix = glm::ortho(0.0, (double)viewport[2] * 2, (double)viewport[3] * 2, 0.0, -1.0, 1.0);
}

FONScontext* glfonsCreate(int width, int height, int flags) {
    FONSparams params;
    GLFONScontext* gl = new GLFONScontext;
    
    params.width = width;
    params.height = height;
    params.flags = (unsigned char)flags;
    params.renderCreate = glfons__renderCreate;
    params.renderResize = glfons__renderResize;
    params.renderUpdate = glfons__renderUpdate;
    params.renderDraw = glfons__renderDraw;
    params.renderDelete = glfons__renderDelete;
    params.userPtr = gl;
    
    return fonsCreateInternal(&params);
}

void glfonsDelete(FONScontext* ctx) {
    fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

void glfonsBufferText(FONScontext* ctx, const char* s, fsuint* id, FONSeffectType effect) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLStash* stash = new GLStash;

    stash->vbo = new GLFONSvbo;

    *id = ++glctx->idct;
    
    fonsDrawText(ctx, 0, 0, s, NULL, 0);

    GLfloat* interleavedArray = new GLfloat[ctx->nverts * 4];
    
    stash->effect = effect;
    stash->glyphsXOffset = new float[ctx->nverts / N_GLYPH_VERTS];
    
    int j = 0;
    for(int i = 0; i < ctx->nverts * 2; i += N_GLYPH_VERTS * 2) {
        stash->glyphsXOffset[j++] = ctx->verts[i];
    }
    
    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;
    for(int i = 0; i < ctx->nverts * 2; i += 2) {
        GLfloat x, y, u, v;

        x = ctx->verts[i];
        y = ctx->verts[i+1];
        u = ctx->tcoords[i];
        v = ctx->tcoords[i+1];

        x0 = x < x0 ? x : x0;
        x1 = x > x1 ? x : x1;
        y0 = y < y0 ? y : y0;
        y1 = y > y1 ? y : y1;

        interleavedArray[i*2] = x;
        interleavedArray[i*2+1] = y;
        interleavedArray[i*2+2] = u;
        interleavedArray[i*2+3] = v;
    }
    
    if(ctx->shaping != NULL && ctx->shaping->useShaping) {
        FONSshapingRes* res = ctx->shaping->shapingRes;
        stash->nbGlyph = res->glyphCount;
        fons__clearShaping(ctx);
    } else {
        stash->nbGlyph = (int)strlen(s);
    }
    
    stash->bbox[0] = x0; stash->bbox[1] = y0;
    stash->bbox[2] = x1; stash->bbox[3] = y1;
    
    stash->length = ctx->verts[(ctx->nverts*2)-2];
    
    glGenBuffers(1, &stash->vbo->buffer);

    glBindBuffer(GL_ARRAY_BUFFER, stash->vbo->buffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float) * ctx->nverts, interleavedArray, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete[] interleavedArray;
    
    stash->vbo->nverts = ctx->nverts;
    
    // hack : reset fontstash state
    ctx->nverts = 0;
    
    glctx->stashes->insert(std::pair<fsuint, GLStash*>(*id, stash));
}

int glfonsGetGlyphCount(FONScontext* ctx, fsuint id) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->stashes->find(id) != glctx->stashes->end()) {
        GLStash* stash = glctx->stashes->at(id);
        return stash->nbGlyph;
    }
    
    return -1;
}

void glfonsUnbufferText(FONScontext* ctx, fsuint id) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->stashes->find(id) != glctx->stashes->end()) {
        GLStash* stash = glctx->stashes->at(id);
        
        glDeleteBuffers(1, &stash->vbo->buffer);
    }
}

void glfonsRotate(FONScontext* ctx, float angle) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glm::vec3 raxis = glm::vec3(0.0, 0.0, -1.0);
    glctx->transform = glm::rotate(glctx->transform, angle, raxis);
}

void glfonsTranslate(FONScontext* ctx, float x, float y) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->transform = glm::translate(glctx->transform, glm::vec3(x, y, 0.0));
}

void glfonsScale(FONScontext* ctx, float x, float y) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->transform = glm::scale(glctx->transform, glm::vec3(x, y, 1.0));
}

void glfonsDrawText(FONScontext* ctx, fsuint id, unsigned int from, unsigned int to) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->tex == 0)
        return;
    
    int d = (to - from) + 1;
    int iFirst = from * N_GLYPH_VERTS;
    int count = d * N_GLYPH_VERTS;
    
    GLStash* stash = glctx->stashes->at(id);
    
    glm::mat4 mvp = glctx->projectionMatrix * glctx->transform;
    glm::vec4 color = glctx->color / 255.0f;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glctx->tex);
    
    GLuint program;
    switch (stash->effect) {
        case FONS_EFFECT_DISTANCE_FIELD:
            program = glctx->sdfShaderProgram;
            break;
        default:
            program = glctx->defaultShaderProgram;
    }
    
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "u_tex"), 0);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform4f(glGetUniformLocation(program, "u_color"), color.r, color.g, color.b, color.a);
    
    if(stash->effect == FONS_EFFECT_DISTANCE_FIELD) {
        glm::vec4 outlineColor = glctx->outlineColor / 255.0f;
        glUniform4f(glGetUniformLocation(program, "u_outlineColor"), outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
        glUniform1f(glGetUniformLocation(program, "u_mixFactor"), glctx->sdfMixFactor);
        glUniform1f(glGetUniformLocation(program, "u_minOutlineD"), glctx->sdfProperties[0]);
        glUniform1f(glGetUniformLocation(program, "u_maxOutlineD"), glctx->sdfProperties[1]);
        glUniform1f(glGetUniformLocation(program, "u_minInsideD"), glctx->sdfProperties[2]);
        glUniform1f(glGetUniformLocation(program, "u_maxInsideD"), glctx->sdfProperties[3]);
    }

    glBindBuffer(GL_ARRAY_BUFFER, stash->vbo->buffer);

    glVertexAttribPointer(glctx->posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(glctx->posAttrib);

    glVertexAttribPointer(glctx->texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const GLvoid*)(2 * sizeof(float)));
    glEnableVertexAttribArray(glctx->texCoordAttrib);
    
    glDrawArrays(GL_TRIANGLES, iFirst, count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    glDisableVertexAttribArray(glctx->posAttrib);
    glDisableVertexAttribArray(glctx->texCoordAttrib);

}

void glfonsDrawText(FONScontext* ctx, fsuint id) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLStash* stash = glctx->stashes->at(id);
    unsigned int last = (stash->vbo->nverts / N_GLYPH_VERTS) - 1;
    
    glfonsDrawText(ctx, id, 0, last);
}

void glfonsSetColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->color = glm::vec4(r, g, b, a);
}

void glfonsPushMatrix(FONScontext* ctx) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->matrixStack.push(glctx->transform);
}

void glfonsPopMatrix(FONScontext* ctx) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->matrixStack.size() > 0) {
        glctx->transform = glctx->matrixStack.top();
        glctx->matrixStack.pop();
    }
}

float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLStash* stash = glctx->stashes->at(id);
    
    return stash->glyphsXOffset[i];
}

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLStash* stash = glctx->stashes->at(id);
    
    *x0 = stash->bbox[0]; *y0 = stash->bbox[1];
    *x1 = stash->bbox[2]; *y1 = stash->bbox[3];
}

float glfonsGetLength(FONScontext* ctx, fsuint id) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLStash* stash = glctx->stashes->at(id);
    
    return stash->length;
}

void glfonsSetOutlineColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->outlineColor = glm::vec4(r, g, b, a);
}

void glfonsSetSDFProperties(FONScontext* ctx, float minOutlineD, float maxOutlineD, float minInsideD, float maxInsideD, float mixFactor) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    glctx->sdfProperties[0] = minOutlineD; glctx->sdfProperties[1] = maxOutlineD;
    glctx->sdfProperties[2] = minInsideD;  glctx->sdfProperties[3] = maxInsideD;
    
    glctx->sdfMixFactor = mixFactor;
}

#endif
