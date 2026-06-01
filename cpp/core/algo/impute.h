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
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

// Eigen plugin configuration must precede inclusion of any Eigen header,
//   including the unsupported Tensor module used here.
#include "eigen_plugins/eigen_plugins.h"
#include <Eigen/SparseCore>
#include <Eigen/SparseLU>
#include <Eigen/SparseQR>
#include <unsupported/Eigen/CXX11/Tensor>

#include "exception.h"
#include "types.h"

#include "math/SH.h"
#include "math/math.h"
#include "misc/voxel2vector.h"

namespace MR::Impute {

//! \defgroup impute Imputation of invalid image intensities
//! \brief Machinery for filling "invalid" (non-finite or masked) voxels by
//!   solving a global linear system over the set of voxels to be imputed.
//! @{

//! the imputation algorithms exposed by the \c mrimpute command
/*! The lower-case names of these enumerators are also the choices presented to
 *  the user via the \c -method command-line option. */
enum class Method { laplacian, laplaciansq, biharmonic, hessian, spring, isotropic2, isotropic4 };

//! optional removal of a parametric trend prior to imputation
/*! A low-order polynomial trend is fit by least squares to the known data
 *  bordering the region to be imputed, subtracted before the solve, and re-added
 *  afterwards (a "universal kriging" decomposition): \c affine fits a
 *  first-order trend, \c quadratic a second-order trend. The trend carries any
 *  global gradient (and, for \c quadratic, curvature) into the extrapolated
 *  region in closed form, leaving the linear solver to resolve only the bounded
 *  residual; this avoids the flattening that a purely harmonic solve exhibits
 *  when extrapolating beyond a one-sided data boundary. The lower-case names are
 *  the choices presented via the \c -detrend command-line option. */
enum class Detrend { none, affine, quadratic };

//! a 3D spatial position or offset, in voxels
using Position = Eigen::Array<ssize_t, 3, 1>;

//! a centred dense weight kernel for a single finite-difference equation
/*! The tensor side length is <tt>2 * radius + 1</tt>; the weight associated
 *  with spatial offset <tt>(dx, dy, dz)</tt> resides at tensor coordinate
 *  <tt>(dx + radius, dy + radius, dz + radius)</tt>. */
using Stencil = Eigen::Tensor<double, 3>;

using Mat = Eigen::MatrixXd;
using Vec = Eigen::VectorXd;

//! the assembled imputation coefficient matrix
/*! Each finite-difference equation references only a handful of unknown columns,
 *  so the global system is sparse: storage and factorisation costs grow with the
 *  number of coupling terms rather than with the (potentially large) square of
 *  the unknown count. Column-major storage matches the column-oriented access of
 *  both the underdetermined-column guard and the sparse QR / LU solvers. */
using SparseMat = Eigen::SparseMatrix<double>;
//! a single (row, column, coefficient) contribution used to build a SparseMat
using Triplet = Eigen::Triplet<double, SparseMat::StorageIndex>;

//! type-erased read access to a known intensity at a spatial position
using ValueFn = std::function<double(const Position &)>;
//! type-erased test of whether a spatial position lies within the image field of view
using InFovFn = std::function<bool(const Position &)>;

//! how the dense linear system is to be solved
enum class SolveType { SquareDirect, LeastSquares };

//! which voxels serve as equation centres during stencil-based assembly
enum class Centres { Unknowns, UnknownsAndNeighbours };

//! a single assembled linear equation: sum of weighted unknowns equals a known right-hand side
struct Equation {
  //! unknown-column index paired with its coefficient
  std::vector<std::pair<Voxel2Vector::index_t, double>> terms;
  double rhs;
};

//! the number of coefficients in a polynomial trend of the given \a degree
/*! Degree 1 (affine) has four terms; degree 2 (quadratic) has ten. */
inline ssize_t num_polynomial_coeffs(const int degree) {
  assert(degree == 1 || degree == 2);
  return (degree == 1) ? 4 : 10;
}

//! the polynomial design row at voxel-space offset \a p, up to the given \a degree
/*! Ordering: <tt>[1, x, y, z]</tt> for degree 1, with
 *  <tt>[x*x, y*y, z*z, x*y, x*z, y*z]</tt> appended for degree 2. \a p is the
 *  position relative to whatever origin the caller has chosen; centring the
 *  basis on the data centroid keeps the design matrix well-conditioned for the
 *  quadratic terms. */
inline Vec polynomial_basis(const Eigen::Vector3d &p, const int degree) {
  Vec row(num_polynomial_coeffs(degree));
  row[0] = 1.0;
  row[1] = p[0];
  row[2] = p[1];
  row[3] = p[2];
  if (degree == 2) {
    row[4] = p[0] * p[0];
    row[5] = p[1] * p[1];
    row[6] = p[2] * p[2];
    row[7] = p[0] * p[1];
    row[8] = p[0] * p[2];
    row[9] = p[1] * p[2];
  }
  return row;
}

//! a low-order spatial polynomial trend in voxel coordinates
/*! The basis is evaluated relative to \c origin (the centroid of the data to
 *  which the trend was fit); this centring keeps the least-squares design matrix
 *  well-conditioned for the quadratic terms without altering the fitted
 *  surface. */
struct PolynomialTrend {
  //! polynomial degree: 1 (affine) or 2 (quadratic)
  int degree;
  //! voxel-space origin about which the basis is centred
  Eigen::Vector3d origin;
  //! fitted coefficients, ordered as in polynomial_basis()
  Vec coeffs;
  //! evaluate the trend at integer voxel position \a p
  double eval(const Position &p) const {
    return polynomial_basis(p.cast<double>().matrix() - origin, degree).dot(coeffs);
  }
};

//! the polynomial degree implied by a detrending mode, or \c std::nullopt for none
inline std::optional<int> detrend_degree(const Detrend detrend) {
  switch (detrend) {
  case Detrend::none:
    return std::nullopt;
  case Detrend::affine:
    return 1;
  case Detrend::quadratic:
    return 2;
  }
  assert(false);
  return std::nullopt;
}

//! allocate a zero-filled centred stencil of the given radius
inline Stencil zero_stencil(const ssize_t radius) {
  const ssize_t side = 2 * radius + 1;
  Stencil result(side, side, side);
  result.setZero();
  return result;
}

//! accumulate weight \a w at spatial offset \a offset within stencil \a s
inline void bake(Stencil &s, const Position &offset, const double w) {
  const ssize_t radius = (s.dimension(0) - 1) / 2;
  s(offset[0] + radius, offset[1] + radius, offset[2] + radius) += w;
}

//! the 7-point del-squared (Laplacian) stencil in 3D
inline Stencil laplacian_stencil() {
  Stencil s = zero_stencil(1);
  bake(s, Position(0, 0, 0), -6.0);
  bake(s, Position(1, 0, 0), 1.0);
  bake(s, Position(-1, 0, 0), 1.0);
  bake(s, Position(0, 1, 0), 1.0);
  bake(s, Position(0, -1, 0), 1.0);
  bake(s, Position(0, 0, 1), 1.0);
  bake(s, Position(0, 0, -1), 1.0);
  return s;
}

//! the 25-point del-to-the-fourth (biharmonic) stencil in 3D
/*! Assembled by summing constituent operators into the kernel rather than by
 *  hand-tabulating coefficients: the pure fourth derivatives
 *  <tt>d4/dx_i4</tt> (the <tt>[1,-4,6,-4,1]</tt> stencil along each axis) plus
 *  twice each mixed term <tt>d2/dx_i2 d2/dx_j2</tt> (the outer product of two
 *  <tt>[1,-2,1]</tt> stencils) for <tt>i < j</tt>. */
inline Stencil biharmonic_stencil() {
  Stencil s = zero_stencil(2);
  const std::array<double, 5> d4{1.0, -4.0, 6.0, -4.0, 1.0};
  for (ssize_t axis = 0; axis != 3; ++axis) {
    for (ssize_t k = -2; k <= 2; ++k) {
      Position offset(0, 0, 0);
      offset[axis] = k;
      bake(s, offset, d4[k + 2]);
    }
  }
  const std::array<double, 3> d2{1.0, -2.0, 1.0};
  for (ssize_t a = 0; a != 3; ++a) {
    for (ssize_t b = a + 1; b != 3; ++b) {
      for (ssize_t i = -1; i <= 1; ++i) {
        for (ssize_t j = -1; j <= 1; ++j) {
          Position offset(0, 0, 0);
          offset[a] = i;
          offset[b] = j;
          bake(s, offset, 2.0 * d2[i + 1] * d2[j + 1]);
        }
      }
    }
  }
  return s;
}

//! the six second-derivative stencils whose squared sum is the Hessian energy
/*! Three pure second derivatives <tt>d2/dx_i2</tt> (the <tt>[1,-2,1]</tt>
 *  stencil along each axis) and three mixed second derivatives
 *  <tt>d2/dx_i dx_j</tt> (the central four-corner stencil). The mixed terms
 *  carry a factor <tt>sqrt(2)</tt> so that the sum of squared finite-difference
 *  residuals equals the squared Frobenius norm of the Hessian,
 *  <tt>||H u||_F^2 = sum_i u_ii^2 + 2 sum_{i<j} u_ij^2</tt>. Every operator
 *  annihilates affine fields, so a least-squares solve over these stencils
 *  extrapolates a linear trend beyond the data rather than flattening to a
 *  constant. */
inline std::vector<Stencil> hessian_stencils() {
  std::vector<Stencil> stencils;
  for (ssize_t axis = 0; axis != 3; ++axis) {
    Position step(0, 0, 0);
    step[axis] = 1;
    Stencil s = zero_stencil(1);
    bake(s, -step, 1.0);
    bake(s, Position(0, 0, 0), -2.0);
    bake(s, step, 1.0);
    stencils.push_back(std::move(s));
  }
  // Mixed second derivative: central four-corner difference / 4, times sqrt(2).
  const double w = std::sqrt(2.0) / 4.0;
  for (ssize_t a = 0; a != 3; ++a) {
    for (ssize_t b = a + 1; b != 3; ++b) {
      Position ea(0, 0, 0);
      ea[a] = 1;
      Position eb(0, 0, 0);
      eb[b] = 1;
      Stencil s = zero_stencil(1);
      bake(s, ea + eb, w);
      bake(s, ea - eb, -w);
      bake(s, -ea + eb, -w);
      bake(s, -ea - eb, w);
      stencils.push_back(std::move(s));
    }
  }
  return stencils;
}

//! generate the spherically-isotropic finite-difference equation templates
/*! Duplicates the 13-direction, spherical-harmonic-weighted stencil generation
 *  of \c mrsense1fix.cpp::VoxelPredict::initialise(), generalised here so that
 *  each finite-difference row is baked into a centred Stencil for inclusion in
 *  the global solve. \a order selects the derivative order (2 or 4). */
inline std::vector<Stencil> isotropic_stencils(const int order) {
  assert(order == 2 || order == 4);
  // The 13 half-sphere neighbour directions (duplicated from mrsense1fix.cpp).
  Eigen::Array<ssize_t, 13, 3> offsets;
  offsets << 1, 0, 0, //
      0, 1, 0,        //
      0, 0, 1,        //
      1, 1, 0,        //
      1, -1, 0,       //
      1, 0, 1,        //
      1, 0, -1,       //
      0, 1, 1,        //
      0, 1, -1,       //
      1, 1, 1,        //
      1, 1, -1,       //
      1, -1, 1,       //
      1, -1, -1;
  const Eigen::Array<double, 13, 1> norms = offsets.cast<double>().matrix().rowwise().norm();
  const Eigen::Matrix<double, 13, 3> unit_offsets = offsets.cast<double>().colwise() / norms;
  // lmax=4 -> 15 components -> over-specify with respect to the direction set,
  //   then solve for per-direction weights cancelling all l>0 SH terms.
  const Eigen::Matrix<double, 13, 15> SH2amp = Math::SH::init_transform_cart(unit_offsets, 4);
  Eigen::Matrix<double, 15, 1> integral_results = Eigen::Matrix<double, 15, 1>::Zero();
  integral_results[0] = 1.0;
  Eigen::Matrix<double, 15, 13> A;
  for (size_t sh_coeff = 0; sh_coeff != 15; ++sh_coeff) {
    Eigen::Matrix<double, 15, 1> SH_in = Eigen::Matrix<double, 15, 1>::Zero();
    SH_in[sh_coeff] = 1.0;
    A.row(sh_coeff) = SH2amp * SH_in;
  }
  const Eigen::Array<double, 13, 1> weights = A.householderQr().solve(integral_results);

  std::vector<Stencil> stencils;
  const ssize_t radius = order;
  for (size_t index = 0; index != 13; ++index) {
    const Position offset(offsets(index, 0), offsets(index, 1), offsets(index, 2));
    if (order == 2) {
      // Second derivatives: distance factor in the finite-difference denominator is squared.
      const double w = weights[index] / Math::pow2(norms[index]);
      {
        Stencil s = zero_stencil(radius);
        bake(s, -2 * offset, w);
        bake(s, -offset, -2.0 * w);
        bake(s, Position(0, 0, 0), w);
        stencils.push_back(std::move(s));
      }
      {
        Stencil s = zero_stencil(radius);
        bake(s, -offset, w);
        bake(s, Position(0, 0, 0), -2.0 * w);
        bake(s, offset, w);
        stencils.push_back(std::move(s));
      }
      {
        Stencil s = zero_stencil(radius);
        bake(s, Position(0, 0, 0), w);
        bake(s, offset, -2.0 * w);
        bake(s, 2 * offset, w);
        stencils.push_back(std::move(s));
      }
    } else {
      // Fourth derivatives: distance factor in the finite-difference denominator is to the fourth power.
      const double w = weights[index] / Math::pow4(norms[index]);
      {
        Stencil s = zero_stencil(radius);
        bake(s, -4 * offset, w);
        bake(s, -3 * offset, -4.0 * w);
        bake(s, -2 * offset, 6.0 * w);
        bake(s, -offset, -4.0 * w);
        bake(s, Position(0, 0, 0), w);
        stencils.push_back(std::move(s));
      }
      {
        Stencil s = zero_stencil(radius);
        bake(s, -3 * offset, w);
        bake(s, -2 * offset, -4.0 * w);
        bake(s, -offset, 6.0 * w);
        bake(s, Position(0, 0, 0), -4.0 * w);
        bake(s, offset, w);
        stencils.push_back(std::move(s));
      }
      {
        Stencil s = zero_stencil(radius);
        bake(s, -2 * offset, w);
        bake(s, -offset, -4.0 * w);
        bake(s, Position(0, 0, 0), 6.0 * w);
        bake(s, offset, -4.0 * w);
        bake(s, 2 * offset, w);
        stencils.push_back(std::move(s));
      }
      {
        Stencil s = zero_stencil(radius);
        bake(s, -offset, w);
        bake(s, Position(0, 0, 0), -4.0 * w);
        bake(s, offset, 6.0 * w);
        bake(s, 2 * offset, -4.0 * w);
        bake(s, 3 * offset, w);
        stencils.push_back(std::move(s));
      }
      {
        Stencil s = zero_stencil(radius);
        bake(s, Position(0, 0, 0), w);
        bake(s, offset, -4.0 * w);
        bake(s, 2 * offset, 6.0 * w);
        bake(s, 3 * offset, -4.0 * w);
        bake(s, 4 * offset, w);
        stencils.push_back(std::move(s));
      }
    }
  }
  return stencils;
}

//! abstract base class for an imputation method
/*! Holds references to the serialisation (\c Voxel2Vector) and the type-erased
 *  image accessors, and owns the shared dense assembly and solve. Derived
 *  classes supply the equation set (via assemble()) and the solver type. */
class Base {
public:
  Base(const Voxel2Vector &v2v, ValueFn value_at, InFovFn in_fov, const Detrend detrend = Detrend::none)
      : v2v(v2v), value_at(std::move(value_at)), in_fov(std::move(in_fov)), detrend(detrend) {}
  virtual ~Base() = default;

