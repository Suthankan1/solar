#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;
out vec2 TexCoords;
out vec3 LocalPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    // Transform normals to world space, avoiding non-uniform scaling issues
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    TexCoords = aTexCoords;
    LocalPos = aPos;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
