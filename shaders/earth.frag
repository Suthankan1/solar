#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec2 TexCoords;
in vec3 LocalPos;

// Uniforms
uniform int earthMode = 0; // 0 = Surface, 1 = Clouds, 2 = Atmosphere
uniform float time;
uniform float globalAlpha = 1.0;

// Lighting params
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform bool useLighting = true;

uniform float lightConstant;
uniform float lightLinear;
uniform float lightQuadratic;

// Texture maps and status flags
uniform sampler2D planetTexture;   // Unit 0: Day diffuse map
uniform sampler2D nightTexture;    // Unit 1: Night lights map
uniform sampler2D cloudTexture;    // Unit 2: Cloud map
uniform sampler2D specularTexture; // Unit 3: Specular map

uniform bool useTexture = false;
uniform bool useNightTexture = false;
uniform bool useCloudTexture = false;
uniform bool useSpecularTexture = false;

// Hash function for 3D noise (Inigo Quilez)
float hash3(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.1, 0.1));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3D Value Noise
float noise(in vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    vec3 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(mix(hash3(i + vec3(0.0,0.0,0.0)), 
                       hash3(i + vec3(1.0,0.0,0.0)), u.x),
                   mix(hash3(i + vec3(0.0,1.0,0.0)), 
                       hash3(i + vec3(1.0,1.0,0.0)), u.x), u.y),
               mix(mix(hash3(i + vec3(0.0,0.0,1.0)), 
                       hash3(i + vec3(1.0,0.0,1.0)), u.x),
                   mix(hash3(i + vec3(0.0,1.0,1.0)), 
                       hash3(i + vec3(1.0,1.0,1.0)), u.x), u.y), u.z);
}

// 4-octave Fractional Brownian Motion (fBm)
float fbm(in vec3 p) {
    float f = 0.0;
    f += 0.5000 * noise(p); p = p * 2.01;
    f += 0.2500 * noise(p); p = p * 2.02;
    f += 0.1250 * noise(p); p = p * 2.03;
    f += 0.0625 * noise(p);
    return f;
}

