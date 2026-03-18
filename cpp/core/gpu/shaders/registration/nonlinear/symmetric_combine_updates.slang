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
    RWTexture3D<float4> update)
{
    let dims = textureSize(forwardUpdate);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let forward = forwardUpdate.Load(int32_t4(globalId, 0)).xyz;
    let backward = backwardUpdate.Load(int32_t4(globalId, 0)).xyz;
    let symmetricUpdate = 0.5F * (forward - backward);
    update.Store(globalId, float4(symmetricUpdate, 0.0F));
}
