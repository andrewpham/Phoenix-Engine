#version 460 core
out vec4 FragColor;

const vec3 SCALE = vec3(0.8f);
const int NUM_STEPS = 30;
const int NUM_ITERATIONS = 5; // Number of binary search iterations
const float SPECULAR_FACTOR = 3.0f;
const float K = 19.19f;
const float STEP = 0.1f; // Minimum ray march step size
const float BINARY_SEARCH_STEP = 0.5f;

noperspective in vec2 TexCoords;

uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform sampler2D gAlbedoSpecularMap;
uniform sampler2D gPreviousFrameMap;
uniform sampler2D gMetallicMap;

uniform mat4 gInverseViewMatrix;
uniform mat4 gProjectionMatrix;

// Map each position to some consistent offset so that we can avoid artifacts in subsequent calculations
vec3 hash(vec3 position)
{
    position = fract(SCALE * position);
    position += dot(position, position.yxz + K);
    return fract((position.xxy + position.yxx) * position.zyx);
}

vec3 binarySearch(inout vec3 position, inout vec3 direction, inout float delta)
{
    vec4 projectedCoords;
    float depth;

    for (int i = 0; i < NUM_ITERATIONS; ++i)
    {
        direction *= BINARY_SEARCH_STEP;

        projectedCoords = gProjectionMatrix * vec4(position, 1.0f);
        projectedCoords.xy /= projectedCoords.w;
        projectedCoords.xy = projectedCoords.xy * 0.5f + 0.5f;

        depth = texture(gPositionMap, projectedCoords.xy).z;

        delta = position.z - depth;

        if (delta > 0.0f)
        {
            position += direction;
        }
        else
        {
            position -= direction;
        }
    }
    projectedCoords = gProjectionMatrix * vec4(position, 1.0f);
    projectedCoords.xy /= projectedCoords.w;
    projectedCoords.xy = projectedCoords.xy * 0.5f + 0.5f;
    return vec3(projectedCoords.xy, depth);
}

vec4 rayMarch(inout vec3 position, vec3 direction, out float delta)
{
    direction *= STEP;

    vec4 projectedCoords;
    float depth;

    for (int i = 0; i < NUM_STEPS; ++i)
    {
        position += direction;

        projectedCoords = gProjectionMatrix * vec4(position, 1.0f);
        projectedCoords.xy /= projectedCoords.w;
        projectedCoords.xy = projectedCoords.xy * 0.5f + 0.5f;

        depth = texture(gPositionMap, projectedCoords.xy).z;

        delta = position.z - depth;

        // if (depth - (position.z - direction.z) < 1.2f)
        // Is the difference between the starting and sampled depths smaller than the width of the unit cube?
        // We don't want to sample too far from the starting position.
        if (direction.z - delta < 1.2f)
        {
            // We're at least past the point where the ray intersects a surface.
            // Now, determine the values at the precise location of intersection.
            if (delta < 0.0f)
            {
                return vec4(binarySearch(position, direction, delta), 1.0f);
            }
        }
    }
    return vec4(projectedCoords.xy, depth, 0.0f);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

void main()
{
    float metalness = texture2D(gMetallicMap, TexCoords).r;
    if (metalness < 0.01f)
    {
        FragColor = texture(gPreviousFrameMap, TexCoords);
        return;
    }

    vec3 N = normalize(vec3(texture2D(gNormalMap, TexCoords) * gInverseViewMatrix)); // View space
    vec3 P = texture(gPositionMap, TexCoords).xyz; // View space
    vec3 albedo = texture(gPreviousFrameMap, TexCoords).rgb;
    float specular = texture(gAlbedoSpecularMap, TexCoords).w;

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metalness);
    vec3 F = fresnelSchlick(clamp(dot(N, normalize(P)), 0.0f, 1.0f), F0);

    vec3 R = normalize(reflect(normalize(P), N));

    // Difference between the depth at the current view space position in our ray marching routine
    // and the depth at the sampled position given by projecting that same view space position back
    // onto our position map. The goal is to determine whether our ray intersects a surface in our scene,
    // and we can figure this out by comparing where we are at in the ray marching to the stored depth
    // value indicating the distance of the object closest to the camera. We iterate the search until
    // we register a hit with our ray marching and record its precise location.
    float delta;
    vec3 worldPos = vec3(vec4(P, 1.0f) * gInverseViewMatrix);
    vec3 jitter = mix(vec3(0.0f), hash(worldPos), specular);
    vec4 hitInfo = rayMarch(P, vec3(jitter) - R * max(STEP, -P.z), delta);

    vec2 offset = abs(vec2(0.5f, 0.5f) - hitInfo.xy); // Offset from center
    float focusFactor = clamp(1.0f - (offset.x + offset.y), 0.0f, 1.0f);
    // Calibrate the result depending on how close the sampled intersection is to the center of the screen.
    // Focused on fragments should have a more pronounced color.
    float correctionFactor = focusFactor * pow(metalness, SPECULAR_FACTOR) * -R.z;

    vec3 fragColor = textureLod(gPreviousFrameMap, hitInfo.xy, 0).rgb * clamp(correctionFactor, 0.0f, 0.9f) * F;
    FragColor = vec4(fragColor, metalness);
}
