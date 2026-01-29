// Separable Gaussian smoothing for float3 update fields stored in float4 textures.
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const uint32_t kKernelRadius;
extern static const float kSigma;

static float gaussianWeight(float x) {
    let sigma2 = kSigma * kSigma;
    return exp(-0.5F * (x * x) / sigma2);
}

// Blur along a single axis defined by axisOffset (should be one of (1,0,0), (0,1,0), (0,0,1))
static float3 blurAxis(
    int32_t3 baseCoord,
    int32_t3 axisOffset,
    Texture3D<float4> input,
    int32_t3 maxCoord)
{
    let radius = int32_t(kKernelRadius);
    var accum = float3(0.0F, 0.0F, 0.0F);
    var weightSum = 0.0F;

    for (int32_t i = -radius; i <= radius; ++i) {
        let offset = axisOffset * i;
        let sampleCoord = baseCoord + offset;
        let clampedCoord = clamp(sampleCoord, int32_t3(0, 0, 0), maxCoord);
        let weight = gaussianWeight(float(i));
        let value = input.Load(int4(clampedCoord, 0)).xyz;
        accum += value * weight;
        weightSum += weight;
    }

    return accum / weightSum;
}

// Wrapper to handle bounds checking and store result
static void blurAxisAndStore(
    uint32_t3 globalId,
    int32_t3 axisOffset,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    let dims = textureSize(inputUpdate);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let baseCoord = int32_t3(globalId);
    let maxCoord = int32_t3(int32_t(dims.x) - 1, int32_t(dims.y) - 1, int32_t(dims.z) - 1);
    let update = blurAxis(baseCoord, axisOffset, inputUpdate, maxCoord);
    outputUpdate.Store(uint32_t3(globalId), float4(update, 0.0F));
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurX(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    blurAxisAndStore(globalId, int32_t3(1, 0, 0), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurY(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    blurAxisAndStore(globalId, int32_t3(0, 1, 0), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurZ(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    blurAxisAndStore(globalId, int32_t3(0, 0, 1), inputUpdate, outputUpdate);
}
