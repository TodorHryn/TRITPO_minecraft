#version 330 core

layout (location = 0) in vec3 aVertexPos;

uniform mat4 u_projection_view;
uniform mat4 u_model;

void main() {
   gl_Position = u_projection_view * u_model * vec4(aVertexPos, 1.0f);
}