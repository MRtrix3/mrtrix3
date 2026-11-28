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
// We compute a per-voxel LNCC update. Unlike the original diffeomorphic log-demons formulation by Vercauteren
// (SSD-based) and its LCC variant by Lorenzi et al, the update step computed here is not a Newton-style second order
// approximation. Instead, we differentiate the local LNCC objective and map that derivative to a local displacement
// update using the moving-image spatial gradient, yielding a first-order ascent direction. Empirically, this appears to
// produce good convergence behaviour but better behaved deformations. For each voxel, we first compute the local LNCC
// moments by summing over a window around the voxel. Let R and M denote the fixed and moving intensities in the window,
// and mu_R and mu_M their respective local means. Then the centered intensities are
//   R_c = R - mu_R,  M_c = M - mu_M.
// The local normalized cross-correlation moments are then
//   A = K[R_c * M_c],  B = K[R_c^2],  C = K[M_c^2],  rho = A / sqrt(B * C).
// We optimise rho directly. For the center sample j of the window, only the moving term depends on the local
// displacement update u, so
//   dA/dM_j = R_c(j) / N,
//   dB/dM_j = 0,
//   dC/dM_j = 2 M_c(j) / N,
// where N is the number of valid samples in the window. Differentiating rho gives
//   d(rho) / dM_j = 1 / (N * sqrt(B * C)) * (R_c(j) - (A / C) * M_c(j)).
// A small displacement perturbs the moving intensity by
//   dM_j ~= grad(M)_j^T du,
// so the ascent direction in scanner coordinates is
//   grad_u(rho) = 1 / (N * sqrt(B * C)) * (R_c(j) - (A / C) * M_c(j)) * grad(M)_j.
// In the code:
//   covariance -> A
//   varianceReference -> B
//   varianceMoving -> C
//   centeredReferenceValue -> R_c(j)
//   centeredMovingValue -> M_c(j)
//   gradMovingScanner -> grad(M)_j
// The final update is assembled as scale * scalar * gradMovingScanner.
// Both updates are represented in scanner-space units (mm).

import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kWindowRadius = 2U;
extern static const float kMomentEpsilon;
extern static const float kMaxUpdateMagnitude = 0.0F;
extern static const uint32_t kUseFixedMask;
extern static const uint32_t kUseMovingMask;

struct UpdateSlabUniforms {
    int32_t slabInputZStart; // Global Z index where the current slab starts in the full volume
    uint32_t reduceChunkDepth; // Number of Z slices to reduce over (without halo)
};

struct LnccMomentSums {
    float sumReference;
    float sumMoving;
    float sumReferenceSquared;
    float sumMovingSquared;
    float sumReferenceMoving;
    float sumWeights;
};

LnccMomentSums initLnccMomentSums() {
    return LnccMomentSums(0.0F,
                          0.0F,
                          0.0F,
                          0.0F,
                          0.0F,
                          0.0F);
}

bool solveLnccUpdate(LnccMomentSums sums,
                     float referenceValue,
                     float movingValue,
                     float3 gradMovingScanner,
                     out float3 update) {
    if (!(sums.sumWeights > 0.0F)) {
        update = float3(0.0F, 0.0F, 0.0F);
        return false;
    }

    let invN = 1.0F / sums.sumWeights;
    let meanReference = sums.sumReference * invN;
    let meanMoving = sums.sumMoving * invN;
    let covariance = sums.sumReferenceMoving * invN - meanReference * meanMoving;
    let varianceReference = max(0.0F, sums.sumReferenceSquared * invN - meanReference * meanReference);
    let varianceMoving = max(0.0F, sums.sumMovingSquared * invN - meanMoving * meanMoving);
    if (!(varianceReference > kMomentEpsilon) || !(varianceMoving > kMomentEpsilon)) {
        update = float3(0.0F, 0.0F, 0.0F);
        return false;
    }

    let centeredReferenceValue = referenceValue - meanReference;
    let centeredMovingValue = movingValue - meanMoving;
    let scale = 1.0F / (sums.sumWeights * sqrt(varianceReference * varianceMoving));
    let scalar = centeredReferenceValue - (covariance / varianceMoving) * centeredMovingValue;

    var candidateUpdate = scale * scalar * gradMovingScanner;
    if (kMaxUpdateMagnitude > 0.0F) {
        let updateNorm = length(candidateUpdate);
        if (updateNorm > kMaxUpdateMagnitude) {
            candidateUpdate = candidateUpdate * (kMaxUpdateMagnitude / updateNorm);
        }
    }

    update = candidateUpdate;
    return true;
}

