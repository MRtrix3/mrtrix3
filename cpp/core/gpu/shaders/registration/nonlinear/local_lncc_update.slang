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
// Entry points:
// - lncc_update_forward: forward update on the fixed lattice.
// - lncc_update_backward: backward update on the moving lattice.
// Both updates are represented in scanner-space units (mm).
// TODO: write a full derivation of the update formula. The formula is basically the same as in
// LCC-Demons: A robust and accurate symmetric diffeomorphic registration algorithm by Lorenzi et al,
// except that it is not symmetric and doesn't require a midway space.
// See Appendix B of the paper.

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

void writeZeroUpdate(WTexture3D<float4> outputUpdate, uint32_t3 voxelId)
{
    outputUpdate.Store(voxelId, float4(0.0F, 0.0F, 0.0F, 0.0F));
}

void writeUpdate(WTexture3D<float4> outputUpdate, uint32_t3 voxelId, float3 update)
{
    outputUpdate.Store(voxelId, float4(update, 0.0F));
}

struct LnccWeightedSums
{
    float sumReference = 0.0F;
    float sumMoving = 0.0F;
    float sumReferenceSquared = 0.0F;
    float sumMovingSquared = 0.0F;
    float sumReferenceMoving = 0.0F;
    float3 sumGradMoving = float3(0.0F, 0.0F, 0.0F);
    float3 sumReferenceGradMoving = float3(0.0F, 0.0F, 0.0F);
    float3 sumMovingGradMoving = float3(0.0F, 0.0F, 0.0F);
    float sumWeights = 0.0F;
    
    [mutating]
    void accumulate(
        float referenceValue,
        float movingValue,
        float3 gradMovingScanner,
        float weight)
    {
        sumReference += weight * referenceValue;
        sumMoving += weight * movingValue;
        sumReferenceSquared += weight * referenceValue * referenceValue;
        sumMovingSquared += weight * movingValue * movingValue;
        sumReferenceMoving += weight * referenceValue * movingValue;
        sumGradMoving += weight * gradMovingScanner;
        sumReferenceGradMoving += weight * referenceValue * gradMovingScanner;
        sumMovingGradMoving += weight * movingValue * gradMovingScanner;
        sumWeights += weight;
    }

