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

void main() {
    vec3 baseColor = useColorOverride ? colorOverride : Color;

    if (!useLighting) {
        // Output base color directly for unlit/glowing objects
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    // Ambient component
    float ambientStrength = 0.15;
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

    // Final color output
    vec3 result = (ambient + diffuse + specular) * baseColor;
    FragColor = vec4(result, 1.0);
}
