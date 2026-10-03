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

#pragma once

namespace MR::Math {

//! Selects which cubic-spline basis weights a single set() call evaluates.
/*! The enumerators are bit flags, so \c ValueAndDerivative is exactly \c (Value | Derivative) and a
 *  consumer queries a requested capability with a bitwise AND. Kept in its own header so that the
 *  matrix-based interpolators in \c Math::CubicSpline (cubic_spline.h) and the lightweight
 *  \c Math::Hermite (hermite.h) share one vocabulary without either depending on the other. The
 *  type is an unscoped enum so that it can serve as a non-type template parameter (see
 *  \c MR::Interp::SplineInterp in interp/cubic.h). */
enum SplineProcessingType { Value = 1, Derivative = 2, ValueAndDerivative = Value | Derivative };

} // namespace MR::Math