  //! impute one value per unknown, optionally removing and re-adding a trend
  /*! When \c Detrend::affine is selected, fit an affine trend to the bordering
   *  known data, solve the imputation system on the detrended field, then add
   *  the trend back at each unknown (a "universal kriging" decomposition). The
   *  matrix of unknown coefficients is unchanged by detrending; only the known
   *  data folded into the right-hand side is shifted, and the trend is re-added
   *  to the solution. The operator is thereby treated as annihilating the
   *  trend, so the (bounded) residual extrapolates flat while the trend carries
   *  any global gradient into the imputed region. */
  Vec solve() {
    const std::optional<int> degree = detrend_degree(detrend);
    const std::optional<PolynomialTrend> trend =
        degree ? fit_polynomial_trend(*degree) : std::optional<PolynomialTrend>();
    if (!trend)
      return assemble_and_solve();
    // Temporarily detrend the known-value accessor used during assembly. The
    //   imputer is single-use, so the swap need not be unwound on the throwing
    //   paths of assemble_and_solve().
    ValueFn original = value_at;
    value_at = [original, t = *trend](const Position &p) -> double { return original(p) - t.eval(p); };
    Vec x = assemble_and_solve();
    value_at = std::move(original);
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    for (ssize_t i = 0; i != num_unknowns; ++i)
      x[i] += trend->eval(position_of(static_cast<Voxel2Vector::index_t>(i)));
    return x;
  }

private:
  //! assemble and solve the sparse linear system, returning one value per unknown
  /*! The coefficient matrix is sparse by construction: every finite-difference
   *  equation couples its centre to only a handful of neighbouring unknowns. The
   *  least-squares methods are solved by sparse QR and the square method by
   *  sparse LU, the direct sparse analogues of the dense column-pivoted QR and
   *  partial-pivot LU they replace. Because each system is full column rank (the
   *  underdetermined-column guard below rejects any unconstrained unknown), the
   *  least-squares solution is unique and so independent of the factorisation
   *  used. */
  Vec assemble_and_solve() {
    const std::vector<Equation> equations = assemble();
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    const ssize_t num_equations = static_cast<ssize_t>(equations.size());
    if (num_equations == 0)
      throw Exception("no linear equations could be assembled for the imputation region");
    // setFromTriplets() sums duplicate (row, column) contributions, reproducing
    //   the coefficient accumulation of the former dense assembly.
    std::vector<Triplet> triplets;
    size_t num_terms = 0;
    for (const Equation &eq : equations)
      num_terms += eq.terms.size();
    triplets.reserve(num_terms);
    Vec b = Vec::Zero(num_equations);
    for (ssize_t row = 0; row != num_equations; ++row) {
      for (const auto &term : equations[row].terms)
        triplets.emplace_back(
            static_cast<SparseMat::StorageIndex>(row), static_cast<SparseMat::StorageIndex>(term.first), term.second);
      b[row] = equations[row].rhs;
    }
    SparseMat A(num_equations, num_unknowns);
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    // Guard against any unknown that is referenced by no equation (or whose
    //   contributions cancel exactly): such a column would be left unconstrained
    //   by the solver and yield an arbitrary value. The matrix is column-major,
    //   so iterating a column's inner entries is the sparse analogue of the
    //   former dense Mat::col().isZero() test.
    for (ssize_t col = 0; col != num_unknowns; ++col) {
      bool referenced = false;
      for (SparseMat::InnerIterator it(A, col); it; ++it) {
        if (it.value() != 0.0) {
          referenced = true;
          break;
        }
      }
      if (!referenced)
        throw Exception("imputation system is underdetermined"
                        " (a voxel to be imputed is not coupled to any usable data)");
    }
    Vec x;
    switch (solve_type()) {
    case SolveType::SquareDirect: {
      if (num_equations != num_unknowns)
        throw Exception("internal error: square imputation solver received a non-square system");
      Eigen::SparseLU<SparseMat> solver;
      solver.compute(A);
      if (solver.info() != Eigen::Success)
        throw Exception("imputation sparse LU factorisation failed");
      x = solver.solve(b);
      if (solver.info() != Eigen::Success)
        throw Exception("imputation sparse LU solve failed");
      break;
    }
    case SolveType::LeastSquares: {
      Eigen::SparseQR<SparseMat, Eigen::COLAMDOrdering<SparseMat::StorageIndex>> solver;
      solver.compute(A);
      if (solver.info() != Eigen::Success)
        throw Exception("imputation sparse QR factorisation failed");
      x = solver.solve(b);
      if (solver.info() != Eigen::Success)
        throw Exception("imputation sparse QR solve failed");
      break;
    }
    }
    if (!x.allFinite())
      throw Exception("imputation linear solver produced non-finite values");
    return x;
  }

protected:
  const Voxel2Vector &v2v;
  ValueFn value_at;
  InFovFn in_fov;
  const Detrend detrend;

