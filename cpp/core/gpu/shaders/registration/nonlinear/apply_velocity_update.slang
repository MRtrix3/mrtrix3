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
// Apply a small SSD-derived update to the velocity field: v(x) = v(x) + gamma * u(x),
// where v is the velocity field, u is the update field, and gamma is a step size parameter.
// This is an approximation of the Baker-Campbell update scheme for diffeomorphic log-demons,
// which states that log(exp(v) o exp(u)) ~= v + u + (1/2)[v,u] + ...
// where [v,u] = Jacobian(v) * u - Jacobian(u) * v is the Lie bracket of the two vector fields.
// Here we simply ignore the Lie bracket term and higher order terms, which is reasonable
// for small updates u.
import texture_utils;

extern static const uint32_t kWorkgroupSizeX = 8U;
extern static const uint32_t kWorkgroupSizeY = 4U;
extern static const uint32_t kWorkgroupSizeZ = 4U;
extern static const float kStepSize;
extern static const float kMaxUpdateMagnitude;
extern static const float kClampEpsilon;

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main(
    uint32_t3 globalId : SV_DispatchThreadID,
    Texture3D<float4> velocity,
    Texture3D<float4> update,
    WTexture3D<float4> outputVelocity)
{
    let dims = textureSize(velocity);
    if (globalId.x >= dims.x || globalId.y >= dims.y || globalId.z >= dims.z)
        return;

    let currentVelocity = velocity.Load(int32_t4(globalId, 0)).xyz;
    let delta = update.Load(int32_t4(globalId, 0)).xyz;
    let delta_norm = length(delta);
    let clamp_scale = min(1.0F, kMaxUpdateMagnitude / (delta_norm + kClampEpsilon));
    let clamped_delta = delta * clamp_scale;
    let newVelocity = currentVelocity + kStepSize * clamped_delta;

    outputVelocity.Store(uint32_t3(globalId), float4(newVelocity, 0.0F));
}
