import atomic_utils;

extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 8;
extern static const uint32_t kWorkgroupSizeZ = 4;
static const uint32_t kMomentCount = 6;
// TODO: Use extern static const bool once the Slang compiler bug is fixed.
extern static const uint32_t kUseMask;

struct MomentUniforms {
    float4 centre;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    RWStructuredBuffer<Atomic<uint32_t>> momentBuffer,
    StructuredBuffer<float4x4> voxelToScanner,
    ConstantBuffer<MomentUniforms> centreScanner,
    Texture3D<float> image,
    Texture3D<float> mask)
{
    var imageDimensions : uint32_t3;
    image.GetDimensions(imageDimensions.x, imageDimensions.y, imageDimensions.z);

    if (!all(globalId < imageDimensions)) {
        return;
    }

    if (kUseMask != 0U) {
        let maskValue = mask.Load(int32_t4(globalId, 0)).r;
        if (maskValue < 0.5F) {
            return;
        }
    }

    let voxelValue = image.Load(int32_t4(globalId, 0)).r;
    if (!isfinite(voxelValue)) {
        return;
    }

    let scannerPos4 = mul(voxelToScanner[0], float4(float3(globalId), 1.0F));
    let scannerPos = scannerPos4.xyz / scannerPos4.w;
    let centered = scannerPos - centreScanner.centre.xyz;

    let contributions = Array<float, kMomentCount>(
        centered.x * centered.x * voxelValue,
        centered.y * centered.y * voxelValue,
        centered.z * centered.z * voxelValue,
        centered.x * centered.y * voxelValue,
        centered.x * centered.z * voxelValue,
        centered.y * centered.z * voxelValue
    );

    for (var i = 0U; i < kMomentCount; ++i) {
        atomicAddF32InMemory(momentBuffer[i], contributions[i]);
    }
}
