#version 330 core

uniform vec3 u_color;

in vec3 normal;
in vec3 world_pos;

out vec4 frag_color;

uniform vec3 light_pos;
uniform float ambient_factor;

void main() {
    vec3 light_col = vec3(1, 1, 1);
    vec3 ambient_col = ambient_factor * light_col;
    vec3 diffuse_col = light_col * max(0.0f, dot(normal, normalize(light_pos)));

    frag_color = vec4(u_color * clamp(ambient_col + diffuse_col, 0.0f, 1.0f), 1.0f);
}