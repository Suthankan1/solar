#pragma once

#include <vector>
#include <memory>
#include <string>

class SceneObject;
class Camera;
class Renderer;

class SceneManager {
public:
    SceneManager();
    ~SceneManager() = default;

    // Prevent copying
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    void registerObject(std::shared_ptr<SceneObject> object);
    void unregisterObject(const std::string& name);
    std::shared_ptr<SceneObject> getObject(const std::string& name) const;

    void setActiveCamera(std::shared_ptr<Camera> camera);
    std::shared_ptr<Camera> getActiveCamera() const { return m_activeCamera; }

    void update(float deltaTime);
    void render(Renderer& renderer);

    void clear();

private:
    std::vector<std::shared_ptr<SceneObject>> m_objects;
    std::shared_ptr<Camera> m_activeCamera;
};
