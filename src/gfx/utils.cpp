#include "gfx/utils.hpp"

#include <algorithm>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>

namespace gfx {
namespace util {

uint32_t find_graphics_queue_family_index(
    std::vector<vk::QueueFamilyProperties> const& queue_family_properties) {
    // get the first index into queueFamiliyProperties which supports graphics
    std::vector<vk::QueueFamilyProperties>::const_iterator
        graphicsQueueFamilyProperty = std::find_if(
            queue_family_properties.begin(), queue_family_properties.end(),
            [](vk::QueueFamilyProperties const& qfp) {
                return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
            });
    assert(graphicsQueueFamilyProperty != queue_family_properties.end());
    return static_cast<uint32_t>(std::distance(queue_family_properties.begin(),
                                               graphicsQueueFamilyProperty));
}

std::tuple<uint32_t, uint32_t> find_graphics_and_present_queue_family_index(
    vk::raii::PhysicalDevice const& physical_device,
    vk::raii::SurfaceKHR const& surface) {
    std::vector<vk::QueueFamilyProperties> queue_family_properties =
        physical_device.getQueueFamilyProperties();
    assert(queue_family_properties.size() <
           (std::numeric_limits<uint32_t>::max)());

    uint32_t graphicsQueueFamilyIndex =
        find_graphics_queue_family_index(queue_family_properties);
    if (physical_device.getSurfaceSupportKHR(graphicsQueueFamilyIndex,
                                             surface)) {
        return std::make_pair(
            graphicsQueueFamilyIndex,
            graphicsQueueFamilyIndex);  // the first graphicsQueueFamilyIndex
                                        // does also support presents
    }

    // the graphicsQueueFamilyIndex doesn't support present -> look for an other
    // family index that supports both graphics and present
    for (size_t i = 0; i < queue_family_properties.size(); i++) {
        if ((queue_family_properties[i].queueFlags &
             vk::QueueFlagBits::eGraphics) &&
            physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                 surface)) {
            return std::make_pair(static_cast<uint32_t>(i),
                                  static_cast<uint32_t>(i));
        }
    }

    // there's nothing like a single family index that supports both graphics
    // and present -> look for an other family index that supports present
    for (size_t i = 0; i < queue_family_properties.size(); i++) {
        if (physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                 surface)) {
            return std::make_pair(graphicsQueueFamilyIndex,
                                  static_cast<uint32_t>(i));
        }
    }

    throw std::runtime_error(
        "Could not find queues for both graphics or present -> terminating");
}
uint32_t clamp_surface_image_count(const uint32_t desired_image_count,
                                   const uint32_t min_image_count,
                                   const uint32_t max_image_count) {
    uint32_t image_count = (std::max)(desired_image_count, min_image_count);
    // Some drivers report maxImageCount as 0, so only clamp to max if it is
    // valid.
    if (max_image_count > 0) {
        image_count = (std::min)(image_count, max_image_count);
    }
    return image_count;
}