  virtual std::vector<Equation> assemble() = 0;
  virtual SolveType solve_type() const = 0;

  //! the spatial position of unknown \a index
  Position position_of(const Voxel2Vector::index_t index) const {
    const std::vector<Voxel2Vector::index_t> &p = v2v[index];
    return Position(static_cast<ssize_t>(p[0]), static_cast<ssize_t>(p[1]), static_cast<ssize_t>(p[2]));
  }

  //! fit a polynomial trend of the given \a degree to the known data bordering the unknown region
  /*! The trend is fit by least squares to every in-FoV known voxel within a
   *  small Chebyshev radius of any unknown, i.e. the band of valid data that
   *  drives the extrapolation. The radius scales with the degree
   *  (<tt>degree + 1</tt>), so that a quadratic trend sees enough distinct
   *  positions along the boundary normal to constrain its curvature. The basis
   *  is centred on the centroid of the gathered data for conditioning. Returns
   *  \c std::nullopt when too few known samples are available to constrain the
   *  coefficients, or when the fit is non-finite; the caller then proceeds
   *  without detrending. */
  std::optional<PolynomialTrend> fit_polynomial_trend(const int degree) const {
    const ssize_t radius = degree + 1;
    const ssize_t ncoeffs = num_polynomial_coeffs(degree);
    std::set<std::array<ssize_t, 3>> known_set;
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    for (ssize_t i = 0; i != num_unknowns; ++i) {
      const Position u = position_of(static_cast<Voxel2Vector::index_t>(i));
      for (ssize_t dx = -radius; dx <= radius; ++dx) {
        for (ssize_t dy = -radius; dy <= radius; ++dy) {
          for (ssize_t dz = -radius; dz <= radius; ++dz) {
            const Position p(u[0] + dx, u[1] + dy, u[2] + dz);
            // Skip unknowns (tracked by v2v) and out-of-FoV positions: only
            //   genuine known data may constrain the trend.
            if (v2v(p) != Voxel2Vector::invalid)
              continue;
            if (!in_fov(p))
              continue;
            known_set.insert({p[0], p[1], p[2]});
          }
        }
      }
    }
    if (static_cast<ssize_t>(known_set.size()) < ncoeffs)
      return std::nullopt;
    const ssize_t num_known = static_cast<ssize_t>(known_set.size());
    // Centre the basis on the centroid of the known samples for conditioning;
    //   this is a reparameterisation that leaves the fitted surface unchanged.
    Eigen::Vector3d origin = Eigen::Vector3d::Zero();
    for (const auto &c : known_set)
      origin += Eigen::Vector3d(static_cast<double>(c[0]), static_cast<double>(c[1]), static_cast<double>(c[2]));
    origin /= static_cast<double>(num_known);
    Mat design(num_known, ncoeffs);
    Vec observations(num_known);
    ssize_t row = 0;
    for (const auto &c : known_set) {
      const Position p(c[0], c[1], c[2]);
      design.row(row) = polynomial_basis(p.cast<double>().matrix() - origin, degree).transpose();
      observations[row] = value_at(p);
      ++row;
    }
    const Vec coeffs = design.colPivHouseholderQr().solve(observations);
    if (!coeffs.allFinite())
      return std::nullopt;
    return PolynomialTrend{degree, origin, coeffs};
  }

