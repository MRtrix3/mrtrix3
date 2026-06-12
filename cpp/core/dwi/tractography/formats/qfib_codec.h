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

#include <cstdint>
#include <optional>
#include <vector>

#include "types.h"

#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography::Formats::QFibCodec {

//! \brief The supported quantization bit depths of the ".qfib" format (Fig. 6).
/*! The enumerator value is the on-disk "quantization" byte (8 or 16) and equally
 * the total number of bits spent per quantized direction; half of those bits are
 * spent on each of the two octahedral coordinates. */
enum class BitDepth : uint8_t { M8 = 8, M16 = 16 };

/* ************************************************************************ */
/*                  Octahedral unit-vector quantization                   */
/*                          (Meyer et al. 2010)                           */
/* ************************************************************************ */

//! \brief quantize a unit vector to a packed octahedral index (Eq. 2, encode).
/*! The vector is projected onto the unit octahedron (L1 normalisation), the lower
 * hemisphere is unwrapped onto the [-1,1]^2 square, and each of the two resulting
 * coordinates is snapped to an unsigned integer of \c BitDepth/2 bits. The two
 * coordinates are packed into a single index that occupies an int8 (M8) or an
 * int16 (M16) on disk. The input need not be normalised; a zero vector yields
 * index 0. */
int32_t octahedral_encode(const Eigen::Vector3d &unit, BitDepth) noexcept;

//! \brief recover a unit vector from a packed octahedral index (Eq. 2, decode).
/*! The exact inverse of the packing performed by octahedral_encode(), up to the
 * angular resolution of the chosen bit depth; the returned vector is normalised. */
Eigen::Vector3d octahedral_decode(int32_t index, BitDepth) noexcept;

/* ************************************************************************ */
/*               Rousseau-Boubekeur cap <-> sphere mapping                 */
/*                      (Rousseau & Boubekeur 2017)                        */
/* ************************************************************************ */

//! \brief area-preserving map of the cap around \a axis onto the whole sphere (encode, Eq. 4).
/*! A direction confined to the spherical cap of the deviation angle psi about
 * \a axis is spread over the entire sphere so that the full octahedral resolution
 * is spent on the small cap. \a ratio is the cap-to-sphere area fraction
 * k = (1 - cos psi)/2 + epsilon; the polar angle theta of \a cap_dir relative to
 * \a axis is remapped through cos theta' = 1 - (1 - cos theta)/k while the
 * azimuth (tangential component about \a axis) is preserved. */
Eigen::Vector3d inverse_mapping(const Eigen::Vector3d &cap_dir, const Eigen::Vector3d &axis, double ratio) noexcept;

//! \brief area-preserving map of the whole sphere back into the cap around \a axis (decode, Eq. 4).
/*! The exact inverse of inverse_mapping(): cos theta = 1 - k(1 - cos theta'),
 * azimuth preserved. mapping(inverse_mapping(d, axis, k), axis, k) == d for any
 * \a d within the cap. */
Eigen::Vector3d mapping(const Eigen::Vector3d &sphere_dir, const Eigen::Vector3d &axis, double ratio) noexcept;

//! \brief the cap-to-sphere area ratio k for a maximum deviation angle (radians).
/*! k = (1 - cos psi)/2 + epsilon, the value stored verbatim in the ".qfib"
 * header. The small epsilon keeps a direction at exactly psi clear of the
 * antipodal singularity of the mapping. */
double ratio_from_angle(double psi_radians) noexcept;

//! \brief recover the maximum deviation angle (radians) from the header ratio.
/*! The inverse of ratio_from_angle(), used to record provenance when reading. */
double angle_from_ratio(double ratio) noexcept;

/* ************************************************************************ */
/*                    Per-streamline compress/decompress                  */
/*                            (Appendix A)                                 */
/* ************************************************************************ */

//! \brief one compressed streamline: the two seed points and the quantized directions.
template <class V> struct Compressed {
  Eigen::Matrix<V, 3, 1> first;
  Eigen::Matrix<V, 3, 1> second;
  std::vector<int32_t> indices;
};

//! \brief the constant per-streamline step size, if uniform within tolerance.
/*! The ".qfib" model reconstructs every vertex at a single per-streamline step
 * size along a decoded tangent, so a streamline whose vertices are not uniformly
 * spaced cannot be represented. The reference step is taken as the mean of the
 * interior segments (robust to the routinely-fractional terminal segments of a
 * tractogram); returns it when every segment — terminals included — matches it
 * within the relative \a tolerance, std::nullopt otherwise (code style: a
 * validity-flagged value becomes std::optional). */
template <class V> std::optional<double> constant_stepsize(const Streamline<V> &, double tolerance);

//! \brief compress one constant-stepsize streamline (Appendix A, compress()).
/*! Throws if the streamline has fewer than two vertices, or is not of constant
 * step size within \a step_tol (which the format cannot represent). Uses
 * error-propagation reduction: each direction is taken from the already-decoded
 * previous point, so re-compressing a decompressed streamline is idempotent. */
template <class V> Compressed<V> compress(const Streamline<V> &, BitDepth, double ratio, double step_tol);

//! \brief decompress one streamline (Appendix A, decompress()).
template <class V> Streamline<V> decompress(const Compressed<V> &, BitDepth, double ratio);

} // namespace MR::DWI::Tractography::Formats::QFibCodec
