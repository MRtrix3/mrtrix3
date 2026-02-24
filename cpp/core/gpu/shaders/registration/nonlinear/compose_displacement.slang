// Compose a displacement field with an update displacement: phi_new = phi o update + update.
// Both fields are in scanner space (mm).
import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputDisplacement,
    Texture3D<float4> updateDisplacement,
    WTexture3D<float4> outputDisplacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    SamplerState linearSampler)
{
    let dims = textureSize(inputDisplacement);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let fixedVoxel = float3(globalId);
    let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let update = updateDisplacement.Load(int32_t4(globalId, 0)).xyz;
    let sampleScanner = fixedScanner + update;
    let sampleVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(sampleScanner, 1.0F)).xyz;

    var warped = float3(0.0F, 0.0F, 0.0F);
    if (sampleVoxel.x >= 0.0F && sampleVoxel.y >= 0.0F && sampleVoxel.z >= 0.0F &&
        sampleVoxel.x <= float(dims.x - 1) &&
        sampleVoxel.y <= float(dims.y - 1) &&
        sampleVoxel.z <= float(dims.z - 1)){
        let field = VoxelSamplingFieldFloat4(inputDisplacement, linearSampler);
        warped = field.sample(sampleVoxel).xyz;
    }

    let newDisplacement = update + warped;
    outputDisplacement.Store(uint32_t3(globalId), float4(newDisplacement, 0.0F));
}