  //! add a contribution to equation \a eq for offset position \a p with weight \a w
  /*! Unknown positions populate the matrix; in-FoV known positions move to the
   *  right-hand side. Returns true if the position referenced an unknown.
   *  \note Each entry in Equation::terms is therefore always an unknown column,
   *    so a non-empty terms vector is equivalent to "this equation constrains at
   *    least one unknown". */
  bool add_term(Equation &eq, const Position &p, const double w) const {
    const Voxel2Vector::index_t index = v2v(p);
    if (index != Voxel2Vector::invalid) {
      eq.terms.emplace_back(index, w);
      return true;
    }
    if (in_fov(p))
      eq.rhs -= w * value_at(p);
    return false;
  }

  //! the set of in-FoV equation centres within Chebyshev radius \a radius of any unknown
  std::vector<std::array<ssize_t, 3>> neighbourhood_centres(const ssize_t radius) const {
    std::set<std::array<ssize_t, 3>> centre_set;
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    for (ssize_t i = 0; i != num_unknowns; ++i) {
      const Position u = position_of(static_cast<Voxel2Vector::index_t>(i));
      for (ssize_t dx = -radius; dx <= radius; ++dx) {
        for (ssize_t dy = -radius; dy <= radius; ++dy) {
          for (ssize_t dz = -radius; dz <= radius; ++dz) {
            const Position centre(u[0] + dx, u[1] + dy, u[2] + dz);
            if (in_fov(centre))
              centre_set.insert({centre[0], centre[1], centre[2]});
          }
        }
      }
    }
    return {centre_set.begin(), centre_set.end()};
  }

