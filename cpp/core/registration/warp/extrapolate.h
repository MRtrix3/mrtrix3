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

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include "eigen_plugins/eigen_plugins.h"
#include <Eigen/Dense>

#include "datatype.h"
#include "header.h"
#include "image.h"
#include "types.h"

#include "algo/impute.h" // Impute::polynomial_basis, Impute::num_polynomial_coeffs
#include "algo/loop.h"
#include "thread_queue.h"

namespace MR::Registration::Warp {

//! \addtogroup registration
// @{

//! the 26 shared-corner neighbour offsets of a voxel
/*! Shared with the warp-inversion region-grow (\c invert.h); a single
 *  definition avoids duplicating the offset table. */
inline const std::vector<std::array<ssize_t, 3>> &neighbour_offsets() {
  static const std::vector<std::array<ssize_t, 3>> offsets = [] {
    std::vector<std::array<ssize_t, 3>> result;
    for (ssize_t dz = -1; dz <= 1; ++dz)
      for (ssize_t dy = -1; dy <= 1; ++dy)
        for (ssize_t dx = -1; dx <= 1; ++dx)
          if (dx != 0 || dy != 0 || dz != 0)
            result.push_back({dx, dy, dz});
    return result;
  }();
  return offsets;
}

//! the polynomial degree policy used when extrapolating a deformation field
/*! \c Adaptive fits the richest model the local support sustains (quadratic,
 *  then affine, then a constant fallback); \c Affine restricts the fit to a
 *  first-order model (with the same constant fallback). A deformation field is a
 *  smooth coordinate map and is therefore well approximated locally by a
 *  low-order polynomial, so the continuation beyond the valid region is solved
 *  directly as such a fit rather than via a global partial-differential
 *  equation. */
enum class ExtrapolateDegree { Adaptive, Affine };

//! the model actually fitted at a given voxel
enum class Fit { Quadratic, Affine, Constant, None };

//! the outcome of a single local polynomial fit
struct LocalFit {
  Fit kind;
  Eigen::Vector3d value;
};

//! fit a local polynomial to gathered deformation samples and evaluate it at the centre
/*! \param sample_offsets the integer voxel-index offsets of the gathered samples
 *    relative to the voxel being filled; the target voxel is therefore offset
 *    \c (0,0,0), so the fitted value is the constant term of the polynomial.
 *    Using small, centred, unit-scaled integer offsets (rather than scanner-space
 *    positions) keeps the design matrix well-conditioned.
 *  \param sample_values the three deformation components of each gathered sample,
 *    one sample per row; all three are solved against a shared design matrix.
 *  \param degree the degree policy; see \c ExtrapolateDegree.
 *  \return the fitted three-component value and the model that produced it. The
 *    degree is dropped (quadratic -> affine -> constant) whenever the support is
 *    too small or rank-deficient for the candidate model. */
inline LocalFit local_polynomial_fit(const std::vector<Eigen::Array<ssize_t, 3, 1>> &sample_offsets,
                                     const Eigen::Matrix<double, Eigen::Dynamic, 3> &sample_values,
                                     const ExtrapolateDegree degree) {
  const ssize_t num_samples = static_cast<ssize_t>(sample_offsets.size());
  if (num_samples == 0)
    return LocalFit{Fit::None, Eigen::Vector3d::Zero()};

  const int max_degree = (degree == ExtrapolateDegree::Affine) ? 1 : 2;
  for (int candidate = max_degree; candidate >= 1; --candidate) {
    const ssize_t num_coeffs = Impute::num_polynomial_coeffs(candidate);
    if (num_samples < num_coeffs)
      continue;
    Eigen::MatrixXd design(num_samples, num_coeffs);
    for (ssize_t i = 0; i != num_samples; ++i)
      design.row(i) = Impute::polynomial_basis(sample_offsets[i].cast<double>().matrix(), candidate).transpose();
    const Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(design);
    if (static_cast<ssize_t>(qr.rank()) < num_coeffs)
      continue; // rank-deficient support at this degree: drop to a lower-order model
    const Eigen::Matrix<double, Eigen::Dynamic, 3> coeffs = qr.solve(sample_values);
    // The polynomial basis at offset (0,0,0) is [1, 0, 0, ...], so the value at
    //   the target voxel is simply the constant-term row of the coefficients.
    const Eigen::Vector3d value = coeffs.row(0).transpose();
    if (!value.allFinite())
      continue;
    return LocalFit{candidate == 2 ? Fit::Quadratic : Fit::Affine, value};
  }

  // Constant fallback: the mean of the gathered samples. The two-pass dilation
  //   that defines the halo guarantees at least one filled neighbour, so this is
  //   always well-defined and finite.
  return LocalFit{Fit::Constant, sample_values.colwise().mean().transpose()};
}

// Internal helpers: kept in a detail namespace (with external linkage) rather
//   than an anonymous one, since they are referenced by the external-linkage
//   function template extrapolate_deformation_halo().
namespace detail {

//! the offsets within a Chebyshev radius of a voxel, excluding the voxel itself
inline std::vector<std::array<ssize_t, 3>> offsets_within_radius(const ssize_t radius) {
  std::vector<std::array<ssize_t, 3>> result;
  for (ssize_t dz = -radius; dz <= radius; ++dz)
    for (ssize_t dy = -radius; dy <= radius; ++dy)
      for (ssize_t dx = -radius; dx <= radius; ++dx)
        if (dx != 0 || dy != 0 || dz != 0)
          result.push_back({dx, dy, dz});
  return result;
}

//! the gather radius appropriate to a degree policy
/*! Radius 3 gives sufficient support for the ten quadratic coefficients; radius
 *  2 suffices for the four affine coefficients. */
inline ssize_t gather_radius(const ExtrapolateDegree degree) { return (degree == ExtrapolateDegree::Affine) ? 2 : 3; }

//! multi-threaded sink that fills one halo voxel by a local polynomial fit
/*! Each instance reads only voxels already marked filled (valid data or
 *  previously committed halo layers) and writes only the single voxel handed to
 *  it, so the voxels of one layer are mutually independent and require no
 *  locking. The image members are per-thread copies created by \c Thread::multi
 *  via the implicit copy constructor; they share their underlying data. */
template <class ValueType> class HaloFillKernel {
public:
  HaloFillKernel(const Image<ValueType> &buffer,
                 const Image<bool> &filled,
                 const std::vector<std::array<ssize_t, 3>> &gather_offsets,
                 const ExtrapolateDegree degree)
      : buffer(buffer), filled(filled), gather_offsets(gather_offsets), degree(degree) {}

  bool operator()(const std::array<ssize_t, 3> &voxel) {
    std::vector<Eigen::Array<ssize_t, 3, 1>> offsets;
    std::vector<Eigen::Vector3d> values;
    offsets.reserve(gather_offsets.size());
    values.reserve(gather_offsets.size());
    for (const auto &offset : gather_offsets) {
      const std::array<ssize_t, 3> q{voxel[0] + offset[0], voxel[1] + offset[1], voxel[2] + offset[2]};
      if (q[0] < 0 || q[0] >= buffer.size(0) || //
          q[1] < 0 || q[1] >= buffer.size(1) || //
          q[2] < 0 || q[2] >= buffer.size(2))
        continue;
      filled.index(0) = q[0];
      filled.index(1) = q[1];
      filled.index(2) = q[2];
      if (!filled.value())
        continue;
      buffer.index(0) = q[0];
      buffer.index(1) = q[1];
      buffer.index(2) = q[2];
      Eigen::Vector3d sample;
      for (ssize_t component = 0; component != 3; ++component) {
        buffer.index(3) = component;
        sample[component] = static_cast<double>(buffer.value());
      }
      offsets.emplace_back(offset[0], offset[1], offset[2]);
      values.push_back(sample);
    }
    if (offsets.empty())
      return true;

    Eigen::Matrix<double, Eigen::Dynamic, 3> sample_values(static_cast<ssize_t>(values.size()), 3);
    for (ssize_t i = 0; i != sample_values.rows(); ++i)
      sample_values.row(i) = values[i].transpose();
    const LocalFit fit = local_polynomial_fit(offsets, sample_values, degree);
    if (fit.kind == Fit::None)
      return true;

    buffer.index(0) = voxel[0];
    buffer.index(1) = voxel[1];
    buffer.index(2) = voxel[2];
    for (ssize_t component = 0; component != 3; ++component) {
      buffer.index(3) = component;
      buffer.value() = static_cast<ValueType>(fit.value[component]);
    }
    return true;
  }

private:
  Image<ValueType> buffer;
  Image<bool> filled;
  const std::vector<std::array<ssize_t, 3>> &gather_offsets;
  ExtrapolateDegree degree;
};

//! single-threaded source feeding the voxels of one layer to the fill kernels
class LayerSource {
public:
  explicit LayerSource(const std::vector<std::array<ssize_t, 3>> &layer) : layer(layer), next(0) {}
  bool operator()(std::array<ssize_t, 3> &voxel) {
    if (next >= layer.size())
      return false;
    voxel = layer[next++];
    return true;
  }

private:
  const std::vector<std::array<ssize_t, 3>> &layer;
  size_t next;
};

} // namespace detail

//! extrapolate a deformation field across its halo by rank-ordered local polynomial fitting
/*! Fills the \a halo voxels of \a buffer (a 4D image with three components along
 *  axis 3) by growing outward from the \a validity region. Halo voxels are
 *  rank-ordered by their count of currently-filled 26-neighbours and processed in
 *  layers of decreasing count (an onion-peel): a voxel is filled only once enough
 *  of its neighbourhood is available, and the voxels within a layer depend solely
 *  on lower layers, so each layer is fitted in parallel via
 *  \c Thread::run_queue. Each voxel is filled by \c local_polynomial_fit over the
 *  filled samples in its vicinity, exploiting the field being locally affine or
 *  quadratic. No global linear system is assembled: the cost is one small
 *  fixed-size least-squares solve per halo voxel.
 *
 *  \param buffer   the deformation-field buffer; halo voxels are written in place.
 *  \param validity the trusted-data mask (true where \a buffer holds valid data).
 *  \param halo     the set of voxels requiring extrapolation.
 *  \param degree   the polynomial degree policy; see \c ExtrapolateDegree. */
template <class ValueType>
void extrapolate_deformation_halo(Image<ValueType> &buffer,
                                  Image<bool> &validity,
                                  Image<bool> &halo,
                                  const ExtrapolateDegree degree) {
  // "Filled" status, seeded from the validity mask and grown as halo layers are
  //   committed: a voxel is a usable fit sample once it is valid data or has been
  //   extrapolated. Updated single-threaded between layers, read concurrently
  //   during each layer.
  Header filled_header(validity);
  Image<bool> filled(Image<bool>::scratch(filled_header, "extrapolation filled mask"));
  for (auto l = Loop(filled)(filled, validity); l; ++l)
    filled.value() = validity.value();

  Header count_header(validity);
  count_header.datatype() = DataType::UInt8;
  Image<uint8_t> count_image(Image<uint8_t>::scratch(count_header, "filled-neighbour count"));

  Image<bool> filled_reader(filled);
  Image<bool> halo_reader(halo);
  const std::vector<std::array<ssize_t, 3>> &offsets = neighbour_offsets();

  const auto in_grid = [&buffer](const std::array<ssize_t, 3> &p) -> bool {
    return p[0] >= 0 && p[0] < buffer.size(0) && //
           p[1] >= 0 && p[1] < buffer.size(1) && //
           p[2] >= 0 && p[2] < buffer.size(2);
  };
  const auto is_filled = [&filled_reader](const std::array<ssize_t, 3> &p) -> bool {
    filled_reader.index(0) = p[0];
    filled_reader.index(1) = p[1];
    filled_reader.index(2) = p[2];
    return filled_reader.value();
  };
  const auto is_halo = [&halo_reader](const std::array<ssize_t, 3> &p) -> bool {
    halo_reader.index(0) = p[0];
    halo_reader.index(1) = p[1];
    halo_reader.index(2) = p[2];
    return halo_reader.value();
  };
  const auto count_filled_neighbours = [&](const std::array<ssize_t, 3> &p) -> size_t {
    size_t count = 0;
    for (const auto &offset : offsets) {
      const std::array<ssize_t, 3> q{p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]};
      if (in_grid(q) && is_filled(q))
        ++count;
    }
    return count;
  };
  const auto current_count = [&count_image](const std::array<ssize_t, 3> &p) -> uint8_t {
    count_image.index(0) = p[0];
    count_image.index(1) = p[1];
    count_image.index(2) = p[2];
    return count_image.value();
  };
  const auto set_count = [&count_image](const std::array<ssize_t, 3> &p, const uint8_t value) {
    count_image.index(0) = p[0];
    count_image.index(1) = p[1];
    count_image.index(2) = p[2];
    count_image.value() = value;
  };
  const auto set_filled = [&filled](const std::array<ssize_t, 3> &p) {
    filled.index(0) = p[0];
    filled.index(1) = p[1];
    filled.index(2) = p[2];
    filled.value() = true;
  };

