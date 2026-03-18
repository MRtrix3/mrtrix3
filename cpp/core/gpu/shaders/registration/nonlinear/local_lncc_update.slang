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
// Compute per-voxel LNCC updates for symmetric log-demons.
// For each voxel x, use a local box window W(x).
// K[phi] is phi convolved with a uniform box kernel over W, evaluated at x.
// Define local means mu_F = K[F], mu_M = K[M], and centered signals
// F_c = F - mu_F, M_c = M - mu_M.
// Then local LNCC moments are:
//   A = K[F_c * M_c], B = K[F_c^2], C = K[M_c^2], rho = A / sqrt(B * C)
// where M is the direction-specific moving term and grad(M_c) is its spatial gradient.
// The LNCC force is:
//   lambda = K[F_c * grad(M_c)] / A - K[M_c * grad(M_c)] / C
//   u(x) = -lambda / (|lambda|^2 - (sigma_i^2 / (rho^2 * sigma_x^2)))
// where kSigmaRatio = sigma_i / sigma_x.
// Both updates are represented in scanner-space units (mm).

import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kWindowRadius = 2U;
extern static const float kSigmaRatio;
extern static const float kMomentEpsilon;
extern static const float kRhoEpsilon;
extern static const float kDenominatorEpsilon;
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
    float3 sumGradMoving;
    float3 sumReferenceGradMoving;
    float3 sumMovingGradMoving;
    float sumWeights;
};

LnccMomentSums initLnccMomentSums() {
    return LnccMomentSums(0.0F,
                          0.0F,
                          0.0F,
                          0.0F,
                          0.0F,
                          float3(0.0F, 0.0F, 0.0F),
                          float3(0.0F, 0.0F, 0.0F),
                          float3(0.0F, 0.0F, 0.0F),
                          0.0F);
}

bool solveLnccUpdate(LnccMomentSums sums, out float3 update) {
    if (!(sums.sumWeights > 0.0F)) {
        update = float3(0.0F, 0.0F, 0.0F);
        return false;
    }

    let invN = 1.0F / sums.sumWeights;
    let meanReference = sums.sumReference * invN;
    let meanMoving = sums.sumMoving * invN;
    let meanGradMoving = sums.sumGradMoving * invN;

    let covariance = sums.sumReferenceMoving * invN - meanReference * meanMoving;
    let varianceReference = max(0.0F, sums.sumReferenceSquared * invN - meanReference * meanReference);
    let varianceMoving = max(0.0F, sums.sumMovingSquared * invN - meanMoving * meanMoving);

    let covarianceEps = covariance + kMomentEpsilon;
    let varianceReferenceEps = varianceReference + kMomentEpsilon;
    let varianceMovingEps = varianceMoving + kMomentEpsilon;
    let rho = covarianceEps / sqrt(max(varianceReferenceEps * varianceMovingEps, kMomentEpsilon));
    let rhoSq = max(rho * rho, kRhoEpsilon);

    let centeredReferenceGrad = sums.sumReferenceGradMoving * invN - meanReference * meanGradMoving;
    let centeredMovingGrad = sums.sumMovingGradMoving * invN - meanMoving * meanGradMoving;
    let lambda = centeredReferenceGrad / covarianceEps - centeredMovingGrad / varianceMovingEps;

    let lambdaNormSq = dot(lambda, lambda);
    let sigmaRatioSq = kSigmaRatio * kSigmaRatio;
    let denominator = lambdaNormSq - sigmaRatioSq / rhoSq;
    // The closed-form solve is stable only in the locally convex regime
    // (equivalently denominator < 0 in this parameterisation).
    if (denominator >= -kDenominatorEpsilon) {
        update = float3(0.0F, 0.0F, 0.0F);
        return false;
    }

    var candidateUpdate = -lambda / denominator;
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
    Texture3D<float4> inputMoments3,
    Texture3D<float4> inputMoments4,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2,
    WTexture3D<float4> outputMoments3,
    WTexture3D<float4> outputMoments4)
{
    if (globalId.x >= filterDims.x || globalId.y >= filterDims.y || globalId.z >= filterDims.z) {
        return;
    }

    let radius = int32_t(kWindowRadius);
    let baseCoord = int32_t3(globalId);

    var sumMoments1 = float4(0.0F, 0.0F, 0.0F, 0.0F);
    var sumMoments2 = float4(0.0F, 0.0F, 0.0F, 0.0F);
    var sumMoments3 = float4(0.0F, 0.0F, 0.0F, 0.0F);
    var sumMoments4 = float4(0.0F, 0.0F, 0.0F, 0.0F);

    for (int32_t i = -radius; i <= radius; ++i) {
        let sampleCoord = baseCoord + axisOffset * i;
        if (!inBoundsIndex(sampleCoord, filterDims)) {
            continue;
        }

        let texelCoord = int32_t4(sampleCoord, 0);
        sumMoments1 += inputMoments1.Load(texelCoord);
        sumMoments2 += inputMoments2.Load(texelCoord);
        sumMoments3 += inputMoments3.Load(texelCoord);
        sumMoments4 += inputMoments4.Load(texelCoord);
    }

    outputMoments1.Store(globalId, sumMoments1);
    outputMoments2.Store(globalId, sumMoments2);
    outputMoments3.Store(globalId, sumMoments3);
    outputMoments4.Store(globalId, sumMoments4);
}

