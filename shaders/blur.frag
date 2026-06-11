#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform bool firstPass;
uniform float threshold = 0.85;

// 5-tap Gaussian blur weights
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

vec3 extractBright(vec3 color) {
    float brightness = max(color.r, max(color.g, color.b));
    // Soft knee threshold to prevent harsh cutoffs
    float knee = 0.1;
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);
    return color * contribution;
}

void main() {             
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result;
    
    if (firstPass) {
        result = extractBright(texture(image, TexCoords).rgb) * weight[0];
    } else {
        result = texture(image, TexCoords).rgb * weight[0];
    }
    
    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            vec2 offset = vec2(tex_offset.x * i, 0.0);
            vec3 c1 = texture(image, TexCoords + offset).rgb;
            vec3 c2 = texture(image, TexCoords - offset).rgb;
            if (firstPass) {
                result += (extractBright(c1) + extractBright(c2)) * weight[i];
            } else {
                result += (c1 + c2) * weight[i];
            }
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            vec2 offset = vec2(0.0, tex_offset.y * i);
            vec3 c1 = texture(image, TexCoords + offset).rgb;
            vec3 c2 = texture(image, TexCoords - offset).rgb;
            if (firstPass) {
                result += (extractBright(c1) + extractBright(c2)) * weight[i];
            } else {
                result += (c1 + c2) * weight[i];
            }
        }
    }
    FragColor = vec4(result, 1.0);
}
