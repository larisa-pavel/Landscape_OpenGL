#version 430

// Input from the vertex shader
in vec3 frag_position;
in vec3 frag_normal;
in vec2 frag_texCoord;

out vec4 frag_color;

// Function to generate noise (Simplex or Perlin noise)
float noise(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

float fbm(vec2 uv) {
    float value = 0.0;
    float scale = 0.5;
    for (int i = 0; i < 5; i++) {
        value += scale * noise(uv);
        uv *= 2.0;
        scale *= 0.5;
    }
    return value;
}

void main()
{
    // Scale the texture coordinates to simulate terrain variation
    vec2 scaled_uv = frag_texCoord * 10.0;
    float elevation = fbm(scaled_uv) * 5.0; // Adjust scale for "mountain" effect

    // Calculate base color
    vec3 baseColor = vec3(0.1, 0.6, 0.3); // Green for grass

    // Adjust color based on elevation
    if (elevation > 2.5) {
        baseColor = vec3(0.5, 0.5, 0.5); // Gray for rocks
    }
    if (elevation > 4.0) {
        baseColor = vec3(1.0, 1.0, 1.0); // White for snow
    }

    // Simulate lighting using a simple diffuse component
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normalize(frag_normal), lightDir), 0.0);

    frag_color = vec4(baseColor * diffuse, 1.0);
}