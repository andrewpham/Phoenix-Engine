#version 460 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 WorldPosGS[];
in vec3 WorldNormalGS[];

out vec3 WorldPos;
out vec3 WorldNormal;

void main()
{
    // Plane normal
    const vec3 N = abs(cross(WorldPosGS[1] - WorldPosGS[0], WorldPosGS[2] - WorldNormalGS[0]));
    for (int i = 0; i < 3; ++i)
    {
        WorldPos = WorldPosGS[i];
        WorldNormal = WorldNormalGS[i];
        if (N.z > N.x && N.z > N.y)
        {
            gl_Position = vec4(WorldPos.x, WorldPos.y, 0.0f, 1.0f);
        }
        else if (N.x > N.y && N.x > N.z)
        {
            gl_Position = vec4(WorldPos.y, WorldPos.z, 0.0f, 1.0f);
        }
        else
        {
            gl_Position = vec4(WorldPos.x, WorldPos.z, 0.0f, 1.0f);
        }
        EmitVertex();
    }
    EndPrimitive();
}
