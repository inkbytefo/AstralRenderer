#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "astral/renderer/camera.hpp"
#include <algorithm>
#include <GLFW/glfw3.h>

namespace astral {

Camera::Camera() {
    updateVectors();
    m_view = glm::lookAt(m_position, m_position + m_front, m_up);
    setPerspective(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
}

void Camera::update(float dt) {
    float velocity = m_speed * dt;
    bool moved = false;
    if (m_keys[GLFW_KEY_W]) { m_position += m_front * velocity; moved = true; }
    if (m_keys[GLFW_KEY_S]) { m_position -= m_front * velocity; moved = true; }
    if (m_keys[GLFW_KEY_A]) { m_position -= m_right * velocity; moved = true; }
    if (m_keys[GLFW_KEY_D]) { m_position += m_right * velocity; moved = true; }
    if (m_keys[GLFW_KEY_Q]) { m_position -= m_up * velocity; moved = true; }
    if (m_keys[GLFW_KEY_E]) { m_position += m_up * velocity; moved = true; }

    m_view = glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::setPerspective(float fov, float aspect, float znear, float zfar) {
    m_near = znear;
    m_far = zfar;
    m_proj = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    m_proj[1][1] *= -1.0f; // Vulkan Y-flip
}

void Camera::processKeyboard(int key, bool pressed) {
    if (key >= 0 && key < 1024) {
        m_keys[key] = pressed;
    }
}

void Camera::processMouse(float xoffset, float yoffset) {
    xoffset *= m_sensitivity;
    yoffset *= m_sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateVectors();
    m_view = glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace astral
