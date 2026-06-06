#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    // Strip translation from view matrix to keep skybox centered at camera position
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
    // Setting z to w ensures that after perspective divide, z/w = 1.0 (maximum depth)
    gl_Position = pos.xyww;
}