    bool solve(out float3 update)
    {
        if (!(sumWeights > 0.0F)) {
            update = float3(0.0F, 0.0F, 0.0F);
            return false;
        }

        let invN = 1.0F / sumWeights;
        let meanReference = sumReference * invN;
        let meanMoving = sumMoving * invN;
        let meanGradMoving = sumGradMoving * invN;

        let covariance = sumReferenceMoving * invN - meanReference * meanMoving;
        let varianceReference = max(0.0F, sumReferenceSquared * invN - meanReference * meanReference);
        let varianceMoving = max(0.0F, sumMovingSquared * invN - meanMoving * meanMoving);

        let covarianceEps = covariance + kMomentEpsilon;
        let varianceReferenceEps = varianceReference + kMomentEpsilon;
        let varianceMovingEps = varianceMoving + kMomentEpsilon;
        let rho = covarianceEps / sqrt(max(varianceReferenceEps * varianceMovingEps, kMomentEpsilon));
        let rhoSq = max(rho * rho, kRhoEpsilon);

        let centeredReferenceGrad = sumReferenceGradMoving * invN - meanReference * meanGradMoving;
        let centeredMovingGrad = sumMovingGradMoving * invN - meanMoving * meanGradMoving;
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
};

bool mapFixedVoxelToMovedMovingVoxel(
    float3 fixedVoxel,
    int32_t3 fixedVoxelInt,
    Texture3D<float4> displacement,
    float4x4 targetVoxelToScanner,
    float4x4 sourceScannerToVoxel,
    uint32_t3 movingDims,
    out float3 movedMovingVoxel)
{
    let fixedScanner = mul(targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let displacementValue = displacement.Load(int32_t4(fixedVoxelInt, 0)).xyz;
    let movedScanner = fixedScanner + displacementValue;
    movedMovingVoxel = mul(sourceScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    return inBoundsVoxel(movedMovingVoxel, movingDims);
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

void accumulateForwardLnccWindow(
    uint32_t3 centerId,
    uint32_t3 fixedDims,
    uint32_t3 movingDims,
    VoxelSamplingField fixedField,
    VoxelSamplingField movingField,
    Texture3D<float4> displacement,
    float4x4 targetVoxelToScanner,
    float4x4 sourceScannerToVoxel,
    inout LnccWeightedSums sums)
{
    let radius = int32_t(kWindowRadius);

    for (int32_t dz = -radius; dz <= radius; ++dz) {
        for (int32_t dy = -radius; dy <= radius; ++dy) {
            for (int32_t dx = -radius; dx <= radius; ++dx) {
                let neighborInt = int32_t3(centerId) + int32_t3(dx, dy, dz);
                if (!inBoundsIndex(neighborInt, fixedDims)) {
                    continue;
                }

                let neighborFixedVoxel = float3(neighborInt);
                var neighborMovedMovingVoxel = float3(0.0F, 0.0F, 0.0F);
                if (!mapFixedVoxelToMovedMovingVoxel(
                        neighborFixedVoxel,
                        neighborInt,
                        displacement,
                        targetVoxelToScanner,
                        sourceScannerToVoxel,
                        movingDims,
                        neighborMovedMovingVoxel)) {
                    continue;
                }

                let referenceValue = fixedField.sampleWithZeroBorder(neighborFixedVoxel);
                let movingValue = movingField.sampleWithZeroBorder(neighborMovedMovingVoxel);
                let gradMovingVoxel = movingField.spatialGradient(neighborMovedMovingVoxel);
                let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), sourceScannerToVoxel).xyz;

                sums.accumulate(referenceValue, movingValue, gradMovingScanner, 1.0F);
            }
        }
    }
}

void accumulateBackwardLnccWindow(
    uint32_t3 centerId,
    uint32_t3 movingDims,
    uint32_t3 fixedDims,
    VoxelSamplingField fixedField,
    VoxelSamplingField movingField,
    VoxelSamplingFieldFloat4 displacementField,
    float4x4 sourceVoxelToScanner,
    float4x4 targetScannerToVoxel,
    inout LnccWeightedSums sums)
{
    let radius = int32_t(kWindowRadius);

    for (int32_t dz = -radius; dz <= radius; ++dz) {
        for (int32_t dy = -radius; dy <= radius; ++dy) {
            for (int32_t dx = -radius; dx <= radius; ++dx) {
                let neighborInt = int32_t3(centerId) + int32_t3(dx, dy, dz);
                if (!inBoundsIndex(neighborInt, movingDims)) {
                    continue;
                }

                let neighborMovingVoxel = float3(neighborInt);
                var neighborMovedFixedVoxel = float3(0.0F, 0.0F, 0.0F);
                if (!mapMovingVoxelToMovedFixedVoxel(
                        neighborMovingVoxel,
                        displacementField,
                        sourceVoxelToScanner,
                        targetScannerToVoxel,
                        fixedDims,
                        neighborMovedFixedVoxel)) {
                    continue;
                }

                let referenceValue = movingField.sampleWithZeroBorder(neighborMovingVoxel);
                let movingValue = fixedField.sampleWithZeroBorder(neighborMovedFixedVoxel);
                let gradMovingVoxel = fixedField.spatialGradient(neighborMovedFixedVoxel);
                let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), targetScannerToVoxel).xyz;

                sums.accumulate(referenceValue, movingValue, gradMovingScanner, 1.0F);
            }
        }
    }
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lncc_update_forward(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float4> displacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    WTexture3D<float4> outputUpdate,
    SamplerState linearSampler)
{
    let fixedDims = textureSize(fixedImage);
    if (globalId.x >= fixedDims.x || globalId.y >= fixedDims.y || globalId.z >= fixedDims.z) {
        return;
    }

    let movingDims = textureSize(movingImage);
    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let targetVoxelToScanner = voxelScannerMatrices.targetVoxelToScanner;
    let sourceScannerToVoxel = voxelScannerMatrices.sourceScannerToVoxel;

    var centerMovedMovingVoxel = float3(0.0F, 0.0F, 0.0F);
    if (!mapFixedVoxelToMovedMovingVoxel(
            float3(globalId),
            int32_t3(globalId),
            displacement,
            targetVoxelToScanner,
            sourceScannerToVoxel,
            movingDims,
            centerMovedMovingVoxel)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    var sums : LnccWeightedSums;
    accumulateForwardLnccWindow(
        globalId,
        fixedDims,
        movingDims,
        fixedField,
        movingField,
        displacement,
        targetVoxelToScanner,
        sourceScannerToVoxel,
        sums);

    var update = float3(0.0F, 0.0F, 0.0F);
    if (!sums.solve(update)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    writeUpdate(outputUpdate, globalId, update);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lncc_update_backward(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float4> displacementBackward,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    WTexture3D<float4> outputUpdate,
    SamplerState linearSampler)
{
    let movingDims = textureSize(movingImage);
    if (globalId.x >= movingDims.x || globalId.y >= movingDims.y || globalId.z >= movingDims.z) {
        return;
    }

    let fixedDims = textureSize(fixedImage);
    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let displacementField = VoxelSamplingFieldFloat4(displacementBackward, linearSampler);
    let sourceVoxelToScanner = voxelScannerMatrices.sourceVoxelToScanner;
    let targetScannerToVoxel = voxelScannerMatrices.targetScannerToVoxel;

    var centerMovedFixedVoxel = float3(0.0F, 0.0F, 0.0F);
    if (!mapMovingVoxelToMovedFixedVoxel(
            float3(globalId),
            displacementField,
            sourceVoxelToScanner,
            targetScannerToVoxel,
            fixedDims,
            centerMovedFixedVoxel)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    var sums : LnccWeightedSums;
    accumulateBackwardLnccWindow(
        globalId,
        movingDims,
        fixedDims,
        fixedField,
        movingField,
        displacementField,
        sourceVoxelToScanner,
        targetScannerToVoxel,
        sums);

    var update = float3(0.0F, 0.0F, 0.0F);
    if (!sums.solve(update)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    writeUpdate(outputUpdate, globalId, update);
}
