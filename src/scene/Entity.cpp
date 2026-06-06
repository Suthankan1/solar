#include "scene/Entity.h"
#include "core/Renderer.h"

Entity::Entity(const std::string& name) 
    : m_name(name), m_active(true) {
}

Entity::~Entity() {
    // unique_ptr will clean up components automatically
}

void Entity::update(float deltaTime) {
    if (!m_active) return;

    // We copy the components pointer array or access directly.
    // Iterating and updating all active, enabled components.
    for (auto& comp : m_components) {
        if (comp && comp->isEnabled()) {
            comp->update(deltaTime);
        }
    }
}

void Entity::render(Renderer& renderer) {
    if (!m_active) return;

    for (auto& comp : m_components) {
        if (comp && comp->isEnabled()) {
            comp->render(renderer);
        }
    }
}
