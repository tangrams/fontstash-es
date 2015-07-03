#ifndef SHADERS_H
#define SHADERS_H

#define USE_VARIABLE __attribute__((used))

namespace glfs {

USE_VARIABLE static const GLchar* vertexShaderSrc = R"END(
#version 150

in vec2 position;
in vec2 screenPosition;
in vec2 uvs;
in float alpha;
in float rotation;

uniform mat4 proj;

out vec2 uv;
out float a;

void main() {
    if (alpha != 0.0) {
        float st = sin(rotation);
        float ct = cos(rotation);

        vec4 p = vec4(
            position.x * ct - position.y * st + screenPosition.x,
            position.x * st + position.y * ct + screenPosition.y,
            0.0, 1.0
        );

        gl_Position = proj * p;
    } else {
        gl_Position = vec4(0.0);
    }

    uv = uvs;
    a = alpha;
}

)END";

USE_VARIABLE static const GLchar* sdfFragShaderSrc = R"END(
#version 150

uniform sampler2D tex;
uniform vec3 color;

in vec2 uv;
in float a;

out vec4 outColor;

const float gamma = 2.2;
const float tint = 1.8;
const float sdf = 0.8;

float contour(in float d, in float w, in float off) {
    return smoothstep(off - w, off + w, d);
}

float sample(in vec2 uv, float w, in float off) {
    return contour(texture(tex, uv).r, w, off);
}

float sampleAlpha(in vec2 uv, float distance, in float off) {
    float alpha = contour(distance, distance, off);
    float dscale = 0.354; // 1 / sqrt(2)
    vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
    vec4 box = vec4(uv - duv, uv + duv);

    float asum = sample(box.xy, distance, off)
               + sample(box.zw, distance, off)
               + sample(box.xw, distance, off)
               + sample(box.zy, distance, off);

    return (alpha + 0.5 * asum) / 2.0;
}

void main(void) {
    if (a == 0.0) {
        discard;
    }

    float distance = texture(tex, uv).r;
    float alpha = sampleAlpha(uv, distance, sdf) * tint;
    alpha = pow(alpha, 1.0 / gamma);

    outColor = vec4(color, alpha * a);
}

)END";

USE_VARIABLE static const GLchar* defaultFragShaderSrc = R"END(
#version 150

uniform sampler2D tex;
uniform vec3 color;

in vec2 uv;
in float a;

out vec3 outColor;

void main(void) {
    if (a == 0.0) {
        discard;
    }

    vec4 texColor = texture(tex, uv);
    outColor = vec4(color.rgb, texColor.a * a);
}

)END";

}

#endif
