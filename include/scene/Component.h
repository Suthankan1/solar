#pragma once

#include <cstddef>
#include <type_traits>

class Entity;
class Renderer;

using ComponentTypeID = std::size_t;

class Component {
public:
    Component() : m_owner(nullptr), m_enabled(true) {}
    virtual ~Component() = default;

    // Called when the component is attached to an entity
    virtual void initialize() {}
    
    // Called once per frame to update state
    virtual void update(float deltaTime) { (void)deltaTime; }
    
    // Called during the render pass
    virtual void render(Renderer& renderer) { (void)renderer; }

    // Owner management
    Entity* getOwner() const { return m_owner; }
    void setOwner(Entity* owner) { m_owner = owner; }

    // Enabled status management
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

protected:
    Entity* m_owner;
    bool m_enabled;
};

namespace detail {
    inline ComponentTypeID getUniqueComponentID() noexcept {
        static ComponentTypeID lastID = 0;
        return lastID++;
    }
}

template <typename T>
inline ComponentTypeID getComponentTypeID() noexcept {
    static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
    static ComponentTypeID typeID = detail::getUniqueComponentID();
    return typeID;
}
