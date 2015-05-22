//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
// Copyright (c) 2014 Mapzen karim@mapzen.com
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

#include <unordered_map>
#include <vector>
#include <climits>
#include <cstdio>

#include "fontstash.h"

typedef unsigned int fsuint;

#define N_GLYPH_VERTS 6
#define INNER_DATA_OFFSET 5 // offset between vertices

typedef struct GLFONScontext GLFonscontext;
typedef struct GLFONSParams GLFONSParams;

enum class GLFONSError {
    ID_OVERFLOW
};

FONScontext* glfonsCreate(int width, int height, int flags, GLFONSParams glParams, void* userPtr);
void glfonsDelete(FONScontext* ctx);

void glfonsUpdateTransforms(FONScontext* ctx, void* ownerPtr = nullptr);
void glfonsTransform(FONScontext* ctx, fsuint id, float tx, float ty, float r, float a);

void glfonsGenText(FONScontext* ctx, unsigned int nb, fsuint* textId);

void glfonsBufferCreate(FONScontext* ctx, unsigned int texTransformRes, fsuint* id);
void glfonsBufferDelete(FONScontext* ctx, fsuint id);
void glfonsBindBuffer(FONScontext* ctx, fsuint id);

void glfonsRasterize(FONScontext* ctx, fsuint textId, const char* s);
bool glfonsVertices(FONScontext* ctx, float* data);
int glfonsVerticesSize(FONScontext* ctx);

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1);
float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i);
float glfonsGetLength(FONScontext* ctx, fsuint id);
int glfonsGetGlyphCount(FONScontext* ctx, fsuint id);
void glfonsScreenSize(FONScontext* ctx, int screenWidth, int screenHeight);
void glfonsProjection(FONScontext* ctx, float* projectionMatrix);
void glfonsExpandTransform(FONScontext* ctx, fsuint buffer, int size);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#endif

#ifdef GLFONTSTASH_IMPLEMENTATION

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

struct GLFONSstash {
    int nbGlyph;
    float bbox[4];
    float length;
    float* glyphsXOffset;
};

struct GLFONSbuffer {
    fsuint id;
    fsuint idct;
    GLuint transform;
    unsigned int maxId;
    int transformRes[2];
    unsigned int* transformData;
    unsigned char* transformDirty;
    std::vector<float> interleavedArray;
    std::vector<GLFONSstash*> stashes;
};

struct GLFONSparams {
    bool useGLBackend;
    bool (*errorCallback)(void* usrPtr, fsuint buffer, GLFONSError error);
    void (*createTexTransforms)(void* usrPtr, unsigned int width, unsigned int height);
    void (*createAtlas)(void* usrPtr, unsigned int width, unsigned int height);
    void (*updateTransforms)(void* usrPtr, unsigned int xoff, unsigned int yoff, unsigned int width,
                             unsigned int height, const unsigned int* pixels, void* ownerPtr);
    void (*updateAtlas)(void* usrPtr, unsigned int xoff, unsigned int yoff,
                        unsigned int width, unsigned int height, const unsigned int* pixels);
};

struct GLFONScontext {
    GLuint atlas;
    GLFONSparams params;
    int atlasRes[2];
    float screenSize[2];
    fsuint idct;
    fsuint boundBuffer;
    std::unordered_map<fsuint, GLFONSbuffer*> buffers;
    void* userPtr;
};

void glfons__id2ij(int res, fsuint id, int* i, int* j) {
    *i = (id*2) % res;
    *j = (id*2) / res;
}

void glfons__ij2id(int res, int i, int j, fsuint* id) {
    *id = (i+1) % (res/2) + j * (res/2);
}

static int glfons__renderCreate(void* userPtr, int width, int height) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;

    gl->idct = 0;
    gl->atlasRes[0] = width;
    gl->atlasRes[1] = height;
    gl->params.createAtlas(gl->userPtr, width, height);

    return 1;
}

static int glfons__renderResize(void* userPtr, int width, int height) {
    return 1;
}

static void glfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;

    int h = rect[3] - rect[1];
    const unsigned char* subdata = data + rect[1] * gl->atlasRes[0];
    gl->params.updateAtlas(gl->userPtr, 0, rect[1], gl->atlasRes[0], h, reinterpret_cast<const unsigned int*>(subdata));
}

