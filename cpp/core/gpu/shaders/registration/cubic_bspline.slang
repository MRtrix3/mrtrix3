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
// Cubic B–spline kernel with compact support = 2
float cubicBSpline(float x)
{
    let ax = abs(x);
    let u = max(0.0, 2.0 - ax);
    let v = max(0.0, 1.0 - ax);
    let u3 = u * u * u;
    let v3 = v * v * v;
    return (u3 - 4.0 * v3) * (1.0 / 6.0);
}

float cubicBSplineDerivative(float x)
{
    let ax = abs(x);
    let u = max(0.0, 2.0 - ax);
    let v = max(0.0, 1.0 - ax);
    let du_dx = (ax < 2.0) ? -sign(x) : 0.0;
    let dv_dx = (ax < 1.0) ? -sign(x) : 0.0;
    let u2 = u * u;
    let v2 = v * v;
    return (3.0 * u2 * du_dx - 12.0 * v2 * dv_dx) * (1.0 / 6.0);
}
