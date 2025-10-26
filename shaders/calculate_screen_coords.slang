struct PushConstants {
    float4x4 view_projection_matrix;
    int2 screen_size;
    uint32_t positions_index;
};

// input global positions buffer
[[vk::binding(0, 1)]]
StructuredBuffer<float3> g_GlobalPositions1;
[[vk::binding(4, 1)]]
StructuredBuffer<float3> g_GlobalPositions2;
[[vk::binding(1, 1)]]
StructuredBuffer<float3> star_tints;

// output screenpos buffer
[[vk::binding(3, 1)]]
RWStructuredBuffer<float2> g_ScreenPositions;

[[vk::push_constant]]
ConstantBuffer<PushConstants> push_constants;

static const float2 OUT_OF_SCREEN = float2(99999.0, 99999.0);

[numthreads(32, 1, 1)]
void main(
    // The unique index of the thread within the entire dispatch grid
    uint3 ID: SV_DispatchThreadID) {
    int idx = ID.x;

    float3 world_pos = float3(0.0);
    if (push_constants.positions_index == 0) {
        world_pos = g_GlobalPositions1[idx];
    } else {
        world_pos = g_GlobalPositions2[idx];
    }

    float4 clip_pos = mul(push_constants.view_projection_matrix, float4(world_pos, 1.0));

    float3 ndc = clip_pos.xyz / clip_pos.w;

    if ((ndc.x > 1.0 || ndc.x < -1.0) || (ndc.y > 1.0 || ndc.y < -1.0) ||
        (ndc.z > 1.0 || ndc.z < 0.0)) {
        g_ScreenPositions[idx] = OUT_OF_SCREEN;
        return;
    }

    float screen_x = (ndc.x * 0.5f + 0.5f) * push_constants.screen_size.x;
    float screen_y = (ndc.y * 0.5f + 0.5f) * push_constants.screen_size.y;

    g_ScreenPositions[idx] = float2(screen_x, screen_y);
}

