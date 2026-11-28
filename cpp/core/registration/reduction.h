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

#include "types.h"
#include <cmath>
#include <vector>

namespace MR::Registration {

//! Partial sums of a multi-threaded reduction, one per chunk of the outer loop
/*! A chunk is one position of the outer loop of a ThreadedLoop(), i.e. the set
 *  of voxels that one thread traverses between two claims on the shared loop
 *  position. Those claims are served in whatever order threads happen to make
 *  them, so accumulating a running total per thread groups the terms of the sum
 *  arbitrarily: with T threads the error of the naive summation of N terms
 *  grows as O(N/T + T), and its magnitude varies between executions of the same
 *  task because the grouping does.
 *
 *  Storing one partial sum per chunk instead groups the terms by the loop
 *  decomposition. For chunks of B voxels this is blocked summation, whose error
 *  grows as O(B + N/B), and sum() merges the chunks with compensated summation,
 *  reducing that to O(B). Because the grouping no longer depends on thread
 *  scheduling, the result is also identical between executions and for any
 *  number of threads.
 */
class PartialSums {
public:
  //! reserve \a nterms accumulators per chunk, for a loop over \a outer_axes of \a source
  template <class HeaderType>
  PartialSums(const HeaderType &source, const std::vector<size_t> &outer_axes, const ssize_t nterms)
      : outer_axes(outer_axes) {
    size_t nchunks = 1;
    chunk_strides.reserve(outer_axes.size());
    for (const size_t axis : outer_axes) {
      chunk_strides.push_back(nchunks);
      nchunks *= source.size(axis);
    }
    data = Eigen::MatrixXd::Zero(nterms, nchunks);
  }

  //! index of the chunk containing position \a pos
  template <class PositionType> size_t chunk(const PositionType &pos) const {
    size_t result = 0;
    for (size_t n = 0; n != outer_axes.size(); ++n)
      result += chunk_strides[n] * pos.index(outer_axes[n]);
    return result;
  }

  //! accumulators of the chunk with index \a index
  Eigen::MatrixXd::ColXpr operator[](const size_t index) { return data.col(index); }

  //! merge the partial sums, in order of increasing chunk index
  /*! Compensated (Neumaier) summation: the low-order bits discarded by each
   *  addition are themselves accumulated, so that the error of the merge does
   *  not grow with the number of chunks. */
  Eigen::VectorXd sum() const {
    Eigen::VectorXd result = Eigen::VectorXd::Zero(data.rows());
    Eigen::VectorXd compensation = Eigen::VectorXd::Zero(data.rows());
    for (ssize_t index = 0; index != data.cols(); ++index) {
      for (ssize_t term = 0; term != data.rows(); ++term) {
        const default_type addend = data(term, index);
        const default_type partial = result[term] + addend;
        compensation[term] += std::fabs(result[term]) >= std::fabs(addend) ? (result[term] - partial) + addend
                                                                           : (addend - partial) + result[term];
        result[term] = partial;
      }
    }
    return result + compensation;
  }

protected:
  const std::vector<size_t> outer_axes;
  std::vector<size_t> chunk_strides;
  Eigen::MatrixXd data;
};

} // namespace MR::Registration
