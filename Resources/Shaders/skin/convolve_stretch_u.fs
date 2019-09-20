#version 460 core
layout (location = 0) out vec4 BlurredStretchMap;

const float GAUSS_WIDTH = 15.0f;

in vec2 TexCoords;

uniform sampler2D gStretchMap;

vec4 convolveU()
{
    float scaleConv = 1.0f / 1024.0f;
    vec4 stretch = texture(gStretchMap, TexCoords);
    float netFilterWidth = scaleConv * GAUSS_WIDTH * stretch.x;
    // Gaussian curve - standard deviation of 1.0
    float curve[7] = {0.006f, 0.061f, 0.242f, 0.383f, 0.242f, 0.061f, 0.006f};
    vec2 coords = TexCoords - vec2(netFilterWidth * 3.0f, 0.0f);
    vec4 sum = vec4(0.0f);
    for (int i = 0; i < 7; ++i)
    {
        vec4 tap = texture(gStretchMap, coords);
        sum += curve[i] * tap;
        coords += vec2(netFilterWidth, 0.0f);
    }
    return sum;
}

void main()
{
    BlurredStretchMap = convolveU();
}
