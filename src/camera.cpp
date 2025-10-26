#include "camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace galaxy {
Camera::Camera() {}

glm::mat4 Camera::view_matrix() {
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    //  so we will create the camera model matrix and invert
    glm::vec3 camera_target = glm::vec3(0.0, 0.0, 1.0);
    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0);
    return glm::lookAt(m_position, camera_target, up);
}

glm::mat4 Camera::rotation_matrix() {
    // fairly typical FPS style camera. we join the pitch and yaw rotations into
    // the final rotation matrix

    glm::quat pitch_rotation =
        glm::angleAxis(m_pitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yaw_rotation = glm::angleAxis(m_yaw, glm::vec3{0.f, -1.f, 0.f});

    return glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);
}

PushConstants Camera::push_constants() {
    // Use the corrected projection
    return PushConstants{m_projection * view_matrix(),
                         glm::ivec2(640, 480)};
}

}  // namespace galaxy