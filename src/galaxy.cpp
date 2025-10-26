#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <galaxy.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "galaxy/star_data.hpp"
#include "gfx/utils.hpp"
#include "push_constants.hpp"
#include "vulkan/vulkan.hpp"

const static uint32_t STAR_COUNT = 2048;

namespace galaxy {
Galaxy::Galaxy() { init_gfx(); }

Galaxy::~Galaxy() {
    for (auto& i : m_device_memories) {
        i.clear();
    }
}

void Galaxy::init_gfx() {
    try {
        vk::ImageCreateInfo image_ci(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D(vk::Extent2D{static_cast<uint32_t>(std::get<0>(
                                          m_gfx_core.window().getSize())),
                                      static_cast<uint32_t>(std::get<1>(
                                          m_gfx_core.window().getSize()))},
                         1),
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eStorage);
        m_intermediate_image =
            std::make_shared<vk::raii::Image>(*m_gfx_core.device(), image_ci);

        vk::PhysicalDeviceMemoryProperties memory_properties =
            m_gfx_core.physical_device()->getMemoryProperties();
        vk::MemoryRequirements memory_requirements =
            *m_intermediate_image->getMemoryRequirements();
        uint32_t memory_type_index = gfx::util::find_memory_type(
            memory_properties, memory_requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible);

        vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size,
                                                    memory_type_index);
        vk::raii::DeviceMemory device_memory =
            vk::raii::DeviceMemory(*m_gfx_core.device(), memory_allocate_info);
        m_intermediate_image->bindMemory(device_memory, 0);

        vk::ImageViewCreateInfo image_view_create_info(
            {}, *m_intermediate_image, vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            vk::ComponentMapping(
                vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                                      1));

        m_intermediate_image_view = std::make_shared<vk::raii::ImageView>(
            *m_gfx_core.device(), image_view_create_info);

        m_sim_set_layout = std::make_shared<vk::raii::DescriptorSetLayout>(
            gfx::util::make_descriptor_set_layout(
                *m_gfx_core.device(), {{vk::DescriptorType::eUniformBuffer, 1,
                                        vk::ShaderStageFlagBits::eCompute}}));

        m_draw_set_layout = std::make_shared<vk::raii::DescriptorSetLayout>(
            gfx::util::make_descriptor_set_layout(
                *m_gfx_core.device(), {{vk::DescriptorType::eUniformBuffer, 1,
                                        vk::ShaderStageFlagBits::eCompute},
                                       {vk::DescriptorType::eStorageImage, 1,
                                        vk::ShaderStageFlagBits::eCompute}}));

        std::vector<vk::DescriptorPoolSize> pool_sizes = {
            {vk::DescriptorType::eUniformBuffer, 1},
            {vk::DescriptorType::eStorageImage, 1}};

        vk::DescriptorPoolCreateInfo pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1,
            pool_sizes);
        m_descriptor_pool = std::make_shared<vk::raii::DescriptorPool>(
            *m_gfx_core.device(), pool_create_info);

        vk::DescriptorSetAllocateInfo draw_set_allocate_info(
            *m_descriptor_pool, **m_draw_set_layout);
        m_draw_descriptor_sets = std::make_shared<vk::raii::DescriptorSets>(
            *m_gfx_core.device(), draw_set_allocate_info);
        vk::DescriptorSetAllocateInfo sim_set_allocate_info(*m_descriptor_pool,
                                                            **m_sim_set_layout);
        // m_sim_descriptor_sets = std::make_shared<vk::raii::DescriptorSets>(
        //     *m_gfx_core.device(), sim_set_allocate_info);

        vk::DescriptorImageInfo descriptor_image_info;
        descriptor_image_info.setSampler(nullptr)
            .setImageView(*m_intermediate_image_view)
            .setImageLayout(vk::ImageLayout::eGeneral);

        m_gfx_core.upload_uniform_buffer(glm::vec3(1.0, 0.0, 0.0));

        /* INIT STAR DATA */
        init_star_data();

