#version 330 core

in vec3 V;           // View direction (from surface to camera)
in vec3 N;           // Surface normal
in vec3 FragPos;     // Front surface world position (P1)
in float d_N;        // Precomputed Blender thickness along normal

out vec4 FragColor;

uniform samplerCube skybox;
uniform sampler2D backfaceNormalTex;
uniform sampler2D backfaceDepthTex;
uniform mat4 projection;
uniform mat4 view;

float airIOR = 1.0;
uniform float modelIOR;
uniform bool reflectEnable;
uniform bool viewSpaceOnly;

// Convert screen-space depth to world-space position
vec3 getWorldPosFromDepth(float depth, vec2 uv)
{
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos = inverse(projection) * clip;
    viewPos /= viewPos.w;
    return vec3(inverse(view) * viewPos);
}

// Compute F0
float computeF0(float IOR_air, float IOR_surface)
{
    float ratio = (IOR_air - IOR_surface) / (IOR_air + IOR_surface);
    return ratio * ratio;
}

// Fresnel-Schlick approximation
float fresnelSchlick(float cosTheta) 
{
    float F0 = computeF0(airIOR, modelIOR);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float computeDistance(float d_N, float d_V, float ratio)
{
    return ratio * d_V + (1.0 - ratio) * d_N;
}

void main()
{
    // Clamp UV to prevent out-of-bounds errors
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(backfaceDepthTex, 0));
    uv = clamp(uv, vec2(0.001), vec2(0.999));

    // View-space depth from backface
    float d_V = texture(backfaceDepthTex, uv).r; 
    if (d_V >= 0.999)
        discard;

    vec3 P1 = FragPos;
    vec3 PV = getWorldPosFromDepth(d_V, uv);
    d_V = length(PV - P1); // Convert depth to real-world view ray thickness

    vec3 I = -V; // Incoming ray (eye to surface)
    vec3 T1 = refract(I, N, airIOR / modelIOR); // First refraction (air -> glass, so 1.0 / eta)

    // If only view-space (no d_N)
    if (viewSpaceOnly)
    {
        // Bail early if N2 is invalid (in case of garbage sampling)
        vec3 N2 = texture(backfaceNormalTex, uv).rgb * 2.0 - 1.0;
        if (length(N2) < 0.001)
            discard;
        
        // Step 3: View direction & surface normal
        vec3 T1 = refract(I, N, airIOR / modelIOR); // Air → Glass
        
        // Step 4: Second refraction (glass → air), bail if T2 is a zero vector (total internal reflection)
        vec3 T2 = refract(T1, -N2, modelIOR / airIOR); // Invert N2 for correct refraction
        if (length(T2) < 0.001)
            T2 = reflect(I, N); // fallback to reflection

        // Set final colour as refracted colour for now
        vec3 finalColor = texture(skybox, T2).rgb;

        // Check if reflection enabled
        if (reflectEnable)
        {
            // Compute Fresnel term
            float cosTheta = clamp(dot(I, -N), 0.0, 1.0);
            float fresnel = fresnelSchlick(cosTheta);

            // Sample skybox for reflection and refraction
            vec3 refractedColor = texture(skybox, T2).rgb;
            vec3 reflectedColor = texture(skybox, reflect(I, N)).rgb;

            // Blend using Fresnel term
            finalColor = mix(refractedColor, reflectedColor, fresnel);
        }

        FragColor = vec4(finalColor, 1.0);
    }
    // Else do weigthed sum of d_V and d_N
    else
    {
        // Compute angles
        float theta_i = acos(clamp(dot(N, I), -1.0, 1.0));
        float theta_t = acos(clamp(dot(-N, T1), -1.0, 1.0));

        // Bail early if angle is degenerate
        if (theta_i < 0.001 || theta_t < 0.001)
            discard;

        // Distance blend from paper
        float ratio = theta_t / theta_i;
        float d = computeDistance(d_N, d_V, ratio);
        vec3 P2 = P1 + T1 * d;

        // Project P2 into screen space
        vec4 clipP2 = projection * view * vec4(P2, 1.0);
        clipP2 /= clipP2.w;
        vec2 uvP2 = clipP2.xy * 0.5 + 0.5;
        uvP2 = clamp(uvP2, vec2(0.001), vec2(0.999));

        // Sample normal
        vec3 N2 = texture(backfaceNormalTex, uvP2).rgb * 2.0 - 1.0;
        if (length(N2) < 0.001)
            discard;

        // Second refraction (TIR check)
        vec3 T2 = refract(T1, -N2, modelIOR / airIOR);
        if (length(T2) < 0.001)
            T2 = reflect(I, N); // fallback is to reflect original incident ray at N1

        // Sample environment
        vec3 refractedColor = texture(skybox, T2).rgb;
        vec3 finalColor = refractedColor;

        // Optional reflection blending
        if (reflectEnable)
        {
            float cosTheta = clamp(dot(I, -N), 0.0, 1.0);
            float fresnel = fresnelSchlick(cosTheta);
            vec3 reflectedColor = texture(skybox, reflect(I, N)).rgb;
            finalColor = mix(refractedColor, reflectedColor, fresnel);
        }

        FragColor = vec4(finalColor, 1.0);
    }
}   