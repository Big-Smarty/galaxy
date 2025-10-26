// The structure matching the CPU-side data (the uniform buffer)
struct ColorData {
    // The vec3 color we want to read from the uniform buffer
    float3 Color;
};

struct PushConstants {
    float4x4 view_matrix;
    int2 screen_dimensions;
    uint32_t positions_index;
};

// -----------------------------------------------------------
// RESOURCES (Bindings)
// -----------------------------------------------------------

// 1. The Uniform Buffer (Constant Buffer in HLSL terms)
// This is bound to set 0, binding 0 (b0 in HLSL)
// The 'register(b0)' syntax is a common HLSL-style binding convention supported by Slang.
[[vk::binding(0, 0)]]
ConstantBuffer<ColorData> g_UniformColor;

// 2. The Output Image (Unordered Access View in DX/Storage Image in Vulkan)
// This is bound to set 0, binding 1 (u0 in HLSL)
// 'rwtexture2d' is the Slang/HLSL-like type for a 2D image you can write to.
// The data type is float4 for color data.
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

// -----------------------------------------------------------
// ENTRY POINT (Compute Kernel)
// -----------------------------------------------------------

// The compute shader entry point.
// We execute this for every pixel (thread) in the dispatch grid.
[numthreads(8, 8, 1)]
void main(
    // The unique index of the thread within the entire dispatch grid
    uint3 ID: SV_DispatchThreadID) {
    // 1. Get the screen coordinates (x, y) from the thread ID
    int2 pos = int2(ID.xy);

    float3 accum = float3(0.0);  // Initialize to zero
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
        if (divisor > 0.0) {  // Avoid division by zero
            accum += star_tints[i] * (star_weight * float3(1.0, 1.0, 1.0) / divisor) /
                     max(pow(distance, 2), 1.0);  // Accumulate
        }
    }

    // 3. Convert the float3 color to float4 (adding 1.0 for the alpha component)
    float4 final_color = float4(accum * 10.0, 1.0f);

    // 4. Write the final color to the output image at the current pixel location
    g_OutputImage[pos] = final_color;
}
