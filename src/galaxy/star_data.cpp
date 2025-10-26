#include "galaxy/star_data.hpp"

#include <glm/fwd.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "gfx/utils.hpp"

namespace galaxy {
GPUStarData::GPUStarData(vk::raii::Device& device,
                         vk::raii::PhysicalDevice& physical_device,
                         vk::raii::CommandBuffer& command_buffer,
                         vk::raii::Queue& queue, StarData star_data) {
    m_star_count = star_data.size();
    /* POSITIONS */
    /* STAGING BUFFER */
    vk::BufferCreateInfo staging_positions_buffer_create_info(
        {}, sizeof(glm::vec3) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc);

    vk::raii::Buffer staging_positions_buffer =
        vk::raii::Buffer(device, staging_positions_buffer_create_info);

    vk::MemoryRequirements staging_positions_memory_requirements =
        staging_positions_buffer.getMemoryRequirements();

    uint32_t staging_positions_type_index = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        staging_positions_memory_requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::MemoryAllocateInfo staging_positions_memory_allocate_info(
        staging_positions_memory_requirements.size,
        staging_positions_type_index);
    vk::raii::DeviceMemory staging_positions_memory =
        vk::raii::DeviceMemory(device, staging_positions_memory_allocate_info);

    staging_positions_buffer.bindMemory(staging_positions_memory, 0);

    /* POSITIONS2 */
    /* STAGING BUFFER */
    vk::BufferCreateInfo staging_positions_buffer_create_info2(
        {}, sizeof(glm::vec3) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc);

    vk::raii::Buffer staging_positions_buffer2 =
        vk::raii::Buffer(device, staging_positions_buffer_create_info2);

    vk::MemoryRequirements staging_positions_memory_requirements2 =
        staging_positions_buffer2.getMemoryRequirements();

    uint32_t staging_positions_type_index2 = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        staging_positions_memory_requirements2.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::MemoryAllocateInfo staging_positions_memory_allocate_info2(
        staging_positions_memory_requirements2.size,
        staging_positions_type_index2);
    vk::raii::DeviceMemory staging_positions_memory2 =
        vk::raii::DeviceMemory(device, staging_positions_memory_allocate_info2);

    staging_positions_buffer2.bindMemory(staging_positions_memory2, 0);

    /* CPU TO BUFFER COPY */
    void* data1 = staging_positions_memory.mapMemory(
        0, sizeof(glm::vec3) * star_data.size());
    // Assuming star_data.positions is a std::vector<glm::vec3>
    memcpy(data1, star_data.positions().data(),
           (size_t)(sizeof(glm::vec3) * star_data.size()));
    staging_positions_memory.unmapMemory();
    /* CPU TO BUFFER COPY */
    void* data2 = staging_positions_memory2.mapMemory(
        0, sizeof(glm::vec3) * star_data.size());
    // Assuming star_data.positions is a std::vector<glm::vec3>
    memcpy(data2, star_data.positions().data(),
           (size_t)(sizeof(glm::vec3) * star_data.size()));
    staging_positions_memory2.unmapMemory();

    /*GPU LOCAL BUFFER*/
    vk::BufferCreateInfo positions_buffer_create_info(
        {}, sizeof(glm::vec3) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferDst);
    m_positions.emplace_back(device, positions_buffer_create_info);
    m_positions.emplace_back(device, positions_buffer_create_info);

    vk::MemoryRequirements positions_memory_requirements =
        m_positions[0].getMemoryRequirements();

    uint32_t positions_type_index = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        positions_memory_requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo positions_memory_allocate_info(
        positions_memory_requirements.size, positions_type_index);
    m_positions_memories.emplace_back(device, positions_memory_allocate_info);
    m_positions_memories.emplace_back(device, positions_memory_allocate_info);
    m_positions[0].bindMemory(m_positions_memories[0], 0);
    m_positions[1].bindMemory(m_positions_memories[1], 0);

    /* TINTS */
    /* STAGING BUFFER */
    vk::BufferCreateInfo staging_tints_buffer_create_info(
        {}, sizeof(glm::vec3) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc);

    vk::raii::Buffer staging_tints_buffer =
        vk::raii::Buffer(device, staging_tints_buffer_create_info);

    vk::MemoryRequirements staging_tints_memory_requirements =
        staging_tints_buffer.getMemoryRequirements();

    uint32_t staging_tints_type_index = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        staging_tints_memory_requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::MemoryAllocateInfo staging_tints_memory_allocate_info(
        staging_tints_memory_requirements.size, staging_tints_type_index);
    vk::raii::DeviceMemory staging_tints_memory =
        vk::raii::DeviceMemory(device, staging_tints_memory_allocate_info);

    staging_tints_buffer.bindMemory(staging_tints_memory, 0);

    /* CPU TO BUFFER COPY */
    void* data =
        staging_tints_memory.mapMemory(0, sizeof(glm::vec3) * star_data.size());
    // Assuming star_data.positions is a std::vector<glm::vec3>
    memcpy(data, star_data.tints().data(),
           (size_t)(sizeof(glm::vec3) * star_data.size()));
    staging_tints_memory.unmapMemory();

    /*GPU LOCAL BUFFER*/
    vk::BufferCreateInfo tints_buffer_create_info(
        {}, sizeof(glm::vec3) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferDst);
    m_tints = vk::raii::Buffer(device, tints_buffer_create_info);

    vk::MemoryRequirements tints_memory_requirements =
        m_tints.getMemoryRequirements();

    uint32_t tints_type_index =
        gfx::util::find_memory_type(physical_device.getMemoryProperties(),
                                    tints_memory_requirements.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo tints_memory_allocate_info(
        tints_memory_requirements.size, tints_type_index);
    m_tints_memory = vk::raii::DeviceMemory(device, tints_memory_allocate_info);
    m_tints.bindMemory(m_tints_memory, 0);

    /* WEIGHTS */
    /* STAGING BUFFER */
    vk::BufferCreateInfo staging_weights_buffer_create_info(
        {}, sizeof(float_t) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc);

    vk::raii::Buffer staging_weights_buffer =
        vk::raii::Buffer(device, staging_weights_buffer_create_info);

    vk::MemoryRequirements staging_weights_memory_requirements =
        staging_weights_buffer.getMemoryRequirements();

    uint32_t staging_weights_type_index = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        staging_weights_memory_requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::MemoryAllocateInfo staging_weights_memory_allocate_info(
        staging_weights_memory_requirements.size, staging_weights_type_index);
    vk::raii::DeviceMemory staging_weights_memory =
        vk::raii::DeviceMemory(device, staging_weights_memory_allocate_info);

    staging_weights_buffer.bindMemory(staging_weights_memory, 0);

    /* CPU TO BUFFER COPY */
    data = staging_weights_memory.mapMemory(
        0, sizeof(glm::float32_t) * star_data.size());
    // Assuming star_data.positions is a std::vector<glm::vec3>
    memcpy(data, star_data.weights().data(),
           (size_t)(sizeof(glm::float32_t) * star_data.size()));
    staging_weights_memory.unmapMemory();

    /*GPU LOCAL BUFFER*/
    vk::BufferCreateInfo weights_buffer_create_info(
        {}, sizeof(float_t) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferDst);
    m_weights = vk::raii::Buffer(device, weights_buffer_create_info);

    vk::MemoryRequirements weights_memory_requirements =
        m_weights.getMemoryRequirements();

    uint32_t weights_type_index =
        gfx::util::find_memory_type(physical_device.getMemoryProperties(),
                                    weights_memory_requirements.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal);
    vk::MemoryAllocateInfo weights_memory_allocate_info(
        weights_memory_requirements.size, weights_type_index);

    m_weights_memory =
        vk::raii::DeviceMemory(device, weights_memory_allocate_info);

    m_weights.bindMemory(m_weights_memory, 0);

    /* GPU LOCAL SCREEN COORDS BUFFER */
    vk::BufferCreateInfo screen_coords_buffer_create_info(
        {}, sizeof(glm::vec2) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer);

    m_screen_pos = vk::raii::Buffer(device, screen_coords_buffer_create_info);

    vk::MemoryRequirements screen_coords_memory_requirements =
        m_screen_pos.getMemoryRequirements();

    uint32_t coords_type_index = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        screen_coords_memory_requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo coords_memory_allocate_info(
        screen_coords_memory_requirements.size, coords_type_index);

    m_screen_pos_memory =
        vk::raii::DeviceMemory(device, coords_memory_allocate_info);

    m_screen_pos.bindMemory(m_screen_pos_memory, 0);

    /* GPU LOCAL VELOCITIES BUFFER */
    vk::BufferCreateInfo velocities_buffer_create_info(
        {}, sizeof(glm::vec3) * star_data.size(),
        vk::BufferUsageFlagBits::eStorageBuffer);

    m_velocities = vk::raii::Buffer(device, velocities_buffer_create_info);

    vk::MemoryRequirements velocities_memory_requirements =
        m_velocities.getMemoryRequirements();

    uint32_t velocities_type_index = gfx::util::find_memory_type(
        physical_device.getMemoryProperties(),
        velocities_memory_requirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo velocities_memory_allocate_info(
        velocities_memory_requirements.size, velocities_type_index);

    m_velocities_memory =
        vk::raii::DeviceMemory(device, velocities_memory_allocate_info);

    m_velocities.bindMemory(m_velocities_memory, 0);

    std::vector<vk::raii::Buffer> staging_vec;
    staging_vec.push_back(std::move(staging_positions_buffer));
    staging_vec.push_back(std::move(staging_positions_buffer2));
    staging_vec.push_back(std::move(staging_tints_buffer));
    staging_vec.push_back(std::move(staging_weights_buffer));

    std::vector<vk::raii::Buffer*> device_vec = {
        &m_positions[0], &m_positions[1], &m_tints, &m_weights};

    gfx::util::copy_buffers_to_device_local(
        device, command_buffer, queue, staging_vec, device_vec,
        {sizeof(glm::vec3) * star_data.size(),
         sizeof(glm::vec3) * star_data.size(),
         sizeof(glm::vec3) * star_data.size(),
         sizeof(glm::float32_t) * star_data.size()});

    m_set_layout =
        vk::raii::DescriptorSetLayout(gfx::util::make_descriptor_set_layout(
            device, {{vk::DescriptorType::eStorageBuffer, 1,
                      vk::ShaderStageFlagBits::eCompute},
                     {vk::DescriptorType::eStorageBuffer, 1,
                      vk::ShaderStageFlagBits::eCompute},
                     {vk::DescriptorType::eStorageBuffer, 1,
                      vk::ShaderStageFlagBits::eCompute},
                     {vk::DescriptorType::eStorageBuffer, 1,
                      vk::ShaderStageFlagBits::eCompute},
                     {vk::DescriptorType::eStorageBuffer, 1,
                      vk::ShaderStageFlagBits::eCompute},
                     {vk::DescriptorType::eStorageBuffer, 1,
                      vk::ShaderStageFlagBits::eCompute}}));

    std::vector<vk::DescriptorPoolSize> pool_sizes = {
        {vk::DescriptorType::eStorageBuffer, 6}};

    vk::DescriptorPoolCreateInfo pool_create_info(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_sizes);
    m_descriptor_pool = vk::raii::DescriptorPool(device, pool_create_info);

    vk::DescriptorSetAllocateInfo set_allocate_info(*m_descriptor_pool,
                                                    *m_set_layout);
    m_descriptor_sets = vk::raii::DescriptorSets(device, set_allocate_info);

    /* position descriptor */
    vk::DescriptorBufferInfo position_descriptor_buffer_info(
        m_positions[0], 0, sizeof(glm::vec3) * m_star_count);
    vk::WriteDescriptorSet write_position_set(
        m_descriptor_sets.front(), 0, 0, vk::DescriptorType::eStorageBuffer, {},
        position_descriptor_buffer_info);
    /* position descriptor */
    vk::DescriptorBufferInfo position_descriptor_buffer_info2(
        m_positions[1], 0, sizeof(glm::vec3) * m_star_count);
    vk::WriteDescriptorSet write_position_set2(
        m_descriptor_sets.front(), 4, 0, vk::DescriptorType::eStorageBuffer, {},
        position_descriptor_buffer_info2);

    /* tint descriptor */
    vk::DescriptorBufferInfo tint_descriptor_buffer_info(
        m_tints, 0, sizeof(glm::vec3) * m_star_count);
    vk::WriteDescriptorSet write_tint_set(m_descriptor_sets.front(), 1, 0,
                                          vk::DescriptorType::eStorageBuffer,
                                          {}, tint_descriptor_buffer_info);

    /* weight descriptor */
    vk::DescriptorBufferInfo weight_descriptor_buffer_info(
        m_weights, 0, sizeof(glm::float32_t) * m_star_count);
    vk::WriteDescriptorSet write_weight_set(m_descriptor_sets.front(), 2, 0,
                                            vk::DescriptorType::eStorageBuffer,
                                            {}, weight_descriptor_buffer_info);

    /* screen coords descriptor */
    vk::DescriptorBufferInfo coords_descriptor_buffer_info(
        m_screen_pos, 0, sizeof(glm::vec2) * m_star_count);
    vk::WriteDescriptorSet write_coords_set(m_descriptor_sets.front(), 3, 0,
                                            vk::DescriptorType::eStorageBuffer,
                                            {}, coords_descriptor_buffer_info);
    /* velocities descriptor */
    vk::DescriptorBufferInfo velocities_descriptor_buffer_info(
        m_screen_pos, 0, sizeof(glm::vec3) * m_star_count);
    vk::WriteDescriptorSet write_velocities_set(
        m_descriptor_sets.front(), 5, 0, vk::DescriptorType::eStorageBuffer, {},
        coords_descriptor_buffer_info);

    device.updateDescriptorSets(
        {write_position_set, write_position_set2, write_tint_set,
         write_weight_set, write_coords_set, write_velocities_set},
        nullptr);
}

GPUStarData::~GPUStarData() {}

StarData::StarData() {
    m_positions = {};
    m_tints = {};
    m_weights = {};
}

void StarData::push(Star star) {
    m_positions.push_back(star.position);
    m_tints.push_back(star.tint);
    m_weights.push_back(star.weight);
}
}  // namespace galaxy
