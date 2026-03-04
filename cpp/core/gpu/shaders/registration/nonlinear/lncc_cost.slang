// TODO: some parts of the code are the same between global and nonlinear case. We could probably
// abstract some of it out to a common file.
// Entry points:
// - lncc_cost_forward: forward cost on the fixed lattice.
// - lncc_cost_backward: backward cost on the moving lattice.

import reduction_utils;
import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kWindowRadius;
extern static const float kMomentEpsilon;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

struct LnccMomentSums
{
    float sumReference;
    float sumMoving;
    float sumReferenceSquared;
    float sumMovingSquared;
    float sumReferenceMoving;
    float sumWeights;
};

LnccMomentSums initLnccMomentSums()
{
    return LnccMomentSums(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F);
}

void accumulateLnccMoment(inout LnccMomentSums sums, float referenceValue, float movingValue, float weight)
{
    sums.sumReference += weight * referenceValue;
    sums.sumMoving += weight * movingValue;
    sums.sumReferenceSquared += weight * referenceValue * referenceValue;
    sums.sumMovingSquared += weight * movingValue * movingValue;
    sums.sumReferenceMoving += weight * referenceValue * movingValue;
    sums.sumWeights += weight;
}

bool solveLnccCost(LnccMomentSums sums, out float cost)
{
    if (!(sums.sumWeights > 0.0F)) {
        cost = 0.0F;
        return false;
    }

    let invN = 1.0F / sums.sumWeights;
    let meanReference = sums.sumReference * invN;
    let meanMoving = sums.sumMoving * invN;
    let covariance = sums.sumReferenceMoving * invN - meanReference * meanMoving;
    let varianceReference = max(0.0F, sums.sumReferenceSquared * invN - meanReference * meanReference);
    let varianceMoving = max(0.0F, sums.sumMovingSquared * invN - meanMoving * meanMoving);

    let covarianceEps = covariance + kMomentEpsilon;
    let varianceReferenceEps = varianceReference + kMomentEpsilon;
    let varianceMovingEps = varianceMoving + kMomentEpsilon;
    let rho = covarianceEps / sqrt(max(varianceReferenceEps * varianceMovingEps, kMomentEpsilon));
    cost = -(rho * rho);
    return true;
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lncc_cost_forward(
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
        let fixedField = VoxelSamplingField(fixedImage, linearSampler);
        let movingField = VoxelSamplingField(movingImage, linearSampler);

        let centerFixedVoxel = float3(globalId);
        let centerFixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(centerFixedVoxel, 1.0F)).xyz;
        let centerDisplacement = displacement.Load(int32_t4(globalId, 0)).xyz;
        let centerMovedScanner = centerFixedScanner + centerDisplacement;
        let centerMovedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(centerMovedScanner, 1.0F)).xyz;
        if (inBoundsVoxel(centerMovedMovingVoxel, movingDims)) {
            let radius = int32_t(kWindowRadius);
            var sums = initLnccMomentSums();

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
                        if (!inBoundsVoxel(neighborMovedMovingVoxel, movingDims)) {
                            continue;
                        }

                        let referenceValue = fixedField.sampleWithZeroBorder(neighborFixedVoxel);
                        let movingValue = movingField.sampleWithZeroBorder(neighborMovedMovingVoxel);
                        accumulateLnccMoment(sums, referenceValue, movingValue, 1.0F);
                    }
                }
            }

            if (solveLnccCost(sums, cost)) {
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

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lncc_cost_backward(
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
    if (globalId.x < movingDims.x && globalId.y < movingDims.y && globalId.z < movingDims.z && kWindowRadius > 0U) {
        let fixedField = VoxelSamplingField(fixedImage, linearSampler);
        let movingField = VoxelSamplingField(movingImage, linearSampler);
        let displacementField = VoxelSamplingFieldFloat4(displacement, linearSampler);

        let centerMovingVoxel = float3(globalId);
        let centerMovingScanner = mul(voxelScannerMatrices.sourceVoxelToScanner, float4(centerMovingVoxel, 1.0F)).xyz;
        let centerDisplacementSampleVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(centerMovingScanner, 1.0F)).xyz;

        if (inBoundsVoxel(centerDisplacementSampleVoxel, fixedDims)) {
            let centerBackwardDisplacement = displacementField.sample(centerDisplacementSampleVoxel).xyz;
            let centerMovedScanner = centerMovingScanner + centerBackwardDisplacement;
            let centerMovedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(centerMovedScanner, 1.0F)).xyz;

            if (inBoundsVoxel(centerMovedFixedVoxel, fixedDims)) {
                let radius = int32_t(kWindowRadius);
                var sums = initLnccMomentSums();

                for (int32_t dz = -radius; dz <= radius; ++dz) {
                    for (int32_t dy = -radius; dy <= radius; ++dy) {
                        for (int32_t dx = -radius; dx <= radius; ++dx) {
                            let neighborInt = int3(globalId) + int3(dx, dy, dz);
                            if (neighborInt.x < 0 || neighborInt.y < 0 || neighborInt.z < 0 ||
                                neighborInt.x >= int32_t(movingDims.x) || neighborInt.y >= int32_t(movingDims.y) ||
                                neighborInt.z >= int32_t(movingDims.z)) {
                                continue;
                            }

                            let neighborMovingVoxel = float3(neighborInt);
                            let neighborMovingScanner =
                                mul(voxelScannerMatrices.sourceVoxelToScanner, float4(neighborMovingVoxel, 1.0F)).xyz;
                            let neighborDisplacementSampleVoxel =
                                mul(voxelScannerMatrices.targetScannerToVoxel, float4(neighborMovingScanner, 1.0F)).xyz;
                            if (!inBoundsVoxel(neighborDisplacementSampleVoxel, fixedDims)) {
                                continue;
                            }

                            let neighborBackwardDisplacement = displacementField.sample(neighborDisplacementSampleVoxel).xyz;
                            let neighborMovedScanner = neighborMovingScanner + neighborBackwardDisplacement;
                            let neighborMovedFixedVoxel =
                                mul(voxelScannerMatrices.targetScannerToVoxel, float4(neighborMovedScanner, 1.0F)).xyz;
                            if (!inBoundsVoxel(neighborMovedFixedVoxel, fixedDims)) {
                                continue;
                            }

                            let referenceValue = movingField.sampleWithZeroBorder(neighborMovingVoxel);
                            let movingValue = fixedField.sampleWithZeroBorder(neighborMovedFixedVoxel);
                            accumulateLnccMoment(sums, referenceValue, movingValue, 1.0F);
                        }
                    }
                }

                if (solveLnccCost(sums, cost)) {
                    count = 1.0F;
                }
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
