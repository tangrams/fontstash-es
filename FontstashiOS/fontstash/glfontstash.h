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
#include <map>
#include <stack>

typedef unsigned int fsuint;

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);

void glfonsBufferText(FONScontext* ctx, const char* s, fsuint* id);
void glfonsUnbufferText(FONScontext* ctx, fsuint* id);
void glfonsAnchorPoint(FONScontext* ctx, float x, float y);
void glfonsRotate(FONScontext* ctx, float angle);
void glfonsTranslate(FONScontext* ctx, float x, float y);
void glfonsDrawText(FONScontext* ctx, fsuint* id);
void glfonsSetColor(FONScontext* ctx, unsigned int color);

void glfonsPushMatrix(FONScontext* ctx);
void glfonsPopMatrix(FONScontext* ctx);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

struct GLFONSvbo {
    int nverts;
    GLuint buffers[3];
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
    glm::vec2 anchorPoint;
    glm::mat4 projectionMatrix;
    
    glm::mat4 rotation;
    glm::mat4 scale;
    glm::mat4 translation;
};

typedef struct GLFONScontext GLFONScontext;

const GLchar* vertexShaderSrc =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "attribute vec4 position;\n"
    "attribute vec2 texCoord;\n"
    "attribute vec4 color;\n"
    "uniform mat4 mvp;\n"
    "varying vec2 fUv;\n"
    "varying vec4 fColor;\n"
    "void main() {\n"
    "  gl_Position = mvp * position;\n"
    "  fUv = texCoord;\n"
    "  fColor = color;\n"
    "}\n";

const GLchar* fragShaderSrc =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D tex;\n"
    "varying vec2 fUv;\n"
    "varying vec4 fColor;\n"
    "void main(void) {\n"
    "  vec4 color = texture2D(tex, fUv);\n"
    "  vec3 invColor = 1.0 - color.rgb;\n"
    "  gl_FragColor = vec4(invColor * fColor.rgb, fColor.a * color.a);\n"
    "}\n";

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
    gl->anchorPoint = glm::vec2(0.5);
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
    gl->projectionMatrix = glm::ortho(0.0, (double)viewport[2], (double)viewport[3], 0.0, -1.0, 1.0);
    gl->translation = glm::mat4(1.0);
    gl->rotation = glm::mat4(1.0);
    gl->scale = glm::mat4(1.0);
    gl->matrixStack.push(glm::mat4(1.0));
    
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
    int w = rect[2] - rect[0];
    int h = rect[3] - rect[1];
    
    if (gl->tex == 0) return;

    // TODO : update smaller part of texture 
    glBindTexture(GL_TEXTURE_2D, gl->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, gl->width, gl->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{

}

static void glfons__renderDelete(void* userPtr)
{
    GLFONScontext* gl = (GLFONScontext*)userPtr;
    if (gl->tex != 0)
        glDeleteTextures(1, &gl->tex);
    gl->tex = 0;
    for(auto& elmt : *gl->vbos) {
        if(elmt.second != NULL) {
            glDeleteBuffers(3, elmt.second->buffers);
            delete elmt.second;
        }
    }
    delete gl->vbos;
    delete gl;
}

FONScontext* glfonsCreate(int width, int height, int flags)
{
    FONSparams params;
    GLFONScontext* gl;
    
    // TODO : use c++ style
    gl = (GLFONScontext*)malloc(sizeof(GLFONScontext));
    if (gl == NULL) goto error;
    memset(gl, 0, sizeof(GLFONScontext));
    
    memset(&params, 0, sizeof(params));
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
    
error:
    if (gl != NULL) free(gl);
    return NULL;
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
    
    // WIP : should be at (0, 0)
    fonsDrawText(ctx, 0, 50, s, NULL, 0);
    
    glGenBuffers(3, vboBufferDesc->buffers);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboBufferDesc->buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * ctx->nverts, ctx->verts, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, vboBufferDesc->buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * ctx->nverts, ctx->tcoords, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vboBufferDesc->buffers[2]);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(unsigned int) * ctx->nverts, ctx->colors, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    vboBufferDesc->nverts = ctx->nverts;

    ctx->nverts = 0;

    glctx->vbos->insert(std::pair<fsuint, GLFONSvbo*>(*id, vboBufferDesc));
}

void glfonsUnbufferText(FONScontext* ctx, fsuint* id)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    // TODO : should test if it already exists before accessing it
    GLFONSvbo* vboInfo = glctx->vbos->at(*id);
    
    if(vboInfo != NULL) {
        glDeleteBuffers(3, vboInfo->buffers);
        glctx->vbos->erase(*id);
        delete vboInfo;
    }
}

void glfonsAnchorPoint(FONScontext* ctx, float x, float y)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->anchorPoint = glm::vec2(x, y);
}

void glfonsRotate(FONScontext* ctx, float angle)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    // TODO : translate by anchor point
    glctx->rotation = glm::rotate(glm::mat4(1.0), angle, glm::vec3(glctx->anchorPoint, 0.0));
}

void glfonsTranslate(FONScontext* ctx, float x, float y)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->translation = glm::translate(glm::mat4(1.0), glm::vec3(x, y, 0.0));
}

void glfonsDrawText(FONScontext* ctx, fsuint* id)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    
    if(!glctx || glctx->tex == 0)
        return;
    
    // TODO : should test if it already exists before accessing it
    GLFONSvbo* vboDesc = glctx->vbos->at(*id);
    
    if(vboDesc != NULL) {
        glm::mat4 model = glctx->matrixStack.top();
        glm::mat4 view = glm::mat4(1.0); // TODO : maybe use this
        glm::mat4 mvp = glctx->projectionMatrix * view * model;
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glctx->tex);
        glEnable(GL_TEXTURE_2D);
    
        glUseProgram(glctx->shaderProgram);
        glUniform1i(glGetUniformLocation(glctx->shaderProgram, "tex"), 0);
        glUniformMatrix4fv(glGetUniformLocation(glctx->shaderProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
    
        glBindBuffer(GL_ARRAY_BUFFER, vboDesc->buffers[0]);
        glVertexAttribPointer(glctx->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glctx->posAttrib);
    
        glBindBuffer(GL_ARRAY_BUFFER, vboDesc->buffers[1]);
        glVertexAttribPointer(glctx->texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glctx->texCoordAttrib);
    
        glBindBuffer(GL_ARRAY_BUFFER, vboDesc->buffers[2]);
        glVertexAttribPointer(glctx->colorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
        glEnableVertexAttribArray(glctx->colorAttrib);
    
        glDrawArrays(GL_TRIANGLES, 0, vboDesc->nverts);
    
        glDisableVertexAttribArray(glctx->posAttrib);
        glDisableVertexAttribArray(glctx->texCoordAttrib);
        glDisableVertexAttribArray(glctx->colorAttrib);
    
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
    }
}

void glfonsSetColor(FONScontext* ctx, unsigned int color)
{
    
}

void glfonsPushMatrix(FONScontext* ctx)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glm::mat4 top = glctx->matrixStack.top();
    glm::mat4 transform = glctx->translation * glctx->rotation * glctx->scale;
    glctx->matrixStack.push(transform * top);
}

void glfonsPopMatrix(FONScontext* ctx)
{
    GLFONScontext* glctx = (GLFONScontext*) ctx->params.userPtr;
    glctx->matrixStack.pop();
}


#endif
