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

namespace MR::Helper {
template <class ImageType> class ConstRow;
template <class ImageType> class Row;
} // namespace MR::Helper
#define EIGEN_DENSEBASE_PLUGIN "eigen_plugins/dense_base.h"  // check_syntax off
#define EIGEN_MATRIXBASE_PLUGIN "eigen_plugins/dense_base.h" // check_syntax off
#define EIGEN_ARRAYBASE_PLUGIN "eigen_plugins/dense_base.h"  // check_syntax off
#define EIGEN_MATRIX_PLUGIN "eigen_plugins/matrix.h"         // check_syntax off
#define EIGEN_ARRAY_PLUGIN "eigen_plugins/array.h"           // check_syntax off
#include <Eigen/Geometry>
#ifdef EIGEN_HAS_OPENMP
#undef EIGEN_HAS_OPENMP
#endif
