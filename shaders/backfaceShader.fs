#version 330 core

in vec3 worldNormal;

layout(location = 0) out vec4 FragColor;

void main()
{
    // Encode the normals: map from [-1, 1] to [0, 1] for GL_RGBA compatibility
    vec3 encodedNormal = normalize(worldNormal) * 0.5 + 0.5;
    FragColor = vec4(encodedNormal, 1.0);
}
