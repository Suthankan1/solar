#pragma once

#include <string>

class Renderer;

class SceneObject {
public:
    SceneObject(const std::string& name) : m_name(name), m_active(true) {}
    virtual ~SceneObject() = default;

    virtual void update(float) {}
    virtual void render(Renderer& renderer) = 0;

    const std::string& getName() const { return m_name; }
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

protected:
    std::string m_name;
    bool m_active;
};
