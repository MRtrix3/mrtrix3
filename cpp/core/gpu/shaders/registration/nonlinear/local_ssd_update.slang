// Compute a per-voxel symmetric log-demons update from SSD terms.
// For each voxel x, the update is computed as
//   u(x) = (r(x) * g_sym(x)) / (|g_sym(x)|^2 + alpha * r(x)^2 + epsilon)
// where r(x) is the residual and g_sym(x) is the symmetric gradient.
//
// Forward direction (ssd_update_forward, fixed lattice):
//   r_fwd(x_f) = I_fixed(x_f) - I_moving(exp(v)(x_f))
//   g_fwd(x_f) = 0.5 * ( grad(I_fixed)(x_f) + grad(I_moving)(exp(v)(x_f)) )
//
// Backward direction (ssd_update_backward, moving lattice):
//   r_bwd(x_m) = I_moving(x_m) - I_fixed(exp(-v)(x_m))
//   g_bwd(x_m) = 0.5 * ( grad(I_moving)(x_m) + grad(I_fixed)(exp(-v)(x_m)) )
//
// The output update vectors are written in scanner-space units (mm).

import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kAlpha;
extern static const float kEpsilon;
// Controls maximum allowed magnitude of update vector; set to 0.0 to disable.
// Units are mm in scanner space.
extern static const float kMaxUpdateMagnitude = 0.0F;

void writeZeroUpdate(WTexture3D<float4> outputUpdate, uint32_t3 voxelId)
{
    outputUpdate.Store(voxelId, float4(0.0F, 0.0F, 0.0F, 0.0F));
}

float3 solveSsdUpdate(float residual, float3 symmetricGradient)
{
    let gradNormSq = dot(symmetricGradient, symmetricGradient);
    let denominator = gradNormSq + kAlpha * residual * residual + kEpsilon;
    var update = (residual * symmetricGradient) / denominator;
    if (kMaxUpdateMagnitude > 0.0F) {
        let updateNorm = length(update);
        if (updateNorm > kMaxUpdateMagnitude) {
            update = update * (kMaxUpdateMagnitude / updateNorm);
        }
    }
    return update;
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void ssd_update_forward(
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
    let fixedVoxel = float3(globalId);
    let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let displacementValue = displacement.Load(int32_t4(globalId, 0)).xyz;
    let movedScanner = fixedScanner + displacementValue;
    let movedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    if (!inBoundsVoxel(movedMovingVoxel, movingDims)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    let fixedField = VoxelSamplingField(fixedImage, linearSampler);
    let movingField = VoxelSamplingField(movingImage, linearSampler);

    let fixedValue = fixedField.sampleWithZeroBorder(fixedVoxel);
    let movingValueMoved = movingField.sampleWithZeroBorder(movedMovingVoxel);
    let gradFixedVoxel = fixedField.spatialGradient(fixedVoxel);
    let gradMovingVoxel = movingField.spatialGradient(movedMovingVoxel);
    let gradFixedScanner = mul(float4(gradFixedVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;
    let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;

    let residual = fixedValue - movingValueMoved;
    let symmetricGradient = 0.5F * (gradFixedScanner + gradMovingScanner);
    let update = solveSsdUpdate(residual, symmetricGradient);
    outputUpdate.Store(globalId, float4(update, 0.0F));
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void ssd_update_backward(
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

    let movingVoxel = float3(globalId);
    let movingScanner = mul(voxelScannerMatrices.sourceVoxelToScanner, float4(movingVoxel, 1.0F)).xyz;
    let displacementSampleVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(movingScanner, 1.0F)).xyz;
    if (!inBoundsVoxel(displacementSampleVoxel, fixedDims)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    let displacementValue = displacementField.sample(displacementSampleVoxel).xyz;
    let movedScanner = movingScanner + displacementValue;
    let movedFixedVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(movedScanner, 1.0F)).xyz;
    if (!inBoundsVoxel(movedFixedVoxel, fixedDims)) {
        writeZeroUpdate(outputUpdate, globalId);
        return;
    }

    let movingValue = movingField.sampleWithZeroBorder(movingVoxel);
    let fixedValueMoved = fixedField.sampleWithZeroBorder(movedFixedVoxel);
    let gradMovingVoxel = movingField.spatialGradient(movingVoxel);
    let gradFixedMovedVoxel = fixedField.spatialGradient(movedFixedVoxel);
    let gradMovingScanner = mul(float4(gradMovingVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;
    let gradFixedScanner = mul(float4(gradFixedMovedVoxel, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;

    let residual = movingValue - fixedValueMoved;
    let symmetricGradient = 0.5F * (gradMovingScanner + gradFixedScanner);
    let update = solveSsdUpdate(residual, symmetricGradient);
    outputUpdate.Store(globalId, float4(update, 0.0F));
}
