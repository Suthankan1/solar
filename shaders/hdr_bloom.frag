#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure = 1.2;
uniform float bloomIntensity = 0.8;
uniform float vignetteStrength = 0.0;

vec3 ACESFilm(vec3 x) {
    return clamp(x * (2.51 * x + 0.03) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {             
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    vec3 result = hdrColor;
    
    if (bloom) {
        result += bloomColor * bloomIntensity;
    }
    
    // Subtle chromatic aberration on brights:
    float lum = dot(result, vec3(0.299, 0.587, 0.114));
    if (lum > 0.5 && vignetteStrength > 0.0) {
        float ca = (lum - 0.5) * 0.003 * vignetteStrength;
        result.r = texture(scene, TexCoords + vec2(ca, 0.0)).r;
        result.b = texture(scene, TexCoords - vec2(ca, 0.0)).b;
    }
    
    result = ACESFilm(result * exposure);
    result = pow(result, vec3(1.0 / 2.2));
    
    // Vignette post-effect:
    vec2 center = TexCoords - vec2(0.5);
    float vign = 1.0 - smoothstep(0.35, 0.75, length(center) * 1.4);
    result *= mix(1.0, vign, vignetteStrength);
    
    FragColor = vec4(result, 1.0);
}