static void glfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
    // called by fontstash, but has nothing to do
}

void glfons__freeStash(GLFONSstash* stash) {
    delete[] stash->glyphsXOffset;
    delete stash;
}

GLFONSbuffer* glfons__bufferBound(GLFONScontext* gl) {
    if(gl->boundBuffer == 0) {
        return nullptr;
    }

    return gl->buffers.at(gl->boundBuffer);
}

void glfonsUpdateTransforms(FONScontext* ctx, void* ownerPtr) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    int inf = INT_MAX;
    int min = inf;
    int max = -inf;
    bool dirty = false;

    for(int i = 0; i < buffer->transformRes[1]; ++i) {
        if(buffer->transformDirty[i]) {
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

    // interleaved array stored as following in texture :
    // | x | y | rot | alpha | precision_x | precision_y | precision_r | Ø |
    const unsigned int* subdata;
    subdata = buffer->transformData + min * buffer->transformRes[0];
    gl->params.updateTransforms(gl->userPtr, 0, min, buffer->transformRes[0], (max - min) + 1, subdata, ownerPtr);
    std::fill(buffer->transformDirty, buffer->transformDirty + buffer->transformRes[1], 0);
}

void glfonsGenText(FONScontext* ctx, unsigned int nb, fsuint* textId) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);


    if(buffer->idct + nb > buffer->maxId) {
        bool solved = false;

        if(gl->params.errorCallback) {
            solved = gl->params.errorCallback(gl->userPtr, buffer->id, GLFONSError::ID_OVERFLOW);
        }

        if(!solved) {
            for(unsigned int i = 0; i < nb; ++i) {
                textId[i] = -1;
            }
            return;
        }
    }

    for(unsigned int i = 0; i < nb; ++i) {
        textId[i] = buffer->idct++;
    }
}

void glfonsRasterize(FONScontext* ctx, fsuint textId, const char* s) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = new GLFONSstash;

    int length = fonsDrawText(ctx, 0, 0, s, NULL, 0);

    stash->glyphsXOffset = new float[ctx->nverts / N_GLYPH_VERTS];

    int j = 0;
    for(int i = 0; i < ctx->nverts * 2; i += N_GLYPH_VERTS * 2) {
        stash->glyphsXOffset[j++] = ctx->verts[i];
    }

    float inf = std::numeric_limits<float>::infinity();
    float x0 = inf, x1 = -inf, y0 = inf, y1 = -inf;
    for(int i = 0; i < ctx->nverts * 2; i += 2) {
        float x, y;

        x = ctx->verts[i];
        y = ctx->verts[i+1];

        x0 = x < x0 ? x : x0;
        x1 = x > x1 ? x : x1;
        y0 = y < y0 ? y : y0;
        y1 = y > y1 ? y : y1;

        buffer->interleavedArray.push_back(x);
        buffer->interleavedArray.push_back(y);
        buffer->interleavedArray.push_back(ctx->tcoords[i]);
        buffer->interleavedArray.push_back(ctx->tcoords[i+1]);
        buffer->interleavedArray.push_back(float(textId));
    }

    // remove extra-offset used for interpolation in fontstash
    stash->bbox[0] = x0 + 3; stash->bbox[1] = y0 - 3;
    stash->bbox[2] = x1 + 3; stash->bbox[3] = y1 - 3;

    stash->length = length - 6;

    if(ctx->shaping != NULL && fons__getState(ctx)->useShaping) {
        FONSshapingRes* res = ctx->shaping->shapingRes;
        stash->nbGlyph = res->glyphCount;
        fons__clearShaping(ctx);
    } else {
        stash->nbGlyph = (int)strlen(s);
    }

    // hack : reset fontstash state
    ctx->nverts = 0;

    buffer->stashes.push_back(stash);
}