bool mapFixedVoxelToWarpedMovingVoxel(
    float3 fixedVoxel,
    int32_t3 fixedVoxelInt,
    Texture3D<float4> displacement,
    float4x4 targetVoxelToScanner,
    float4x4 sourceScannerToVoxel,
    uint32_t3 movingDims,
    out float3 warpedMovingVoxel)
{
    let fixedScanner = mul(targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let displacementValue = displacement.Load(int32_t4(fixedVoxelInt, 0)).xyz;
    let movedScanner = fixedScanner + displacementValue;
    warpedMovingVoxel = mul(sourceScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    return inBoundsVoxel(warpedMovingVoxel, movingDims);
}

bool mapMovingVoxelToMovedFixedVoxel(
    float3 movingVoxel,
    VoxelSamplingFieldFloat4 displacementField,
    float4x4 sourceVoxelToScanner,
    float4x4 targetScannerToVoxel,
    uint32_t3 fixedDims,
    out float3 movedFixedVoxel)
{
    let movingScanner = mul(sourceVoxelToScanner, float4(movingVoxel, 1.0F)).xyz;
    let displacementSampleVoxel = mul(targetScannerToVoxel, float4(movingScanner, 1.0F)).xyz;
    if (!inBoundsVoxel(displacementSampleVoxel, fixedDims)) {
        movedFixedVoxel = float3(0.0F, 0.0F, 0.0F);
        return false;
    }

    let displacementValue = displacementField.sample(displacementSampleVoxel).xyz;
    let movedScanner = movingScanner + displacementValue;
    movedFixedVoxel = mul(targetScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    return inBoundsVoxel(movedFixedVoxel, fixedDims);
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
        if (!inBoundsIndex(sampleCoord, filterDims)) {
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
    inout LnccMomentSums sums)
{
    let radius = int32_t(kWindowRadius);

    for (int32_t dz = -radius; dz <= radius; ++dz) {
        let sliceCoord = baseCoord + int32_t3(0, 0, dz);
        if (!inBoundsIndex(sliceCoord, textureDims)) {
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
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccUpdatePrepareRawMomentsForward(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float> fixedMask,
    Texture3D<float> movingMask,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    ConstantBuffer<UpdateSlabUniforms> slabUniforms,
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
    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let fixedMaskField = VoxelSamplingField(fixedMask, linearSampler);
    let movingMaskField = VoxelSamplingField(movingMask, linearSampler);

    let fixedVoxel = float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z));
    if (!fixedMaskField.maskAccepts(fixedVoxel, kUseFixedMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    var warpedMovingVoxel = float3(0.0F, 0.0F, 0.0F);
    if (!mapFixedVoxelToWarpedMovingVoxel(
            fixedVoxel,
            globalCoord,
            displacement,
            voxelScannerMatrices.targetVoxelToScanner,
            voxelScannerMatrices.sourceScannerToVoxel,
            movingDims,
            warpedMovingVoxel)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    if (!movingMaskField.maskAccepts(warpedMovingVoxel, kUseMovingMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let referenceValue = fixedField.sampleWithZeroBorder(fixedVoxel);
    let movingValue = movingField.sampleWithZeroBorder(warpedMovingVoxel);

    let R = referenceValue;
    let M = movingValue;
    let R2 = R * R;
    let M2 = M * M;
    let RM = R * M;
    let W = 1.0F;

    outputMoments1.Store(globalId, float4(R, M, R2, M2));
    outputMoments2.Store(globalId, float4(RM, W, 0.0F, 0.0F));
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccUpdatePrepareRawMomentsBackward(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float> fixedMask,
    Texture3D<float> movingMask,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    ConstantBuffer<UpdateSlabUniforms> slabUniforms,
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
    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let fixedMaskField = VoxelSamplingField(fixedMask, linearSampler);
    let movingMaskField = VoxelSamplingField(movingMask, linearSampler);
    let displacementField = VoxelSamplingFieldFloat4(displacement, linearSampler);

    let movingVoxel = float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z));
    if (!movingMaskField.maskAccepts(movingVoxel, kUseMovingMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    var movedFixedVoxel = float3(0.0F, 0.0F, 0.0F);
    if (!mapMovingVoxelToMovedFixedVoxel(
            movingVoxel,
            displacementField,
            voxelScannerMatrices.sourceVoxelToScanner,
            voxelScannerMatrices.targetScannerToVoxel,
            fixedDims,
            movedFixedVoxel)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    if (!fixedMaskField.maskAccepts(movedFixedVoxel, kUseFixedMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let referenceValue = movingField.sampleWithZeroBorder(movingVoxel);
    let movingValue = fixedField.sampleWithZeroBorder(movedFixedVoxel);

    let R = referenceValue;
    let M = movingValue;
    let R2 = R * R;
    let M2 = M * M;
    let RM = R * M;
    let W = 1.0F;

    outputMoments1.Store(globalId, float4(R, M, R2, M2));
    outputMoments2.Store(globalId, float4(RM, W, 0.0F, 0.0F));
}

[shader("compute")]
[numthreads(16, 4, 1)]
void lnccUpdateFilterXPass(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> latticeImage, // only used for dimensions
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(inputMoments1);
    let filterDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    lnccFilterAlongAxis(globalId,
                        filterDims,
                        int32_t3(1, 0, 0),
                        inputMoments1,
                        inputMoments2,
                        outputMoments1,
                        outputMoments2);
}

[shader("compute")]
[numthreads(4, 16, 1)]
void lnccUpdateFilterYPass(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> latticeImage, // only used for dimensions
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2)
{
    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(inputMoments1);
    let filterDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    lnccFilterAlongAxis(globalId,
                        filterDims,
                        int32_t3(0, 1, 0),
                        inputMoments1,
                        inputMoments2,
                        outputMoments1,
                        outputMoments2);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccUpdateForwardReduceZ(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> latticeImage,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float4> displacement,
    ConstantBuffer<UpdateSlabUniforms> slabUniforms,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    Texture3D<float4> xyFilteredMoments1,
    Texture3D<float4> xyFilteredMoments2,
    WTexture3D<float4> outputUpdate)
{
    static_assert(kWindowRadius > 0U, "LNCC update doesn't make sense for a window radius of 0.");

    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(xyFilteredMoments1);
    let reduceDims = uint32_t3(latticeDims.x, latticeDims.y, slabUniforms.reduceChunkDepth);
    if (!inBoundsIndex(int32_t3(globalId), reduceDims)) {
        return;
    }

    let globalZ = slabUniforms.slabInputZStart + int32_t(kWindowRadius) + int32_t(globalId.z);
    if (globalZ < 0 || globalZ >= int32_t(latticeDims.z)) {
        return;
    }
    let globalCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), globalZ);
    let outputCoord = uint32_t3(globalId.x, globalId.y, uint32_t(globalZ));

    let movingDims = textureSize(movingImage);
    var centerWarpedMovingVoxel = float3(0.0F, 0.0F, 0.0F);
    if (!mapFixedVoxelToWarpedMovingVoxel(
            float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z)),
            globalCoord,
            displacement,
            voxelScannerMatrices.targetVoxelToScanner,
            voxelScannerMatrices.sourceScannerToVoxel,
            movingDims,
            centerWarpedMovingVoxel)) {
        outputUpdate.Store(outputCoord, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let slabDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    let slabCenterCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), int32_t(globalId.z) + int32_t(kWindowRadius));
    var sums = initLnccMomentSums();
    lnccAccumulateZSlabs(slabCenterCoord,
                         slabDims,
                         xyFilteredMoments1,
                         xyFilteredMoments2,
                         sums);

    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let referenceValue = fixedImage.Load(int32_t4(globalCoord, 0));
    let movingValue = movingField.sampleWithZeroBorder(centerWarpedMovingVoxel);
    let gradMovingVoxel = movingField.spatialGradient(centerWarpedMovingVoxel);
    let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;

    var update = float3(0.0F, 0.0F, 0.0F);
    if (!solveLnccUpdate(sums, referenceValue, movingValue, gradMovingScanner, update)) {
        outputUpdate.Store(outputCoord, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    outputUpdate.Store(outputCoord, float4(update, 0.0F));
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lnccUpdateBackwardReduceZ(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> latticeImage,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler,
    ConstantBuffer<UpdateSlabUniforms> slabUniforms,
    Texture3D<float4> xyFilteredMoments1,
    Texture3D<float4> xyFilteredMoments2,
    WTexture3D<float4> outputUpdate)
{
    static_assert(kWindowRadius > 0U, "LNCC update doesn't make sense for a window radius of 0.");

    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(xyFilteredMoments1);
    let reduceDims = uint32_t3(latticeDims.x, latticeDims.y, slabUniforms.reduceChunkDepth);
    if (!inBoundsIndex(int32_t3(globalId), reduceDims)) {
        return;
    }

    let globalZ = slabUniforms.slabInputZStart + int32_t(kWindowRadius) + int32_t(globalId.z);
    if (globalZ < 0 || globalZ >= int32_t(latticeDims.z)) {
        return;
    }
    let globalCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), globalZ);
    let outputCoord = uint32_t3(globalId.x, globalId.y, uint32_t(globalZ));

    let fixedDims = textureSize(fixedImage);
    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let displacementField = VoxelSamplingFieldFloat4(displacement, linearSampler);
    var centerMovedFixedVoxel = float3(0.0F, 0.0F, 0.0F);
    if (!mapMovingVoxelToMovedFixedVoxel(
            float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z)),
            displacementField,
            voxelScannerMatrices.sourceVoxelToScanner,
            voxelScannerMatrices.targetScannerToVoxel,
            fixedDims,
            centerMovedFixedVoxel)) {
        outputUpdate.Store(outputCoord, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let slabDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    let slabCenterCoord = int32_t3(int32_t(globalId.x), int32_t(globalId.y), int32_t(globalId.z) + int32_t(kWindowRadius));
    var sums = initLnccMomentSums();
    lnccAccumulateZSlabs(slabCenterCoord,
                         slabDims,
                         xyFilteredMoments1,
                         xyFilteredMoments2,
                         sums);

    let referenceValue = movingField.sampleWithZeroBorder(float3(float(globalCoord.x), float(globalCoord.y), float(globalCoord.z)));
    let movingValue = fixedField.sampleWithZeroBorder(centerMovedFixedVoxel);
    let gradMovingVoxel = fixedField.spatialGradient(centerMovedFixedVoxel);
    let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;

    var update = float3(0.0F, 0.0F, 0.0F);
    if (!solveLnccUpdate(sums, referenceValue, movingValue, gradMovingScanner, update)) {
        outputUpdate.Store(outputCoord, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    outputUpdate.Store(outputCoord, float4(update, 0.0F));
}
