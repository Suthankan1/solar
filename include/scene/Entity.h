#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "scene/Component.h"

class Renderer;

class Entity {
public:
    Entity(const std::string& name);
    ~Entity();

    // Lifecycle methods
    void update(float deltaTime);
    void render(Renderer& renderer);

    // Getters and Setters
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    // Component Management Templates
    template <typename T, typename... Args>
    T* addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        
        // Optionally prevent duplicate components of the same type
        if (hasComponent<T>()) {
            return getComponent<T>();
        }

        T* component = new T(std::forward<Args>(args)...);
        component->setOwner(this);
        m_components.emplace_back(component);
        component->initialize();
        return component;
    }

    template <typename T>
    T* getComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        for (const auto& comp : m_components) {
            T* casted = dynamic_cast<T*>(comp.get());
            if (casted) {
                return casted;
            }
        }
        return nullptr;
    }

    template <typename T>
    bool hasComponent() const {
        return getComponent<T>() != nullptr;
    }

    template <typename T>
    void removeComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        m_components.erase(
            std::remove_if(m_components.begin(), m_components.end(),
                [](const std::unique_ptr<Component>& comp) {
                    return dynamic_cast<T*>(comp.get()) != nullptr;
                }),
            m_components.end()
        );
    }

private:
    std::string m_name;
    bool m_active;
    std::vector<std::unique_ptr<Component>> m_components;
};
