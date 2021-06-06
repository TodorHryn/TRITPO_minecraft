#version 330 core
layout (location = 0) in vec3 pos;

out vec2 texCoords;

uniform mat4 model;
uniform vec3 texturePos;

void main() {
	texCoords = texturePos.xy / 8 + (pos.xy + vec2(1.0f, 1.0f)) / 2 / 8;
	gl_Position = model * vec4(pos, 1.0f);
}  