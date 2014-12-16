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
    GLuint buffer;
};

typedef struct GLFONSvbo GLFONSvbo;

struct GLStash {
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
    GLuint idAttrib;
    
    std::map<fsuint, GLStash*>* stashes;

    glm::mat4 projectionMatrix;
    
    glm::vec4 color;
    glm::vec4 outlineColor;
    
    glm::vec4 sdfProperties;
    float sdfMixFactor;

    GLuint texTransform;
    glm::ivec2 transformRes;
    unsigned int* transformData;
    unsigned char* transformDirty;

    GLFONSvbo* vbo;
    float* interleavedArray;
    glm::vec2 resolution;
};

typedef struct GLFONScontext GLFONScontext;

static const GLchar* vertexShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#endif

attribute lowp float a_fsid;
attribute vec4 a_position;
attribute vec2 a_texCoord;

uniform sampler2D u_transforms;
uniform lowp vec2 u_tresolution;
uniform lowp vec2 u_resolution;
uniform mat4 u_proj;

varying vec2 f_uv;

#define PI 3.14159265358979323846

#define alpha   tdata.a
#define tx      tdata.x
#define ty      tdata.y
#define theta   tdata.z

/*
 * Converts (i, j) pixel coordinates to the corresponding (u, v) in
 * texture space. The (u,v) targets the center of pixel
 */
vec2 ij2uv(float i, float j, float w, float h) {
    return vec2(
        (2.0*i+1.0) / (2.0*w),
        (2.0*j+1.0) / (2.0*h)
    );
}

/*
 * Decodes the id and find its place for its transform inside the texture
 * Returns the (i,j) position inside texture
 */
vec2 id2ij(int fsid, float w, float h) {
    float i = mod(float(fsid), w);
    float j = floor(float(fsid) / h);
    return vec2(i, j);
}

void main() {
    vec2 ij = id2ij(int(a_fsid), u_tresolution.x, u_tresolution.y);
    vec2 uv = ij2uv(ij.x, ij.y, u_tresolution.x, u_tresolution.y);

    vec4 tdata = texture2D(u_transforms, uv);

    tx = u_resolution.x * tx;
    ty = u_resolution.y * ty;
    theta = theta * 2.0 * PI;

    float st = sin(theta);
    float ct = cos(theta);

    vec4 p = vec4(
        a_position.x * ct - a_position.y * st + tx,
        a_position.x * st + a_position.y * ct + ty,
        a_position.z,
        a_position.w
    );

    gl_Position = u_proj * p;
    f_uv = a_texCoord;
}
)END";

static const GLchar* sdfFragShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#endif

uniform lowp sampler2D u_tex;
uniform lowp vec4 u_color;
uniform lowp vec4 u_outlineColor;
uniform lowp vec4 u_sdfParams;
uniform lowp float u_mixFactor;

#define minOutlineD u_sdfParams.x
#define maxOutlineD u_sdfParams.y
#define minInsideD  u_sdfParams.z
#define maxInsideD  u_sdfParams.w

varying vec2 f_uv;

void main(void) {
    float distance = texture2D(u_tex, f_uv).a;
    vec4 inside = smoothstep(minInsideD, maxInsideD, distance) * u_color;
    vec4 outline = smoothstep(minOutlineD, maxOutlineD, distance) * u_outlineColor;

    gl_FragColor = mix(outline, inside, u_mixFactor);
}
)END";

static const GLchar* defaultFragShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#endif

uniform lowp sampler2D u_tex;
uniform lowp vec4 u_color;

varying vec2 f_uv;

void main(void) {
    vec4 texColor = texture2D(u_tex, f_uv);
    gl_FragColor = vec4(u_color.rgb, u_color.a * texColor.a);
}
)END";

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);

void glfonsBufferText(FONScontext* ctx, const char* s, fsuint* id, FONSeffectType effect);

void glfonsDraw(FONScontext* ctx);

void glfonsSetColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void glfonsSetOutlineColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void glfonsSetSDFProperties(FONScontext* ctx, float minOutlineD, float maxOutlineD, float minInsideD, float maxInsideD, float mixFactor);

void glfonsUpdateViewport(FONScontext* ctx, int scale);

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1);
float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i);
float glfonsGetLength(FONScontext* ctx, fsuint id);

int glfonsGetGlyphCount(FONScontext* ctx, fsuint id);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

static void glfons__printShaderInfoLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if(length > 1) {
        char* log = (char*)malloc(sizeof(char) * length);
        glGetShaderInfoLog(shader, length, NULL, log);
        printf("Log: %s\n", log);
        free(log);
    }
}

static GLuint glfons__compileShader(const GLchar* src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    
    if(!status) {
        glfons__printShaderInfoLog(shader);
        glDeleteShader(shader);
        exit(-1);
    }
    return shader;
}

static GLuint glfons__linkShaderProgram(const GLchar* vertexSrc, const GLchar* fragmentSrc) {
    GLuint program = glCreateProgram();
    GLuint vertex = glfons__compileShader(vertexSrc, GL_VERTEX_SHADER);
    GLuint fragment = glfons__compileShader(fragmentSrc, GL_FRAGMENT_SHADER);
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    
    if(!linkStatus) {
        glfons__printShaderInfoLog(program);
        glDeleteProgram(program);
        exit(-1);
    }
    return program;
}

