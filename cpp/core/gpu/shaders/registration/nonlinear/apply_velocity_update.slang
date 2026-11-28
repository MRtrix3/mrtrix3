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
// Apply one nonlinear SVF update step by taking the raw local update field produced by the metric shader,
// forming the effective update
//   u_eff(x) = (kStepSize * dynamicScale) * u_raw(x)
// with an additional per-voxel magnitude clamp, and composing that effective update into the current velocity field.
// The composition uses a Baker-Campbell-Hausdorff truncation controlled by kBchTerms:
//   kBchTerms = 1: log(exp(v) o exp(u)) ~= v + u
//   kBchTerms = 2: log(exp(v) o exp(u)) ~= v + u + (1/2)[v,u]
// where v is the current velocity field, u is the effective local update, and
//   [v,u] = Jacobian(v) * u - Jacobian(u) * v
// is the Lie bracket of the two vector fields.
import texture_utils;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kStepSize;
extern static const float kMaxUpdateMagnitude;
extern static const float kClampEpsilon;
extern static const uint32_t kBchTerms = 1U;

struct Uniforms {
  float dynamicScale;
};

// Load the current velocity sample with boundary clamping.
float3 loadVelocity(Texture3D<float4> velocityField, int32_t3 coord, uint32_t3 dims) {
  let clampedCoord = clamp(coord, int32_t3(0, 0, 0), int32_t3(dims) - 1);
  return velocityField.Load(int32_t4(clampedCoord, 0)).xyz;
}

// Load, clamp, and globally rescale the update that will be applied at this voxel.
float3 loadEffectiveUpdate(Texture3D<float4> updateField, int32_t3 coord, uint32_t3 dims, float dynamicScale) {
  let clampedCoord = clamp(coord, int32_t3(0, 0, 0), int32_t3(dims) - 1);
  let rawUpdate = updateField.Load(int32_t4(clampedCoord, 0)).xyz;
  let rawUpdateNorm = length(rawUpdate);
  let clampScale = min(1.0F, kMaxUpdateMagnitude / (rawUpdateNorm + kClampEpsilon));
  return (kStepSize * dynamicScale) * rawUpdate * clampScale;
}

// Estimate a voxel-space directional derivative of the velocity field along one axis, using a central difference
// in the interior and a one-sided difference at the volume boundary.
float3 finiteDifferenceVelocity(Texture3D<float4> velocityField, int32_t3 coord, uint32_t3 dims, int32_t3 axisOffset) {
  let lowerCoord = coord - axisOffset;
  let upperCoord = coord + axisOffset;
  let centerValue = loadVelocity(velocityField, coord, dims);
  let hasLower = inBoundsIndex(lowerCoord, dims);
  let hasUpper = inBoundsIndex(upperCoord, dims);
  if (hasLower && hasUpper) {
    return 0.5F * (loadVelocity(velocityField, upperCoord, dims) - loadVelocity(velocityField, lowerCoord, dims));
  }
  if (hasUpper) {
    return loadVelocity(velocityField, upperCoord, dims) - centerValue;
  }
  if (hasLower) {
    return centerValue - loadVelocity(velocityField, lowerCoord, dims);
  }
  return float3(0.0F, 0.0F, 0.0F);
}

// Estimate a voxel-space directional derivative of the effective update field along one axis.
float3 finiteDifferenceEffectiveUpdate(
    Texture3D<float4> updateField, int32_t3 coord, uint32_t3 dims, int32_t3 axisOffset, float dynamicScale) {
  let lowerCoord = coord - axisOffset;
  let upperCoord = coord + axisOffset;
  let centerValue = loadEffectiveUpdate(updateField, coord, dims, dynamicScale);
  let hasLower = inBoundsIndex(lowerCoord, dims);
  let hasUpper = inBoundsIndex(upperCoord, dims);
  if (hasLower && hasUpper) {
    return 0.5F * (loadEffectiveUpdate(updateField, upperCoord, dims, dynamicScale) -
                   loadEffectiveUpdate(updateField, lowerCoord, dims, dynamicScale));
  }
  if (hasUpper) {
    return loadEffectiveUpdate(updateField, upperCoord, dims, dynamicScale) - centerValue;
  }
  if (hasLower) {
    return centerValue - loadEffectiveUpdate(updateField, lowerCoord, dims, dynamicScale);
  }
  return float3(0.0F, 0.0F, 0.0F);
}

// Convert voxel-axis partials into the directional derivative along a scanner-space vector.
float3 directionalDerivativeFromVoxelPartials(
    float3 partialX, float3 partialY, float3 partialZ, float3 directionScanner, float4x4 scannerToVoxel) {
  let directionVoxel = mul(scannerToVoxel, float4(directionScanner, 0.0F)).xyz;
  return partialX * directionVoxel.x + partialY * directionVoxel.y + partialZ * directionVoxel.z;
}

[shader("compute")][numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
    // Apply the rescaled local update using the requested BCH truncation order.
    void
    main(uint32_t3 globalId : SV_DispatchThreadID,
         ConstantBuffer<Uniforms> uniforms,
         Texture3D<float4> velocity,
         Texture3D<float4> update,
         ConstantBuffer<VoxelScannerMatrices> voxelScannerMatrices,
         WTexture3D<float4> outputVelocity) {
      let dims = textureSize(velocity);
      if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

      let coord = int32_t3(globalId);
      let currentVelocity = velocity.Load(int32_t4(globalId, 0)).xyz;
      let effectiveUpdate = loadEffectiveUpdate(update, coord, dims, uniforms.dynamicScale);
      var newVelocity = currentVelocity + effectiveUpdate;

      if (kBchTerms >= 2U) {
        let axisX = int32_t3(1, 0, 0);
        let axisY = int32_t3(0, 1, 0);
        let axisZ = int32_t3(0, 0, 1);
        let dVelocityDx = finiteDifferenceVelocity(velocity, coord, dims, axisX);
        let dVelocityDy = finiteDifferenceVelocity(velocity, coord, dims, axisY);
        let dVelocityDz = finiteDifferenceVelocity(velocity, coord, dims, axisZ);
        let dUpdateDx = finiteDifferenceEffectiveUpdate(update, coord, dims, axisX, uniforms.dynamicScale);
        let dUpdateDy = finiteDifferenceEffectiveUpdate(update, coord, dims, axisY, uniforms.dynamicScale);
        let dUpdateDz = finiteDifferenceEffectiveUpdate(update, coord, dims, axisZ, uniforms.dynamicScale);
        let velocityAlongUpdate = directionalDerivativeFromVoxelPartials(
            dVelocityDx, dVelocityDy, dVelocityDz, effectiveUpdate, voxelScannerMatrices.targetScannerToVoxel);
        let updateAlongVelocity = directionalDerivativeFromVoxelPartials(
            dUpdateDx, dUpdateDy, dUpdateDz, currentVelocity, voxelScannerMatrices.targetScannerToVoxel);
        let lieBracket = velocityAlongUpdate - updateAlongVelocity;
        newVelocity += 0.5F * lieBracket;
      }

      outputVelocity.Store(uint32_t3(globalId), float4(newVelocity, 0.0F));
    }