void lnccAccumulateZSlabs(
    int32_t3 baseCoord,
    uint32_t3 textureDims,
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    Texture3D<float4> inputMoments3,
    Texture3D<float4> inputMoments4,
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
        let moments3 = inputMoments3.Load(texelCoord);
        let moments4 = inputMoments4.Load(texelCoord);

        sums.sumReference += moments1.x;
        sums.sumMoving += moments1.y;
        sums.sumReferenceSquared += moments1.z;
        sums.sumMovingSquared += moments1.w;
        sums.sumReferenceMoving += moments2.x;
        sums.sumGradMoving += float3(moments2.z, moments2.w, moments3.x);
        sums.sumReferenceGradMoving += float3(moments3.y, moments3.z, moments3.w);
        sums.sumMovingGradMoving += moments4.xyz;
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
    WTexture3D<float4> outputMoments2,
    WTexture3D<float4> outputMoments3,
    WTexture3D<float4> outputMoments4)
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
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
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
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
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
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    if (!movingMaskField.maskAccepts(warpedMovingVoxel, kUseMovingMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let referenceValue = fixedField.sampleWithZeroBorder(fixedVoxel);
    let movingValue = movingField.sampleWithZeroBorder(warpedMovingVoxel);
    let gradMovingVoxel = movingField.spatialGradient(warpedMovingVoxel);
    let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;

    let R = referenceValue;
    let M = movingValue;
    let R2 = R * R;
    let M2 = M * M;
    let RM = R * M;
    let W = 1.0F;
    let RGrad = R * gradMovingScanner;
    let MGrad = M * gradMovingScanner;

    outputMoments1.Store(globalId, float4(R, M, R2, M2));
    outputMoments2.Store(globalId, float4(RM, W, gradMovingScanner.x, gradMovingScanner.y));
    outputMoments3.Store(globalId, float4(gradMovingScanner.z, RGrad.x, RGrad.y, RGrad.z));
    outputMoments4.Store(globalId, float4(MGrad.x, MGrad.y, MGrad.z, 0.0F));
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
    WTexture3D<float4> outputMoments2,
    WTexture3D<float4> outputMoments3,
    WTexture3D<float4> outputMoments4)
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
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
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
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
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
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }
    if (!fixedMaskField.maskAccepts(movedFixedVoxel, kUseFixedMask != 0U)) {
        outputMoments1.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments2.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments3.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        outputMoments4.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let referenceValue = movingField.sampleWithZeroBorder(movingVoxel);
    let movingValue = fixedField.sampleWithZeroBorder(movedFixedVoxel);
    let gradMovingVoxel = fixedField.spatialGradient(movedFixedVoxel);
    let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;

    let R = referenceValue;
    let M = movingValue;
    let R2 = R * R;
    let M2 = M * M;
    let RM = R * M;
    let W = 1.0F;
    let RGrad = R * gradMovingScanner;
    let MGrad = M * gradMovingScanner;

    outputMoments1.Store(globalId, float4(R, M, R2, M2));
    outputMoments2.Store(globalId, float4(RM, W, gradMovingScanner.x, gradMovingScanner.y));
    outputMoments3.Store(globalId, float4(gradMovingScanner.z, RGrad.x, RGrad.y, RGrad.z));
    outputMoments4.Store(globalId, float4(MGrad.x, MGrad.y, MGrad.z, 0.0F));
}

[shader("compute")]
[numthreads(16, 4, 1)]
void lnccUpdateFilterXPass(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> latticeImage, // only used for dimensions
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    Texture3D<float4> inputMoments3,
    Texture3D<float4> inputMoments4,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2,
    WTexture3D<float4> outputMoments3,
    WTexture3D<float4> outputMoments4)
{
    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(inputMoments1);
    let filterDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    lnccFilterAlongAxis(globalId,
                        filterDims,
                        int32_t3(1, 0, 0),
                        inputMoments1,
                        inputMoments2,
                        inputMoments3,
                        inputMoments4,
                        outputMoments1,
                        outputMoments2,
                        outputMoments3,
                        outputMoments4);
}

[shader("compute")]
[numthreads(4, 16, 1)]
void lnccUpdateFilterYPass(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> latticeImage, // only used for dimensions
    Texture3D<float4> inputMoments1,
    Texture3D<float4> inputMoments2,
    Texture3D<float4> inputMoments3,
    Texture3D<float4> inputMoments4,
    WTexture3D<float4> outputMoments1,
    WTexture3D<float4> outputMoments2,
    WTexture3D<float4> outputMoments3,
    WTexture3D<float4> outputMoments4)
{
    let latticeDims = textureSize(latticeImage);
    let scratchDims = textureSize(inputMoments1);
    let filterDims = uint32_t3(latticeDims.x, latticeDims.y, scratchDims.z);
    lnccFilterAlongAxis(globalId,
                        filterDims,
                        int32_t3(0, 1, 0),
                        inputMoments1,
                        inputMoments2,
                        inputMoments3,
                        inputMoments4,
                        outputMoments1,
                        outputMoments2,
                        outputMoments3,
                        outputMoments4);
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
    Texture3D<float4> xyFilteredMoments3,
    Texture3D<float4> xyFilteredMoments4,
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
                         xyFilteredMoments3,
                         xyFilteredMoments4,
                         sums);

    var update = float3(0.0F, 0.0F, 0.0F);
    if (!solveLnccUpdate(sums, update)) {
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
    Texture3D<float4> xyFilteredMoments3,
    Texture3D<float4> xyFilteredMoments4,
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
                         xyFilteredMoments3,
                         xyFilteredMoments4,
                         sums);

    var update = float3(0.0F, 0.0F, 0.0F);
    if (!solveLnccUpdate(sums, update)) {
        outputUpdate.Store(outputCoord, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    outputUpdate.Store(outputCoord, float4(update, 0.0F));
}