void glfons__id2ij(GLFONScontext* gl, fsuint id, int* i, int* j) {
    glm::ivec2 res = gl->transformRes;
    *i = id % res.x;
    *j = id / res.y;
}

void glfons__updateTransform(GLFONScontext* gl, fsuint id, float tx, float ty, float r, float a) {
    int i, j;

    // TODO : manage out of screen translations

    glfons__id2ij(gl, id, &i, &j);

    // scaling between 0..resolution to 0..255
    tx = (tx * 255.0) / gl->resolution.x;
    ty = (ty * 255.0) / gl->resolution.y;

    // scaling between 0..2Pi to 0..255
    r = (r / (2.0 * M_PI)) * 255.0;

    // encode in an unsigned int
    unsigned int data = glfonsRGBA(tx, ty, r, a);

    // modify transforms only if it has changed from last frame
    if(data != gl->transformData[j*gl->transformRes.x+i]) {
        gl->transformData[j*gl->transformRes.x+i] = data;
        // make the whole texture line dirty
        gl->transformDirty[j] = 1;
    }
}

void glfons__uploadTransforms(GLFONScontext* gl) {
    if (gl->texTransform == 0) return;

    int inf = INT_MAX;
    int min = inf;
    int max = -inf;
    bool dirty = false;

    for(int i = 0; i < gl->transformRes.y; ++i) {
        if(gl->transformDirty[i]) {
            dirty = true;
            if(min > max) {
                min = max = i;
            } else {
                max = i;
            }
        }
    }

    if(!dirty) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, gl->texTransform);

    // update smaller part of texture, only the part where transforms has been modified
    const unsigned int* subdata = gl->transformData + min * gl->transformRes.x;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, min, gl->transformRes.x, (max - min) + 1, GL_RGBA, GL_UNSIGNED_BYTE, subdata);

    glBindTexture(GL_TEXTURE_2D, 0);

    for(int i = 0; i < gl->transformRes.y; ++i) {
        gl->transformDirty[i] = 0;
    }
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

    gl->transformRes = glm::vec2(32, 32); // 1024 potential text ids
    gl->transformData = new unsigned int[gl->transformRes.x * gl->transformRes.y] ();
    gl->transformDirty = new unsigned char[gl->transformRes.y] ();

    glGenTextures(1, &gl->texTransform);
    glBindTexture(GL_TEXTURE_2D, gl->texTransform);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl->transformRes.x, gl->transformRes.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, gl->transformData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // create texture
    glGenTextures(1, &gl->tex);
    if (!gl->tex) return 0;
    gl->width = width;
    gl->height = height;
    
    glBindTexture(GL_TEXTURE_2D, gl->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    // create shader
    gl->defaultShaderProgram = glfons__linkShaderProgram(vertexShaderSrc, defaultFragShaderSrc);
    gl->sdfShaderProgram = glfons__linkShaderProgram(vertexShaderSrc, sdfFragShaderSrc);
    
    gl->posAttrib = glGetAttribLocation(gl->defaultShaderProgram, "a_position");
    gl->texCoordAttrib = glGetAttribLocation(gl->defaultShaderProgram, "a_texCoord");
    gl->idAttrib = glGetAttribLocation(gl->defaultShaderProgram, "a_fsid");

    gl->color = glm::vec4(1.0);
    
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

    if(gl->texTransform != 0) {
        glDeleteTextures(1, &gl->texTransform);
    }
    
    for(auto& elmt : *gl->stashes) {
        GLStash* stash = elmt.second;
        if(stash != NULL) {
            delete[] stash->glyphsXOffset;
            delete stash;
        }
    }
    
    glDeleteProgram(gl->defaultShaderProgram);
    glDeleteProgram(gl->sdfShaderProgram);

    glDeleteBuffers(1, &gl->vbo->buffer);
    delete gl->vbo;
    if(gl->interleavedArray) {
        delete[] gl->interleavedArray;
    }
    delete[] gl->transformDirty;
    delete[] gl->transformData;
    
    delete gl->stashes;
    delete gl;
}

void glfonsUpdateViewport(FONScontext* ctx, int scale) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gl->projectionMatrix = glm::ortho(0.0, (double)viewport[2] * scale, (double)viewport[3] * scale, 0.0, -1.0, 1.0);
    gl->resolution = glm::vec2((double)viewport[2] * scale, (double)viewport[3] * scale);
}

