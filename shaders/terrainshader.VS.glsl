#version 430

layout(location = 0) in vec3 in_position; // Vertex position
layout(location = 1) in vec3 in_normal;   // Vertex normal
layout(location = 2) in vec2 in_texCoord; // Vertex texture coordinates

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Pass data to the fragment shader
out vec3 frag_position; 
out vec3 frag_normal;
out vec2 frag_texCoord;

void main()
{
    // Transform the vertex position
    frag_position = vec3(model * vec4(in_position, 1.0));
    gl_Position = projection * view * vec4(frag_position, 1.0);

    // Transform the normal
    frag_normal = mat3(transpose(inverse(model))) * in_normal;

    // Pass the texture coordinates
    frag_texCoord = in_texCoord;
}