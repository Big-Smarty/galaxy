#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace galaxy {
struct Star {
    glm::vec3 position;
    glm::vec3 tint;
    glm::float32_t weight;
};

class StarData {
public:
    StarData();

    Star operator[](uint32_t i) {
        return Star{
            .position = m_positions[i],
            .tint = m_tints[i],
            .weight = m_weights[i],
        };
    }

    void push(Star star);

    std::vector<glm::vec3>& positions() { return m_positions; }
    std::vector<glm::vec3>& tints() { return m_tints; }
    std::vector<glm::float32_t>& weights() { return m_weights; }

    uint32_t size() { return m_positions.size(); }

private:
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_tints;
    std::vector<glm::float32_t> m_weights;
};

class GPUStarData {
public:
    GPUStarData() = delete;
    ~GPUStarData();

    GPUStarData(vk::raii::Device& device,
                vk::raii::PhysicalDevice& physical_device,
                vk::raii::CommandBuffer& command_buffer, vk::raii::Queue& queue,
                StarData star_data);

    vk::raii::DescriptorSetLayout& descriptor_set_layout() {
        return m_set_layout;
    }

    vk::raii::DescriptorSets& descriptor_sets() { return m_descriptor_sets; }

    std::vector<vk::raii::Buffer>& positions() { return m_positions; }
    vk::raii::Buffer& tints() { return m_tints; }
    vk::raii::Buffer& weights() { return m_weights; }
    vk::raii::Buffer& coords() { return m_screen_pos; }
    vk::raii::Buffer& velocities() { return m_velocities; }

private:
    std::vector<vk::raii::DeviceMemory> m_positions_memories;
    std::vector<vk::raii::Buffer> m_positions;

    vk::raii::DeviceMemory m_weights_memory{nullptr};
    vk::raii::Buffer m_weights{nullptr};

    vk::raii::DeviceMemory m_tints_memory{nullptr};
    vk::raii::Buffer m_tints{nullptr};

    vk::raii::DeviceMemory m_screen_pos_memory{nullptr};
    vk::raii::Buffer m_screen_pos{nullptr};

    vk::raii::DeviceMemory m_velocities_memory{nullptr};
    vk::raii::Buffer m_velocities{nullptr};

    vk::raii::DescriptorPool m_descriptor_pool{nullptr};
    vk::raii::DescriptorSetLayout m_set_layout{nullptr};
    vk::raii::DescriptorSets m_descriptor_sets{nullptr};

    uint32_t m_star_count = 0;
};
}  // namespace galaxy