  //! whether every non-zero offset of stencil \a s lies in-FoV when centred at \a centre
  bool stencil_supported(const Stencil &s, const Position &centre) const {
    const ssize_t radius = (s.dimension(0) - 1) / 2;
    for (ssize_t dx = -radius; dx <= radius; ++dx) {
      for (ssize_t dy = -radius; dy <= radius; ++dy) {
        for (ssize_t dz = -radius; dz <= radius; ++dz) {
          if (s(dx + radius, dy + radius, dz + radius) == 0.0)
            continue;
          if (!in_fov(centre + Position(dx, dy, dz)))
            return false;
        }
      }
    }
    return true;
  }

  //! build one equation from stencil \a s centred at \a centre (assumes full support)
  Equation emit(const Stencil &s, const Position &centre) const {
    Equation eq;
    eq.rhs = 0.0;
    const ssize_t radius = (s.dimension(0) - 1) / 2;
    for (ssize_t dx = -radius; dx <= radius; ++dx) {
      for (ssize_t dy = -radius; dy <= radius; ++dy) {
        for (ssize_t dz = -radius; dz <= radius; ++dz) {
          const double w = s(dx + radius, dy + radius, dz + radius);
          if (w == 0.0)
            continue;
          add_term(eq, centre + Position(dx, dy, dz), w);
        }
      }
    }
    return eq;
  }

