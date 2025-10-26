#pragma once

#include "camera.hpp"
#include "gfx.hpp"
#include "galaxy/star_data.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace galaxy {
  class Galaxy {
    public:
      Galaxy();
      ~Galaxy();

      void init_gfx();
      void init_star_data();

      void run();
      void update();
      
    private:
      gfx::Core m_gfx_core;

      std::shared_ptr<vk::raii::ShaderModule> m_sim_module;
      std::shared_ptr<vk::raii::ShaderModule> m_calc_coords_module;
      std::shared_ptr<vk::raii::ShaderModule> m_draw_module;
      std::shared_ptr<vk::raii::PipelineLayout> m_sim_pipeline_layout;
      std::shared_ptr<vk::raii::PipelineLayout> m_calc_coords_pipeline_layout;
      std::shared_ptr<vk::raii::PipelineLayout> m_draw_pipeline_layout;
      std::shared_ptr<vk::raii::Pipeline> m_sim_pipeline;
      std::shared_ptr<vk::raii::Pipeline> m_calc_coords_pipeline;
      std::shared_ptr<vk::raii::Pipeline> m_draw_pipeline;

      std::shared_ptr<vk::raii::DescriptorPool> m_descriptor_pool;
      std::shared_ptr<vk::raii::DescriptorSetLayout> m_sim_set_layout;
      std::shared_ptr<vk::raii::DescriptorSetLayout> m_draw_set_layout;
      std::shared_ptr<vk::raii::DescriptorSets> m_sim_descriptor_sets;
      std::shared_ptr<vk::raii::DescriptorSets> m_draw_descriptor_sets;


      std::vector<vk::raii::DeviceMemory> m_device_memories;
      
      std::shared_ptr<vk::raii::Image> m_intermediate_image;
      std::shared_ptr<vk::raii::ImageView> m_intermediate_image_view;

      std::shared_ptr<vk::raii::Semaphore> m_image_acquired_semaphore;
      std::shared_ptr<vk::raii::Semaphore> m_drawing_done_semaphore;
      std::shared_ptr<vk::raii::Fence> m_fence;

      std::shared_ptr<galaxy::GPUStarData> m_gpu_star_data;

      galaxy::Camera m_camera;

      uint32_t m_image_index = 0;
      uint64_t m_positions_index = 0;
  };
}
