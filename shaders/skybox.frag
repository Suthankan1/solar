#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform float time;

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
    vec3 dir = normalize(TexCoords);

    // Nebula layers with slow, drifting motion (made slower and more organic)
    vec3 nebulaDir1 = dir + vec3(time * 0.002, time * 0.001, time * 0.0005);
    vec3 nebulaDir2 = dir * 1.5 - vec3(time * 0.0005, time * 0.0015, -time * 0.001);
    vec3 nebulaDir3 = dir * 2.2 + vec3(-time * 0.001, time * 0.0005, time * 0.002);

    float n1 = fbm(nebulaDir1 * 2.0);
    float n2 = fbm(nebulaDir2 * 2.5);
    float n3 = fbm(nebulaDir3 * 1.5);

    // Dark realistic color palette for deep space (almost black, very subtle colors)
    vec3 spaceBg = vec3(0.001, 0.0015, 0.003);    // Deep dark void
    vec3 nebulaBlue = vec3(0.01, 0.03, 0.12);     // Very faint cosmic blue dust
    vec3 nebulaPurple = vec3(0.06, 0.02, 0.08);   // Very faint cosmic purple dust
    vec3 nebulaTeal = vec3(0.005, 0.04, 0.03);    // Very faint cosmic teal dust
    float vertical = dir.y * 0.5 + 0.5;
    float galactic = pow(1.0 - abs(dir.y), 2.2);
    vec3 ambientTint = mix(vec3(0.002, 0.004, 0.014), vec3(0.010, 0.004, 0.020), vertical);

    // Accumulate colors
    vec3 finalColor = spaceBg;
    finalColor += ambientTint * 0.35;
    finalColor += vec3(0.010, 0.006, 0.024) * galactic * 0.12;
    finalColor += nebulaBlue * pow(n1, 2.5) * 0.6;
    finalColor += nebulaPurple * pow(n2, 2.8) * 0.5;
    finalColor += nebulaTeal * pow(n3, 3.0) * 0.35;

    // Faint unresolved background galaxies (extremely tiny, static pinpoints)
    float n_star = noise(dir * 300.0);
    float s = pow(max(n_star - 0.85, 0.0) * 6.6, 12.0) * 0.08; // extremely faint and small

    finalColor += vec3(s) * 1.3;

    FragColor = vec4(finalColor, 1.0);
}
