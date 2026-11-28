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
// Upsample a velocity field by a fixed scale factor using linear sampling.
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kScaleX;
extern static const float kScaleY;
extern static const float kScaleZ;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> coarseVelocity,
    WTexture3D<float4> outputVelocity,
    SamplerState linearSampler)
{
    let dims = textureSize(outputVelocity);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let coarseSampler = VoxelSamplingFieldFloat4(coarseVelocity, linearSampler);
    let fineCoord = float3(globalId);
    let coarseCoord = float3(fineCoord.x / kScaleX, fineCoord.y / kScaleY, fineCoord.z / kScaleZ);
    let sampled = coarseSampler.sample(coarseCoord).xyz;
    outputVelocity.Store(uint32_t3(globalId), float4(sampled, 0.0F));
}
