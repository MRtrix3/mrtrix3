import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 4;
extern static const uint32_t kWorkgroupSizeZ = 4;

// Apply displacement map to a single channel 3D image
[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> image,
    Texture3D<float4> displacement,
    WTexture3D<float4> outputImage,
    SamplerState linearSampler)
{
    var dims : uint32_t3;
    image.GetDimensions(dims.x, dims.y, dims.z);

    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let samplingField = VoxelSamplingField(image, linearSampler);
    let disp = displacement.Load(int4(int3(globalId), 0));
    let origPos = float3(globalId);
    let newPos = origPos + disp.xyz;
    // TODO: handle out-of-bounds sampling more gracefully
    let newValue = samplingField.sample(newPos);
    outputImage.Store(uint32_t3(globalId), float4(newValue, 0.0F, 0.0F, 0.0F));
}