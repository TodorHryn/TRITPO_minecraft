#version 330 core

layout (location = 0) in vec3 aVertexPos;
layout (location = 1) in vec3 aVertexNormal;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 lightSpaceMatrix1;
uniform mat4 lightSpaceMatrix2;
uniform mat4 lightSpaceMatrix3;
uniform mat4 lightSpaceMatrix4;

out vec3 normal;
out vec3 world_pos;
out vec4 posLightSpace1;
out vec4 posLightSpace2;
out vec4 posLightSpace3;
out vec4 posLightSpace4;


void main() {
	gl_Position = u_projection * u_view * u_model * vec4(aVertexPos, 1.0f);
	normal = aVertexNormal;
	world_pos = (u_model * vec4(aVertexPos, 1.0f)).xyz;

	posLightSpace1 = lightSpaceMatrix1 * vec4(world_pos, 1.0f);
	posLightSpace2 = lightSpaceMatrix2 * vec4(world_pos, 1.0f);
	posLightSpace3 = lightSpaceMatrix3 * vec4(world_pos, 1.0f);
	posLightSpace4 = lightSpaceMatrix4 * vec4(world_pos, 1.0f);
}