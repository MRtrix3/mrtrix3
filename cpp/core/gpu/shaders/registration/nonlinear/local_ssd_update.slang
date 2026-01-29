// Shader that compute a per-voxel update from SSD metric
// At each voxel x, we compute:
// - residual r(x) = I_fixed(x) - Warped_moving(x)
// - gradient of moving image at warped location: g(x) = grad(Warped_moving)(x)
// - update u(x) = (r(x) * g(x))/(|g(x)|^2 + alpha * r(x)^2 + epsilon)
// where alpha and epsilon are regularisation parameters to avoid instability in low-gradient regions.

// See Tom Vercauteren, Xavier Pennec, Aymeric Perchant, Nicholas Ayache, “Diffeomorphic Demons: Efficient Non-parametric Image Registration,” NeuroImage (2008)
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kAlpha;
extern static const float kEpsilon;
// Controls maximum allowed magnitude of update vector; set to 0.0 to disable
extern static const float kMaxUpdateMagnitude = 0.0F;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> fixedImage,
    Texture3D<float> movingImage,
    Texture3D<float4> displacement,
    WTexture3D<float4> outputUpdate,
    SamplerState linearSampler)
{
    let dims = textureSize(fixedImage);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let fixedValue = fixedImage.Load(int32_t4(globalId, 0)).r;
    let disp = displacement.Load(int32_t4(globalId, 0)).xyz;
    // TODO: what happens if warpedCoord is out of bounds?
    let warpedCoord = float3(globalId) + disp;

    let movingField = VoxelSamplingField(movingImage, linearSampler);
    let movingValue = movingField.sample(warpedCoord);
    let residual = fixedValue - movingValue;

    let grad = movingField.spatialGradient(warpedCoord);
    let gradNormSq = dot(grad, grad);
    let denom = gradNormSq + kAlpha * residual * residual + kEpsilon;
    var update = (residual * grad) / denom;

    if (kMaxUpdateMagnitude > 0.0F) {
        let updateNorm = length(update);
        if (updateNorm > kMaxUpdateMagnitude) {
            update = update * (kMaxUpdateMagnitude / updateNorm);
        }
    }

    outputUpdate.Store(uint32_t3(globalId), float4(update, 0.0F));
}
