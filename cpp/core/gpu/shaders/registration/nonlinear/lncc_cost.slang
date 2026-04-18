/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */
// TODO: some parts of the code are the same between global and nonlinear case. We could probably
// abstract some of it out to a common file.
// Entry points:
// - lnccPrepareRawMomentsForward / lnccPrepareRawMomentsBackward
// - lnccCostFilterXPass / lnccCostFilterYPass
// - lnccCostForwardReduceZ / lnccCostBackwardReduceZ
// LNCC = Cov(r, m) / sqrt(Var(r) * Var(m))
// where r is the reference (fixed) image and m is the moving image.
// Cov(r, m) = 1/N Σ_i r_i * m_i - mu_r * mu_m
// Var(r) = 1/N Σ_i r_i^2 - mu_r^2
// Var(m) = 1/N Σ_i m_i^2 - mu_m^2
// The sum is taken over a window around the current voxel with weights w_i (currently all 1s, 
// but could be e.g. a Gaussian). mu_r and mu_m are the weighted means of r and m, respectively.

import texture_utils;
import reduction_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kWindowRadius;
extern static const float kMomentEpsilon;
extern static const uint32_t kUseFixedMask;
extern static const uint32_t kUseMovingMask;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

struct CostSlabUniforms {
    int32_t slabInputZStart;
    uint32_t reduceChunkDepth;
    uint32_t reduceWorkgroupZOffset;
};

struct LnccMomentSums
{
    float sumReference; // Σ_i w_i * r_i
    float sumMoving; // Σ_i w_i * m_i
    float sumReferenceSquared; // Σ_i w_i * r_i^2
    float sumMovingSquared; // Σ_i w_i * m_i^2
    float sumReferenceMoving; // Σ_i w_i * r_i * m_i
    float sumWeights; // Σ_i w_i
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
    let varianceProduct = varianceReference * varianceMoving;

    // Flat or near-flat windows do not define a stable local correlation.
    if (!(varianceProduct > kMomentEpsilon)) {
        cost = 0.0F;
        return false;
    }

    let rho = covariance / sqrt(varianceProduct);
    cost = -rho;
    return true;
}

struct CostAndCount {
    float cost;
    float count;
};

struct ReduceCostAndCountOp : IReduceOp<CostAndCount> {
    static CostAndCount identityElement() {
        return CostAndCount(0.0F, 0.0F);
    }
    static CostAndCount reduce(CostAndCount a, CostAndCount b) {
        return CostAndCount(a.cost + b.cost, a.count + b.count);
    }
};