void glfonsExpandTransform(FONScontext* ctx, fsuint bufferId, int newSize) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = gl->buffers.at(bufferId);

    int oldSize = buffer->transformRes[0];

    if(newSize > oldSize) {
        buffer->transformRes[0] = newSize;
        buffer->transformRes[1] = newSize * 2;

        unsigned int* tmp = buffer->transformData;
        buffer->transformData = new unsigned int[newSize * newSize * 2] ();

        int maxDirtyLine = 0;
        for(int j = 0; j < oldSize * 2; j++) {
            for(int i = 0; i < oldSize; i+=2) {
                fsuint id;
                int ni, nj;

                glfons__ij2id(oldSize, i, j, &id);
                glfons__id2ij(newSize, id, &ni, &nj);

                buffer->transformData[nj * newSize + ni] = tmp[j * oldSize + i];
                buffer->transformData[nj * newSize + ni + 1] = tmp[j * oldSize + i + 1];

                maxDirtyLine = nj;
            }
        }
        delete[] tmp;

        delete[] buffer->transformDirty;
        buffer->transformDirty = new unsigned char[newSize * 2] ();
        for(int i = 0; i < maxDirtyLine + 1; ++i) {
            buffer->transformDirty[i] = 1;
        }

        buffer->maxId = pow(newSize, 2);
    }
}

void glfonsBufferCreate(FONScontext* ctx, unsigned int texTransformRes, fsuint* id) {
    if((texTransformRes & (texTransformRes-1)) != 0) {
        *id = 0;
        return;
    }

    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = new GLFONSbuffer;

    *id = ++gl->idct;

    buffer->idct = 0;
    buffer->transformRes[0] = texTransformRes;
    buffer->transformRes[1] = texTransformRes * 2;
    buffer->maxId = pow(texTransformRes, 2);
    buffer->transformData = new unsigned int[texTransformRes * texTransformRes * 2] ();
    buffer->transformDirty = new unsigned char[texTransformRes * 2] ();
    buffer->id = *id;
    gl->boundBuffer = *id;

    gl->params.createTexTransforms(gl->userPtr, buffer->transformRes[0], buffer->transformRes[1]);

    gl->buffers[*id] = buffer;
}

void glfonsBufferDelete(GLFONScontext* gl, fsuint id) {
    GLFONSbuffer* buffer = gl->buffers.at(id);

    delete[] buffer->transformData;
    delete[] buffer->transformDirty;

    for(auto& elt : buffer->stashes) {
        glfons__freeStash(elt);
    }
    buffer->stashes.clear();
    buffer->interleavedArray.clear();
}

void glfonsBufferDelete(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;

    if(gl->buffers.size() == 0) {
        return;
    }

    if(gl->buffers.find(id) != gl->buffers.end()) {
        glfonsBufferDelete(gl, id);
        gl->buffers.erase(id);
    }
}

void glfonsBindBuffer(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    gl->boundBuffer = id;
}

static void glfons__renderDelete(void* userPtr) {
    GLFONScontext* gl = (GLFONScontext*)userPtr;

    for(auto elt : gl->buffers) {
        glfonsBufferDelete(gl, elt.first);
    }
    gl->buffers.clear();
    delete gl;
}

void glfonsScreenSize(FONScontext* ctx, int screenWidth, int screenHeight) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    gl->screenSize[0] = screenWidth;
    gl->screenSize[1] = screenHeight;
}

void glfons__createTexTransform(void* usrPtr, unsigned int width, unsigned int height) {
    GLFONScontext* gl = (GLFONScontext*) usrPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    glGenTextures(1, &buffer->transform);
    glBindTexture(GL_TEXTURE_2D, buffer->transform);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}
    
