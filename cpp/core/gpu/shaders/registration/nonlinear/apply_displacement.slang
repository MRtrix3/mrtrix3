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
