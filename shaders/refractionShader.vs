#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 V; // View direction (from fragment to camera)
out vec3 N; // Normal vector

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec3 camPos = vec3(inverse(view) * vec4(0.0, 0.0, 0.0, 1.0));
    V = normalize(camPos - worldPos.xyz);
    N = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    gl_Position = projection * view * worldPos;
}
