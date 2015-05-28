//
//  shaders.h
//  FontstashiOS
//
//  Created by Karim Naaji on 12/16/14.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#ifndef SHADERS_H
#define SHADERS_H

namespace glfs {

static const GLchar* vertexShaderSrc = R"END(
#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

attribute LOWP float a_fsid;
attribute vec2 a_position;
attribute vec2 a_texCoord;

uniform sampler2D u_transforms;
uniform LOWP vec2 u_tresolution;
uniform LOWP vec2 u_resolution;
uniform mat4 u_proj;

varying vec2 v_uv;
varying float v_alpha;

#define PI 3.14159

#define alpha   tdata.a
#define tx      tdata.x
#define ty      tdata.y
#define theta   tdata.z
#define txp     tdataPrecision.x
#define typ     tdataPrecision.y
#define trp     tdataPrecision.z

/*
 * Converts (i, j) pixel coordinates to the corresponding (u, v) in
 * texture space. The (u,v) targets the center of pixel
 */
vec2 ij2uv(float _i, float _j, float _w, float _h) {
    return vec2(
        (2.0*_i+1.0) / (2.0*_w),
        (2.0*_j+1.0) / (2.0*_h)
    );
}

/*
 * Decodes the id and find its place for its transform inside the texture
 * Returns the (i,j) position inside texture
 */
vec2 id2ij(int _fsid, float _w) {
    float i = mod(float(_fsid * 2), _w);
    float j = floor(float(_fsid * 2) / _w);
    return vec2(i, j);
}

void main() {
    // decode the uv from a text id
    vec2 ij = id2ij(int(a_fsid), u_tresolution.x);
    vec2 uv1 = ij2uv(ij.x, ij.y, u_tresolution.x, u_tresolution.y);

    // reads the transform data and its precision
    vec4 tdata = texture2D(u_transforms, uv1);
    
    if (alpha != 0.0) {
        vec2 uv2 = ij2uv(ij.x+1.0, ij.y, u_tresolution.x, u_tresolution.y);
        vec4 tdataPrecision = texture2D(u_transforms, uv2);

        float txe = u_resolution.x / 255.0; // max error on x
        float tye = u_resolution.y / 255.0; // max error on y
        float tre = (2.0 * PI) / 255.0;

        // transforms from [0..1] to [0..resolution] and add lost precision
        tx = u_resolution.x * tx + (txp * txe);
        ty = u_resolution.y * ty + (typ * tye);

        // scale from [0..1] to [0..2pi]
        theta = theta * 2.0 * PI + (trp * tre);

        float st = sin(theta);
        float ct = cos(theta);

        // rotates first around +z-axis (0,0,1) and then translates by (tx,ty,0)
        vec4 p = vec4(
            a_position.x * ct - a_position.y * st + tx,
            a_position.x * st + a_position.y * ct + ty,
            0.0,
            1.0
        );

        gl_Position = u_proj * p;
    } else {
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    }
    
    v_uv = a_texCoord;
    v_alpha = alpha;
}

)END";

static const GLchar* sdfFragShaderSrc = R"END(
#extension GL_OES_standard_derivatives : enable
    
#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

uniform sampler2D u_tex;
uniform LOWP vec3 u_color;

varying vec2 v_uv;
varying float v_alpha;

const float gamma = 2.2;
const float tint = 1.8;
const float sdf = 0.8;

float contour(in float d, in float w, in float off) {
    return smoothstep(off - w, off + w, d);
}

float sample(in vec2 uv, float w, in float off) {
    return contour(texture2D(u_tex, uv).a, w, off);
}

float sampleAlpha(in vec2 uv, float distance, in float off) {
    float alpha = contour(distance, distance, off);

    float dscale = 0.354; // 1 / sqrt(2)
    vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
    vec4 box = vec4(uv - duv, uv + duv);

#ifdef GL_OES_standard_derivatives
    float asum = sample(box.xy, distance, off)
               + sample(box.zw, distance, off)
               + sample(box.xw, distance, off)
               + sample(box.zy, distance, off);

    alpha = (alpha + 0.5 * asum) / 2.0;
#endif

    return alpha;
}

void main(void) {
    if (v_alpha == 0.0) {
        discard;
    }

    float distance = texture2D(u_tex, v_uv).a;
    float alpha = sampleAlpha(v_uv, distance, sdf) * tint;
    alpha = pow(alpha, 1.0 / gamma);

    gl_FragColor = vec4(u_color, v_alpha * alpha);
}

)END";

static const GLchar* defaultFragShaderSrc = R"END(

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP 
#endif

uniform sampler2D u_tex;
uniform LOWP vec3 u_color;

varying vec2 v_uv;
varying float v_alpha;

void main(void) {
    if (v_alpha == 0.0) {
        discard;
    }
    
    vec4 texColor = texture2D(u_tex, v_uv);
    gl_FragColor = vec4(u_color.rgb, texColor.a * v_alpha);
}

)END";

}

#endif