void glfons__createAtlas(void* usrPtr, unsigned int width, unsigned int height) {
    GLFONScontext* gl = (GLFONScontext*) usrPtr;

    glGenTextures(1, &gl->atlas);
    glBindTexture(GL_TEXTURE_2D, gl->atlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
    
void glfons__updateTransforms(void* usrPtr, unsigned int xoff, unsigned int yoff, unsigned int width,
                             unsigned int height, const unsigned int* pixels, void* ownerPtr) {
    GLFONScontext* gl = (GLFONScontext*) usrPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    glBindTexture(GL_TEXTURE_2D, buffer->transform);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void glfons__udpateAtas(void* usrPtr, unsigned int xoff, unsigned int yoff,
                        unsigned int width, unsigned int height, const unsigned int* pixels) {
    GLFONScontext* gl = (GLFONScontext*) usrPtr;

    glBindTexture(GL_TEXTURE_2D, gl->atlas);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, width, height, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

FONScontext* glfonsCreate(int width, int height, int flags, GLFONSparams glParams, void* userPtr) {
    FONSparams params;
    GLFONScontext* gl = new GLFONScontext;

    if(glParams.useGLBackend) {
        glParams.createTexTransforms = glfons__createTexTransform;
        glParams.createAtlas = glfons__createAtlas;
        glParams.updateTransforms = glfons__updateTransforms;
        glParams.updateAtlas = glfons__udpateAtas;
    }

    gl->params = glParams;
    gl->userPtr = userPtr;

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

void glfonsProjection(FONScontext* ctx, float* projectionMatrix) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;

    float width = gl->screenSize[0];
    float height = gl->screenSize[1];

    float r = width;
    float l = 0.0;
    float b = height;
    float t = 0.0;
    float n = -1.0;
    float f = 1.0;

    // ensure array is 0-filled
    memset(projectionMatrix, 0, 16 * sizeof(float));

    // could be simplified, exposing it like this for comprehension
    projectionMatrix[0] = 2.0 / (r-l);
    projectionMatrix[5] = 2.0 / (t-b);
    projectionMatrix[10] = 2.0 / (f-n);
    projectionMatrix[12] = -(r+l)/(r-l);
    projectionMatrix[13] = -(t+b)/(t-b);
    projectionMatrix[14] = -(f+n)/(f-n);
    projectionMatrix[15] = 1.0;
}

void glfonsDelete(FONScontext* ctx) {
    fonsDeleteInternal(ctx);
}

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

int glfonsVerticesSize(FONScontext* ctx) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    if(buffer == nullptr) {
        return 0;
    }

    return (int) buffer->interleavedArray.size() / INNER_DATA_OFFSET;
}

bool glfonsVertices(FONScontext* ctx, float* data) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    if(buffer == nullptr) {
        return false;
    }

    memcpy(data, buffer->interleavedArray.data(), buffer->interleavedArray.size() * sizeof(float));

    buffer->interleavedArray.clear();

    return true;
}

float glfons__precision(float value) {
    return floor((1.0 - ((int)(value + 1) - value)) * 255.0 + 0.5);
}

void glfonsTransform(FONScontext* ctx, fsuint id, float tx, float ty, float r, float a) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    int i, j;

    glfons__id2ij(buffer->transformRes[0], id, &i, &j);

    // TODO : manage out of screen translations, the scaling step due to texture transform
    // would happen to behave like a module width or height translation

    // scaling to [0..255]
    tx = (tx * 255.0) / gl->screenSize[0];
    ty = (ty * 255.0) / gl->screenSize[1];

    r = fmod(r, 2.0 * M_PI);
    r = (r < 0) ? r + 2.0 * M_PI : r;
    r = (r / (2.0 * M_PI)) * 255.0;
    a = a * 255.0;

    // scale decimal part from [0..1] to [0..255] rounding to the closest value
    // known floating point error here of 0.5/255 ~= 0.00196 due to rounding
    float dx = glfons__precision(tx);
    float dy = glfons__precision(ty);
    float dr = glfons__precision(r);

    unsigned int data = glfonsRGBA(tx, ty, r, a);
    unsigned int fract = glfonsRGBA(dx, dy, dr, /* byte not used -> */ 0);
    unsigned int index = j*buffer->transformRes[0]+i;

    if(data != buffer->transformData[index] || fract != buffer->transformData[index+1]) {
        buffer->transformData[index] = data;
        buffer->transformData[index+1] = fract;
        buffer->transformDirty[j] = 1;
    }
}

int glfonsGetGlyphCount(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);

    GLFONSstash* stash = buffer->stashes[id];
    return stash->nbGlyph;
}

float glfonsGetGlyphOffset(FONScontext* ctx, fsuint id, int i) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = buffer->stashes[id];

    return stash->glyphsXOffset[i];
}

void glfonsGetBBox(FONScontext* ctx, fsuint id, float* x0, float* y0, float* x1, float* y1) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = buffer->stashes[id];

    *x0 = stash->bbox[0]; *y0 = stash->bbox[1];
    *x1 = stash->bbox[2]; *y1 = stash->bbox[3];
}

float glfonsGetLength(FONScontext* ctx, fsuint id) {
    GLFONScontext* gl = (GLFONScontext*) ctx->params.userPtr;
    GLFONSbuffer* buffer = glfons__bufferBound(gl);
    GLFONSstash* stash = buffer->stashes[id];

    return stash->length;
}

#endif
