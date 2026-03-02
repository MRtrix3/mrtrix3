import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kInertiaWeight = 0.9F;

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
    let previous = update.Load(int32_t3(globalId)).xyz;
    let currentRaw = 0.5F * (forward - backward);
    let inertialUpdate = currentRaw + (kInertiaWeight * previous);
    update.Store(globalId, float4(inertialUpdate, 0.0F));
}
