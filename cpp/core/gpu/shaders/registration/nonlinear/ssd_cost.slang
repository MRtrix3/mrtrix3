// Compute SSD cost between fixed image and warped moving image, given a displacement field.
import reduction_utils;
import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kIncludeOutOfBounds;
extern static const uint32_t kSymmetricDirection; // 0 = forward, 1 = backward
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
        let directionBackward = (kSymmetricDirection != 0U);

        let fixedVoxel = float3(globalId);
        let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
        let displacementValue = displacement.Load(int32_t4(globalId, 0)).xyz;
        let movedScanner = fixedScanner + displacementValue;
        let movedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
        let movedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
        let baseMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(fixedScanner, 1.0F)).xyz;

        let inBoundsMovedMoving = movedMovingVoxel.x >= 0.0F && movedMovingVoxel.y >= 0.0F &&
                                  movedMovingVoxel.z >= 0.0F && movedMovingVoxel.x <= float(movingDims.x - 1) &&
                                  movedMovingVoxel.y <= float(movingDims.y - 1) &&
                                  movedMovingVoxel.z <= float(movingDims.z - 1);
        let inBoundsMovedFixed = movedFixedVoxel.x >= 0.0F && movedFixedVoxel.y >= 0.0F &&
                                 movedFixedVoxel.z >= 0.0F && movedFixedVoxel.x <= float(fixedDims.x - 1) &&
                                 movedFixedVoxel.y <= float(fixedDims.y - 1) &&
                                 movedFixedVoxel.z <= float(fixedDims.z - 1);
        let inBounds = directionBackward ? inBoundsMovedFixed : inBoundsMovedMoving;

        let fixedField = VoxelSamplingField(fixedImage, linearSampler);
        let movingField = VoxelSamplingField(movingImage, linearSampler);
        let fixedValue = fixedField.sampleWithZeroBorder(fixedVoxel);
        let movingValueMoved = movingField.sampleWithZeroBorder(movedMovingVoxel);
        let movingValueBase = movingField.sampleWithZeroBorder(baseMovingVoxel);
        let fixedValueMoved = fixedField.sampleWithZeroBorder(movedFixedVoxel);
        let error = directionBackward ? (movingValueBase - fixedValueMoved) : (fixedValue - movingValueMoved);

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
