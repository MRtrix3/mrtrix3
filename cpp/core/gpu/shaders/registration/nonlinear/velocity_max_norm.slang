// Compute per-workgroup maxima of squared velocity norms in target-voxel units.
import texture_utils;
import reduction_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float4> velocityTexture,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    RWStructuredBuffer<float> maxNormSqPartials)
{
    static groupshared Array<float, workgroupInvocations> localNormSq;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let dims = textureSize(velocityTexture);

    var normSq = 0.0F;
    if (globalId.x < dims.x && globalId.y < dims.y && globalId.z < dims.z) {
        let velocityScanner = velocityTexture.Load(int32_t4(globalId, 0)).xyz;
        let velocityTargetVoxel = mul(float4(velocityScanner, 0.0F), voxelScannerMatrices.targetScannerToVoxel).xyz;
        normSq = dot(velocityTargetVoxel, velocityTargetVoxel);
    }

    localNormSq[localIndex] = normSq;
    GroupMemoryBarrierWithGroupSync();

    let reducedMaxNormSq = workgroupReduce<float, MaxFloatOp, workgroupInvocations>(localNormSq, localIndex);
    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid.x
                    + workgroupId.z * uniforms.dispatchGrid.x * uniforms.dispatchGrid.y;
        maxNormSqPartials[wgIndex] = reducedMaxNormSq;
    }
}
