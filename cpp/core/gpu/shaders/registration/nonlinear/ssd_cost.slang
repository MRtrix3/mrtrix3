// Compute SSD cost between fixed image and warped moving image, given a displacement field.
import reduction_utils;
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kIncludeOutOfBounds;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float4> displacement,
    SamplerState linearSampler,
    RWStructuredBuffer<float> ssdPartials,
    RWStructuredBuffer<float> ssdCounts)
{
    static groupshared Array<float, workgroupInvocations> localCosts;
    static groupshared Array<float, workgroupInvocations> localCounts;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let fixedDim = textureSize(fixedImage);
    let movingDim = textureSize(movingImage);

    var cost = 0.0F;
    var count = 0.0F;
    if (globalId.x < fixedDim.x && globalId.y < fixedDim.y && globalId.z < fixedDim.z) {
        let fixedValue = fixedImage.Load(int32_t4(globalId, 0)).r;
        let disp = displacement.Load(int32_t4(globalId, 0)).xyz;
        let warpedCoord = float3(globalId) + disp;
        let inBounds = warpedCoord.x >= 0.0F && warpedCoord.y >= 0.0F && warpedCoord.z >= 0.0F &&
                       warpedCoord.x <= float(movingDim.x - 1) &&
                       warpedCoord.y <= float(movingDim.y - 1) &&
                       warpedCoord.z <= float(movingDim.z - 1);

        let movingField = VoxelSamplingField(movingImage, linearSampler);
        let movingValue = movingField.sampleWithZeroBorder(warpedCoord);
        let error = fixedValue - movingValue;

        let includeOutOfBounds = (kIncludeOutOfBounds != 0U);
        let includeCount = includeOutOfBounds ? 1.0F : (inBounds ? 1.0F : 0.0F);
        cost = error * error * includeCount;
        count = includeCount;
    }

    localCosts[localIndex] = cost;
    localCounts[localIndex] = count;
    GroupMemoryBarrierWithGroupSync();

    let reducedCost = workgroupReduce<float, SumFloatOp, workgroupInvocations>(localCosts, localIndex);
    let reducedCount = workgroupReduce<float, SumFloatOp, workgroupInvocations>(localCounts, localIndex);
    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid.x
                    + workgroupId.z * uniforms.dispatchGrid.x * uniforms.dispatchGrid.y;
        ssdPartials[wgIndex] = reducedCost;
        ssdCounts[wgIndex] = reducedCount;
    }
}
