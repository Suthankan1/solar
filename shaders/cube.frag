// Phong lighting shader
// Supports: flat color, emissive glow, Phong illumination, alpha transparency
// Uniforms: useLighting, useColorOverride, colorOverride, emissiveStrength, globalAlpha,
//           lightPos, lightColor, viewPos, time, isStarfield
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform bool useLighting;

uniform vec3 colorOverride;
uniform bool useColorOverride;
uniform float emissiveStrength;
uniform float globalAlpha = 1.0;

uniform float time;
uniform bool isStarfield = false;

void main() {
    vec3 baseColor = useColorOverride ? colorOverride : Color;

    if (!useLighting) {
        // Output base color directly for unlit/glowing objects
        if (isStarfield) {
            // Twinkle: use the green channel as a per-star phase offset
            float twinklePhase = Color.g * 6.28318;  // 2*PI * phase
            float twinkle = 0.85 + 0.15 * sin(time * 3.0 + twinklePhase);
            FragColor = vec4(vec3(Color.r, Color.r, Color.b) * twinkle * (useColorOverride ? colorOverride : vec3(1.0)), globalAlpha);
        } else if (emissiveStrength > 0.0) {
            FragColor = vec4(baseColor * (1.0 + emissiveStrength), globalAlpha);
        } else {
            FragColor = vec4(baseColor, globalAlpha);
        }
        return;
    }

    // Ambient component
    float ambientStrength = 0.08;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse component
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular component
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Rim / atmospheric scatter effect
    float rimStrength = 0.0;
    if (useLighting) {
        float rim = 1.0 - max(dot(normalize(viewPos - FragPos), normalize(Normal)), 0.0);
        rim = pow(rim, 3.0);
        rimStrength = rim * 0.4;
    }

    // Final color output
    vec3 result = (ambient + diffuse + specular) * baseColor;
    result += rimStrength * lightColor * baseColor * 0.5;
    FragColor = vec4(result, globalAlpha);
}
