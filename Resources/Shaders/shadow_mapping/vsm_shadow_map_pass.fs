#version 460 core

varying vec4 LightSpacePos;

void main()
{
    float depth = LightSpacePos.z / LightSpacePos.w;
    depth = depth * 0.5f + 0.5f;

    float moment1 = depth;
    float moment2 = depth * depth;

    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25f * (dx * dx + dy * dy);

    gl_FragColor = vec4(moment1, moment2, 0.0f, 0.0f);
}