        vk::DescriptorBufferInfo descriptor_buffer_info(
            *m_gfx_core.uniform_buffer(), 0, sizeof(glm::vec3));
        vk::WriteDescriptorSet write_ubo_set(
            (*m_draw_descriptor_sets).front(), 0, 0,
            vk::DescriptorType::eUniformBuffer, {}, descriptor_buffer_info);
        vk::WriteDescriptorSet write_image_set(
            (*m_draw_descriptor_sets).front(), 1, 0,
            vk::DescriptorType::eStorageImage, descriptor_image_info, nullptr);

        m_gfx_core.device()->updateDescriptorSets(
            {write_ubo_set, write_image_set}, nullptr);

        std::array<vk::DescriptorSetLayout, 2> set_layouts = {
            **m_draw_set_layout, *m_gpu_star_data->descriptor_set_layout()};

        vk::PushConstantRange push_constant_range =
            m_camera.push_constants().push_constant_range();

        m_sim_pipeline_layout = std::make_shared<vk::raii::PipelineLayout>(
            *m_gfx_core.device().get(),
            vk::PipelineLayoutCreateInfo({}, set_layouts, push_constant_range));

        m_calc_coords_pipeline_layout =
            std::make_shared<vk::raii::PipelineLayout>(
                *m_gfx_core.device().get(),
                vk::PipelineLayoutCreateInfo({}, set_layouts,
                                             push_constant_range));

        m_draw_pipeline_layout = std::make_shared<vk::raii::PipelineLayout>(
            *m_gfx_core.device().get(),
            vk::PipelineLayoutCreateInfo({}, set_layouts, push_constant_range));

        vk::raii::ShaderModule screen_coords_shader =
            m_gfx_core.create_shader_module(
                "./shaders/calculate_screen_coords.slang.spirv");
        vk::raii::ShaderModule draw_shader =
            m_gfx_core.create_shader_module("./shaders/draw.slang.spirv");
        vk::raii::ShaderModule sim_shader =
            m_gfx_core.create_shader_module("./shaders/sim.slang.spirv");

        vk::PipelineShaderStageCreateInfo
            screen_coords_stage_pipeline_create_info(
                {}, vk::ShaderStageFlagBits::eCompute, screen_coords_shader,
                "main");
        vk::PipelineShaderStageCreateInfo draw_stage_pipeline_create_info(
            {}, vk::ShaderStageFlagBits::eCompute, draw_shader, "main");
        vk::PipelineShaderStageCreateInfo sim_stage_pipeline_create_info(
            {}, vk::ShaderStageFlagBits::eCompute, sim_shader, "main");

        vk::ComputePipelineCreateInfo screen_coords_pipeline_ci =
            vk::ComputePipelineCreateInfo()
                .setStage(screen_coords_stage_pipeline_create_info)
                .setLayout(*m_calc_coords_pipeline_layout);
        vk::ComputePipelineCreateInfo sim_pipeline_ci =
            vk::ComputePipelineCreateInfo()
                .setStage(sim_stage_pipeline_create_info)
                .setLayout(*m_sim_pipeline_layout);
        vk::ComputePipelineCreateInfo draw_pipeline_ci =
            vk::ComputePipelineCreateInfo()
                .setStage(draw_stage_pipeline_create_info)
                .setLayout(*m_draw_pipeline_layout);

        m_calc_coords_pipeline = std::make_shared<vk::raii::Pipeline>(
            *m_gfx_core.device(), nullptr, screen_coords_pipeline_ci);
        m_sim_pipeline = std::make_shared<vk::raii::Pipeline>(
            *m_gfx_core.device(), nullptr, sim_pipeline_ci);
        m_draw_pipeline = std::make_shared<vk::raii::Pipeline>(
            *m_gfx_core.device(), nullptr, draw_pipeline_ci);

