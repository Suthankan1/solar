#include "lighting/LightingComponent.h"
#include <algorithm>
#include <cmath>

// Define static registry
std::vector<LightingComponent*> LightingComponent::s_activeLights;

LightingComponent::LightingComponent()
    : m_type(LightType::Point),
      m_color(1.0f),
      m_intensity(1.0f),
      m_constant(1.0f),
      m_linear(0.09f),
      m_quadratic(0.032f),
      m_innerCutoffCos(std::cos(glm::radians(12.5f))),
      m_outerCutoffCos(std::cos(glm::radians(17.5f))),
      m_castShadows(false),
      m_shadowMapWidth(1024),
      m_shadowMapHeight(1024) {
}

LightingComponent::~LightingComponent() {
    // Unregister this light from the active list
    s_activeLights.erase(
        std::remove(s_activeLights.begin(), s_activeLights.end(), this),
        s_activeLights.end()
    );
}

void LightingComponent::initialize() {
    // Register this light as active
    if (std::find(s_activeLights.begin(), s_activeLights.end(), this) == s_activeLights.end()) {
        s_activeLights.push_back(this);
    }
}

void LightingComponent::setAttenuation(float constant, float linear, float quadratic) {
    m_constant = constant;
    m_linear = linear;
    m_quadratic = quadratic;
}

void LightingComponent::setSpotCutoff(float innerAngleDegrees, float outerAngleDegrees) {
    m_innerCutoffCos = std::cos(glm::radians(innerAngleDegrees));
    m_outerCutoffCos = std::cos(glm::radians(outerAngleDegrees));
}

void LightingComponent::setShadowResolution(unsigned int width, unsigned int height) {
    m_shadowMapWidth = width;
    m_shadowMapHeight = height;
}
