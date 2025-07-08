#version 330 core 

layout(location = 0) in vec3 aPos;      // Vertex position
layout(location = 1) in vec3 aNormal;   // Vertex normal
layout(location = 2) in float aD_N;     // Vertex precomputed d_N

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 V; // View direction (in view space)
out vec3 N; // Normal vector (in view space)
out vec3 FragPos; // Position in world space
out float d_N; // Precomptuted d_N

void main() 
{
    // Transform vertex to world space
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // d_N
    d_N = aD_N;

    // Compute view direction in world space
    vec3 viewPos = vec3(inverse(view) * vec4(0.0, 0.0, 0.0, 1.0)); 
    V = normalize(viewPos - worldPos.xyz); 

    // Transform normal properly
    N = normalize(mat3(transpose(inverse(model))) * aNormal);

    // Project the vertex
    gl_Position = projection * view * worldPos;
}
