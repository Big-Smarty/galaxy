#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan_raii.hpp>

namespace galaxy {
struct PushConstants {
  vk::PushConstantRange push_constant_range();

  glm::mat4 view_projection_matrix;
  glm::ivec2 screen_dimensions;
  uint32_t positions_index;
};
}  // namespace galaxy
