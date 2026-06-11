#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure = 1.0;
uniform float bloomIntensity = 0.45;

void main() {             
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    
    if (bloom) {
        hdrColor += bloomColor * bloomIntensity; // Additive blending
    }
    
    // Tone mapping (Exposure tone mapping offers a cinematic look)
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, 1.0);
}