  //! build an axis-reduced discrete-Laplacian equation centred at \a centre
  /*! Along each axis where both neighbours are in-FoV, the second-difference is
   *  included; this remains exactly zero for linear and harmonic fields, so the
   *  reduced operator at the image boundary (face -> 2D 5-point; edge -> 1D
   *  [1,-2,1]) reproduces such fields without bias. A voxel with no complete
   *  axis falls back to an averaging (spring) equation. */
  Equation laplacian_equation(const Position &centre) const {
    Equation eq;
    eq.rhs = 0.0;
    double centre_weight = 0.0;
    size_t axes_kept = 0;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      Position step(0, 0, 0);
      step[axis] = 1;
      const Position plus = centre + step;
      const Position minus = centre - step;
      if (in_fov(plus) && in_fov(minus)) {
        add_term(eq, plus, 1.0);
        add_term(eq, minus, 1.0);
        centre_weight -= 2.0;
        ++axes_kept;
      }
    }
    if (axes_kept > 0) {
      add_term(eq, centre, centre_weight);
      return eq;
    }
    return spring_equation(centre);
  }

  //! build an averaging (spring) equation: centre equals the mean of its in-FoV neighbours
  Equation spring_equation(const Position &centre) const {
    Equation eq;
    eq.rhs = 0.0;
    std::vector<Position> neighbours;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      Position step(0, 0, 0);
      step[axis] = 1;
      const std::array<Position, 2> candidates{Position(centre + step), Position(centre - step)};
      for (const Position &p : candidates) {
        if (in_fov(p))
          neighbours.push_back(p);
      }
    }
    if (neighbours.empty())
      throw Exception("imputation voxel has no in-bounds neighbours; image is degenerate");
    add_term(eq, centre, 1.0);
    const double w = -1.0 / static_cast<double>(neighbours.size());
    for (const Position &p : neighbours)
      add_term(eq, p, w);
    return eq;
  }

  //! build a biharmonic equation as the composition of two axis-reduced Laplacians
  /*! Realises the natural (free-plate) boundary conditions of the thin-plate
   *  energy: del-squared is applied twice, each application reduced per axis at
   *  the image boundary (see laplacian_equation()), and the inner Laplacian
   *  field is taken as zero wherever no axis is complete (the natural condition
   *  del^2 u = 0 on a free boundary). Because the reduced Laplacian annihilates
   *  affine fields, so does its square: the operator extrapolates linearly
   *  beyond the data rather than flattening to a constant, as the single
   *  Laplacian fallback does. The composition of two full 7-point Laplacians
   *  equals the dense 25-point biharmonic stencil, so the operator is exact
   *  wherever that full support lies in the field of view. */
  Equation biharmonic_natural_equation(const Position &centre) const {
    // Outer Laplacian: the weighted positions whose inner Laplacian fields are
    //   summed (centre plus each in-FoV axis-neighbour pair).
    std::vector<std::pair<Position, double>> outer;
    double outer_centre_weight = 0.0;
    size_t outer_axes = 0;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      Position step(0, 0, 0);
      step[axis] = 1;
      const Position plus = centre + step;
      const Position minus = centre - step;
      if (in_fov(plus) && in_fov(minus)) {
        outer.emplace_back(plus, 1.0);
        outer.emplace_back(minus, 1.0);
        outer_centre_weight -= 2.0;
        ++outer_axes;
      }
    }
    // With no complete axis the biharmonic cannot be formed; defer to the
    //   Laplacian (which itself falls back to an averaging equation).
    if (outer_axes == 0)
      return laplacian_equation(centre);
    outer.emplace_back(centre, outer_centre_weight);

    Equation eq;
    eq.rhs = 0.0;
    for (const auto &term : outer) {
      const Position &p = term.first;
      const double outer_weight = term.second;
      // Inner axis-reduced Laplacian field at p, scaled by the outer weight.
      double inner_centre_weight = 0.0;
      for (ssize_t axis = 0; axis != 3; ++axis) {
        Position step(0, 0, 0);
        step[axis] = 1;
        const Position plus = p + step;
        const Position minus = p - step;
        if (in_fov(plus) && in_fov(minus)) {
          add_term(eq, plus, outer_weight);
          add_term(eq, minus, outer_weight);
          inner_centre_weight -= 2.0;
        }
      }
      add_term(eq, p, outer_weight * inner_centre_weight);
    }
    return eq;
  }

  //! shared stencil-based assembly
  /*! A stencil contributes an equation at a centre only if its full support is
   *  in-FoV: a finite-difference operator that is identically zero for the
   *  target field class would otherwise be biased by truncating its support.
   *  Boundary handling is therefore "skip the stencil", never "drop entries". */
  std::vector<Equation> assemble_stencils(const std::vector<Stencil> &stencils, const Centres policy) const {
    std::set<std::array<ssize_t, 3>> centre_set;
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    for (ssize_t i = 0; i != num_unknowns; ++i) {
      const Position u = position_of(static_cast<Voxel2Vector::index_t>(i));
      if (policy == Centres::Unknowns) {
        centre_set.insert({u[0], u[1], u[2]});
        continue;
      }
      // Every voxel whose stencil support reaches an unknown becomes a candidate
      //   equation centre; this maximises the number of unknown columns referenced.
      for (const auto &s : stencils) {
        const ssize_t radius = (s.dimension(0) - 1) / 2;
        for (ssize_t dx = -radius; dx <= radius; ++dx) {
          for (ssize_t dy = -radius; dy <= radius; ++dy) {
            for (ssize_t dz = -radius; dz <= radius; ++dz) {
              if (s(dx + radius, dy + radius, dz + radius) == 0.0)
                continue;
              centre_set.insert({u[0] - dx, u[1] - dy, u[2] - dz});
            }
          }
        }
      }
    }
    std::vector<Equation> equations;
    for (const auto &c : centre_set) {
      const Position centre(c[0], c[1], c[2]);
      if (!in_fov(centre))
        continue;
      for (const auto &s : stencils) {
        if (!stencil_supported(s, centre))
          continue;
        Equation eq = emit(s, centre);
        if (!eq.terms.empty())
          equations.push_back(std::move(eq));
      }
    }
    return equations;
  }
};

