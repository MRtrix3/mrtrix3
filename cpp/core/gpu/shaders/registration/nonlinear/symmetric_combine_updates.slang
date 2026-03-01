import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> forwardUpdate,
    Texture3D<float4> backwardUpdate,
    WTexture3D<float4> outputUpdate)
{
    let dims = textureSize(forwardUpdate);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let forward = forwardUpdate.Load(int32_t4(globalId, 0)).xyz;
    let backward = backwardUpdate.Load(int32_t4(globalId, 0)).xyz;
    outputUpdate.Store(globalId, float4(0.5F * (forward - backward), 0.0F));
}
