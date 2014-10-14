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
typedef unsigned int fsenum;

#define BUFFER_SIZE     2
#define N_GLYPH_VERTS   6

struct GLFONSvbo {
    int nverts;
    GLuint buffers[BUFFER_SIZE];
    glm::vec2 bbox[2];
};

typedef struct GLFONSvbo GLFONSvbo;

struct GLFONScontext {
    GLuint tex;
    int width, height;
    // the id counter
    fsuint idct;
    
    GLuint shaderProgram;
    GLuint posAttrib;
    GLuint texCoordAttrib;
    GLuint colorAttrib;
    
    std::map<fsuint, GLFONSvbo*>* vbos;
    std::stack<glm::mat4> matrixStack;
    glm::mat4 projectionMatrix;
    
    glm::vec4 color;
    
    glm::mat4 rotation;
    glm::mat4 scale;
    glm::mat4 translation;
};

typedef struct GLFONScontext GLFONScontext;

static const GLchar* vertexShaderSrc =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"attribute vec4 position;\n"
"attribute vec2 texCoord;\n"
"uniform mat4 mvp;\n"
"varying vec2 fUv;\n"
"void main() {\n"
"  gl_Position = mvp * position;\n"
"  fUv = texCoord;\n"
"}\n";

static const GLchar* fragShaderSrc =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"uniform sampler2D tex;\n"
"uniform vec4 color;\n"
"varying vec2 fUv;\n"
"void main(void) {\n"
"  vec4 texColor = texture2D(tex, fUv);\n"
"  vec3 invColor = 1.0 - texColor.rgb;\n"
"  gl_FragColor = vec4(invColor * color.rgb, color.a * texColor.a);\n"
"}\n";

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);

void glfonsBufferText(FONScontext* ctx, const char* s, fsuint* id);
void glfonsUnbufferText(FONScontext* ctx, fsuint id);
void glfonsAnchorPoint(FONScontext* ctx, float x, float y);
void glfonsRotate(FONScontext* ctx, float angle);
void glfonsTranslate(FONScontext* ctx, float x, float y);
void glfonsDrawText(FONScontext* ctx, fsuint id);
void glfonsDrawText(FONScontext* ctx, fsuint id, unsigned int from, unsigned int to);
void glfonsSetColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void glfonsScale(FONScontext* ctx, float x, float y);

void glfonsPushMatrix(FONScontext* ctx);
void glfonsPopMatrix(FONScontext* ctx);

// TODO : use a set of pixel shaders to change the font rendering style
void glfonsSetShadingStyle(fsenum style);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

void resetMatrices(GLFONScontext* gl)
{
    gl->translation = glm::mat4(1.0);
    gl->rotation = glm::mat4(1.0);
    gl->scale = glm::mat4(1.0);
}

glm::mat4 currentModel(GLFONScontext* gl)
{
    return gl->translation * gl->rotation * gl->scale * gl->matrixStack.top();
}

static void printShaderInfoLog(GLuint shader)
{
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if(length > 1)
    {
        char* log = (char*)malloc(sizeof(char) * length);
        glGetShaderInfoLog(shader, length, NULL, log);
        printf("Log: %s\n", log);
        free(log);
    }
}

static GLuint compileShader(const GLchar* src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    
    if(!status)
    {
        printShaderInfoLog(shader);
        glDeleteShader(shader);
        exit(-1);
    }
    return shader;
}

static GLuint linkShaderProgram(const GLchar* vertexSrc, const GLchar* fragmentSrc)
{
    GLuint program = glCreateProgram();
    GLuint vertex = compileShader(vertexSrc, GL_VERTEX_SHADER);
    GLuint fragment = compileShader(fragmentSrc, GL_FRAGMENT_SHADER);
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    
    if(!linkStatus)
    {
        printShaderInfoLog(program);
        glDeleteProgram(program);
        exit(-1);
    }
    return program;
}

