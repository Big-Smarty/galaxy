// Matches the CPU-side uniform buffer.
struct ColorData {
    float3 Color;
};

struct PushConstants {
    float4x4 view_matrix;
    int2 screen_dimensions;
    uint32_t positions_index;
};

// RESOURCES (Bindings)

// Uniform buffer, set 0 binding 0.
[[vk::binding(0, 0)]]
ConstantBuffer<ColorData> g_UniformColor;

// Storage image the shader writes into, set 0 binding 1.
[[vk::binding(1, 0)]]
RWTexture2D<float4> g_OutputImage;

[[vk::binding(0, 1)]]
StructuredBuffer<float3> global_positions1;

[[vk::binding(4, 1)]]
StructuredBuffer<float3> global_positions2;

[[vk::binding(3, 1)]]
StructuredBuffer<float2> screen_positions;

[[vk::binding(2, 1)]]
StructuredBuffer<float> star_weights;

[[vk::binding(1, 1)]]
StructuredBuffer<float3> star_tints;

[[vk::push_constant]]
PushConstants push_constants;

static const float2 OUT_OF_SCREEN = float2(99999.0, 99999.0);

// ENTRY POINT (Compute Kernel)

// One thread per pixel of the output image.
[numthreads(8, 8, 1)]
void main(uint3 ID: SV_DispatchThreadID) {
    int2 pos = int2(ID.xy);

    float3 accum = float3(0.0);
    for (int i = 0; i < 2048; i++) {
        float3 star_pos = float3(0.0);
        if (push_constants.positions_index == 0) {
            star_pos = global_positions1[i];
        } else {
            star_pos = global_positions2[i];
        }
        float2 star_coords = screen_positions[i];
        float star_weight = star_weights[i];

        if (star_coords.x == OUT_OF_SCREEN.x) {
            continue;
        }

        float distance = length(float3(0.0) - star_pos);

        float dx = abs(pos.x - star_coords.x);
        float dy = abs(pos.y - star_coords.y);

        float divisor = pow(dx + dy, 2);
        if (divisor > 0.0) {  // guard against division by zero
            accum += star_tints[i] * (star_weight * float3(1.0, 1.0, 1.0) / divisor) /
                     max(pow(distance, 2), 1.0);
        }
    }

    // float3 to float4, with alpha 1.0.
    float4 final_color = float4(accum * 10.0, 1.0f);

    g_OutputImage[pos] = final_color;
}