// For each voxel compute and store 6 moments: R, M, R^2, M^2, R*M, and the sum of weights W.
// To do this we use two RGBA float4 3D textures as output, where the first texture stores R, M, R^2, and M^2
// and the second texture stores R*M, W, and two unused channels.
[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccPrepareRawMomentsForward(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float> fixedMask,
    Texture3D<float> movingMask,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    ConstantBuffer<CostSlabUniforms> slabUniforms,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    let slabDims = textureSize(outputMoments1);
    if (!inBoundsIndex(int32_t3(globalId), slabDims)) {
        return;
    }

    let fixedDims = textureSize(fixedImage);
    let globalZ = slabUniforms.slabInputZStart + int32_t(globalId.z);
    let globalCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), globalZ);
    if (!inBoundsIndex(globalCoord, fixedDims)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let movingDims = textureSize(movingImage);
    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let fixedMaskField = VoxelSamplingField(fixedMask, linearSampler);
    let movingMaskField = VoxelSamplingField(movingMask, linearSampler);
    let centreFixedVoxel = float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z));
    if (!fixedMaskField.maskAccepts(centreFixedVoxel, kUseFixedMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    let centreFixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(centreFixedVoxel, 1.0F)).xyz;
    let centreDisplacement = displacement.Load(int32_t4(globalCoord, 0)).xyz;
    let centreWarpedScanner = centreFixedScanner + centreDisplacement;
    let centreWarpedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(centreWarpedScanner, 1.0F)).xyz;
    if (!inBoundsVoxel(centreWarpedMovingVoxel, movingDims)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    if (!movingMaskField.maskAccepts(centreWarpedMovingVoxel, kUseMovingMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let fixedValue = fixedImage.Load(int32_t4(globalCoord, 0));
    let movingValue = movingField.sample(centreWarpedMovingVoxel);
    let R = fixedValue; let M = movingValue;
    let R2 = fixedValue * fixedValue; let M2 = movingValue * movingValue;
    let W = 1.0F;

    outputMoments1.Store(globalId, float4(R, M, R2, M2));
    outputMoments2.Store(globalId, float4(R * M, W, 0.0F, 0.0F));
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccPrepareRawMomentsBackward(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,

    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float> fixedMask,
    Texture3D<float> movingMask,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    ConstantBuffer<CostSlabUniforms> slabUniforms,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    let slabDims = textureSize(outputMoments1);
    if (!inBoundsIndex(int32_t3(globalId), slabDims)) {
        return;
    }

    let movingDims = textureSize(movingImage);
    let globalZ = slabUniforms.slabInputZStart + int32_t(globalId.z);
    let globalCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), globalZ);
    if (!inBoundsIndex(globalCoord, movingDims)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let fixedDims = textureSize(fixedImage);
    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let fixedMaskField = VoxelSamplingField(fixedMask, linearSampler);
    let movingMaskField = VoxelSamplingField(movingMask, linearSampler);
    let displacementField = VoxelSamplingFieldFloat4(displacement, linearSampler);

    let centreMovingVoxel = float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z));
    if (!movingMaskField.maskAccepts(centreMovingVoxel, kUseMovingMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    let centreMovingScanner = mul(voxelScannerMatrices.sourceVoxelToScanner, float4(centreMovingVoxel, 1.0F)).xyz;
    let centreDisplacementSampleVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(centreMovingScanner, 1.0F)).xyz;

    if (!inBoundsVoxel(centreDisplacementSampleVoxel, fixedDims)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let centreBackwardDisplacement = displacementField.sample(centreDisplacementSampleVoxel).xyz;
    let centreMovedScanner = centreMovingScanner + centreBackwardDisplacement;
    let centreMovedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(centreMovedScanner, 1.0F)).xyz;

    if (!inBoundsVoxel(centreMovedFixedVoxel, fixedDims)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    if (!fixedMaskField.maskAccepts(centreMovedFixedVoxel, kUseFixedMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let referenceValue = movingImage.Load(int32_t4(globalCoord, 0));
    let movingValue = fixedField.sample(centreMovedFixedVoxel);

    let R = referenceValue;
    let M = movingValue;
    let R2 = R * R;
    let M2 = M * M;
    let RM = R * M;
    let W = 1.0F;

    outputMoments1.Store(globalId, float4(R, M, R2, M2));
    outputMoments2.Store(globalId, float4(RM, W, 0.0F, 0.0F));
}


void lnccFilterAlongAxis(
    uint32_t3 globalId,
    uint32_t3 filterDims,
    int32_t3 axisOffset,
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    if (globalId.x >= filterDims.x || globalId.y >= filterDims.y || globalId.z >= filterDims.z) {
        return;
    }

    let radius = int32_t(kWindowRadius);
    let baseCoord = int32_t3(globalId);
    var sumMoments1 = float4(0.0F, 0.0F, 0.0F, 0.0F);
    var sumMoments2 = float4(0.0F, 0.0F, 0.0F, 0.0F);
    for (int32_t i = -radius; i <= radius; ++i) {
        let sampleCoord = baseCoord + axisOffset * i;
        if(!inBoundsIndex(sampleCoord, filterDims)) {
            continue;
        }
        let texelCoord = int32_t4(sampleCoord, 0);
        sumMoments1 += inputMoments1.Load(texelCoord);
        sumMoments2 += inputMoments2.Load(texelCoord);
    }
    outputMoments1.Store(globalId, sumMoments1);
    outputMoments2.Store(globalId, sumMoments2);
}

void lnccAccumulateZSlabs(
    int32_t3 baseCoord,
    uint32_t3 textureDims,
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    inout LnccMomentSums sums
)
{
    let radius = int32_t(kWindowRadius);
    for (int32_t dz = -radius; dz <= radius; ++dz) {
        let sliceCoord = baseCoord + int32_t3(0, 0, dz);
        if(!inBoundsIndex(sliceCoord, textureDims)) {
            continue;
        }
        let texelCoord = int32_t4(sliceCoord, 0);
        let moments1 = inputMoments1.Load(texelCoord);
        let moments2 = inputMoments2.Load(texelCoord);
        sums.sumReference += moments1.x;
        sums.sumMoving += moments1.y;
        sums.sumReferenceSquared += moments1.z;
        sums.sumMovingSquared += moments1.w;
        sums.sumReferenceMoving += moments2.x;
        sums.sumWeights += moments2.y;
    }
}

[shader("compute")]
[numthreads(16, 4, 1)]
void lnccCostFilterXPass(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    Texture3D<float> latticeImage, // Only needed to enforce bounds for current lattice
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(inputMoments1);
    let filterDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    lnccFilterAlongAxis(globalId, filterDims, int32_t3(1, 0, 0), inputMoments1, inputMoments2, outputMoments1, outputMoments2);
}

[shader("compute")]
[numthreads(4, 16, 1)]
void lnccCostFilterYPass(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    Texture3D<float> latticeImage, // Only needed to enforce bounds for current lattice
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(inputMoments1);
    let filterDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    lnccFilterAlongAxis(globalId, filterDims, int32_t3(0, 1, 0), inputMoments1, inputMoments2, outputMoments1, outputMoments2);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccCostForwardReduceZ(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    ConstantBuffer<CostSlabUniforms> slabUniforms,
    Texture3D<float> fixedImage,
    Texture3D<float4> xyFilteredMoments1, // contains summed R, M, R^2, M^2 over X and Y
    Texture3D<float4> xyFilteredMoments2, // contains summed R*M, W over X and Y
    RWStructuredBuffer<float> ssdPartials,
    RWStructuredBuffer<float> ssdCounts)
{
    static groupshared Array<CostAndCount, workgroupInvocations> localCostAndCounts;

    static_assert(kWindowRadius > 0U, "LNCC cost doesn't make sense for a window radius of 0.");

    let localIndex = localId.x
                   + localId.y * kWorkgroupSizeX
                   + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;

    let latticeDims = textureSize(fixedImage);
    let scratchDims = textureSize(xyFilteredMoments1);
    let reduceDims = uint32_t3(latticeDims.x, latticeDims.y, slabUniforms.reduceChunkDepth);

    var cost = 0.0F;
    var count = 0.0F;

    if (inBoundsIndex(int32_t3(globalId), reduceDims)) {
        let globalZ = slabUniforms.slabInputZStart + int32_t(kWindowRadius) + int32_t(globalId.z);
        if (globalZ >= 0 && globalZ < int32_t(latticeDims.z)) {
            let slabDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
            let slabCenterCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), int32_t(globalId.z) + int32_t(kWindowRadius));
            var sums = initLnccMomentSums();
            lnccAccumulateZSlabs(slabCenterCoord, slabDims, xyFilteredMoments1, xyFilteredMoments2, sums);

            if (solveLnccCost(sums, cost)) {
                count = 1.0F;
            }
        }
    }

    localCostAndCounts[localIndex] = CostAndCount(cost, count);
    GroupMemoryBarrierWithGroupSync();

    let reducedCostAndCount = workgroupReduce<CostAndCount, ReduceCostAndCountOp, workgroupInvocations>(
        localCostAndCounts, localIndex);

    if (localIndex == 0U) {
        let globalWorkgroupZ = workgroupId.z + slabUniforms.reduceWorkgroupZOffset;
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid.x
                    + globalWorkgroupZ * uniforms.dispatchGrid.x * uniforms.dispatchGrid.y;
        ssdPartials[wgIndex] = reducedCostAndCount.cost;
        ssdCounts[wgIndex] = reducedCostAndCount.count;
    }

}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccCostBackwardReduceZ(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    ConstantBuffer<CostSlabUniforms> slabUniforms,
    Texture3D<float> movingImage,
    Texture3D<float4> xyFilteredMoments1, // contains summed R, M, R^2, M^2 over X and Y
    Texture3D<float4> xyFilteredMoments2, // contains summed R*M, W over X and Y
    RWStructuredBuffer<float> ssdPartials,
    RWStructuredBuffer<float> ssdCounts)
{
    static groupshared Array<CostAndCount, workgroupInvocations> localCostAndCounts;

    static_assert(kWindowRadius > 0U, "LNCC cost doesn't make sense for a window radius of 0.");

    let localIndex = localId.x
                   + localId.y * kWorkgroupSizeX
                   + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;

    let latticeDims = textureSize(movingImage);
    let scratchDims = textureSize(xyFilteredMoments1);
    let reduceDims = uint32_t3(latticeDims.x, latticeDims.y, slabUniforms.reduceChunkDepth);

    var cost = 0.0F;
    var count = 0.0F;

    if (inBoundsIndex(int32_t3(globalId), reduceDims)) {
        let globalZ = slabUniforms.slabInputZStart + int32_t(kWindowRadius) + int32_t(globalId.z);
        if (globalZ >= 0 && globalZ < int32_t(latticeDims.z)) {
            let slabDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
            let slabCenterCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), int32_t(globalId.z) + int32_t(kWindowRadius));
            var sums = initLnccMomentSums();
            lnccAccumulateZSlabs(slabCenterCoord, slabDims, xyFilteredMoments1, xyFilteredMoments2, sums);

            if (solveLnccCost(sums, cost)) {
                count = 1.0F;
            }
        }
    }

    localCostAndCounts[localIndex] = CostAndCount(cost, count);
    GroupMemoryBarrierWithGroupSync();

    let reduced = workgroupReduce<CostAndCount, ReduceCostAndCountOp, workgroupInvocations>(localCostAndCounts, localIndex);

    if (localIndex == 0U) {
        let globalWorkgroupZ = workgroupId.z + slabUniforms.reduceWorkgroupZOffset;
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid.x
                    + globalWorkgroupZ * uniforms.dispatchGrid.x * uniforms.dispatchGrid.y;
        ssdPartials[wgIndex] = reduced.cost;
        ssdCounts[wgIndex] = reduced.count;
    }
}
