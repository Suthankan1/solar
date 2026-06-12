// Phong lighting shader with procedural planet generation and bump mapping
// Uniforms: useLighting, useColorOverride, colorOverride, emissiveStrength, globalAlpha,
//           lightPos, lightColor, viewPos, time, isStarfield, planetId
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec2 TexCoords;
in vec3 LocalPos;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform bool useLighting;

uniform float lightConstant;
uniform float lightLinear;
uniform float lightQuadratic;

uniform vec3 colorOverride;
uniform bool useColorOverride;
uniform float emissiveStrength;
uniform float globalAlpha = 1.0;

uniform float time;
uniform bool isStarfield = false;
uniform bool isAsteroidPointSprite = false;
uniform int planetId = -1;

uniform sampler2D planetTexture;
uniform bool useTexture = false;

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
    vec3 baseColor = useColorOverride ? colorOverride : Color;
    vec3 norm = normalize(Normal);
    float specularVal = 0.5; // Default specular strength
    float activeEmissive = emissiveStrength;
    float pointSpriteAlpha = 1.0;

    if (isAsteroidPointSprite) {
        vec2 coord = gl_PointCoord - vec2(0.5);
        float dist = dot(coord, coord);
        if (dist > 0.25) discard;
        pointSpriteAlpha = 1.0 - smoothstep(0.15, 0.25, dist);
    }

    if (planetId == 200) {
        // Space Station custom rim light and shading
        float dotVN = dot(normalize(viewPos - FragPos), norm);
        float rim = 1.0 - max(dotVN, 0.0);
        float rimLight = pow(rim, 4.0) * 0.7; // prominent edge glow
        vec3 rimColor = vec3(0.5, 0.8, 1.0);  // subtle blue/white rim color
        
        // Standard lighting components
        float diff = max(dot(norm, normalize(lightPos - FragPos)), 0.0);
        vec3 diffuse = diff * lightColor;
        
        vec3 reflectDir = reflect(-normalize(lightPos - FragPos), norm);
        float spec = pow(max(dot(normalize(viewPos - FragPos), reflectDir), 0.0), 32.0);
        vec3 specular = specularVal * spec * lightColor;
        
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);
        
        // Low ambient for space station shadow areas
        float ambientStrength = 0.05;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 result = (ambient + attenuation * (diffuse + specular)) * baseColor;
        result += rimLight * rimColor * (0.3 + 0.7 * attenuation); // Add the white/blue rim light
        
        FragColor = vec4(result, globalAlpha);
        return;
    }

    if (planetId == 100) {
        // SunHalo: Radial fade using TexCoords.y (0.0 is inner, 1.0 is outer)
        float alpha = 1.0 - TexCoords.y;
        alpha = pow(alpha, 3.0); // cubic falloff for a soft glow
        
        vec3 glowColor = useColorOverride ? colorOverride : vec3(1.0, 0.55, 0.05);
        // Apply emissive multiplier for extra intensity near the sun
        vec3 finalGlow = glowColor * (1.0 + activeEmissive);
        
        FragColor = vec4(finalGlow, alpha * globalAlpha);
        return;
    }

    if (planetId == 14) {
        // Atmosphere: subtle blue scattering halo
        float dotVN = dot(normalize(viewPos - FragPos), norm);
        float rim = 1.0 - max(dotVN, 0.0);
        
        // Subtle blue atmosphere color
        vec3 atmosphereColor = useColorOverride ? colorOverride : vec3(0.25, 0.55, 1.0);
        
        // Wrapped lighting to scatter light into the terminator
        vec3 lightDir = normalize(lightPos - FragPos);
        float cosTheta = dot(norm, lightDir);
        float scatter = smoothstep(-0.2, 0.4, cosTheta);
        
        // Atmosphere density: high at the edges (rim), thin in the middle
        float density = pow(rim, 4.0) * 0.85 + pow(rim, 2.0) * 0.15;
        
        // The atmosphere glows on the lit side
        vec3 col = atmosphereColor * 1.3;
        float alpha = density * scatter * globalAlpha;
        
        // Distance attenuation from the Sun
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);
        
        FragColor = vec4(col * scatter * attenuation, alpha);
        return;
    }
    
    if (planetId == 13) {
        // Cloud layer: animated and lit
        vec3 cloudScroll = LocalPos + vec3(time * 0.015, 0.0, time * 0.008);
        float density = fbm(cloudScroll * 5.0);
        
        // Filter the noise to get distinct cloud patches
        float alpha = smoothstep(0.48, 0.72, density);
        
        // Base cloud color is white/light grey
        baseColor = vec3(0.95, 0.95, 0.98);
        
        // Diffuse lighting from the Sun
        vec3 lightDir = normalize(lightPos - FragPos);
        
        // Wrapped lighting to scatter light into the terminator for clouds too
        float wrapDiff = max(dot(norm, lightDir) * 0.6 + 0.4, 0.0);
        vec3 diffuse = wrapDiff * lightColor;
        
        // Ambient to avoid pitch black cloud on the dark side (but very low)
        float ambientStrength = 0.04;
        vec3 ambient = ambientStrength * lightColor;
        
        // Distance attenuation from the Sun
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);
        
        vec3 litCloudColor = (ambient + attenuation * diffuse) * baseColor;
        
        FragColor = vec4(litCloudColor, alpha * 0.85 * globalAlpha);
        return;
    }

    if (useTexture) {
        baseColor = texture(planetTexture, TexCoords).rgb;
        if (planetId == 0) {
            activeEmissive = 0.6;
        } else if (planetId == 3) {
            // Earth: shine on oceans, matte on land
            if (baseColor.b > baseColor.r * 1.2 && baseColor.b > baseColor.g * 1.1) {
                specularVal = 1.5; // Stronger specular reflection for Earth oceans
            } else {
                specularVal = 0.05;
            }
        } else if (planetId == 9) {
            specularVal = 0.05;
            norm = perturbNormal(LocalPos, norm, 22.0, 0.06);
        }
    } else {
        // Apply procedural planet surface shaders
        if (planetId == 0) {
            // SUN: Dynamic convective boiling plasma
            vec3 p = LocalPos * 3.5;
            float n1 = fbm(p + vec3(0.0, 0.0, time * 0.25));
            float n2 = fbm(p * 2.0 - vec3(0.0, time * 0.4, 0.0));
            float plasma = (n1 + n2) * 0.5;
            
            vec3 color1 = vec3(1.0, 0.92, 0.45); // hot white-yellow
            vec3 color2 = vec3(1.0, 0.45, 0.02); // orange
            vec3 color3 = vec3(0.65, 0.08, 0.0);  // cool red-brown spots
            
            baseColor = mix(color3, color2, plasma);
            baseColor = mix(baseColor, color1, pow(plasma, 2.5));
            
            // Dynamic flare highlights
            float flare = fbm(LocalPos * 15.0 + time * 0.6);
            baseColor *= (0.8 + 0.35 * flare);
            activeEmissive = 0.8 + 0.3 * flare;
        } 
        else if (planetId == 1) {
            // MERCURY: Cratered rocky grey planet
            float n = fbm(LocalPos * 10.0) * 0.5 + 0.5;
            baseColor = mix(vec3(0.3, 0.28, 0.27), vec3(0.6, 0.58, 0.55), n);
            
            float craterVal = fract(fbm(LocalPos * 25.0) * 3.0);
            float craterMask = smoothstep(0.85, 0.95, craterVal);
            baseColor = mix(baseColor, vec3(0.72, 0.72, 0.7), craterMask * 0.2);
            
            specularVal = 0.08;
            norm = perturbNormal(LocalPos, norm, 18.0, 0.04);
        } 
        else if (planetId == 2) {
            // VENUS: Swirling yellowish acid clouds
            float band = sin(LocalPos.y * 12.0 + fbm(LocalPos * 4.0 + vec3(time * 0.06, 0.0, 0.0)) * 6.0);
            float n = fbm(LocalPos * 6.0 + vec3(time * 0.03)) * 0.5 + 0.5;
            baseColor = mix(vec3(0.8, 0.65, 0.4), vec3(0.95, 0.88, 0.68), (band * 0.5 + 0.5) * n);
            specularVal = 0.05;
        } 
        else if (planetId == 3) {
            // EARTH: Oceans, continents, glaciers, specularity, moving clouds
            float continent = fbm(LocalPos * 5.0 + vec3(0.12));
            float poleMask = abs(LocalPos.y);
            
            if (poleMask > 0.83 + 0.08 * fbm(LocalPos * 10.0)) {
                baseColor = vec3(0.96, 0.96, 0.98);
                specularVal = 0.4;
            } else if (continent > 0.46) {
                float landNoise = fbm(LocalPos * 16.0);
                baseColor = mix(vec3(0.12, 0.35, 0.1), vec3(0.38, 0.28, 0.16), landNoise);
                if (landNoise > 0.65) {
                    baseColor = mix(baseColor, vec3(0.9, 0.9, 0.9), (landNoise - 0.65) * 2.8);
                }
                specularVal = 0.05;
                norm = perturbNormal(LocalPos, norm, 14.0, 0.035);
            } else {
                float depth = continent / 0.46;
                baseColor = mix(vec3(0.01, 0.06, 0.22), vec3(0.04, 0.22, 0.42), depth);
                specularVal = 1.8; // Stronger specular reflection for Earth oceans (was 0.85)
            }
            
            // Scroll white cloud layer
            float cloudNoise = fbm(LocalPos * 5.5 + vec3(time * 0.08, 0.0, time * 0.04));
            float cloudMask = smoothstep(0.52, 0.78, cloudNoise);
            baseColor = mix(baseColor, vec3(0.95, 0.95, 0.95), cloudMask * 0.72);
            specularVal = mix(specularVal, 0.01, cloudMask * 0.85);
        } 
        else if (planetId == 4) {
            // MARS: Red oxide planet with volcanoes and polar ice
            float n = fbm(LocalPos * 7.0);
            baseColor = mix(vec3(0.72, 0.28, 0.08), vec3(0.45, 0.15, 0.05), n);
            
            float darkMask = fbm(LocalPos * 3.5 + vec3(4.0));
            if (darkMask > 0.56) {
                baseColor = mix(baseColor, vec3(0.28, 0.16, 0.1), (darkMask - 0.56) * 1.6);
            }
            
            float poleMask = abs(LocalPos.y);
            if (poleMask > 0.86 + 0.06 * fbm(LocalPos * 9.0)) {
                baseColor = vec3(0.95, 0.94, 0.97);
            }
            
            specularVal = 0.1;
            norm = perturbNormal(LocalPos, norm, 16.0, 0.045);
        } 
        else if (planetId == 5) {
            // JUPITER: Gas bands and the Great Red Spot
            float yCoord = LocalPos.y;
            float bandPert = fbm(LocalPos * 3.5 + vec3(time * 0.06, 0.0, 0.0));
            float bandVal = sin(yCoord * 22.0 + bandPert * 4.2);
            
            baseColor = mix(vec3(0.85, 0.72, 0.58), vec3(0.55, 0.36, 0.22), bandVal * 0.5 + 0.5);
            
            float fineBands = sin(yCoord * 60.0 + bandPert * 1.8);
            baseColor = mix(baseColor, vec3(0.4, 0.24, 0.14), max(0.0, fineBands) * 0.3);
            
            // Spot located at y ~ -0.36, x ~ 0.78, z ~ 0.52
            vec3 spotCenter = normalize(vec3(0.78, -0.36, 0.52));
            vec3 diff = LocalPos - spotCenter;
            float distEllipse = sqrt(diff.y * diff.y * 4.0 + (diff.x * diff.x + diff.z * diff.z) * 0.85);
            if (distEllipse < 0.16) {
                float spotFactor = smoothstep(0.16, 0.04, distEllipse);
                float storm = fbm(LocalPos * 24.0);
                vec3 spotColor = mix(vec3(0.62, 0.12, 0.04), vec3(0.85, 0.28, 0.08), storm);
                baseColor = mix(baseColor, spotColor, spotFactor);
            }
            
            specularVal = 0.1;
        } 
        else if (planetId == 6) {
            // SATURN: Golden gas bands
            float yCoord = LocalPos.y;
            float bandPert = fbm(LocalPos * 3.0 + vec3(time * 0.04, 0.0, 0.0));
            float bandVal = sin(yCoord * 16.0 + bandPert * 2.0);
            
            baseColor = mix(vec3(0.92, 0.85, 0.65), vec3(0.76, 0.68, 0.48), bandVal * 0.5 + 0.5);
            specularVal = 0.1;
        } 
        else if (planetId == 7) {
            // URANUS: Smooth cyan
            float bandVal = sin(LocalPos.y * 8.0);
            baseColor = mix(vec3(0.55, 0.82, 0.86), vec3(0.48, 0.74, 0.8), bandVal * 0.5 + 0.5);
            specularVal = 0.15;
        } 
        else if (planetId == 8) {
            // NEPTUNE: Blue gas with methane clouds
            float bandPert = fbm(LocalPos * 4.0);
            float bandVal = sin(LocalPos.y * 14.0 + bandPert * 2.2);
            baseColor = mix(vec3(0.15, 0.32, 0.82), vec3(0.08, 0.18, 0.52), bandVal * 0.5 + 0.5);
            
            float cloudNoise = fbm(LocalPos * 7.5 + vec3(time * 0.06, time * 0.03, 0.0));
            float cloudMask = smoothstep(0.64, 0.82, cloudNoise);
            baseColor = mix(baseColor, vec3(0.78, 0.86, 1.0), cloudMask * 0.55);
            specularVal = 0.12;
        } 
        else if (planetId == 9) {
            // MOON: Grey cratered moon
            float n = fbm(LocalPos * 14.0) * 0.5 + 0.5;
            baseColor = mix(vec3(0.3, 0.3, 0.3), vec3(0.65, 0.65, 0.62), n);
            
            float craterVal = fract(fbm(LocalPos * 18.0) * 4.0);
            float craterMask = smoothstep(0.88, 0.98, craterVal);
            baseColor = mix(baseColor, vec3(0.8, 0.8, 0.78), craterMask * 0.3);
            
            specularVal = 0.05;
            norm = perturbNormal(LocalPos, norm, 20.0, 0.05);
        } 
        else if (planetId == 10) {
            // PHOBOS: Lumpy asteroid-like moon
            float n = fbm(LocalPos * 18.0);
            baseColor = mix(vec3(0.38, 0.35, 0.32), vec3(0.52, 0.48, 0.44), n);
            specularVal = 0.02;
            norm = perturbNormal(LocalPos, norm, 24.0, 0.085);
        } 
        else if (planetId == 11) {
            // IO: Sulfur yellow with active volcanoes
            float sulfur = fbm(LocalPos * 8.5);
            baseColor = mix(vec3(0.86, 0.82, 0.18), vec3(0.92, 0.62, 0.08), sulfur);
            
            float spot = fbm(LocalPos * 16.0 + vec3(3.0));
            if (spot > 0.6) {
                baseColor = mix(baseColor, vec3(0.24, 0.08, 0.02), (spot - 0.6) * 2.2);
            }
            specularVal = 0.08;
            norm = perturbNormal(LocalPos, norm, 12.0, 0.025);
        } 
        else if (planetId == 12) {
            // EUROPA: Ice cracked veins
            float ice = fbm(LocalPos * 6.5);
            baseColor = mix(vec3(0.88, 0.9, 0.96), vec3(0.72, 0.75, 0.84), ice);
            
            float cracks = 1.0 - abs(noise(LocalPos * 14.0) - 0.5) * 2.0;
            cracks = pow(cracks, 8.5);
            baseColor = mix(baseColor, vec3(0.42, 0.28, 0.16), cracks * 0.58);
            
            specularVal = 0.35;
            norm = perturbNormal(LocalPos, norm, 15.0, 0.012);
        }
    }

    if (!useLighting) {
        // Output base color directly for unlit/glowing objects
        if (isStarfield) {
            float dist = length(gl_PointCoord - vec2(0.5));
            if (dist > 0.5) discard;

            float isGalaxy = Normal.x;
            float maxAlpha = Normal.y;
            
            float alpha = 0.0;
            vec3 finalStarColor = Color;

            if (isGalaxy > 0.5) {
                // Galaxy band/nebula dust: soft radial falloff
                float falloff = 1.0 - (dist / 0.5);
                alpha = pow(falloff, 2.5) * maxAlpha;
                
                // Add a very subtle slow nebula shimmer
                float shimmer = 0.9 + 0.1 * sin(time * 0.5 + TexCoords.x * 6.28);
                finalStarColor *= shimmer;
            } else {
                // Sharp star: very steep falloff to avoid cartoon dots, with a tiny glow core
                float falloff = smoothstep(0.5, 0.0, dist);
                alpha = pow(falloff, 4.0) * maxAlpha;
                
                // Star twinkle
                float twinklePhase = TexCoords.x * 6.28318;
                float twinkle = 0.4 + 0.6 * sin(time * (1.5 + TexCoords.x * 3.0) + twinklePhase);
                finalStarColor *= twinkle;
            }
            
            FragColor = vec4(finalStarColor, alpha * globalAlpha);
        } else if (activeEmissive > 0.0) {
            FragColor = vec4(baseColor * (1.0 + activeEmissive), globalAlpha * pointSpriteAlpha);
        } else {
            FragColor = vec4(baseColor, globalAlpha * pointSpriteAlpha);
        }
        return;
    }

    // Ambient component (lowered to 0.04 for realistic shadow visibility)
    float ambientStrength = 0.04;
    vec3 ambient = ambientStrength * lightColor;

    // Distance attenuation from the Sun
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

    // Diffuse component
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular component: Phong lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    
    // Higher shininess (96.0) for Earth's ocean, 32.0 default for planets
    float shininess = (planetId == 3) ? 96.0 : 32.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularVal * spec * lightColor;

    // Rim / atmospheric scatter effect
    float rimStrength = 0.0;
    if (useLighting) {
        float rim = 1.0 - max(dot(normalize(viewPos - FragPos), norm), 0.0);
        rim = pow(rim, 3.0);
        rimStrength = rim * 0.4;
    }

    if (useTexture) {
        vec3 texColor = texture(planetTexture, TexCoords).rgb;
        vec3 textureAmbient = 0.03 * lightColor;
        vec3 textureDiffuse = diff * lightColor;
        FragColor = vec4((textureAmbient + textureDiffuse) * texColor, globalAlpha * pointSpriteAlpha);
        return;
    }

    // Final color output: ambient remains constant, while diffuse, specular and rim scatter are attenuated
    vec3 result = (ambient + attenuation * (diffuse + specular)) * baseColor;
    result += attenuation * rimStrength * lightColor * baseColor * 0.5;
    FragColor = vec4(result, globalAlpha * pointSpriteAlpha);
}
