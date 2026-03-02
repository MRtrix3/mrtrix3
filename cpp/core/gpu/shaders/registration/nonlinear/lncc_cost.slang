// TODO: some parts of the code are the same between global and nonlinear case. We could probably
// abstract some of it out to a common file.

import reduction_utils;
import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kSymmetricDirection; // 0 = forward, 1 = backward
extern static const uint32_t kWindowRadius;
extern static const float kMomentEpsilon;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

bool inBoundsVoxel(float3 voxel, uint32_t3 dims)
{
    return voxel.x >= 0.0F && voxel.y >= 0.0F && voxel.z >= 0.0F &&
           voxel.x <= float(dims.x - 1U) && voxel.y <= float(dims.y - 1U) && voxel.z <= float(dims.z - 1U);
}

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
    if (globalId.x < fixedDims.x && globalId.y < fixedDims.y && globalId.z < fixedDims.z && kWindowRadius > 0U) {
        let directionBackward = (kSymmetricDirection != 0U);
        let fixedField = VoxelSamplingField(fixedImage, linearSampler);
        let movingField = VoxelSamplingField(movingImage, linearSampler);

        let centerFixedVoxel = float3(globalId);
        let centerFixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(centerFixedVoxel, 1.0F)).xyz;
        let centerDisplacement = displacement.Load(int32_t4(globalId, 0)).xyz;
        let centerMovedScanner = centerFixedScanner + centerDisplacement;
        let centerMovedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(centerMovedScanner, 1.0F)).xyz;
        let centerMovedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(centerMovedScanner, 1.0F)).xyz;

        let centerDirectionInBounds = directionBackward ? inBoundsVoxel(centerMovedFixedVoxel, fixedDims) : inBoundsVoxel(centerMovedMovingVoxel, movingDims);

        if (centerDirectionInBounds) {
            let radius = int32_t(kWindowRadius);
            var sumReference : float = 0.0F;
            var sumMoving : float = 0.0F;
            var sumReferenceSquared : float = 0.0F;
            var sumMovingSquared : float = 0.0F;
            var sumReferenceMoving : float = 0.0F;
            var validSamples : float = 0.0F;

            for (int32_t dz = -radius; dz <= radius; ++dz) {
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        let neighborInt = int3(globalId) + int3(dx, dy, dz);
                        if (neighborInt.x < 0 || neighborInt.y < 0 || neighborInt.z < 0 ||
                            neighborInt.x >= int32_t(fixedDims.x) || neighborInt.y >= int32_t(fixedDims.y) ||
                            neighborInt.z >= int32_t(fixedDims.z)) {
                            continue;
                        }

                        let neighborFixedVoxel = float3(neighborInt);
                        let neighborFixedScanner =
                            mul(voxelScannerMatrices.targetVoxelToScanner, float4(neighborFixedVoxel, 1.0F)).xyz;
                        let neighborDisplacement = displacement.Load(int32_t4(neighborInt, 0)).xyz;
                        let neighborMovedScanner = neighborFixedScanner + neighborDisplacement;
                        let neighborMovedMovingVoxel =
                            mul(voxelScannerMatrices.sourceScannerToVoxel, float4(neighborMovedScanner, 1.0F)).xyz;
                        let neighborMovedFixedVoxel =
                            mul(voxelScannerMatrices.targetScannerToVoxel, float4(neighborMovedScanner, 1.0F)).xyz;
                        let neighborBaseMovingVoxel =
                            mul(voxelScannerMatrices.sourceScannerToVoxel, float4(neighborFixedScanner, 1.0F)).xyz;

                        let directionInBounds = directionBackward ? inBoundsVoxel(neighborMovedFixedVoxel, fixedDims) : inBoundsVoxel(neighborMovedMovingVoxel, movingDims);
                        if (!directionInBounds) {
                            continue;
                        }

                        let referenceValue = directionBackward ? movingField.sampleWithZeroBorder(neighborBaseMovingVoxel) : fixedField.sampleWithZeroBorder(neighborFixedVoxel);
                        let movingValue = directionBackward ? fixedField.sampleWithZeroBorder(neighborMovedFixedVoxel) : movingField.sampleWithZeroBorder(neighborMovedMovingVoxel);
                        sumReference += referenceValue;
                        sumMoving += movingValue;
                        sumReferenceSquared += referenceValue * referenceValue;
                        sumMovingSquared += movingValue * movingValue;
                        sumReferenceMoving += referenceValue * movingValue;
                        validSamples += 1.0F;
                    }
                }
            }

            if (validSamples > 0.0F) {
                let invN = 1.0F / validSamples;
                let meanReference = sumReference * invN;
                let meanMoving = sumMoving * invN;
                let covariance = sumReferenceMoving * invN - meanReference * meanMoving;
                let varianceReference = max(0.0F, sumReferenceSquared * invN - meanReference * meanReference);
                let varianceMoving = max(0.0F, sumMovingSquared * invN - meanMoving * meanMoving);

                let covarianceEps = covariance + kMomentEpsilon;
                let varianceReferenceEps = varianceReference + kMomentEpsilon;
                let varianceMovingEps = varianceMoving + kMomentEpsilon;
                let rho = covarianceEps / sqrt(max(varianceReferenceEps * varianceMovingEps, kMomentEpsilon));
                cost = -(rho * rho);
                count = 1.0F;
            }
        }
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
