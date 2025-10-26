#pragma once


#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/glm.hpp>
#include <glm/integer.hpp>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include <glfwpp/glfwpp.h>

namespace gfx {
class Core {
public:
    Core();
    ~Core();
    Core(const Core&&) = delete;
    Core& operator=(const Core&& other) = delete;
    Core(Core&& other) = delete;
    Core& operator=(Core&& other) noexcept { return *this; };

    Core init();
    void update();

    bool should_close();

    template <typename T>
    void upload_uniform_buffer(const T& data);

    vk::raii::ShaderModule create_shader_module(std::string path);

    std::shared_ptr<vk::raii::Context> context() { return m_context; }
    std::shared_ptr<vk::raii::Instance> instance() { return m_instance; }
    std::shared_ptr<vk::raii::PhysicalDevice> physical_device() {
        return m_physical_device;
    }
    std::shared_ptr<vk::raii::Device> device() { return m_device; }
    std::shared_ptr<vk::raii::CommandPool> command_pool() {
        return m_command_pool;
    }
    std::shared_ptr<vk::raii::CommandBuffers> command_buffers() {
        return m_command_buffers;
    }
    std::shared_ptr<vk::raii::SurfaceKHR> surface() { return m_surface; }
    std::shared_ptr<vk::raii::SwapchainKHR> swapchain() { return m_swapchain; }
    std::vector<vk::Image>& swapchain_images() { return m_swapchain_images; }
    std::vector<vk::raii::ImageView>& swapchain_image_views() {
        return m_swapchain_image_views;
    }
    vk::raii::Buffer& uniform_buffer() { return m_uniform_buffer; }
    vk::raii::DeviceMemory& uniform_buffer_memory() {
        return m_uniform_buffer_memory;
    }
    std::shared_ptr<vk::raii::Queue> graphics_queue() {
        return m_graphics_queue;
    }
    std::shared_ptr<vk::raii::Queue> present_queue() { return m_present_queue; }
    vk::Format swapchain_format() { return m_format; }
    glfw::GlfwLibrary& glfw() { return m_glfw; }

    glfw::Window& window() { return m_window; }

    uint32_t graphics_family_index() {
        return m_graphics_family_index;
    }

    uint32_t present_family_index() {
        return m_present_family_index;
    }

private:
    std::shared_ptr<vk::raii::Context> m_context;
    std::shared_ptr<vk::raii::Instance> m_instance;
    std::shared_ptr<vk::raii::PhysicalDevice> m_physical_device;
    std::shared_ptr<vk::raii::Device> m_device;
    std::shared_ptr<vk::raii::Queue> m_graphics_queue;
    std::shared_ptr<vk::raii::Queue> m_present_queue;
    std::shared_ptr<vk::raii::CommandPool> m_command_pool;
    std::shared_ptr<vk::raii::CommandBuffers> m_command_buffers;
    std::shared_ptr<vk::raii::SurfaceKHR> m_surface;
    std::shared_ptr<vk::raii::SwapchainKHR> m_swapchain;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<vk::raii::ImageView> m_swapchain_image_views;

    vk::raii::Buffer m_uniform_buffer{nullptr};
    vk::raii::DeviceMemory m_uniform_buffer_memory{nullptr};

    vk::Format m_format;

    uint32_t m_graphics_family_index = 0;
    uint32_t m_present_family_index = 0;

    glfw::GlfwLibrary m_glfw = glfw::init();
    glfw::Window m_window;
};
}  // namespace gfx
