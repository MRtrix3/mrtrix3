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

#include "dwi/sdeconv/csd.h"
#include <fmt/format.h>

namespace MR::DWI::SDeconv {

using namespace App;
// clang-format off
const OptionGroup CSD_options =
    OptionGroup("Options for the Constrained Spherical Deconvolution algorithm")
    + Option("filter",
             "the linear frequency filtering parameters used"
             " for the initial linear spherical deconvolution step"
             " (default = [ 1 1 1 0 0 ])."
             " These should be supplied as a text file containing the filtering coefficients"
             " for each even harmonic order.")
      + Argument("spec").type_file_in()

    + Option("neg_lambda",
             fmt::format("the regularisation parameter lambda that controls the strength"
                         " of the non-negativity constraint"
                         " (default = {:.2g}).", default_csd_neglambda))
      + Argument("value").type_float(0.0)

    + Option("norm_lambda",
             fmt::format("the regularisation parameter lambda that controls the strength "
                         "of the constraint on the norm of the solution"
                         " (default = {:.2g}).", default_csd_normlambda))
      + Argument("value").type_float(0.0)

    + Option("threshold",
             fmt::format("the threshold below which the amplitude of the FOD is assumed to be zero,"
                         " expressed as an absolute amplitude"
                         " (default = {:.2g}).", default_csd_threshold))
      + Argument("value").type_float(-1.0, 10.0)

    + Option("niter",
             fmt::format("the maximum number of iterations to perform for each voxel"
                         " (default = {})."
                         // TODO Explicit SD algorithm?
                         " Use '-niter 0' for a linear unconstrained spherical deconvolution.",
                         default_csd_maxiterations))
      + Argument("number").type_integer(0, 1000);
// clang-format on

} // namespace MR::DWI::SDeconv
