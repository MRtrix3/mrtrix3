// Compute a per-voxel symmetric log-demons update from SSD terms.
// For each voxel x, the update is computed as
//   u(x) = (r(x) * g_sym(x)) / (|g_sym(x)|^2 + alpha * r(x)^2 + epsilon)
// where r(x) is the residual and g_sym(x) is the symmetric gradient,
// Forward direction (kSymmetricDirection = 0):
//   r_fwd(x) = I_fixed(x) - I_moving(exp(v)(x))
//   g_fwd(x) = 0.5 * ( grad(I_fixed)(x) + grad(I_moving)(exp(v)(x)) )
// Backward direction (kSymmetricDirection = 1):
//   r_bwd(x) = I_moving(x) - I_fixed(exp(-v)(x))
//   g_bwd(x) = 0.5 * ( grad(I_moving)(x) + grad(I_fixed)(exp(-v)(x)) )

// NOTE: minus sign in the residual is flipped between forward and backward directions.
// The output update is written in scanner-space units (mm) on the fixed lattice.

import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kSymmetricDirection;
extern static const float kAlpha;
extern static const float kEpsilon;
// Controls maximum allowed magnitude of update vector; set to 0.0 to disable.
// Units are mm in scanner space.
extern static const float kMaxUpdateMagnitude = 0.0F;

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
    if (globalId.x >= fixedDims.x || globalId.y >= fixedDims.y || globalId.z >= fixedDims.z)
        return;

    let fixedVoxel = float3(globalId);
    let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let displacementValue = displacement.Load(int32_t4(globalId, 0)).xyz;
    let movedScanner = fixedScanner + displacementValue;

    let movedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    let movedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    let baseMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(fixedScanner, 1.0F)).xyz;

    let movingDims = textureSize(movingImage);
    let inBoundsMovedMoving = movedMovingVoxel.x >= 0.0F && movedMovingVoxel.y >= 0.0F &&
                              movedMovingVoxel.z >= 0.0F && movedMovingVoxel.x <= float(movingDims.x - 1) &&
                              movedMovingVoxel.y <= float(movingDims.y - 1) &&
                              movedMovingVoxel.z <= float(movingDims.z - 1);
    let inBoundsMovedFixed = movedFixedVoxel.x >= 0.0F && movedFixedVoxel.y >= 0.0F &&
                             movedFixedVoxel.z >= 0.0F && movedFixedVoxel.x <= float(fixedDims.x - 1) &&
                             movedFixedVoxel.y <= float(fixedDims.y - 1) &&
                             movedFixedVoxel.z <= float(fixedDims.z - 1);

    let directionBackward = (kSymmetricDirection != 0U);
    let directionInBounds = directionBackward ? inBoundsMovedFixed : inBoundsMovedMoving;
    if (!directionInBounds) {
        outputUpdate.Store(uint32_t3(globalId), float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);

    let fixedValue = fixedField.sampleWithZeroBorder(fixedVoxel);
    let movingValueMoved = movingField.sampleWithZeroBorder(movedMovingVoxel);
    let movingValueBase = movingField.sampleWithZeroBorder(baseMovingVoxel);
    let fixedValueMoved = fixedField.sampleWithZeroBorder(movedFixedVoxel);

    let gradFixedBaseVoxel = fixedField.spatialGradient(fixedVoxel);
    let gradFixedMovedVoxel = fixedField.spatialGradient(movedFixedVoxel);
    let gradMovingBaseVoxel = movingField.spatialGradient(baseMovingVoxel);
    let gradMovingMovedVoxel = movingField.spatialGradient(movedMovingVoxel);

    let gradFixedBaseScanner = mul(float4(gradFixedBaseVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;
    let gradFixedMovedScanner = mul(float4(gradFixedMovedVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;
    let gradMovingBaseScanner = mul(float4(gradMovingBaseVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;
    let gradMovingMovedScanner = mul(float4(gradMovingMovedVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;

    let residual = directionBackward ? (movingValueBase - fixedValueMoved) : (fixedValue - movingValueMoved);
    let gradSymmetric = directionBackward ? (0.5F * (gradMovingBaseScanner + gradFixedMovedScanner))
                                          : (0.5F * (gradFixedBaseScanner + gradMovingMovedScanner));
    let gradNormSq = dot(gradSymmetric, gradSymmetric);
    let denom = gradNormSq + kAlpha * residual * residual + kEpsilon;
    var update = (residual * gradSymmetric) / denom;

    if (kMaxUpdateMagnitude > 0.0F) {
        let updateNorm = length(update);
        if (updateNorm > kMaxUpdateMagnitude) {
            update = update * (kMaxUpdateMagnitude / updateNorm);
        }
    }

    outputUpdate.Store(uint32_t3(globalId), float4(update, 0.0F));
}
