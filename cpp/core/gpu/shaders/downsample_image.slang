// This shader downsamples by a factor of 2 in each dimension using a 4-tap binomial filter.
// TODO: Consider using shared memory for better performance.
// TODO: Handle anisotropic downsampling factors.
extern static const uint kWorkgroupSizeX = 8;
extern static const uint kWorkgroupSizeY = 8;
extern static const uint kWorkgroupSizeZ = 4;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint3 id: SV_DispatchThreadID,
    Texture3D<float> inputTexture,
    WTexture3D<float> outputTexture)
{
    uint3 inputDims;
    inputTexture.GetDimensions(inputDims.x, inputDims.y, inputDims.z);
    uint3 outputDims;
    outputTexture.GetDimensions(outputDims.x, outputDims.y, outputDims.z);

    if (any(id >= outputDims))
    {
        return;
    }

    let baseCoord = 2 * id;

    // Binomial coefficients for a 4-tap filter
    let weightsX = float4(1.0, 3.0, 3.0, 1.0);
    let weightsY = float4(1.0, 3.0, 3.0, 1.0);
    let weightsZ = float4(1.0, 3.0, 3.0, 1.0);

    var accum = 0.0;
    var totalWeight = 0.0;
    for(var dz = -1; dz <= 2; dz++) {
        let wz = weightsZ[dz + 1];
        let z = clamp(int32_t(baseCoord.z) + dz, 0, int32_t(inputDims.z) - 1);
        for(var dy = -1; dy <= 2; dy++) {
            let wy = weightsY[dy + 1];
            let y = clamp(int32_t(baseCoord.y) + dy, 0, int32_t(inputDims.y) - 1);
            for(var dx = -1; dx <= 2; dx++) {
                let wx = weightsX[dx + 1];
                let x = clamp(int32_t(baseCoord.x) + dx, 0, int32_t(inputDims.x) - 1);

                let weight = wx * wy * wz;
                accum += weight * inputTexture[uint3(x, y, z)];
                totalWeight += weight;
            }
        }
    }

    let result = accum / totalWeight;

    outputTexture.Store(id, result);
}
