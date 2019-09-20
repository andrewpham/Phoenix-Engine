#version 460 core
layout (location = 1) out vec4 BlurredIrradianceMap;

const float GAUSS_WIDTH = 15.0f;

in vec2 TexCoords;

uniform sampler2D gIrradianceMap;
uniform sampler2D gStretchMap;

vec4 convolveV()
{
    float scaleConv = 1.0f / 1024.0f;
    vec4 stretch = texture(gStretchMap, TexCoords);
    float netFilterWidth = scaleConv * GAUSS_WIDTH * stretch.y;
    // Gaussian curve - standard deviation of 1.0
    float curve[7] = {0.006f, 0.061f, 0.242f, 0.383f, 0.242f, 0.061f, 0.006f};
    vec2 coords = TexCoords - vec2(0.0f, netFilterWidth * 3.0f);
    vec4 sum = vec4(0.0f);
    for (int i = 0; i < 7; ++i)
    {
        vec4 tap = texture(gIrradianceMap, coords);
        sum += curve[i] * tap;
        coords += vec2(0.0f, netFilterWidth);
    }
    return sum;
}

void main()
{
    BlurredIrradianceMap = convolveV();
}