uint32_t find_memory_type(
    vk::PhysicalDeviceMemoryProperties const& memory_properties,
    uint32_t type_bits, vk::MemoryPropertyFlags requirements_mask) {
    uint32_t type_index = uint32_t(~0);
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_bits & 1) &&
            ((memory_properties.memoryTypes[i].propertyFlags &
              requirements_mask) == requirements_mask)) {
            type_index = i;
            break;
        }
        type_bits >>= 1;
    }
    assert(type_index != uint32_t(~0));
    return type_index;
}
vk::raii::DescriptorSetLayout make_descriptor_set_layout(
    vk::raii::Device const& device,
    std::vector<std::tuple<vk::DescriptorType, uint32_t,
                           vk::ShaderStageFlags>> const& binding_data,
    vk::DescriptorSetLayoutCreateFlags flags) {
    std::vector<vk::DescriptorSetLayoutBinding> bindings(binding_data.size());

    for (size_t i = 0; i < binding_data.size(); i++) {
        bindings[i] = vk::DescriptorSetLayoutBinding(
            static_cast<uint32_t>(i), std::get<0>(binding_data[i]),
            std::get<1>(binding_data[i]), std::get<2>(binding_data[i]));
    }
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(flags,
                                                                    bindings);
    return vk::raii::DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);
}
std::vector<uint32_t> load_spv(std::string path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }

    size_t fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    return buffer;
}
void set_image_layout(vk::raii::CommandBuffer const& commandBuffer,
                      vk::Image image, vk::Format format,
                      vk::ImageLayout oldImageLayout,
                      vk::ImageLayout newImageLayout) {
    vk::AccessFlags sourceAccessMask;
    switch (oldImageLayout) {
        case vk::ImageLayout::eTransferDstOptimal:
            sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::ePreinitialized:
            sourceAccessMask = vk::AccessFlagBits::eHostWrite;
            break;
        case vk::ImageLayout::eGeneral:  // sourceAccessMask is empty
        case vk::ImageLayout::eUndefined:
            break;
        default:
            assert(false);
            break;
    }

    vk::PipelineStageFlags sourceStage;
    switch (oldImageLayout) {
        case vk::ImageLayout::eGeneral:
        case vk::ImageLayout::ePreinitialized:
            sourceStage = vk::PipelineStageFlagBits::eHost;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            break;
        case vk::ImageLayout::eUndefined:
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            break;
        default:
            assert(false);
            break;
    }

    vk::AccessFlags destinationAccessMask;
    switch (newImageLayout) {
        case vk::ImageLayout::eColorAttachmentOptimal:
            destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            destinationAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eGeneral:  // empty destinationAccessMask
        case vk::ImageLayout::ePresentSrcKHR:
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            destinationAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            destinationAccessMask = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
            break;
        default:
            assert(false);
            break;
    }

    vk::PipelineStageFlags destinationStage;
    switch (newImageLayout) {
        case vk::ImageLayout::eColorAttachmentOptimal:
            destinationStage =
                vk::PipelineStageFlagBits::eColorAttachmentOutput;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            break;
        case vk::ImageLayout::eGeneral:
            destinationStage = vk::PipelineStageFlagBits::eHost;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
        case vk::ImageLayout::eTransferSrcOptimal:
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
            break;
        default:
            assert(false);
            break;
    }

    vk::ImageAspectFlags aspectMask;
    if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (format == vk::Format::eD32SfloatS8Uint ||
            format == vk::Format::eD24UnormS8Uint) {
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, 1, 0, 1);
    vk::ImageMemoryBarrier imageMemoryBarrier(
        sourceAccessMask, destinationAccessMask, oldImageLayout, newImageLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image,
        imageSubresourceRange);
    return commandBuffer.pipelineBarrier(sourceStage, destinationStage, {},
                                         nullptr, nullptr, imageMemoryBarrier);
}
void copy_buffers_to_device_local(
    vk::raii::Device& device, vk::raii::CommandBuffer& command_buffer,
    vk::raii::Queue queue, std::vector<vk::raii::Buffer> const& staging_buffers,
    std::vector<vk::raii::Buffer*> const& device_buffers,
    std::vector<vk::DeviceSize> sizes) {
    if (staging_buffers.size() == 0 | device_buffers.size() == 0) {
        printf("error: Buffers cannot be of size zero!");
        exit(-1);
    }
    if (staging_buffers.size() != device_buffers.size()) {
        printf("Error: Buffers arent same sized! (staging: %zu, device: %zu)\n",
               staging_buffers.size(), device_buffers.size());
        exit(-1);
    }

    command_buffer.begin(vk::CommandBufferBeginInfo());
    for (auto i = 0; i < staging_buffers.size(); i++) {
        command_buffer.copyBuffer(*staging_buffers[i], *device_buffers[i],
                                  {vk::BufferCopy(0, 0, sizes[i])});
    }
    command_buffer.end();

    vk::raii::Fence transfer_fence(device, vk::FenceCreateInfo());

    vk::SubmitInfo submit_info({}, {}, *command_buffer);

    queue.submit({submit_info}, *transfer_fence);

    if (device.waitForFences({transfer_fence}, vk::True, util::TIMEOUT) !=
        vk::Result::eSuccess) {
        printf("Failed to wait for fence during transfer stage!\n");
    }
}
}  // namespace util
}  // namespace gfx