  // Seed the 27 bucket lists (counts 0..26) from the halo voxels' filled-neighbour counts.
  constexpr uint8_t claimed_marker = std::numeric_limits<uint8_t>::max();
  std::array<std::vector<std::array<ssize_t, 3>>, 27> buckets;
  size_t remaining = 0;
  for (auto l = Loop(halo)(halo); l; ++l) {
    if (!halo.value())
      continue;
    const std::array<ssize_t, 3> p{halo.index(0), halo.index(1), halo.index(2)};
    const size_t count = count_filled_neighbours(p);
    set_count(p, static_cast<uint8_t>(count));
    buckets[count].push_back(p);
    ++remaining;
  }

  const std::vector<std::array<ssize_t, 3>> gather_offsets =
      detail::offsets_within_radius(detail::gather_radius(degree));

  while (remaining != 0) {
    ssize_t level = 26;
    while (level >= 0 && buckets[level].empty())
      --level;
    if (level < 0)
      break; // safety: no further connected halo voxels

    // Snapshot the highest-count voxels into one layer; voxels within a layer are
    //   independent of one another because their fit reads only lower-layer data.
    std::vector<std::array<ssize_t, 3>> layer;
    for (const auto &p : buckets[level]) {
      if (is_filled(p))
        continue;
      if (current_count(p) != static_cast<uint8_t>(level))
        continue; // stale: this voxel's count has since increased
      layer.push_back(p);
      set_count(p, claimed_marker); // guard against a voxel being collected twice this layer
    }
    buckets[level].clear();
    if (layer.empty())
      continue;

    // Fit and write the layer's voxels in parallel.
    detail::LayerSource source(layer);
    detail::HaloFillKernel<ValueType> kernel(buffer, filled, gather_offsets, degree);
    Thread::run_queue(source, std::array<ssize_t, 3>(), Thread::multi(kernel));

    // Commit the layer, then raise the neighbour counts of adjacent halo voxels.
    for (const auto &p : layer) {
      set_filled(p);
      --remaining;
    }
    for (const auto &p : layer) {
      for (const auto &offset : offsets) {
        const std::array<ssize_t, 3> q{p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]};
        if (!in_grid(q) || is_filled(q) || !is_halo(q))
          continue;
        const uint8_t updated = static_cast<uint8_t>(current_count(q) + 1);
        set_count(q, updated);
        buckets[updated].push_back(q);
      }
    }
  }
}

//! @}

} // namespace MR::Registration::Warp
