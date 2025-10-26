#include "gfx.hpp"

#include <glfwpp/glfwpp.h>
#include <vulkan/vulkan_core.h>

#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <system_error>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include "gfx/utils.hpp"
#include "glfwpp/event.h"
#include "glfwpp/window.h"

namespace gfx {
Core::Core()
    : m_context(std::make_shared<vk::raii::Context>(vk::raii::Context())) {
    try {
        vk::ApplicationInfo application_info("Penis", 1, "Penis2", 1,
                                             VK_API_VERSION_1_4);
        vk::InstanceCreateInfo instance_ci({}, &application_info);
        std::vector<const char*> enabled_layer_names = {
            "VK_LAYER_KHRONOS_validation"};
        std::vector<const char*> enabled_extension_names = {
            "VK_KHR_wayland_surface", "VK_KHR_surface", "VK_KHR_xcb_surface"};
        instance_ci.setEnabledLayerCount(enabled_layer_names.size())
            .setPpEnabledLayerNames(enabled_layer_names.data())
            .setEnabledExtensionCount(enabled_extension_names.size())
            .setPpEnabledExtensionNames(enabled_extension_names.data());

        m_instance = std::make_shared<vk::raii::Instance>(
            vk::raii::Instance(*this->m_context, instance_ci));

        VkSurfaceKHR _surface;

        glfw::WindowHints{.clientApi = glfw::ClientApi::None}.apply();
        m_window = glfw::Window(640, 480, "Galaxy");
        VkResult result =
            m_window.createSurface(**m_instance.get(), nullptr, &_surface);
        switch (result) {
            case VK_SUCCESS: {
                break;
            }
            default: {
                throw vk::SystemError(
                    std::error_code(),
                    std::format("VkResult: %i", static_cast<int>(result)));
            }
        }

        m_surface =
            std::make_shared<vk::raii::SurfaceKHR>(*m_instance, _surface);

        vk::raii::PhysicalDevices physical_devices(*this->m_instance);
        m_physical_device = std::make_shared<vk::raii::PhysicalDevice>(
            physical_devices.front());

        m_graphics_family_index = util::find_graphics_queue_family_index(
            m_physical_device->getQueueFamilyProperties());
        float queue_priority = 0.0;

        std::vector<vk::QueueFamilyProperties> queue_family_properties =
            m_physical_device->getQueueFamilyProperties();

        m_present_family_index = m_physical_device->getSurfaceSupportKHR(
                                     m_graphics_family_index, *m_surface)
                                     ? m_graphics_family_index
                                     : queue_family_properties.size();

        if (m_present_family_index == queue_family_properties.size()) {
            // the graphicsQueueFamilyIndex doesn't support present -> look for
            // an other family index that supports both graphics and present
            for (size_t i = 0; i < queue_family_properties.size(); i++) {
                if ((queue_family_properties[i].queueFlags &
                     vk::QueueFlagBits::eGraphics) &&
                    m_physical_device->getSurfaceSupportKHR((i), *m_surface)) {
                    m_graphics_family_index = (i);
                    m_present_family_index = m_graphics_family_index;
                    break;
                }
            }
            if (m_present_family_index == queue_family_properties.size()) {
                // there's nothing like a single family index that supports both
                // graphics and present -> look for an other family index that
                // supports present
                for (size_t i = 0; i < queue_family_properties.size(); i++) {
                    if (m_physical_device->getSurfaceSupportKHR((i),
                                                                *m_surface)) {
                        m_present_family_index = (i);
                        break;
                    }
                }
            }
        }
        if ((m_graphics_family_index == queue_family_properties.size()) ||
            (m_present_family_index == queue_family_properties.size())) {
            throw std::runtime_error(
                "Could not find a queue for graphics or present -> "
                "terminating");
        }

        std::vector<const char*> device_extension_names = {"VK_KHR_swapchain"};

        std::vector<const char*> device_feature_names = {
            "VK_KHR_synchronization2"};

        vk::PhysicalDeviceSynchronization2Features sync2feature = {true};
        sync2feature.sType =
            vk::StructureType::ePhysicalDeviceSynchronization2Features;
        vk::PhysicalDeviceFeatures2 features({}, &sync2feature);

        vk::DeviceQueueCreateInfo device_queue_ci({}, m_graphics_family_index,
                                                  1, &queue_priority);
        vk::DeviceCreateInfo device_ci({}, device_queue_ci);
        device_ci.setPpEnabledExtensionNames(device_extension_names.data())
            .setEnabledExtensionCount(device_extension_names.size());
        device_ci.setPNext(&features);

        m_device =
            std::make_shared<vk::raii::Device>(*m_physical_device, device_ci);

        // get the supported VkFormats
        std::vector<vk::SurfaceFormatKHR> formats =
            m_physical_device->getSurfaceFormatsKHR(*m_surface);
        assert(!formats.empty());
        m_format = (formats[0].format == vk::Format::eUndefined)
                       ? vk::Format::eB8G8R8A8Unorm
                       : formats[0].format;

        vk::SurfaceCapabilitiesKHR surface_capabilities =
            m_physical_device->getSurfaceCapabilitiesKHR(*m_surface);
        vk::Extent2D swapchain_extent;
        if (surface_capabilities.currentExtent.width ==
            (std::numeric_limits<uint32_t>::max)()) {
            // If the surface size is undefined, the size is set to the size of
            // the images requested.
            swapchain_extent.width = glm::clamp(
                static_cast<uint32_t>(std::get<0>(m_window.getSize())),
                surface_capabilities.minImageExtent.width,
                surface_capabilities.maxImageExtent.width);
            swapchain_extent.height = glm::clamp(
                static_cast<uint32_t>(std::get<1>(m_window.getSize())),
                surface_capabilities.minImageExtent.height,
                surface_capabilities.maxImageExtent.height);
        } else {
            // If the surface size is defined, the swap chain size must match
            swapchain_extent = surface_capabilities.currentExtent;
        }

        // The FIFO present mode is guaranteed by the spec to be supported
        vk::PresentModeKHR swapchain_present_mode = vk::PresentModeKHR::eFifo;

        vk::SurfaceTransformFlagBitsKHR pre_transform =
            (surface_capabilities.supportedTransforms &
             vk::SurfaceTransformFlagBitsKHR::eIdentity)
                ? vk::SurfaceTransformFlagBitsKHR::eIdentity
                : surface_capabilities.currentTransform;

        vk::CompositeAlphaFlagBitsKHR composite_alpha =
            (surface_capabilities.supportedCompositeAlpha &
             vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
                ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
            : (surface_capabilities.supportedCompositeAlpha &
               vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
                ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
            : (surface_capabilities.supportedCompositeAlpha &
               vk::CompositeAlphaFlagBitsKHR::eInherit)
                ? vk::CompositeAlphaFlagBitsKHR::eInherit
                : vk::CompositeAlphaFlagBitsKHR::eOpaque;

        vk::SwapchainCreateInfoKHR swapchain_create_info(
            vk::SwapchainCreateFlagsKHR(), *m_surface,
            gfx::util::clamp_surface_image_count(
                3u, surface_capabilities.minImageCount,
                surface_capabilities.maxImageCount),
            m_format, vk::ColorSpaceKHR::eSrgbNonlinear, swapchain_extent, 1,
            vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst,
            vk::SharingMode::eExclusive, {}, pre_transform, composite_alpha,
            swapchain_present_mode, true, nullptr);

        std::array<uint32_t, 2> queue_family_indices = {m_graphics_family_index,
                                                        m_present_family_index};
        if (m_graphics_family_index != m_present_family_index) {
            swapchain_create_info.imageSharingMode =
                vk::SharingMode::eConcurrent;
            swapchain_create_info.queueFamilyIndexCount =
                static_cast<uint32_t>(queue_family_indices.size());
            swapchain_create_info.pQueueFamilyIndices =
                queue_family_indices.data();
        }

        m_swapchain = std::make_shared<vk::raii::SwapchainKHR>(
            *m_device, swapchain_create_info);
        m_swapchain_images = m_swapchain->getImages();

        m_swapchain_image_views.reserve(m_swapchain_images.size());
        vk::ImageViewCreateInfo image_view_create_info(
            {}, {}, vk::ImageViewType::e2D, m_format, {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        for (auto i = 0; i < m_swapchain_images.size(); i++) {
            image_view_create_info.image = m_swapchain_images[i];
            m_swapchain_image_views.push_back(
                {*m_device, image_view_create_info});
        }

        vk::CommandPoolCreateInfo command_pool_create_info(
            {}, m_graphics_family_index);
        command_pool_create_info.setFlags(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        m_command_pool = std::make_shared<vk::raii::CommandPool>(
            *m_device, command_pool_create_info);

        vk::CommandBufferAllocateInfo command_buffer_allocate_info(
            *m_command_pool, vk::CommandBufferLevel::ePrimary, 1);

        m_command_buffers = std::make_shared<vk::raii::CommandBuffers>(
            *m_device, command_buffer_allocate_info);

        m_graphics_queue = std::make_shared<vk::raii::Queue>(
            *m_device, m_graphics_family_index, 0);
        m_present_queue = std::make_shared<vk::raii::Queue>(
            *m_device, m_present_family_index, 0);

        vk::BufferCreateInfo buffer_create_info(
            {}, sizeof(glm::mat4x4), vk::BufferUsageFlagBits::eUniformBuffer);
        m_uniform_buffer = vk::raii::Buffer(*m_device, buffer_create_info);
        vk::MemoryRequirements memory_requirements =
            m_uniform_buffer.getMemoryRequirements();

        uint32_t type_index = util::find_memory_type(
            m_physical_device->getMemoryProperties(),
            memory_requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size,
                                                    type_index);
        m_uniform_buffer_memory =
            vk::raii::DeviceMemory(*m_device, memory_allocate_info);

        m_uniform_buffer.bindMemory(m_uniform_buffer_memory, 0);

        std::cout << "Set everything up\n";

    } catch (vk::SystemError& err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    } catch (std::exception& err) {
        std::cout << "std::exception: " << err.what() << std::endl;
        exit(-1);
    } catch (...) {
        std::cout << "unknown error\n";
        exit(-1);
    }
}
Core::~Core() {
    if (m_swapchain.use_count() != 0) {
        m_swapchain->clear();
    }
}

void Core::update() { glfw::pollEvents(); }

bool Core::should_close() { return m_window.shouldClose(); }

template <typename T>
void Core::upload_uniform_buffer(const T& data) {
    if (sizeof(T) > sizeof(glm::mat4x4)) {
        throw std::runtime_error("UBO data size mismatch!");
    }

    // Map, copy, and unmap only.
    uint8_t* p_data =
        static_cast<uint8_t*>(m_uniform_buffer_memory.mapMemory(0, sizeof(T)));

    memcpy(p_data, &data, sizeof(T));

    m_uniform_buffer_memory.unmapMemory();
}
template void Core::upload_uniform_buffer<glm::vec3>(const glm::vec3& data);

vk::raii::ShaderModule Core::create_shader_module(std::string path) {
    auto spv = util::load_spv(path);
    return vk::raii::ShaderModule(
        *m_device,
        vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), spv));
}
}  // namespace gfx
