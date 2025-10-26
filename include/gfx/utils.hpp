#pragma once

#include <cstdint>
#include <tuple>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace gfx {
namespace util {

const uint32_t TIMEOUT = 100000000;

uint32_t find_graphics_queue_family_index(
    std::vector<vk::QueueFamilyProperties> const& queue_family_properties);
std::tuple<uint32_t, uint32_t> find_graphics_and_present_queue_family_index(
    vk::raii::PhysicalDevice const& physical_device,
    vk::raii::SurfaceKHR const& surface);
uint32_t clamp_surface_image_count(const uint32_t desired_image_count,
                                   const uint32_t min_image_count,
                                   const uint32_t max_image_count);
uint32_t find_memory_type(
    vk::PhysicalDeviceMemoryProperties const& memory_properties,
    uint32_t type_bits, vk::MemoryPropertyFlags requirements_mask);
vk::raii::DescriptorSetLayout make_descriptor_set_layout(
    vk::raii::Device const& device,
    std::vector<std::tuple<vk::DescriptorType, uint32_t,
                           vk::ShaderStageFlags>> const& binding_data,
    vk::DescriptorSetLayoutCreateFlags flags = {});
std::vector<unsigned int> load_spv(std::string path);
void set_image_layout(vk::raii::CommandBuffer const& commandBuffer,
                      vk::Image image, vk::Format format,
                      vk::ImageLayout oldImageLayout,
                      vk::ImageLayout newImageLayout);
void copy_buffers_to_device_local(
    vk::raii::Device& device, vk::raii::CommandBuffer& command_buffer,
    vk::raii::Queue queue, std::vector<vk::raii::Buffer> const& staging_buffers,
    std::vector<vk::raii::Buffer*> const& device_buffers,
    std::vector<vk::DeviceSize> sizes);
}  // namespace util
}  // namespace gfx
