/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <Eigen/Dense>
#include <string>
#include <vector>

#include "app.h"
#include "header.h"

// Switch from SelfAdjointEigenSolver to BDCSVD
// Note that since it is the singular values that are solved for,
//   and the eigenvalues are taken as the square of such,
//   it is not necessary to explicitly check for negativity of eigenvalues for safety
//   like it is with the SelfAdjointEigenSolver
#define DWIDENOISE2_USE_BDCSVD

namespace MR::Denoise {

constexpr ssize_t default_subsample_ratio = 2;

using eigenvalues_type = Eigen::Matrix<double, Eigen::Dynamic, 1>;
using vector_type = Eigen::Array<double, Eigen::Dynamic, 1>;

extern const char *first_step_description;
extern const char *non_gaussian_noise_description;
extern const char *filter_description;
extern const char *aggregation_description;

const std::vector<std::string> dtypes = {"float32", "float64"};
extern const App::Option datatype_option;

const std::vector<std::string> filters = {"optshrink", "optthresh", "truncate"};
enum class filter_type { OPTSHRINK, OPTTHRESH, TRUNCATE };

const std::vector<std::string> aggregators = {"exclusive", "gaussian", "invl0", "rank", "uniform"};
enum class aggregator_type { EXCLUSIVE, GAUSSIAN, INVL0, RANK, UNIFORM };

// These functions resolve dimensions of the matrix decomposition
//   in the presence of precoditioning that make the data rank-deficient
// - m = number of volumes
// - n = number of voxels in patch
// - rp = rank of preconditioner
ssize_t dimlong_nonzero(const ssize_t m, const ssize_t n, const ssize_t rp);
ssize_t rank_nonzero(const ssize_t m, const ssize_t n, const ssize_t rp);
ssize_t rank_zero(const ssize_t m, const ssize_t n, const ssize_t rp);

} // namespace MR::Denoise
