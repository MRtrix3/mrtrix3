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
// Compute per-workgroup maxima of squared velocity norms in target-voxel units.
import texture_utils;
import reduction_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

struct Uniforms {
    uint32_t3 dispatchGrid;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float4> velocityTexture,
    ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
    RWStructuredBuffer<float> maxNormSqPartials)
{
    static groupshared Array<float, workgroupInvocations> localNormSq;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let dims = textureSize(velocityTexture);

    var normSq = 0.0F;
    if (globalId.x < dims.x && globalId.y < dims.y && globalId.z < dims.z) {
        let velocityScanner = velocityTexture.Load(int32_t4(globalId, 0)).xyz;
        let velocityTargetVoxel = mul(voxelScannerMatrices.targetScannerToVoxel, float4(velocityScanner, 0.0F)).xyz;
        normSq = dot(velocityTargetVoxel, velocityTargetVoxel);
    }

    localNormSq[localIndex] = normSq;
    GroupMemoryBarrierWithGroupSync();

    let reducedMaxNormSq = workgroupReduce<float, MaxFloatOp, workgroupInvocations>(localNormSq, localIndex);
    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid.x
                    + workgroupId.z * uniforms.dispatchGrid.x * uniforms.dispatchGrid.y;
        maxNormSqPartials[wgIndex] = reducedMaxNormSq;
    }
}
