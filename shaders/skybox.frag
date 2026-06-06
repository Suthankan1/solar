#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform float time;

// Support future cubemap texture mapping
uniform bool useTexture;
uniform samplerCube skyboxTex;

// Simple 3D hash function
float hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.1, 0.1));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D Value Noise
float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f); // Smooth interpolation

    return mix(
        mix(mix(hash(i + vec3(0.0, 0.0, 0.0)), hash(i + vec3(1.0, 0.0, 0.0)), f.x),
            mix(hash(i + vec3(0.0, 1.0, 0.0)), hash(i + vec3(1.0, 1.0, 0.0)), f.x), f.y),
        mix(mix(hash(i + vec3(0.0, 0.0, 1.0)), hash(i + vec3(1.0, 0.0, 1.0)), f.x),
            mix(hash(i + vec3(0.0, 1.0, 1.0)), hash(i + vec3(1.0, 1.0, 1.0)), f.x), f.y), f.z
    );
}

// Fractional Brownian Motion (fBm) for realistic organic gas clouds
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    vec3 shift = vec3(100.0);
    for (int i = 0; i < 4; ++i) {
        value += amplitude * noise(p);
        p = p * 2.0 + shift;
        amplitude *= 0.5;
    }
    return value;
}

// Generates tiny, sharp stars
float stars(vec3 p) {
    float n = noise(p * 180.0);
    // Threshold and sharpen to create small pinpoints
    return pow(max(n - 0.75, 0.0) * 4.0, 8.0);
}

void main() {
    // Check if textured skybox is enabled (future support)
    if (useTexture) {
        FragColor = texture(skyboxTex, TexCoords);
        return;
    }

    vec3 dir = normalize(TexCoords);

    // Nebula layers with slow, drifting motion
    vec3 nebulaDir1 = dir + vec3(time * 0.004, time * 0.002, time * 0.001);
    vec3 nebulaDir2 = dir * 1.5 - vec3(time * 0.001, time * 0.003, -time * 0.002);
    vec3 nebulaDir3 = dir * 2.2 + vec3(-time * 0.002, time * 0.001, time * 0.004);

    float n1 = fbm(nebulaDir1 * 2.5);
    float n2 = fbm(nebulaDir2 * 3.0);
    float n3 = fbm(nebulaDir3 * 1.8);

    // Color Palette for deep space
    vec3 spaceBg = vec3(0.015, 0.015, 0.03);    // Near-black deep navy background
    vec3 nebulaBlue = vec3(0.05, 0.15, 0.45);    // Deep cosmic blue gas
    vec3 nebulaPurple = vec3(0.35, 0.08, 0.45);  // Deep space purple/magenta gas
    vec3 nebulaTeal = vec3(0.02, 0.25, 0.22);    // Subtle teal dust cloud

    // Accumulate colors
    vec3 finalColor = spaceBg;
    finalColor += nebulaBlue * pow(n1, 1.5) * 0.7;
    finalColor += nebulaPurple * pow(n2, 1.8) * 0.6;
    finalColor += nebulaTeal * pow(n3, 2.0) * 0.3;

    // Twinkling stars (asynchronous twinkle frequency per star based on hash of position)
    float starPosHash = hash(floor(dir * 180.0));
    float twinkle = 0.55 + 0.45 * sin(time * (1.5 + starPosHash * 2.0) + starPosHash * 6.28);
    float s = stars(dir) * twinkle;

    // Add stars to nebula final color
    finalColor += vec3(s);

    FragColor = vec4(finalColor, 1.0);
}
