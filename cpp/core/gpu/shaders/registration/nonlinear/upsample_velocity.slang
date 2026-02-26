// Upsample a velocity field by a fixed scale factor using linear sampling.
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kScaleX;
extern static const float kScaleY;
extern static const float kScaleZ;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> coarseVelocity,
    WTexture3D<float4> outputVelocity,
    SamplerState linearSampler)
{
    let dims = textureSize(outputVelocity);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let coarseSampler = VoxelSamplingFieldFloat4(coarseVelocity, linearSampler);
    let fineCoord = float3(globalId);
    let coarseCoord = float3(fineCoord.x / kScaleX, fineCoord.y / kScaleY, fineCoord.z / kScaleZ);
    let sampled = coarseSampler.sample(coarseCoord).xyz;
    outputVelocity.Store(uint32_t3(globalId), float4(sampled, 0.0F));
}
