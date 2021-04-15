#version 330 core

uniform vec3 u_color;

in vec3 normal;

out vec4 frag_color;

void main() {
    vec3 light_pos = vec3(0.5f, 0.8f, 1.0f);
    vec3 light_col = vec3(1, 1, 1);
    float ambient_factor = 0.3f;
    vec3 ambient_col = ambient_factor * light_col;
    vec3 diffuse_col = light_col * max(0, dot(normal, normalize(light_pos)));
    frag_color = vec4(u_color * clamp(ambient_col + diffuse_col, 0.0f, 1.0f), 1.0f);
}