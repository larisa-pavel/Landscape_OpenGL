#version 330 core

in vec3 frag_normal;
in float frag_height;

out vec4 frag_color;

void main() {
    vec3 normal_color = normalize(frag_normal) * 0.5 + 0.5;
    float height_factor = clamp(frag_height / 10.0, 0.0, 1.0);
    vec3 base_color = mix(vec3(0.1, 0.05, 0.0), vec3(0.8, 0.7, 0.2), height_factor);
    vec3 final_color = mix(base_color, normal_color, 0.3);
    frag_color = vec4(final_color, 1.0);
}