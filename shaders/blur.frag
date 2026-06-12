#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform bool firstPass;
uniform float threshold = 0.85;

uniform float weight[7] = float[] (0.0625, 0.125, 0.1875, 0.25, 0.1875, 0.125, 0.0625);

vec3 extractBright(vec3 color) {
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold) {
        return color;
    }
    return vec3(0.0);
}

void main() {             
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result = vec3(0.0);
    
    if (horizontal) {
        for (int i = -3; i <= 3; ++i) {
            vec2 offset = vec2(tex_offset.x * float(i), 0.0);
            vec3 color = texture(image, TexCoords + offset).rgb;
            if (firstPass) {
                color = extractBright(color);
            }
            result += color * weight[i + 3];
        }
    } else {
        for (int i = -3; i <= 3; ++i) {
            vec2 offset = vec2(0.0, tex_offset.y * float(i));
            vec3 color = texture(image, TexCoords + offset).rgb;
            if (firstPass) {
                color = extractBright(color);
            }
            result += color * weight[i + 3];
        }
    }
    FragColor = vec4(result, 1.0);
}
