// Compute a per-voxel asymmetric LNCC update for one symmetric log-demons direction.
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
// Forward direction (kSymmetricDirection = 0):
//   F = fixed(x), M = moving(exp(v)(x))
// Backward direction (kSymmetricDirection = 1):
//   F = moving(x), M = fixed(exp(-v)(x))
// The output update is written in scanner-space units (mm) on the fixed lattice.
// TODO: write a full derivation of the update formula. The formula is basically the same as in
// LCC-Demons: A robust and accurate symmetric diffeomorphic registration algorithm by Lorenzi et al,
// except that it is not symmetric and doesn't require a midway space.
// See Appendix B of the paper.

import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kSymmetricDirection;
extern static const uint32_t kWindowRadius = 2U;
extern static const float kSigmaRatio;
extern static const float kMomentEpsilon;
extern static const float kRhoEpsilon;
extern static const float kDenominatorEpsilon;
extern static const float kMaxUpdateMagnitude = 0.0F;

bool inBoundsVoxel(float3 voxel, uint32_t3 dims)
{
    return voxel.x >= 0.0F && voxel.y >= 0.0F && voxel.z >= 0.0F &&
           voxel.x <= float(dims.x - 1U) && voxel.y <= float(dims.y - 1U) && voxel.z <= float(dims.z - 1U);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
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

    let directionBackward = (kSymmetricDirection != 0U);
    let movingDims = textureSize(movingImage);
    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);

    let centerFixedVoxel = float3(globalId);
    let centerFixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(centerFixedVoxel, 1.0F)).xyz;
    let centerDisplacement = displacement.Load(int32_t4(globalId, 0)).xyz;
    let centerMovedScanner = centerFixedScanner + centerDisplacement;
    let centerMovedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(centerMovedScanner, 1.0F)).xyz;
    let centerMovedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(centerMovedScanner, 1.0F)).xyz;

    let centerDirectionInBounds = directionBackward
        ? inBoundsVoxel(centerMovedFixedVoxel, fixedDims)
        : inBoundsVoxel(centerMovedMovingVoxel, movingDims);
    if (!centerDirectionInBounds) {
        outputUpdate.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let radius = int32_t(kWindowRadius);
    var sumReference : float = 0.0F;
    var sumMoving : float = 0.0F;
    var sumReferenceSquared : float = 0.0F;
    var sumMovingSquared : float = 0.0F;
    var sumReferenceMoving : float = 0.0F;
    var sumGradMoving : float3 = float3(0.0F, 0.0F, 0.0F);
    var sumReferenceGradMoving : float3 = float3(0.0F, 0.0F, 0.0F);
    var sumMovingGradMoving : float3 = float3(0.0F, 0.0F, 0.0F);
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
                let neighborFixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(neighborFixedVoxel, 1.0F)).xyz;
                let neighborDisplacement = displacement.Load(int32_t4(neighborInt, 0)).xyz;
                let neighborMovedScanner = neighborFixedScanner + neighborDisplacement;
                let neighborMovedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(neighborMovedScanner, 1.0F)).xyz;
                let neighborMovedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(neighborMovedScanner, 1.0F)).xyz;
                let neighborBaseMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(neighborFixedScanner, 1.0F)).xyz;

                let directionInBounds = directionBackward ? inBoundsVoxel(neighborMovedFixedVoxel, fixedDims) : inBoundsVoxel(neighborMovedMovingVoxel, movingDims);
                if (!directionInBounds) {
                    continue;
                }

                var referenceValue : float;
                var movingValue : float;
                var gradMovingScanner : float3;
                if (directionBackward) {
                    referenceValue = movingField.sampleWithZeroBorder(neighborBaseMovingVoxel);
                    movingValue = fixedField.sampleWithZeroBorder(neighborMovedFixedVoxel);
                    let gradMovingVoxel = fixedField.spatialGradient(neighborMovedFixedVoxel);
                    gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;
                } else {
                    referenceValue = fixedField.sampleWithZeroBorder(neighborFixedVoxel);
                    movingValue = movingField.sampleWithZeroBorder(neighborMovedMovingVoxel);
                    let gradMovingVoxel = movingField.spatialGradient(neighborMovedMovingVoxel);
                    gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;
                }

                sumReference += referenceValue;
                sumMoving += movingValue;
                sumReferenceSquared += referenceValue * referenceValue;
                sumMovingSquared += movingValue * movingValue;
                sumReferenceMoving += referenceValue * movingValue;
                sumGradMoving += gradMovingScanner;
                sumReferenceGradMoving += referenceValue * gradMovingScanner;
                sumMovingGradMoving += movingValue * gradMovingScanner;
                validSamples += 1.0F;
            }
        }
    }

    if (!(validSamples > 0.0F)) {
        outputUpdate.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let invN = 1.0F / validSamples;
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
        outputUpdate.Store(globalId, float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    var update = -lambda / denominator;
    if (kMaxUpdateMagnitude > 0.0F) {
        let updateNorm = length(update);
        if (updateNorm > kMaxUpdateMagnitude) {
            update = update * (kMaxUpdateMagnitude / updateNorm);
        }
    }

    outputUpdate.Store(globalId, float4(update, 0.0F));
}