//! Laplacian (del-squared) imputation; Inpaint_nans method 0
/*! Least-squares solve of the discrete Laplace equation, with equations centred
 *  at every unknown and at each of its in-FoV neighbours. The operator is
 *  reduced per axis at the image boundary (see laplacian_equation()). */
class Laplacian : public Base {
public:
  using Base::Base;

protected:
  SolveType solve_type() const override { return SolveType::LeastSquares; }
  std::vector<Equation> assemble() override {
    const std::vector<std::array<ssize_t, 3>> centres = neighbourhood_centres(1);
    std::vector<Equation> equations;
    for (const auto &c : centres) {
      const Position centre(c[0], c[1], c[2]);
      Equation eq = laplacian_equation(centre);
      if (!eq.terms.empty())
        equations.push_back(std::move(eq));
    }
    return equations;
  }
};

//! Laplacian imputation as a square system; Inpaint_nans method 2
/*! Exactly one del-squared equation per unknown, producing a square system
 *  solved by LU decomposition; the operator is reduced per axis at the image
 *  boundary (see laplacian_equation()). */
class LaplacianSquare : public Base {
public:
  using Base::Base;

protected:
  SolveType solve_type() const override { return SolveType::SquareDirect; }
  std::vector<Equation> assemble() override {
    std::vector<Equation> equations;
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    for (ssize_t i = 0; i != num_unknowns; ++i)
      equations.push_back(laplacian_equation(position_of(static_cast<Voxel2Vector::index_t>(i))));
    return equations;
  }
};

//! Biharmonic (del-to-the-fourth) imputation; Inpaint_nans method 3
/*! Least-squares solve of the discrete biharmonic equation. Where the full
 *  del^4 support is in-FoV the dense biharmonic stencil is used; otherwise the
 *  equation is built as the composition of two axis-reduced Laplacians, which
 *  imposes the natural (free-plate) boundary conditions and so extrapolates
 *  linearly beyond the data rather than flattening (see
 *  biharmonic_natural_equation()). */
