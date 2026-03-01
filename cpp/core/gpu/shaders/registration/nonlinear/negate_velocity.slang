import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> velocity,
    WTexture3D<float4> outputVelocity)
{
    let dims = textureSize(velocity);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let v = velocity.Load(int32_t4(globalId, 0)).xyz;
    outputVelocity.Store(globalId, float4(-v, 0.0F));
}
