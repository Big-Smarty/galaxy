struct PushConstants {
    float4x4 view_matrix;
    int2 screen_dimensions;
    uint32_t positions_index;
};

[[vk::push_constant]]
PushConstants push_constants;

[[vk::binding(0, 1)]]
RWStructuredBuffer<float3> global_positions1;
[[vk::binding(1, 1)]]
StructuredBuffer<float3> star_tints;
[[vk::binding(2, 1)]]
StructuredBuffer<float> star_weights;
[[vk::binding(4, 1)]]
RWStructuredBuffer<float3> global_positions2;
[[vk::binding(5, 1)]]
RWStructuredBuffer<float3> velocities;

static const float G = 6.67 * pow(10.0, -11);
static const float EPSILON_SQ = 1.0e-5;

void sim(RWStructuredBuffer<float3> current_star_positions,
         RWStructuredBuffer<float3> new_star_positions, uint idx) {
    float3 fnet = float3(0.0);
    for (int i = 0; i < 2048; i++) {
        if (i != idx) {
            float3 dir = current_star_positions[i] - current_star_positions[idx];
            float r_sq = dot(dir, dir);
            float denominator_pow3_2 = pow(r_sq + EPSILON_SQ, 1.5);  // (r^2 + epsilon^2)^(3/2)

            // Correct vector force formula: F = G * m1 * m2 * dir / (r^2 + epsilon^2)^(3/2)
            float3 f = (G * star_weights[i] * star_weights[idx] / denominator_pow3_2) * dir;
            fnet += f;
        }
    }
    float3 a = fnet / star_weights[idx];
    velocities[idx] += a * 10.0;
    new_star_positions[idx] = current_star_positions[idx] + velocities[idx] * 10.0;
}

[shader("compute")]
[numthreads(32, 1, 1)]
void main(uint3 ID: SV_DispatchThreadID) {
    uint idx = ID.x;

    if (push_constants.positions_index == 0) {
        sim(global_positions1, global_positions2, idx);
    } else {
        sim(global_positions2, global_positions1, idx);
    }
}