static int glfons__renderCreate(void* userPtr, int width, int height)
{
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    // Create may be called multiple times, delete existing texture.
    if (gl->tex != 0) {
        glDeleteTextures(1, &gl->tex);
        gl->tex = 0;
    }
    gl->idct = 0;
    gl->vbos = new std::map<fsuint, GLFONSvbo*>();
    
    glGenTextures(1, &gl->tex);
    if (!gl->tex) return 0;
    gl->width = width;
    gl->height = height;
    glBindTexture(GL_TEXTURE_2D, gl->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    // create shader
    gl->shaderProgram = linkShaderProgram(vertexShaderSrc, fragShaderSrc);
    gl->posAttrib = glGetAttribLocation(gl->shaderProgram, "position");
    gl->texCoordAttrib = glGetAttribLocation(gl->shaderProgram, "texCoord");
    gl->colorAttrib = glGetAttribLocation(gl->shaderProgram, "color");
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gl->projectionMatrix = glm::ortho(0.0, (double)viewport[2] * 2, (double)viewport[3] * 2, 0.0, -1.0, 1.0);
    gl->matrixStack.push(glm::mat4(1.0));
    gl->color = glm::vec4(1.0);
    
    resetMatrices(gl);
    
    return 1;
}

static int glfons__renderResize(void* userPtr, int width, int height)
{
    // Reuse create to resize too.
    return glfons__renderCreate(userPtr, width, height);
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data)
{
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    if (gl->tex == 0) return;
    
    int w = rect[2] - rect[0];
    int h = rect[3] - rect[1];
    
    // TODO : store the bounding box

    // TODO : update smaller part of texture 
    glBindTexture(GL_TEXTURE_2D, gl->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{
    // called by fontstash, but has nothing to do
}

static void glfons__renderDelete(void* userPtr)
{
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    if (gl->tex != 0)
        glDeleteTextures(1, &gl->tex);
    gl->tex = 0;
    for(auto& elmt : *gl->vbos) {
        if(elmt.second != NULL) {
            glDeleteBuffers(BUFFER_SIZE, elmt.second->buffers);
            delete elmt.second;
        }
    }
    delete gl->vbos;
    delete gl;
}

FONScontext* glfonsCreate(int width, int height, int flags)
{
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

void glfonsDelete(FONScontext* ctx)
{
    fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

void glfonsBufferText(FONScontext* ctx, const char* s, fsuint* id)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLFONSvbo* vboBufferDesc = new GLFONSvbo;
    
    *id = ++glctx->idct;
    
    fonsDrawText(ctx, 0, 0, s, NULL, 0);
    
    float inf = -std::numeric_limits<float>::infinity();
    float x0 = -inf, x1 = inf, y0 = -inf, y1 = inf;
    for(int i = 0; i < ctx->nverts; i += 2) {
        x0 = ctx->verts[i] < x0 ? ctx->verts[i] : x0;
        x1 = ctx->verts[i] > x1 ? ctx->verts[i] : x1;
        y0 = ctx->verts[i+1] < y0 ? ctx->verts[i+1] : y0;
        y1 = ctx->verts[i+1] > y1 ? ctx->verts[i+1] : y1;
    }
    
    vboBufferDesc->bbox[0] = glm::vec2(x0, y0);
    vboBufferDesc->bbox[1] = glm::vec2(x1, y1);
    //vboBufferDesc->anchorPoint = glm::vec2(0.5);
    
    glGenBuffers(BUFFER_SIZE, vboBufferDesc->buffers);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboBufferDesc->buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * ctx->nverts, ctx->verts, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboBufferDesc->buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * ctx->nverts, ctx->tcoords, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    vboBufferDesc->nverts = ctx->nverts;

    // reset fontstash state
    ctx->nverts = 0;

    glctx->vbos->insert(std::pair<fsuint, GLFONSvbo*>(*id, vboBufferDesc));
}

void glfonsUnbufferText(FONScontext* ctx, fsuint id)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;

    if(glctx->vbos->find(id) != glctx->vbos->end()) {
        // TODO : should test if it already exists before accessing it
        GLFONSvbo* vboInfo = glctx->vbos->at(id);
    
        if(vboInfo != NULL) {
            glDeleteBuffers(3, vboInfo->buffers);
            glctx->vbos->erase(id);
            delete vboInfo;
        }
    }
}

void glfonsAnchorPoint(FONScontext* ctx, fsuint id, float x, float y)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->vbos->find(id) != glctx->vbos->end()) {
        GLFONSvbo* vboDesc = glctx->vbos->at(id);
        //vboDesc->anchorPoint = glm::vec2(x, y);
    }
}

void glfonsRotate(FONScontext* ctx, float angle)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    // TODO : translate by anchor point
    glm::vec3 raxis = glm::vec3(0.0, 0.0, 1.0);
    
    glctx->rotation = glctx->matrixStack.top()
        * glm::rotate(glctx->rotation, angle, raxis)
        * glm::inverse(glctx->matrixStack.top());
}

