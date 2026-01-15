import atomic_utils;

extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 8;
extern static const uint32_t kWorkgroupSizeZ = 4;
// TODO: Use extern static const bool once the Slang compiler bug is fixed.
extern static const uint32_t kUseMask;

static const uint32_t kWorkgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t localIndex: SV_GroupIndex,
    RWStructuredBuffer<Atomic<uint32_t>> totalIntensity,
    RWStructuredBuffer<Atomic<uint32_t>> weightedPositions,
    Texture3D<float> image,
    Texture3D<float> mask)
{
    static groupshared float localTotalIntensity[kWorkgroupInvocations];
    static groupshared float3 localWeightedPositions[kWorkgroupInvocations];

    var imageDim : uint32_t3;
    image.GetDimensions(imageDim.x, imageDim.y, imageDim.z);

    localWeightedPositions[localIndex] = float3(0.0f, 0.0f, 0.0f);
    localTotalIntensity[localIndex] = 0.0f;

    if (all(globalId < imageDim)) {
        var includeVoxel = true;
        if (kUseMask != 0U) {
            let maskValue = mask.Load(int4(globalId, 0)).r;
            if (maskValue < 0.5f) {
                includeVoxel = false;
            }
        }

        if (includeVoxel) {
            let voxelValue = image.Load(int4(globalId, 0)).r;
            let voxelCoord = float3(globalId);
            localWeightedPositions[localIndex] = voxelValue * voxelCoord;
            localTotalIntensity[localIndex] = voxelValue;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint offset = kWorkgroupInvocations / 2; offset > 0; offset /= 2) {
        if (localIndex < offset) {
            localWeightedPositions[localIndex] += localWeightedPositions[localIndex + offset];
            localTotalIntensity[localIndex] += localTotalIntensity[localIndex + offset];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (localIndex == 0) {
        atomicAddF32InMemory(weightedPositions[0], localWeightedPositions[0].x);
        atomicAddF32InMemory(weightedPositions[1], localWeightedPositions[0].y);
        atomicAddF32InMemory(weightedPositions[2], localWeightedPositions[0].z);
        atomicAddF32InMemory(totalIntensity[0], localTotalIntensity[0]);
    }
}
