#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gDepthMap;
uniform float gResolution;
uniform float gRadius;
uniform vec2 gDir;

void main()
{
    vec4 sum = vec4(0.0f);

    vec2 tc = TexCoords;

    // Sample radius in texels
    float blur = gRadius / gResolution; 

    // Blur direction (y-axis or x-axis)
    float hStep = gDir.x;
    float vStep = gDir.y;

    // 9-tap Gaussian filter
    sum += texture(gDepthMap, vec2(tc.x - 4.0f*blur*hStep, tc.y - 4.0f*blur*vStep)) * 0.0162162162f;
    sum += texture(gDepthMap, vec2(tc.x - 3.0f*blur*hStep, tc.y - 3.0f*blur*vStep)) * 0.0540540541f;
    sum += texture(gDepthMap, vec2(tc.x - 2.0f*blur*hStep, tc.y - 2.0f*blur*vStep)) * 0.1216216216f;
    sum += texture(gDepthMap, vec2(tc.x - 1.0f*blur*hStep, tc.y - 1.0f*blur*vStep)) * 0.1945945946f;

    sum += texture(gDepthMap, tc) * 0.2270270270f;

    sum += texture(gDepthMap, vec2(tc.x + 1.0f*blur*hStep, tc.y + 1.0f*blur*vStep)) * 0.1945945946f;
    sum += texture(gDepthMap, vec2(tc.x + 2.0f*blur*hStep, tc.y + 2.0f*blur*vStep)) * 0.1216216216f;
    sum += texture(gDepthMap, vec2(tc.x + 3.0f*blur*hStep, tc.y + 3.0f*blur*vStep)) * 0.0540540541f;
    sum += texture(gDepthMap, vec2(tc.x + 4.0f*blur*hStep, tc.y + 4.0f*blur*vStep)) * 0.0162162162f;

    FragColor = vec4(sum.rgb, 1.0f);
}
