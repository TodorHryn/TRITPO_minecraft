#version 330 core

layout (location = 0) in vec3 aVertexPos;
layout (location = 1) in vec3 aVertexNormal;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec3 normal;

void main() {
   gl_Position = u_model * u_projection * u_view * vec4(aVertexPos, 1.0f);
   normal = aVertexNormal;
}