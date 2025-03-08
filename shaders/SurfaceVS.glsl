#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 v_texture_coord;

out vec3 frag_normal;
out float frag_height;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

void main() {
    frag_normal = normalize(mat3(transpose(inverse(Model))) * normal);
    frag_height = position.y;
    gl_Position = Projection * View * Model * vec4(position, 1.0);
}