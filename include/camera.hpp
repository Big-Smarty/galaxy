#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "push_constants.hpp"

namespace galaxy {
class Camera {
public:
    Camera();

    Camera(glm::vec3 position, glm::vec3 direction, glm::float32_t fov);

    void update();

    PushConstants push_constants();

    glm::mat4 view_matrix();
    glm::mat4 rotation_matrix();

private:
    glm::vec3 m_position = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 m_direction = glm::vec3(0.0, 0.0, -1.0);
    glm::mat4 m_projection =
        glm::perspective(glm::radians(90.0), (640.0 / 480.0), 0.1, 10000000000000000000.0);
    float m_fov = 90.0;
    float m_pitch = 0.0;
    float m_yaw = 0.0;
};
}  // namespace galaxy
