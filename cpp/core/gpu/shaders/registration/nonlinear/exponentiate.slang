// A shader that performs exponentiation of a velocity field using a scale-then-square approach.
// Given a velocity field v, we want to compute the displacement field d = exp(v).
// We do this by first scaling v down by 2^numSteps, then repeatedly squaring the result numSteps times.
// This approximates the exponential map for small velocities.
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kNumSteps = 6U;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void initialise(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> velocityTexture,
    WTexture3D<float4> output)
{
    var dims : uint32_t3;
    velocityTexture.GetDimensions(dims.x, dims.y, dims.z);

    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let velocity = velocityTexture.Load(int32_t4(globalId, 0));
    let scaled = exp2(-float(kNumSteps));

    output.Store(uint32_t3(globalId), velocity * scaled);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void square(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputDisplacement,
    WTexture3D<float4> outputDisplacement,
    SamplerState linearSampler)
{
    var dims : uint32_t3;
    inputDisplacement.GetDimensions(dims.x, dims.y, dims.z);

    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let oldDisplacement = inputDisplacement.Load(int32_t4(globalId, 0));
    let displacementField = VoxelSamplingFieldFloat4(inputDisplacement, linearSampler);
    let warp = displacementField.sample(float3(globalId) + oldDisplacement.xyz);
    let newDisplacement = oldDisplacement + warp;
    outputDisplacement.Store(uint32_t3(globalId), newDisplacement);
}