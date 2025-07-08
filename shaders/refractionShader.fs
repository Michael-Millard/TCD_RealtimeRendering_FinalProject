#version 330 core

in vec3 V; // View direction
in vec3 N; // Normal at the fragment

out vec4 FragColor;

// Skybox
uniform samplerCube skybox;

// Index of refratction
float airIOR = 1.0;
uniform float modelIOR;
uniform bool reflectEnable;

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

void main() 
{
    // Incident direction from eye to surface
    vec3 I = -V; 
    
    // Compute refraction direction (air -> glass)
    vec3 refractedDir = refract(I, N, airIOR / modelIOR);
    vec3 finalColor = texture(skybox, refractedDir).rgb;

    // Check if reflection enabled
    if (reflectEnable)
    {
        // Compute Fresnel term
        float cosTheta = clamp(dot(I, -N), 0.0, 1.0);
        float fresnel = fresnelSchlick(cosTheta);

        // Compute reflection direction
        vec3 reflectedDir = reflect(I, N);

        // Sample skybox for reflection and refraction
        vec3 reflectedColor = texture(skybox, reflectedDir).rgb;
        vec3 refractedColor = texture(skybox, refractedDir).rgb;

        // Blend using Fresnel term
        finalColor = mix(refractedColor, reflectedColor, fresnel);
    }

    FragColor = vec4(finalColor, 1.0);
}
