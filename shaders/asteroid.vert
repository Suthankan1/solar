#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoords;
layout (location = 4) in mat4 aInstanceMatrix;
layout (location = 8) in vec3 aInstanceColor;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;
out vec2 TexCoords;
out vec3 LocalPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Combine overall belt orbit/rotation matrix with the individual asteroid's instance matrix
    mat4 finalModel = model * aInstanceMatrix;
    
    FragPos = vec3(finalModel * vec4(aPos, 1.0));
    
    // Transform normals to world space, avoiding non-uniform scaling issues
    Normal = mat3(transpose(inverse(finalModel))) * aNormal;
    
    Color = aInstanceColor;
    TexCoords = aTexCoords;
    LocalPos = aPos;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
