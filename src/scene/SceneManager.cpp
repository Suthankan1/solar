#include "scene/SceneManager.h"
#include "scene/SceneObject.h"
#include "camera/Camera.h"
#include "core/Renderer.h"
#include <glad/glad.h>
#include <algorithm>
#include <iostream>

SceneManager::SceneManager() : m_activeCamera(nullptr) {}

void SceneManager::registerObject(std::shared_ptr<SceneObject> object) {
    if (!object) return;
    
    // De-duplicate registry based on name
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&object](const std::shared_ptr<SceneObject>& obj) {
            return obj->getName() == object->getName();
        });

    if (it != m_objects.end()) {
        std::cout << "Info: Replacing scene object with name: " << object->getName() << std::endl;
        *it = object;
    } else {
        m_objects.push_back(object);
    }
}

void SceneManager::unregisterObject(const std::string& name) {
    auto it = std::remove_if(m_objects.begin(), m_objects.end(),
        [&name](const std::shared_ptr<SceneObject>& obj) {
            return obj->getName() == name;
        });

    if (it != m_objects.end()) {
        m_objects.erase(it, m_objects.end());
    }
}

std::shared_ptr<SceneObject> SceneManager::getObject(const std::string& name) const {
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const std::shared_ptr<SceneObject>& obj) {
            return obj->getName() == name;
        });

    return (it != m_objects.end()) ? *it : nullptr;
}

void SceneManager::setActiveCamera(std::shared_ptr<Camera> camera) {
    m_activeCamera = camera;
    if (camera) {
        registerObject(camera);
    }
}

void SceneManager::update(float deltaTime) {
    for (auto& object : m_objects) {
        if (object && object->isActive()) {
            object->update(deltaTime);
        }
    }
}

void SceneManager::render(Renderer& renderer) {
    if (m_activeCamera) {
        // Query active viewport dimensions from OpenGL directly
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        
        float aspect = 1.777f;
        if (viewport[3] > 0) {
            aspect = static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
        }

        renderer.setViewMatrix(m_activeCamera->getViewMatrix());
        renderer.setProjectionMatrix(m_activeCamera->getProjectionMatrix(aspect));
        renderer.setCameraPosition(m_activeCamera->getPosition());
    }

    for (auto& object : m_objects) {
        if (object && object->isActive()) {
            object->render(renderer);
        }
    }
}

void SceneManager::clear() {
    m_objects.clear();
    m_activeCamera = nullptr;
}
