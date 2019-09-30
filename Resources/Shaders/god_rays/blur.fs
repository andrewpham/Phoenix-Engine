#version 460 core
out vec4 FragColor;

const float H_BLUR = 3.0f / 2460.0f; // Sample radius in texels
const float V_BLUR = 3.0f / 1440.0f;

in vec2 TexCoords;

uniform sampler2D gPreviousFrameMap;
uniform vec2 gDir;

void main()
{
    vec4 sum = vec4(0.0f);

    // 9-tap Gaussian filter
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x - 4.0f * H_BLUR * gDir.x, TexCoords.y - 4.0f * V_BLUR * gDir.y)) * 0.0162162162f;
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x - 3.0f * H_BLUR * gDir.x, TexCoords.y - 3.0f * V_BLUR * gDir.y)) * 0.0540540541f;
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x - 2.0f * H_BLUR * gDir.x, TexCoords.y - 2.0f * V_BLUR * gDir.y)) * 0.1216216216f;
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x - 1.0f * H_BLUR * gDir.x, TexCoords.y - 1.0f * V_BLUR * gDir.y)) * 0.1945945946f;

    sum += texture(gPreviousFrameMap, TexCoords) * 0.2270270270f;

    sum += texture(gPreviousFrameMap, vec2(TexCoords.x + 1.0f * H_BLUR * gDir.x, TexCoords.y + 1.0f * V_BLUR * gDir.y)) * 0.1945945946f;
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x + 2.0f * H_BLUR * gDir.x, TexCoords.y + 2.0f * V_BLUR * gDir.y)) * 0.1216216216f;
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x + 3.0f * H_BLUR * gDir.x, TexCoords.y + 3.0f * V_BLUR * gDir.y)) * 0.0540540541f;
    sum += texture(gPreviousFrameMap, vec2(TexCoords.x + 4.0f * H_BLUR * gDir.x, TexCoords.y + 4.0f * V_BLUR * gDir.y)) * 0.0162162162f;

    FragColor = vec4(sum.rgb, 1.0f);
}