void glfonsTranslate(FONScontext* ctx, float x, float y)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->translation = glm::translate(glctx->translation, glm::vec3(x, y, 0.0));
}

void glfonsScale(FONScontext* ctx, float x, float y) {
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->scale = glm::scale(glctx->scale, glm::vec3(x, y, 1.0));
}

void glfonsDrawText(FONScontext* ctx, fsuint id, unsigned int from, unsigned int to)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->tex == 0)
        return;
    
    int d = (to - from) + 1;
    int iFirst = from * N_GLYPH_VERTS;
    int count = d * N_GLYPH_VERTS;
    
    GLFONSvbo* vboDesc = glctx->vbos->at(id);
    
    if(vboDesc != NULL) {
        glm::mat4 view = glm::mat4(1.0); // TODO : maybe use this
        glm::mat4 mvp = glctx->projectionMatrix * view * currentModel(glctx);
        glm::vec4 color = glctx->color / 255.0f;
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glctx->tex);
        glEnable(GL_TEXTURE_2D);
        
        glUseProgram(glctx->shaderProgram);
        glUniform1i(glGetUniformLocation(glctx->shaderProgram, "tex"), 0);
        glUniformMatrix4fv(glGetUniformLocation(glctx->shaderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform4f(glGetUniformLocation(glctx->shaderProgram, "color"), color.r, color.g, color.b, color.a);
        
        glBindBuffer(GL_ARRAY_BUFFER, vboDesc->buffers[0]);
        glVertexAttribPointer(glctx->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glctx->posAttrib);
        
        glBindBuffer(GL_ARRAY_BUFFER, vboDesc->buffers[1]);
        glVertexAttribPointer(glctx->texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glctx->texCoordAttrib);
        
        glDrawArrays(GL_TRIANGLES, iFirst, count);
        
        glDisableVertexAttribArray(glctx->posAttrib);
        glDisableVertexAttribArray(glctx->texCoordAttrib);
        glDisableVertexAttribArray(glctx->colorAttrib);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
    }
}

void glfonsDrawText(FONScontext* ctx, fsuint id)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    GLFONSvbo* vboDesc = glctx->vbos->at(id);
    unsigned int last = (vboDesc->nverts / N_GLYPH_VERTS) - 1;
    glfonsDrawText(ctx, id, 0, last);
}

void glfonsSetColor(FONScontext* ctx, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->color = glm::vec4(r, g, b, a);
}

void glfonsPushMatrix(FONScontext* ctx)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glm::mat4 top = glctx->matrixStack.top();
    glm::mat4 transform = glctx->translation * glctx->rotation * glctx->scale;
    glctx->matrixStack.push(transform * top);
    
    // new matrices context
    resetMatrices(glctx);
}

void glfonsPopMatrix(FONScontext* ctx)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(glctx->matrixStack.size() > 0) {
        glctx->matrixStack.pop();
    
        // TODO : fix this, should, retrieve old state
        resetMatrices(glctx);
    }
}


#endif
