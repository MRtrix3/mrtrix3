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
#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// Eigen plugin configuration must precede inclusion of any Eigen header,
//   including the unsupported Tensor module used here.
#include "eigen_plugins/eigen_plugins.h"
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
enum class Method { laplacian, laplaciansq, biharmonic, spring, isotropic2, isotropic4 };

//! a 3D spatial position or offset, in voxels
using Position = Eigen::Array<ssize_t, 3, 1>;

//! a centred dense weight kernel for a single finite-difference equation
/*! The tensor side length is <tt>2 * radius + 1</tt>; the weight associated
 *  with spatial offset <tt>(dx, dy, dz)</tt> resides at tensor coordinate
 *  <tt>(dx + radius, dy + radius, dz + radius)</tt>. */
using Stencil = Eigen::Tensor<double, 3>;

using Mat = Eigen::MatrixXd;
using Vec = Eigen::VectorXd;

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
  Base(const Voxel2Vector &v2v, ValueFn value_at, InFovFn in_fov)
      : v2v(v2v), value_at(std::move(value_at)), in_fov(std::move(in_fov)) {}
  virtual ~Base() = default;

  //! assemble and solve the dense linear system, returning one value per unknown
  Vec solve() {
    const std::vector<Equation> equations = assemble();
    const ssize_t num_unknowns = static_cast<ssize_t>(v2v.size());
    const ssize_t num_equations = static_cast<ssize_t>(equations.size());
    if (num_equations == 0)
      throw Exception("no linear equations could be assembled for the imputation region");
    Mat A = Mat::Zero(num_equations, num_unknowns);
    Vec b = Vec::Zero(num_equations);
    for (ssize_t row = 0; row != num_equations; ++row) {
      for (const auto &term : equations[row].terms)
        A(row, term.first) += term.second;
      b[row] = equations[row].rhs;
    }
    // Guard against any unknown that is referenced by no equation: such a column
    //   would be left unconstrained by the solver and yield an arbitrary value.
    for (ssize_t col = 0; col != num_unknowns; ++col) {
      if (A.col(col).isZero(0.0))
        throw Exception("imputation system is underdetermined"
                        " (a voxel to be imputed is not coupled to any usable data)");
    }
    Vec x;
    switch (solve_type()) {
    case SolveType::SquareDirect:
      if (num_equations != num_unknowns)
        throw Exception("internal error: square imputation solver received a non-square system");
      x = A.partialPivLu().solve(b);
      break;
    case SolveType::LeastSquares:
      x = A.colPivHouseholderQr().solve(b);
      break;
    }
    if (!x.allFinite())
      throw Exception("imputation linear solver produced non-finite values");
    return x;
  }

protected:
  const Voxel2Vector &v2v;
  ValueFn value_at;
  InFovFn in_fov;

  virtual std::vector<Equation> assemble() = 0;
  virtual SolveType solve_type() const = 0;

  //! the spatial position of unknown \a index
  Position position_of(const Voxel2Vector::index_t index) const {
    const std::vector<Voxel2Vector::index_t> &p = v2v[index];
    return Position(static_cast<ssize_t>(p[0]), static_cast<ssize_t>(p[1]), static_cast<ssize_t>(p[2]));
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
 *  del^4 support is in-FoV the biharmonic operator is used; otherwise the
 *  equation degrades gracefully to the axis-reduced Laplacian (which itself
 *  falls back to an averaging equation at a corner). */
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
      Equation eq = stencil_supported(del4, centre) ? emit(del4, centre) : laplacian_equation(centre);
      if (!eq.terms.empty())
        equations.push_back(std::move(eq));
    }
    return equations;
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
  Isotropic(const Voxel2Vector &v2v, ValueFn value_at, InFovFn in_fov, const int order)
      : Base(v2v, std::move(value_at), std::move(in_fov)), order(order) {}

protected:
  SolveType solve_type() const override { return SolveType::LeastSquares; }
  std::vector<Equation> assemble() override {
    return assemble_stencils(isotropic_stencils(order), Centres::UnknownsAndNeighbours);
  }

private:
  const int order;
};

//! construct the imputation method object selected by \a method
inline std::unique_ptr<Base>
make_imputer(const Method method, const Voxel2Vector &v2v, ValueFn value_at, InFovFn in_fov) {
  switch (method) {
  case Method::laplacian:
    return std::make_unique<Laplacian>(v2v, std::move(value_at), std::move(in_fov));
  case Method::laplaciansq:
    return std::make_unique<LaplacianSquare>(v2v, std::move(value_at), std::move(in_fov));
  case Method::biharmonic:
    return std::make_unique<Biharmonic>(v2v, std::move(value_at), std::move(in_fov));
  case Method::spring:
    return std::make_unique<Spring>(v2v, std::move(value_at), std::move(in_fov));
  case Method::isotropic2:
    return std::make_unique<Isotropic>(v2v, std::move(value_at), std::move(in_fov), 2);
  case Method::isotropic4:
    return std::make_unique<Isotropic>(v2v, std::move(value_at), std::move(in_fov), 4);
  }
  assert(false);
  return nullptr;
}

//! @}

} // namespace MR::Impute
