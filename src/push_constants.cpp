#include "push_constants.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace galaxy {
vk::PushConstantRange PushConstants::push_constant_range() {
    return vk::PushConstantRange(
        vk::ShaderStageFlagBits::eCompute, 0,
        sizeof(glm::mat4x4) + sizeof(glm::ivec2) + sizeof(uint32_t));
}
}  // namespace galaxy