        m_device_memories.push_back(std::move(device_memory));
        m_image_acquired_semaphore = std::make_shared<vk::raii::Semaphore>(
            *m_gfx_core.device(), vk::SemaphoreCreateInfo());
        m_drawing_done_semaphore = std::make_shared<vk::raii::Semaphore>(
            *m_gfx_core.device(), vk::SemaphoreCreateInfo());
        m_fence = std::make_shared<vk::raii::Fence>(*m_gfx_core.device(),
                                                    vk::FenceCreateInfo());

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

void Galaxy::init_star_data() {
    galaxy::StarData star_data;
    for (auto i = 0; i < STAR_COUNT; i++) {
        Star random_star;

        std::random_device r;
        std::default_random_engine e1(r());
        // tint
        std::uniform_real_distribution<float> tint_dist(0.0, 1.0);
        glm::vec3 tint(tint_dist(e1), tint_dist(e1), tint_dist(e1));
        random_star.tint = tint;

        // position (only x and y)
        std::uniform_real_distribution<float> pos_dist(-10.0, 10.0);
        glm::vec3 pos(pos_dist(e1), pos_dist(e1), pos_dist(e1));
        random_star.position = pos * 3000000000.0f;

        // weight
        std::uniform_real_distribution<float> weight_dist(
            pow(18.0, 8.0), 2.0 * pow(10.0, 20.0));
        random_star.weight = weight_dist(e1);

        star_data.push(random_star);
    }

    m_gpu_star_data = std::make_shared<galaxy::GPUStarData>(
        *m_gfx_core.device(), *m_gfx_core.physical_device(),
        (*m_gfx_core.command_buffers()).front(), *m_gfx_core.graphics_queue(),
        star_data);
}

void Galaxy::run() {
    try {
        m_gfx_core.upload_uniform_buffer(glm::vec3(1.0, 0.0, 0.0));
        while (!m_gfx_core.should_close()) {
            this->update();
        }
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
void Galaxy::update() {
    vk::Result result;
    std::tie(result, m_image_index) = m_gfx_core.swapchain()->acquireNextImage(
        gfx::util::TIMEOUT, *m_image_acquired_semaphore);
    if (result != vk::Result::eSuccess) {
        printf("bad result: %i\n", static_cast<uint32_t>(result));
        exit(static_cast<uint32_t>(result));
    }
    assert(m_image_index < m_gfx_core.swapchain_images().size());
    m_gfx_core.update();

    uint32_t read_buffer_index = m_positions_index % 2;
    uint32_t write_buffer_index = (m_positions_index + 1) % 2;

    (*m_gfx_core.command_buffers())
        .front()
        .begin(vk::CommandBufferBeginInfo(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    gfx::util::set_image_layout((*m_gfx_core.command_buffers()).front(),
                                m_gfx_core.swapchain_images()[m_image_index],
                                m_gfx_core.swapchain_format(),
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eTransferDstOptimal);

    gfx::util::set_image_layout(
        (*m_gfx_core.command_buffers()).front(), *m_intermediate_image,
        m_gfx_core.swapchain_format(), vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral);

    PushConstants push_constants = m_camera.push_constants();
    push_constants.positions_index = read_buffer_index;

    (*m_gfx_core.command_buffers())
        .front()
        .pushConstants<PushConstants>(*m_sim_pipeline_layout,
                                      vk::ShaderStageFlagBits::eCompute, 0,
                                      {push_constants});
    (*m_gfx_core.command_buffers())
        .front()
        .bindPipeline(vk::PipelineBindPoint::eCompute, *m_sim_pipeline);
    (*m_gfx_core.command_buffers())
        .front()
        .bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            *m_sim_pipeline_layout, 0,
                            {(*m_draw_descriptor_sets).front(),
                             m_gpu_star_data->descriptor_sets().front()},
                            nullptr);
    (*m_gfx_core.command_buffers()).front().dispatch(STAR_COUNT / 32, 1, 1);

    vk::BufferMemoryBarrier2KHR sim_to_calc_positions_barrier(
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderWrite,
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderRead, m_gfx_core.present_family_index(),
        m_gfx_core.present_family_index(),
        m_gpu_star_data->positions()[write_buffer_index], 0, vk::WholeSize);

    vk::BufferMemoryBarrier2KHR calc_coords_write_barrier(
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderWrite,  // Since Calc Coords only writes,
                                            // this is safe to leave as 'Write'
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderWrite, m_gfx_core.present_family_index(),
        m_gfx_core.present_family_index(), m_gpu_star_data->coords(), 0,
        vk::WholeSize);

    std::vector<vk::BufferMemoryBarrier2KHR> buffers_to_sync = {
        sim_to_calc_positions_barrier, calc_coords_write_barrier};

    vk::DependencyInfoKHR sim_calc_coords_dependency({}, {}, buffers_to_sync,
                                                     {});
    (*m_gfx_core.command_buffers())
        .front()
        .pipelineBarrier2(sim_calc_coords_dependency);

    push_constants.positions_index =
        write_buffer_index;  // ***CRITICAL: Must read the recently written
                             // positions***
    (*m_gfx_core.command_buffers())
        .front()
        .pushConstants<PushConstants>(*m_calc_coords_pipeline_layout,
                                      vk::ShaderStageFlagBits::eCompute, 0,
                                      {push_constants});

    (*m_gfx_core.command_buffers())
        .front()
        .bindPipeline(vk::PipelineBindPoint::eCompute, *m_calc_coords_pipeline);

    (*m_gfx_core.command_buffers())
        .front()
        .bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            *m_calc_coords_pipeline_layout, 0,
                            {(*m_draw_descriptor_sets).front(),
                             m_gpu_star_data->descriptor_sets().front()},
                            nullptr);
    (*m_gfx_core.command_buffers()).front().dispatch(STAR_COUNT / 32, 1, 1);

    vk::BufferMemoryBarrier2KHR calc_to_draw_coords_barrier(
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderWrite,
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderRead, m_gfx_core.present_family_index(),
        m_gfx_core.present_family_index(), m_gpu_star_data->coords(), 0,
        vk::WholeSize);  // m_gpu_star_data->coords() is the screen position
                         // buffer

    vk::DependencyInfoKHR calc_draw_dependency({}, {},
                                               calc_to_draw_coords_barrier, {});
    (*m_gfx_core.command_buffers())
        .front()
        .pipelineBarrier2(calc_draw_dependency);

    (*m_gfx_core.command_buffers())
        .front()
        .bindPipeline(vk::PipelineBindPoint::eCompute, *m_draw_pipeline);
    (*m_gfx_core.command_buffers())
        .front()
        .bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            *m_draw_pipeline_layout, 0,
                            {(*m_draw_descriptor_sets).front(),
                             m_gpu_star_data->descriptor_sets().front()},
                            nullptr);
    (*m_gfx_core.command_buffers()).front().dispatch(640 / 8, 480 / 8, 1);

    vk::ImageSubresourceLayers image_subresource_layers(
        vk::ImageAspectFlagBits::eColor, 0, 0, 1);

    gfx::util::set_image_layout(
        (*m_gfx_core.command_buffers()).front(), *m_intermediate_image,
        m_gfx_core.swapchain_format(), vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferSrcOptimal);

    vk::ImageCopy image_copy(image_subresource_layers, vk::Offset3D(),
                             image_subresource_layers, vk::Offset3D(0, 0, 0),
                             vk::Extent3D(640, 480, 1));

    (*m_gfx_core.command_buffers())
        .front()
        .copyImage(*m_intermediate_image, vk::ImageLayout::eTransferSrcOptimal,
                   m_gfx_core.swapchain_images()[m_image_index],
                   vk::ImageLayout::eTransferDstOptimal, image_copy);

    vk::ImageMemoryBarrier pre_present_barrier(
        vk::AccessFlagBits::eTransferWrite, {},
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        m_gfx_core.swapchain_images()[m_image_index],
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    (*m_gfx_core.command_buffers())
        .front()
        .pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                         vk::PipelineStageFlagBits::eTopOfPipe, {}, nullptr,
                         nullptr, pre_present_barrier);
    (*m_gfx_core.command_buffers()).front().end();

    vk::PipelineStageFlags wait_destination_stage_mask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submit_info(**m_image_acquired_semaphore,
                               wait_destination_stage_mask,
                               *(*m_gfx_core.command_buffers()).front());
    m_gfx_core.graphics_queue()->submit(submit_info, **m_fence);

    while (m_gfx_core.device()->waitForFences(
               {*m_fence}, true, gfx::util::TIMEOUT) == vk::Result::eTimeout);
    m_gfx_core.device()->resetFences({*m_fence});

    vk::PresentInfoKHR present_info(nullptr, **m_gfx_core.swapchain(),
                                    m_image_index);
    result = m_gfx_core.present_queue()->presentKHR(present_info);
    switch (result) {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eSuboptimalKHR:
            printf(
                "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR!\n");
            break;
        default:
            assert(false);
    }
    m_gfx_core.device()->waitIdle();
    m_positions_index += 1;
}
}  // namespace galaxy
