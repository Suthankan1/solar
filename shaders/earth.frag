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
        // 1. Get Day Diffuse Color and Specular Value
        vec3 dayColor;
        float specularVal = 0.05;

        // Land/Water variables for fallback procedural city lights
        float continentVal = 0.0;
        float poleVal = abs(LocalPos.y);

        if (useTexture) {
            dayColor = texture(planetTexture, TexCoords).rgb;
            if (useSpecularTexture) {
                // Read from specular map (white = water/shiny, black = land/matte)
                float specMask = texture(specularTexture, TexCoords).r;
                specularVal = mix(0.05, 1.8, specMask);
            } else {
                // Ocean heuristic fallback
                if (dayColor.b > dayColor.r * 1.2 && dayColor.b > dayColor.g * 1.1) {
                    specularVal = 1.8;
                } else {
                    specularVal = 0.05;
                }
            }
        } else {
            // Procedural Earth surface fallback
            continentVal = fbm(LocalPos * 5.0 + vec3(0.12));
            if (poleVal > 0.83 + 0.08 * fbm(LocalPos * 10.0)) {
                dayColor = vec3(0.96, 0.96, 0.98); // Ice cap
                specularVal = 0.4;
            } else if (continentVal > 0.46) {
                float landNoise = fbm(LocalPos * 16.0);
                dayColor = mix(vec3(0.12, 0.35, 0.1), vec3(0.38, 0.28, 0.16), landNoise);
                if (landNoise > 0.65) {
                    dayColor = mix(dayColor, vec3(0.9, 0.9, 0.9), (landNoise - 0.65) * 2.8);
                }
                specularVal = 0.05;
                norm = perturbNormal(LocalPos, norm, 14.0, 0.035);
                diff = dot(norm, lightDir); // Re-evaluate diffuse with perturbed normal
            } else {
                float depth = continentVal / 0.46;
                dayColor = mix(vec3(0.01, 0.06, 0.22), vec3(0.04, 0.22, 0.42), depth);
                specularVal = 1.8;
            }
        }

        // 2. Get Night Lights Color
        vec3 nightColor;
        if (useNightTexture) {
            nightColor = texture(nightTexture, TexCoords).rgb;
            // Boost city lights slightly for cinematic bloom and HDR glow
            nightColor *= 2.0;
        } else {
            // Procedural city lights fallback
            float cityGlow = 0.0;
            if (useTexture) {
                // Using day map colors to estimate land areas (low saturation, low blue value)
                bool isOcean = (dayColor.b > dayColor.r * 1.15 && dayColor.b > dayColor.g * 1.05);
                if (!isOcean && dayColor.r > 0.05 && poleVal < 0.83) {
                    float cityNoise = fbm(LocalPos * 48.0);
                    cityGlow = smoothstep(0.68, 0.88, cityNoise) * 0.95;
                }
            } else {
                if (continentVal > 0.46 && poleVal < 0.83) {
                    float cityNoise = fbm(LocalPos * 48.0);
                    cityGlow = smoothstep(0.68, 0.88, cityNoise) * 0.95;
                }
            }
            nightColor = vec3(1.0, 0.75, 0.38) * cityGlow * 2.5;
        }

        // 3. Day / Night Blending at the Terminator
        // Blending window: diff in [-0.12, 0.12]
        float dayWeight = smoothstep(-0.12, 0.12, diff);

        // Lit side color (Diffuse lighting + Ambient)
        vec3 diffuseColor = max(diff, 0.0) * lightColor;
        vec3 litSurface = (ambient + diffuseColor * attenuation) * dayColor;

        // Dark side color (Unlit/Ambient + city lights)
        // City lights only show on the dark side, so we multiply them by (1.0 - dayWeight)
        vec3 darkSurface = ambient * dayColor * 0.2 + nightColor * (1.0 - dayWeight);

        // Blend Day and Night surface colors
        vec3 finalColor = mix(darkSurface, litSurface, dayWeight);

        // 4. Sunset/Sunrise Twilight Band (Atmosphere scattering at terminator)
        // Cosine angle between normal and light direction represents the terminator when close to 0.0
        // We create an atmospheric scattering band peaking at diff = 0.03 (slightly into the light side)
        float twilightFactor = smoothstep(0.20, 0.0, abs(diff - 0.03));
        vec3 sunsetGlowColor = vec3(1.0, 0.42, 0.08); // Hot orange/red sunset color
        vec3 sunsetGlow = sunsetGlowColor * twilightFactor * 1.8 * attenuation;
        
        // Land/Water sunset modulation: sunset is visible on land & clouds (gives gorgeous rim color)
        finalColor += sunsetGlow * (1.0 - specularVal * 0.4) * dayWeight;

        // 5. Specular highlight (Oceans only)
        vec3 reflectDir = reflect(-lightDir, norm);
        float shininess = 96.0;
        float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specularGlint = specularVal * specFactor * lightColor * dayWeight * attenuation;
        finalColor += specularGlint;

        // 6. Atmospheric Rim Glow (Subtle blue scattering layer on Earth edge)
        float rim = 1.0 - max(dot(viewDir, norm), 0.0);
        rim = pow(rim, 5.0);
        vec3 blueRimGlowColor = vec3(0.28, 0.58, 1.0);
        vec3 rimGlow = blueRimGlowColor * rim * 1.6 * dayWeight * attenuation;
        finalColor += rimGlow;

        FragColor = vec4(finalColor, globalAlpha);
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
