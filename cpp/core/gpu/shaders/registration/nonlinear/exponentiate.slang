// A shader that performs exponentiation of a velocity field using a scale-then-square approach.
// Given a velocity field v, we want to compute the displacement field d = exp(v).
// We do this by first scaling v down by 2^numSteps, then repeatedly squaring the result numSteps times.
// This approximates the exponential map for small velocities.
// Note that this shader assumes the velocity field is in scanner space (mm).
import texture_utils;
import voxelscannermatrices;

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
    let dims : uint32_t3 = textureSize(velocityTexture);

    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let velocity = velocityTexture.Load(int32_t4(globalId, 0)).xyz;
    let scaled = exp2(-float(kNumSteps));

    output.Store(uint32_t3(globalId), float4(velocity * scaled, 0.0F));
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void square(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputDisplacement,
    WTexture3D<float4> outputDisplacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler)
{
    let dims = textureSize(inputDisplacement);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let oldDisplacement = inputDisplacement.Load(int32_t4(globalId, 0)).xyz;
    let fixedVoxel = float3(globalId);
    let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let sampleScanner = fixedScanner + oldDisplacement;
    let sampleVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(sampleScanner, 1.0F)).xyz;

    var warp = float3(0.0F, 0.0F, 0.0F);
    if (sampleVoxel.x >= 0.0F && sampleVoxel.y >= 0.0F && sampleVoxel.z >= 0.0F &&
        sampleVoxel.x <= float(dims.x - 1) &&
        sampleVoxel.y <= float(dims.y - 1) &&
        sampleVoxel.z <= float(dims.z - 1))
    {
        let displacementField = VoxelSamplingFieldFloat4(inputDisplacement, linearSampler);
        warp = displacementField.sample(sampleVoxel).xyz;
    }
    let newDisplacement = oldDisplacement + warp;
    outputDisplacement.Store(uint32_t3(globalId), float4(newDisplacement, 0.0F));
}
