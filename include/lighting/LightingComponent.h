#pragma once

#include "scene/Component.h"
#include <glm/glm.hpp>
#include <vector>

enum class LightType {
    Directional,
    Point,
    Spot
};

class LightingComponent : public Component {
public:
    LightingComponent();
    virtual ~LightingComponent();

    // Overridden from Component
    void initialize() override;
    
    // Light settings
    void setLightType(LightType type) { m_type = type; }
    LightType getLightType() const { return m_type; }

    void setColor(const glm::vec3& color) { m_color = color; }
    const glm::vec3& getColor() const { return m_color; }

    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }

    // Attenuation settings (Point & Spot lights)
    void setAttenuation(float constant, float linear, float quadratic);
    float getConstantAttenuation() const { return m_constant; }
    float getLinearAttenuation() const { return m_linear; }
    float getQuadraticAttenuation() const { return m_quadratic; }

    // Spot light specific settings
    void setSpotCutoff(float innerAngleDegrees, float outerAngleDegrees);
    float getInnerCutoffCos() const { return m_innerCutoffCos; }
    float getOuterCutoffCos() const { return m_outerCutoffCos; }

    // Future extension: Shadow settings
    void setCastShadows(bool cast) { m_castShadows = cast; }
    bool getCastShadows() const { return m_castShadows; }
    void setShadowResolution(unsigned int width, unsigned int height);
    
    // Global registry helpers (to fetch all active lights in the scene)
    static const std::vector<LightingComponent*>& getActiveLights() { return s_activeLights; }

private:
    LightType m_type;
    glm::vec3 m_color;
    float m_intensity;

    // Attenuation coefficients
    float m_constant;
    float m_linear;
    float m_quadratic;

    // Spot limits (cosine values)
    float m_innerCutoffCos;
    float m_outerCutoffCos;

    // Shadows
    bool m_castShadows;
    unsigned int m_shadowMapWidth;
    unsigned int m_shadowMapHeight;

    // Global list of active lights
    static std::vector<LightingComponent*> s_activeLights;
};
