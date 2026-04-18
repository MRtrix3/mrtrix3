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
// Pull a moving-lattice update field back onto the fixed lattice using exp(v):
// u_bwd_fixed(x) = u_bwd_moving(exp(v)(x)).
// Both update vectors and displacement are represented in scanner-space units (mm).

import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> movingUpdate,
    Texture3D<float4> forwardDisplacement,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    WTexture3D<float4> outputUpdate,
    SamplerState linearSampler)
{
    let fixedDims = textureSize(forwardDisplacement);
    if (globalId.x >= fixedDims.x || globalId.y >= fixedDims.y || globalId.z >= fixedDims.z) {
        return;
    }

    let movingDims = textureSize(movingUpdate);
    let movingUpdateField = VoxelSamplingFieldFloat4(movingUpdate, linearSampler);

    let fixedVoxel = float3(globalId);
    let fixedScanner = mul(voxelScannerMatrices.targetVoxelToScanner, float4(fixedVoxel, 1.0F)).xyz;
    let forwardDisp = forwardDisplacement.Load(int32_t4(globalId, 0)).xyz;
    let movedMovingScanner = fixedScanner + forwardDisp;
    let movedMovingVoxel = mul(voxelScannerMatrices.sourceScannerToVoxel, float4(movedMovingScanner, 1.0F)).xyz;

    var resampled = float3(0.0F, 0.0F, 0.0F);
    if (inBoundsVoxel(movedMovingVoxel, movingDims)) {
        resampled = movingUpdateField.sample(movedMovingVoxel).xyz;
    }

    outputUpdate.Store(globalId, float4(resampled, 0.0F));
}