// Normal perturbation (bump mapping) using local-space gradients
vec3 perturbNormal(vec3 pos, vec3 normal, float scale, float strength) {
    float eps = 0.01;
    vec3 localNormal = normalize(pos);
    vec3 localTangent = normalize(cross(localNormal, vec3(0.0, 1.0, 0.0) + vec3(0.001)));
    vec3 localBitangent = cross(localNormal, localTangent);
    
    // Sample heights
    float h = fbm(pos * scale);
    float h_t = fbm((pos + localTangent * eps) * scale);
    float h_b = fbm((pos + localBitangent * eps) * scale);
    
    float dh_dt = (h_t - h) / eps;
    float dh_db = (h_b - h) / eps;
    
    // Construct TBN frame from interpolated world normal
    vec3 worldNormal = normalize(normal);
    vec3 worldTangent = normalize(cross(worldNormal, vec3(0.0, 1.0, 0.0) + vec3(0.001)));
    vec3 worldBitangent = cross(worldNormal, worldTangent);
    
    return normalize(worldNormal - strength * (worldTangent * dh_dt + worldBitangent * dh_db));
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Distance attenuation from the Sun
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

    // Standard Phong Ambient component
    float ambientStrength = 0.04;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse Dot
    float diff = dot(norm, lightDir);

    // -------------------------------------------------------------
    // MODE 0: Earth Surface
    // -------------------------------------------------------------
    if (earthMode == 0) {
        // Day/night blend in earth.frag:
        float sunDot = max(dot(normalize(Normal), normalize(lightPos - FragPos)), 0.0);
        float dayFactor = smoothstep(0.0, 0.15, sunDot);
        vec3 dayColor = texture(planetTexture, TexCoords).rgb;
        vec3 nightColor = texture(nightTexture, TexCoords).rgb * 1.8;
        vec3 surface = mix(nightColor, dayColor, dayFactor);

        // Twilight orange rim:
        float twi = smoothstep(0.0, 0.2, sunDot) - smoothstep(0.15, 0.3, sunDot);
        surface += vec3(0.8, 0.35, 0.05) * twi * 0.3;

        // Specular highlight (Oceans only)
        if (useSpecularTexture) {
            float specMask = texture(specularTexture, TexCoords).r;
            float specularVal = mix(0.05, 1.8, specMask);
            vec3 reflectDir = reflect(-lightDir, norm);
            float shininess = 96.0;
            float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specularGlint = specularVal * specFactor * lightColor * dayFactor * attenuation;
            surface += specularGlint;
        }

        // Atmospheric Rim Glow (blue scattering at the Earth limb)
        float rimDot = max(dot(normalize(viewDir), normalize(Normal)), 0.0);
        float rim = 1.0 - rimDot;
        vec3 atmosBlue = vec3(0.25, 0.55, 1.0) * pow(rim, 3.0) * 2.0;
        vec3 oxyGreen = vec3(0.30, 1.00, 0.4) * step(0.92, rim) * 0.4 * (rim - 0.92) / 0.08;
        surface += atmosBlue + oxyGreen;

        FragColor = vec4(surface, globalAlpha);
        return;
    }

    // -------------------------------------------------------------
    // MODE 1: Cloud Layer
    // -------------------------------------------------------------
    else if (earthMode == 1) {
        float cloudAlpha = 0.0;
        
        // Scroll cloud texture over time
        vec2 scrolledCoords = TexCoords + vec2(time * 0.004, time * 0.002);

        if (useCloudTexture) {
            // Sample cloud texture (usually grayscale, red channel or RGB average represents opacity)
            cloudAlpha = texture(cloudTexture, scrolledCoords).r;
        } else {
            // Procedural FBM noise cloud fallback
            vec3 cloudScrollPos = LocalPos + vec3(time * 0.015, 0.0, time * 0.008);
            float density = fbm(cloudScrollPos * 5.0);
            cloudAlpha = smoothstep(0.48, 0.72, density);
        }

        // Apply light scatter and attenuation
        float wrapDiff = max(diff * 0.6 + 0.4, 0.0);
        vec3 diffuseCloud = wrapDiff * lightColor;
        vec3 cloudColor = vec3(0.95, 0.95, 0.98) * (ambient + diffuseCloud * attenuation);

        // City lights underneath can bleed through clouds on the night side slightly,
        // but we mainly want to fade out the clouds on the dark side of the terminator
        float litFade = smoothstep(-0.25, 0.20, diff);
        float finalAlpha = cloudAlpha * mix(0.12, 0.88, litFade) * globalAlpha;

        FragColor = vec4(cloudColor, finalAlpha);
        return;
    }

    // -------------------------------------------------------------
    // MODE 2: Atmosphere Layer
    // -------------------------------------------------------------
    else if (earthMode == 2) {
        // Atmospheric Rayleigh-like scattering approximation
        float dotVN = dot(viewDir, norm);
        float rim = 1.0 - max(dotVN, 0.0);
        
        // Atmosphere thickness: high at edges (rim), thin in center
        float density = pow(rim, 4.0) * 0.85 + pow(rim, 2.0) * 0.15;
        
        // Scatter multiplier based on light direction
        float scatter = smoothstep(-0.2, 0.4, diff);
        
        // Sunset color modulation: orange near terminator, blue in full light
        float sunsetFactor = smoothstep(0.25, -0.05, diff) * smoothstep(-0.25, 0.05, diff);
        vec3 blueColor = vec3(0.24, 0.52, 1.0);
        vec3 orangeColor = vec3(1.0, 0.45, 0.08);
        vec3 atmosphereColor = mix(blueColor, orangeColor, sunsetFactor * 0.85);

        vec3 col = atmosphereColor * 1.5 * scatter * attenuation;
        float alpha = density * scatter * globalAlpha * 0.9;

        FragColor = vec4(col, alpha);
        return;
    }

    // Fallback: white
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