FONScontext* glfonsCreate(int width, int height, int flags) {
    FONSparams params;
    GLFONScontext* gl = new GLFONScontext;

    gl->interleavedArray = nullptr;

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

    *id = glctx->idct++;

    if(*id > glctx->transformRes.x * glctx->transformRes.y) {
        std::cerr << "No more texture transform data available" << std::endl;
        std::cerr << "Consider resizing the transform texture" << std::endl;
    }
    
    fonsDrawText(ctx, 0, 0, s, NULL, 0);

    float* data = nullptr;

    if(glctx->interleavedArray == nullptr) {
        glctx->interleavedArray = (float*) malloc(sizeof(float) * ctx->nverts * 5);
        glctx->vbo = new GLFONSvbo;
        glctx->vbo->nverts = 0;
        data = glctx->interleavedArray;
    } else {
        // TODO : realloc can be expensive, improve
        glctx->interleavedArray = (float*) realloc(glctx->interleavedArray, sizeof(float) * (glctx->vbo->nverts + ctx->nverts) * 5);
        data = glctx->interleavedArray + glctx->vbo->nverts * 5;
    }
    
    stash->effect = effect;
    stash->glyphsXOffset = new float[ctx->nverts / N_GLYPH_VERTS];
    
    int j = 0;
    for(int i = 0; i < ctx->nverts * 2; i += N_GLYPH_VERTS * 2) {
        stash->glyphsXOffset[j++] = ctx->verts[i];
    }
    
    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;
    for(int i = 0, off = 0; i < ctx->nverts * 2; i += 2, off += 5) {
        GLfloat x, y, u, v;

        x = ctx->verts[i];
        y = ctx->verts[i+1];
        u = ctx->tcoords[i];
        v = ctx->tcoords[i+1];

        x0 = x < x0 ? x : x0;
        x1 = x > x1 ? x : x1;
        y0 = y < y0 ? y : y0;
        y1 = y > y1 ? y : y1;

        data[off] = x;
        data[off+1] = y;
        data[off+2] = u;
        data[off+3] = v;
        data[off+4] = float(*id);
        // TODO : maybe add padding
    }

    if(ctx->shaping != NULL && fons__getState(ctx)->useShaping) {
        FONSshapingRes* res = ctx->shaping->shapingRes;
        stash->nbGlyph = res->glyphCount;
        fons__clearShaping(ctx);
    } else {
        stash->nbGlyph = (int)strlen(s);
    }
    
    stash->bbox[0] = x0; stash->bbox[1] = y0;
    stash->bbox[2] = x1; stash->bbox[3] = y1;
    
    stash->length = ctx->verts[(ctx->nverts*2)-2];
    
    glctx->vbo->nverts += ctx->nverts;
    
    // hack : reset fontstash state
    ctx->nverts = 0;
    
    glctx->stashes->insert(std::pair<fsuint, GLStash*>(*id, stash));
}

void glfonsUploadVertices(FONScontext* ctx) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glGenBuffers(1, &glctx->vbo->buffer);

    glBindBuffer(GL_ARRAY_BUFFER, glctx->vbo->buffer);
    glBufferData(GL_ARRAY_BUFFER, 5 * sizeof(float) * glctx->vbo->nverts, glctx->interleavedArray, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if(glctx->interleavedArray) {
        free(glctx->interleavedArray);
    }
}

void glfonsTransform(FONScontext* ctx, fsuint id, float tx, float ty, float r) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glfons__updateTransform(glctx, id, tx, ty, r, 0.0);
}

int glfonsGetGlyphCount(FONScontext* ctx, fsuint id) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->stashes->find(id) != glctx->stashes->end()) {
        GLStash* stash = glctx->stashes->at(id);
        return stash->nbGlyph;
    }
    
    return -1;
}

void glfonsDraw(FONScontext* ctx) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;

    if(glctx->tex == 0)
        return;

    // would be uploaded only if some transforms has been modified
    glfons__uploadTransforms(glctx);

    glm::vec4 color = glctx->color / 255.0f;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glctx->tex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, glctx->texTransform);

    GLuint program = glctx->defaultShaderProgram;

    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "u_transforms"), 1);
    glUniform2f(glGetUniformLocation(program, "u_tresolution"), glctx->transformRes.x, glctx->transformRes.y);
    glUniform1i(glGetUniformLocation(program, "u_tex"), 0);
    glUniform4f(glGetUniformLocation(program, "u_color"), color.r, color.g, color.b, color.a);
    glUniform2f(glGetUniformLocation(program, "u_resolution"), glctx->resolution.x, glctx->resolution.y);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_proj"), 1, GL_FALSE, glm::value_ptr(glctx->projectionMatrix));

    // TODO : add back sdf feature effects, not anymore by id, but by context

    glBindBuffer(GL_ARRAY_BUFFER, glctx->vbo->buffer);

    glVertexAttribPointer(glctx->posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(glctx->posAttrib);

    glVertexAttribPointer(glctx->texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const GLvoid*)(2 * sizeof(float)));
    glEnableVertexAttribArray(glctx->texCoordAttrib);

    glVertexAttribPointer(glctx->idAttrib, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const GLvoid*)(4 * sizeof(float)));
    glEnableVertexAttribArray(glctx->idAttrib);

    glDrawArrays(GL_TRIANGLES, 0, glctx->vbo->nverts);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    glDisableVertexAttribArray(glctx->posAttrib);
    glDisableVertexAttribArray(glctx->texCoordAttrib);
}

void glfonsSetColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->color = glm::vec4(r, g, b, a);
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