class Biharmonic : public Base {
public:
  using Base::Base;

protected:
  SolveType solve_type() const override { return SolveType::LeastSquares; }
  std::vector<Equation> assemble() override {
    const Stencil del4 = biharmonic_stencil();
    const ssize_t radius4 = (del4.dimension(0) - 1) / 2;
    const std::vector<std::array<ssize_t, 3>> centres = neighbourhood_centres(radius4);
    std::vector<Equation> equations;
    for (const auto &c : centres) {
      const Position centre(c[0], c[1], c[2]);
      Equation eq = stencil_supported(del4, centre) ? emit(del4, centre) : biharmonic_natural_equation(centre);
      if (!eq.terms.empty())
        equations.push_back(std::move(eq));
    }
    return equations;
  }
};

//! Hessian-energy imputation with natural (free) boundary conditions
/*! Least-squares minimisation of the discrete Hessian (Frobenius) energy
 *  assembled from hessian_stencils(). As with the biharmonic method the
 *  operator annihilates affine fields and so extrapolates a linear trend beyond
 *  a one-sided boundary rather than flattening; but where the squared-Laplacian
 *  energy realised by the biharmonic solve introduces a boundary bias, the
 *  Hessian energy's natural boundary conditions (here imposed implicitly by
 *  emitting each second-derivative equation only where its stencil is fully
 *  supported) introduce none. See Stein, Jacobson et al., "Natural Boundary
 *  Conditions for Smoothing in Geometry Processing", ACM TOG 2018. This makes
 *  it the preferred default when extrapolating a field across a wide non-finite
 *  border (see \c MR::Interp::Deform). */
class Hessian : public Base {
public:
  using Base::Base;

protected:
  SolveType solve_type() const override { return SolveType::LeastSquares; }
  std::vector<Equation> assemble() override {
    return assemble_stencils(hessian_stencils(), Centres::UnknownsAndNeighbours);
  }
};

//! Spring imputation; Inpaint_nans method 4
/*! For each unknown, one equation per in-FoV axis-neighbour constrains the two
 *  intensities to be equal (a unit "spring"). Yields constant extrapolation
 *  behaviour; solved as an overdetermined least-squares problem. */
class Spring : public Base {
public:
  using Base::Base;

protected:
  SolveType solve_type() const override { return SolveType::LeastSquares; }
  std::vector<Equation> assemble() override {
    std::vector<Stencil> stencils;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      for (const ssize_t sign : {static_cast<ssize_t>(1), static_cast<ssize_t>(-1)}) {
        Position offset(0, 0, 0);
        offset[axis] = sign;
        Stencil s = zero_stencil(1);
        bake(s, Position(0, 0, 0), 1.0);
        bake(s, offset, -1.0);
        stencils.push_back(std::move(s));
      }
    }
    return assemble_stencils(stencils, Centres::Unknowns);
  }
};

//! Spherically-isotropic imputation (new); del-squared (\a order 2) or biharmonic (\a order 4)
/*! Lifts the 13-direction, SH-weighted prediction stencil of \c mrsense1fix
 *  into the same global solve as the other methods, so mutually-dependent
 *  unknowns are resolved simultaneously. Least-squares. */
class Isotropic : public Base {
public:
  Isotropic(
      const Voxel2Vector &v2v, ValueFn value_at, InFovFn in_fov, const int order, const Detrend detrend = Detrend::none)
      : Base(v2v, std::move(value_at), std::move(in_fov), detrend), order(order) {}

protected:
  SolveType solve_type() const override { return SolveType::LeastSquares; }
  std::vector<Equation> assemble() override {
    return assemble_stencils(isotropic_stencils(order), Centres::UnknownsAndNeighbours);
  }

private:
  const int order;
};

//! construct the imputation method object selected by \a method
inline std::unique_ptr<Base> make_imputer(const Method method,
                                          const Voxel2Vector &v2v,
                                          ValueFn value_at,
                                          InFovFn in_fov,
                                          const Detrend detrend = Detrend::none) {
  switch (method) {
  case Method::laplacian:
    return std::make_unique<Laplacian>(v2v, std::move(value_at), std::move(in_fov), detrend);
  case Method::laplaciansq:
    return std::make_unique<LaplacianSquare>(v2v, std::move(value_at), std::move(in_fov), detrend);
  case Method::biharmonic:
    return std::make_unique<Biharmonic>(v2v, std::move(value_at), std::move(in_fov), detrend);
  case Method::hessian:
    return std::make_unique<Hessian>(v2v, std::move(value_at), std::move(in_fov), detrend);
  case Method::spring:
    return std::make_unique<Spring>(v2v, std::move(value_at), std::move(in_fov), detrend);
  case Method::isotropic2:
    return std::make_unique<Isotropic>(v2v, std::move(value_at), std::move(in_fov), 2, detrend);
  case Method::isotropic4:
    return std::make_unique<Isotropic>(v2v, std::move(value_at), std::move(in_fov), 4, detrend);
  }
  assert(false);
  return nullptr;
}

//! @}

} // namespace MR::Impute
