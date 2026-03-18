/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */
// Separable Gaussian smoothing for 3D textures.
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

// Blur a scalar voxel along one axis.
static float blurScalar(
    int32_t3 baseCoord,
    int32_t3 axisOffset,
    Texture3D<float> input,
    int32_t3 maxCoord)
{
    let radius = int32_t(kKernelRadius);
    var accum = 0.0F;
    var weightSum = 0.0F;

    for (var i = -radius; i <= radius; ++i) {
        let offset = axisOffset * i;
        let sampleCoord = baseCoord + offset;
        let clampedCoord = clamp(sampleCoord, int32_t3(0, 0, 0), maxCoord);
        let weight = gaussianWeight(float(i));
        let value = input.Load(int32_t4(clampedCoord, 0));
        accum += value * weight;
        weightSum += weight;
    }

    return accum / weightSum;
}

// Blur a RGBA voxel along one axis.
static float4 blurRGBA(
    int32_t3 baseCoord,
    int32_t3 axisOffset,
    Texture3D<float4> input,
    int32_t3 maxCoord)
{
    let radius = int32_t(kKernelRadius);
    var accum = float4(0.0F);
    var weightSum = 0.0F;

    for (var i = -radius; i <= radius; ++i) {
        let offset = axisOffset * i;
        let sampleCoord = baseCoord + offset;
        let clampedCoord = clamp(sampleCoord, int32_t3(0, 0, 0), maxCoord);
        let weight = gaussianWeight(float(i));
        let value = input.Load(int32_t4(clampedCoord, 0));
        accum += value * float4(weight);
        weightSum += weight;
    }

    return accum / float4(weightSum);
}

static void blurAxisAndStoreScalar(
    uint32_t3 globalId,
    int32_t3 axisOffset,
    Texture3D<float> inputTexture,
    WTexture3D<float> outputTexture)
{
    let dims = textureSize(inputTexture);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let baseCoord = int32_t3(globalId);
    let maxCoord = int32_t3(int32_t(dims.x) - 1, int32_t(dims.y) - 1, int32_t(dims.z) - 1);
    let value = blurScalar(baseCoord, axisOffset, inputTexture, maxCoord);
    outputTexture.Store(uint32_t3(globalId), value);
}

static void blurAxisAndStoreRGBA(
    uint32_t3 globalId,
    int32_t3 axisOffset,
    Texture3D<float4> inputTexture,
    WTexture3D<float4> outputTexture)
{
    let dims = textureSize(inputTexture);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let baseCoord = int32_t3(globalId);
    let maxCoord = int32_t3(int32_t(dims.x) - 1, int32_t(dims.y) - 1, int32_t(dims.z) - 1);
    let value = blurRGBA(baseCoord, axisOffset, inputTexture, maxCoord);
    outputTexture.Store(uint32_t3(globalId), value);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurXScalar(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> inputUpdate,
    WTexture3D<float> outputUpdate)
{
    blurAxisAndStoreScalar(globalId, int32_t3(1, 0, 0), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurYScalar(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> inputUpdate,
    WTexture3D<float> outputUpdate)
{
    blurAxisAndStoreScalar(globalId, int32_t3(0, 1, 0), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurZScalar(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float> inputUpdate,
    WTexture3D<float> outputUpdate)
{
    blurAxisAndStoreScalar(globalId, int32_t3(0, 0, 1), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurXRGBA(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    blurAxisAndStoreRGBA(globalId, int32_t3(1, 0, 0), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurYRGBA(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    blurAxisAndStoreRGBA(globalId, int32_t3(0, 1, 0), inputUpdate, outputUpdate);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void blurZRGBA(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> inputUpdate,
    WTexture3D<float4> outputUpdate)
{
    blurAxisAndStoreRGBA(globalId, int32_t3(0, 0, 1), inputUpdate, outputUpdate);
}
