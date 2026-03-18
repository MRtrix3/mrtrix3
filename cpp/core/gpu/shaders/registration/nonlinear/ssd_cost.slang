// Compute SSD cost for symmetric log-demons.
// Entry points:
// - ssd_cost_forward: forward cost on the fixed lattice.
// - ssd_cost_backward: backward cost on the moving lattice.

import reduction_utils;
import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kIncludeOutOfBounds;
extern static const uint32_t kUseFixedMask;
extern static const uint32_t kUseMovingMask;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

void reduceAndStore(
    uint32_t localIndex,
    uint32_t3 workgroupId,
    uint32_t3 dispatchGrid,
    inout Array<float, workgroupInvocations> localCosts,
    inout Array<float, workgroupInvocations> localCounts,
    RWStructuredBuffer<float> ssdPartials,
    RWStructuredBuffer<float> ssdCounts)
{
    GroupMemoryBarrierWithGroupSync();
    let reducedCost = workgroupReduce<float, SumFloatOp, workgroupInvocations>(localCosts, localIndex);
    let reducedCount = workgroupReduce<float, SumFloatOp, workgroupInvocations>(localCounts, localIndex);
    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * dispatchGrid.x
                    + workgroupId.z * dispatchGrid.x * dispatchGrid.y;
        ssdPartials[wgIndex] = reducedCost;
        ssdCounts[wgIndex] = reducedCount;
    }
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void ssd_cost_forward(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float> fixedMask,
    Texture3D<float> movingMask,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    RWStructuredBuffer<float> ssdPartials,
    RWStructuredBuffer<float> ssdCounts)
{
    static groupshared Array<float, workgroupInvocations> localCosts;
    static groupshared Array<float, workgroupInvocations> localCounts;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let fixedDims = textureSize(fixedImage);
    let movingDims = textureSize(movingImage);

    var cost = 0.0F;
    var count = 0.0F;
    if (globalId.x < fixedDims.x && globalId.y < fixedDims.y && globalId.z < fixedDims.z) {
        let fixedField = VoxelSamplingField(fixedImage, linearSampler);
        let movingField = VoxelSamplingField(movingImage, linearSampler);
        let fixedMaskField = VoxelSamplingField(fixedMask, linearSampler);
        let movingMaskField = VoxelSamplingField(movingMask, linearSampler);

        let fixedVoxel = float3(globalId);
        let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
        let displacementValue = displacement.Load(int32_t4(globalId, 0)).xyz;
        let movedScanner = fixedScanner + displacementValue;
        let movedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
        let inBoundsMovedMoving = inBoundsVoxel(movedMovingVoxel, movingDims);
        let fixedMaskAccepted = fixedMaskField.maskAccepts(fixedVoxel, kUseFixedMask != 0U);
        let movingMaskAccepted = (kUseMovingMask == 0U)
                               || (inBoundsMovedMoving && movingMaskField.maskAccepts(movedMovingVoxel, true));

        let fixedValue = fixedField.sampleWithZeroBorder(fixedVoxel);
        let movingValueMoved = movingField.sampleWithZeroBorder(movedMovingVoxel);
        let error = fixedValue - movingValueMoved;

        let includeOutOfBounds = (kIncludeOutOfBounds != 0U);
        let masksAccept = fixedMaskAccepted && movingMaskAccepted;
        let includeCount = masksAccept ? (includeOutOfBounds ? 1.0F : (inBoundsMovedMoving ? 1.0F : 0.0F)) : 0.0F;
        cost = error * error * includeCount;
        count = includeCount;
    }

    localCosts[localIndex] = cost;
    localCounts[localIndex] = count;
    reduceAndStore(
        localIndex,
        workgroupId,
        uniforms.dispatchGrid,
        localCosts,
        localCounts,
        ssdPartials,
        ssdCounts);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void ssd_cost_backward(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float> fixedMask,
    Texture3D<float> movingMask,
    Texture3D<float4> displacementBackward,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    RWStructuredBuffer<float> ssdPartials,
    RWStructuredBuffer<float> ssdCounts)
{
    static groupshared Array<float, workgroupInvocations> localCosts;
    static groupshared Array<float, workgroupInvocations> localCounts;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let fixedDims = textureSize(fixedImage);
    let movingDims = textureSize(movingImage);

    var cost = 0.0F;
    var count = 0.0F;
    if (globalId.x < movingDims.x && globalId.y < movingDims.y && globalId.z < movingDims.z) {
        let fixedField = VoxelSamplingField(fixedImage, linearSampler);
        let movingField = VoxelSamplingField(movingImage, linearSampler);
        let fixedMaskField = VoxelSamplingField(fixedMask, linearSampler);
        let movingMaskField = VoxelSamplingField(movingMask, linearSampler);
        let displacementField = VoxelSamplingFieldFloat4(displacementBackward, linearSampler);

        let movingVoxel = float3(globalId);
        let movingScanner = mul(voxelScannerMatrices.sourceVoxelToScanner, float4(movingVoxel, 1.0F)).xyz;
        let displacementSampleVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(movingScanner, 1.0F)).xyz;
        var inBoundsMovedFixed = false;
        var movedFixedVoxel = float3(-1.0F, -1.0F, -1.0F);
        if (inBoundsVoxel(displacementSampleVoxel, fixedDims)) {
            let displacementValue = displacementField.sample(displacementSampleVoxel).xyz;
            let movedScanner = movingScanner + displacementValue;
            movedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
            inBoundsMovedFixed = inBoundsVoxel(movedFixedVoxel, fixedDims);
        }
        let movingMaskAccepted = movingMaskField.maskAccepts(movingVoxel, kUseMovingMask != 0U);
        let fixedMaskAccepted = (kUseFixedMask == 0U)
                              || (inBoundsMovedFixed && fixedMaskField.maskAccepts(movedFixedVoxel, true));

        let movingValue = movingField.sampleWithZeroBorder(movingVoxel);
        let fixedValueMoved = fixedField.sampleWithZeroBorder(movedFixedVoxel);
        let error = movingValue - fixedValueMoved;

        let includeOutOfBounds = (kIncludeOutOfBounds != 0U);
        let masksAccept = movingMaskAccepted && fixedMaskAccepted;
        let includeCount = masksAccept ? (includeOutOfBounds ? 1.0F : (inBoundsMovedFixed ? 1.0F : 0.0F)) : 0.0F;
        cost = error * error * includeCount;
        count = includeCount;
    }

    localCosts[localIndex] = cost;
    localCounts[localIndex] = count;
    reduceAndStore(
        localIndex,
        workgroupId,
        uniforms.dispatchGrid,
        localCosts,
        localCounts,
        ssdPartials,
        ssdCounts);
}
