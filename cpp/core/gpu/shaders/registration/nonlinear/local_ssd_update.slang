// Shader that compute a per-voxel update from SSD metric
// At each voxel x, we compute:
// - residual r(x) = I_fixed(x) - Warped_moving(x)
// - gradient of moving image at warped location: g(x) = grad(Warped_moving)(x)
// - update u(x) = (r(x) * g(x))/(|g(x)|^2 + alpha * r(x)^2 + epsilon)
// where alpha and epsilon are regularisation parameters to avoid instability in low-gradient regions.

// See Tom Vercauteren, Xavier Pennec, Aymeric Perchant, Nicholas Ayache, “Diffeomorphic Demons: Efficient Non-parametric Image Registration,” NeuroImage (2008)
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
    let dims = textureSize(fixedImage);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let fixedValue = fixedImage.Load(int32_t4(globalId, 0)).r;
    let disp = displacement.Load(int32_t4(globalId, 0)).xyz;
    let fixedVoxel = float3(globalId);
    let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let movingScanner = fixedScanner + disp;
    let movingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(movingScanner, 1.0F)).xyz;

    let movingDim = textureSize(movingImage);
    if (movingVoxel.x < 0.0F || movingVoxel.y < 0.0F || movingVoxel.z < 0.0F ||
        movingVoxel.x > float(movingDim.x - 1) ||
        movingVoxel.y > float(movingDim.y - 1) ||
        movingVoxel.z > float(movingDim.z - 1))
    {
        outputUpdate.Store(uint32_t3(globalId), float4(0.0F, 0.0F, 0.0F, 0.0F));
        return;
    }

    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let movingValue = movingField.sampleWithZeroBorder(movingVoxel);
    let residual = fixedValue - movingValue;

    let gradVoxel = movingField.spatialGradient(movingVoxel);
    let gradScanner = mul(float4(gradVoxel, 0.0F), voxelScannerMatrices.sourceScannerToVoxel).xyz;
    let gradNormSq = dot(gradScanner, gradScanner);
    let denom = gradNormSq + kAlpha * residual * residual + kEpsilon;
    var update = (residual * gradScanner) / denom;

    if (kMaxUpdateMagnitude > 0.0F) {
        let updateNorm = length(update);
        if (updateNorm > kMaxUpdateMagnitude) {
            update = update * (kMaxUpdateMagnitude / updateNorm);
        }
    }

    outputUpdate.Store(uint32_t3(globalId), float4(update, 0.0F));
}
