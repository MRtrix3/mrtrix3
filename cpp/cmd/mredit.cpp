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

#include <array>
#include <cstddef>
#include <filesystem>
#include <set>
#include <vector>

#include "app.h"
#include "command.h"
#include "exception.h"
#include "image.h"
#include "image_helpers.h"
#include "mrtrix.h"
#include "transform.h"
#include "types.h"

#include "algo/copy.h"
#include "algo/loop.h"

using namespace MR;
using namespace App;

// TODO:
// * Operate on mask images rather than arbitrary images?
// * Remove capability to edit in-place - just deal with image swapping in the script?
// clang-format off

void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Directly edit the intensities within an image from the command-line";

  DESCRIPTION
  + "A range of options are provided to enable direct editing of"
    " voxel intensities based on voxel / real-space coordinates."
    " If only one image path is provided,"
    " the image will be edited in-place "
    "(use at own risk);"
    " if input and output image paths are provided,"
    " the output will contain the edited image,"
    " and the original image will not be modified in any way."

  + "All editing options are by default interpreted with respect to the image voxel grid:"
    " -plane fills a whole image plane perpendicular to one image axis;"
    " -voxel addresses a single voxel by its indices;"
    " and the geometric primitives -sphere, -ellipsoid, -cuboid and -line take positions as voxel indices,"
    " sizes (radii and side lengths) measured in voxels,"
    " and principal axes aligned with the image axes."
    " If the -scanner option is used,"
    " these inputs are instead interpreted with respect to scanner space:"
    " -plane fills a digital plane perpendicular to one scanner axis at a given offset in millimetres;"
    " -voxel modifies the single voxel that contains the specified scanner-space position;"
    " positions are scanner-space coordinates in millimetres;"
    " sizes are measured in millimetres;"
    " and the principal axes of each shape are aligned with the scanner axes."
    " For an image whose voxel axes are not aligned with the scanner axes,"
    " or whose voxels are not isotropic,"
    " the two interpretations can yield substantially different results."

  + "Unlike most MRtrix3 commands,"
    " the order in which editing options are provided on the command-line is significant:"
    " each stencil is applied to the image in turn,"
    " in the order specified,"
    " so that wherever two stencils overlap the one provided later takes precedence.";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in()
  + Argument ("output", "the (optional) output image").type_image_out().optional();

  OPTIONS
  + Option ("plane", "fill one or more planes perpendicular to the specified axis").allow_multiple()
    + Argument ("axis").type_integer (0, 2)
    + Argument ("coord").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("sphere", "draw a sphere of the specified radius").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("radius").type_float()
    + Argument ("value").type_float()

  + Option ("ellipsoid", "draw an ellipsoid"
                         " (a single radius value yields a sphere)").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("radii").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("cuboid", "draw a rectangular cuboid"
                      " (a single side-length value yields a cube)").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("size").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("line", "draw a single-voxel-thick line between two points"
                    " using Bresenham's algorithm").allow_multiple()
    + Argument ("first").type_sequence_float()
    + Argument ("second").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("voxel", "change the image value within a single voxel").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("scanner", "interpret all stencil positions, sizes and orientations in scanner space (mm),"
                       " rather than with respect to the voxel grid");

}
// clang-format on

namespace {

class Vox : public Eigen::Array3i {
public:
  using Eigen::Array3i::Array3i;
  Vox(const Eigen::Vector3d &p)
      : Eigen::Array3i{static_cast<int>(std::round(p[0])),
                       static_cast<int>(std::round(p[1])),
                       static_cast<int>(std::round(p[2]))} {}
  bool operator<(const Vox &i) const {
    return (i[0] == (*this)[0] ? (i[1] == (*this)[1] ? (i[2] < (*this)[2]) : (i[1] < (*this)[1]))
                               : (i[0] < (*this)[0]));
  }
};

const std::array<Vox, 6> voxel_offsets = {
    Vox(0, 0, -1), Vox(0, 0, 1), Vox(0, -1, 0), Vox(0, 1, 0), Vox(-1, 0, 0), Vox(1, 0, 0)};

//! The coordinate space in which stencil geometry (position, size, orientation) is defined
enum class Space { voxel, scanner };

//! A resolved stencil reference point:
//!   `centre` is its location within the active working space
//!   (voxel indices for Space::voxel, scanner-space mm for Space::scanner),
//!   while `seed` is the nearest integer voxel used to seed the region-growing fill
struct Stencil {
  Eigen::Vector3d centre;
  Vox seed;
};

//! Resolve a triplet of command-line coordinates within the active working space
Stencil resolve(const std::vector<default_type> &coords, const Space space, const Transform &transform) {
  if (coords.size() != 3)
    throw Exception("Coordinates must be specified using 3 comma-separated values");
  const Eigen::Vector3d input(coords[0], coords[1], coords[2]);
  if (space == Space::scanner)
    return {input, Vox(Eigen::Vector3d(transform.scanner2voxel * input))};
  return {input, Vox(input)};
}

//! Region-grow from one or more seed voxels,
//!   assigning `value` to every voxel whose centre
//!   (expressed within the active working space)
//!   satisfies the `inside` predicate;
//!   valid for any convex region that contains at least one seed
template <class Predicate>
void flood_fill(Image<float> &out,
                const Space space,
                const Transform &transform,
                const std::vector<Vox> &seeds,
                const Predicate &inside,
                const float value) {
  std::set<Vox> processed;
  std::vector<Vox> to_expand;
  for (const auto &seed : seeds) {
    if (processed.insert(seed).second)
      to_expand.push_back(seed);
  }
  while (!to_expand.empty()) {
    const Vox v(to_expand.back());
    to_expand.pop_back();
    const Eigen::Vector3d v_voxel = v.matrix().cast<default_type>();
    const Eigen::Vector3d v_working =
        (space == Space::scanner) ? Eigen::Vector3d(transform.voxel2scanner * v_voxel) : v_voxel;
    if (!inside(v_working))
      continue;
    if (!is_out_of_bounds(out, v)) {
      assign_pos_of(v).to(out);
      out.value() = value;
    }
    for (size_t i = 0; i != 6; ++i) {
      const Vox v_adj(v + voxel_offsets[i]);
      if (processed.insert(v_adj).second)
        to_expand.push_back(v_adj);
    }
  }
}

//! Rasterize a single-voxel-thick line between two voxels using Bresenham's algorithm.
/*! The line is traced one voxel at a time along the "driving" axis,
 *  i.e. the axis along which the two endpoints are furthest apart in voxel indices;
 *  the two minor axes are advanced by accumulating fractional error terms,
 *  with a step taken whenever the accumulated error crosses the half-voxel threshold.
 *  Note that, in keeping with the request that drove this implementation,
 *  the error terms are deliberately retained as floating-point quantities
 *  rather than reformulated into the customary integer-only arithmetic. */
void draw_line(Image<float> &out, const Vox &start, const Vox &end, const float value) {
  const Eigen::Array3i delta = end - start;
  const Eigen::Array3i abs_delta = delta.abs();
  const Eigen::Array3i step = delta.sign();
  Eigen::Index drive = 0;
  const int n_steps = abs_delta.maxCoeff(&drive);
  // Floating-point per-step increments for the two minor axes
  const Eigen::Array3d increment =
      abs_delta.cast<default_type>() / static_cast<default_type>(n_steps > 0 ? n_steps : 1);
  Eigen::Array3d error = Eigen::Array3d::Zero();
  Vox v(start);
  for (int i = 0;; ++i) {
    if (!is_out_of_bounds(out, v)) {
      assign_pos_of(v).to(out);
      out.value() = value;
    }
    if (i == n_steps)
      break;
    v[drive] += step[drive];
    for (Eigen::Index axis = 0; axis != 3; ++axis) {
      if (axis == drive)
        continue;
      error[axis] += increment[axis];
      if (error[axis] >= 0.5) {
        v[axis] += step[axis];
        error[axis] -= 1.0;
      }
    }
  }
}

//! Fill one or more planes (-plane option)
void apply_plane(Image<float> &out, const ParsedOption &opt, const Space space, const Transform &transform) {
  if (out.ndim() != 3)
    throw Exception("-plane option only works for 3D images");
  const size_t axis = opt[0];
  const std::vector<default_type> coords = parse_floats(opt[1]);
  const float value = opt[2];

  if (space == Space::voxel) {
    // `axis` is an image axis; each `coord` is a voxel index of a whole image plane
    const std::array<size_t, 2> loop_axes{{axis == 0 ? size_t(1) : size_t(0), axis == 2 ? size_t(1) : size_t(2)}};
    for (const default_type coord : coords) {
      if (std::round(coord) != coord)
        throw Exception("Voxel-grid -plane coordinates must be integers"
                        " (use -scanner for millimetre offsets)");
      if (coord < 0.0 || coord >= out.size(axis))
        throw Exception("-plane coordinate " + str(coord) + " is outside the image field of view along axis " +
                        str(axis));
      out.index(axis) = static_cast<ssize_t>(coord);
      for (auto outer = Loop(loop_axes[0])(out); outer; ++outer) {
        for (auto inner = Loop(loop_axes[1])(out); inner; ++inner)
          out.value() = value;
      }
    }
    return;
  }

  // `axis` is a scanner axis; each `coord` is an offset (mm) along that scanner axis.
  // The scanner coordinate along `axis` is an affine function of the voxel indices:
  //   scanner[axis](v) = gradient . v + offset
  // To colour a hole-free, single-voxel-thick "digital plane" without any slab-thickness
  //   parameter, the image axis whose unit voxel step induces the largest change in this
  //   scanner coordinate (the largest |gradient| component) is solved for, while the other
  //   two image axes are enumerated exhaustively; this colours exactly one voxel per column
  //   and is correct for anisotropic and obliquely-oriented voxel grids.
  const Eigen::Vector3d gradient = transform.voxel2scanner.linear().row(axis).transpose();
  const default_type offset = transform.voxel2scanner.translation()[axis];
  Eigen::Index free_axis = 0;
  gradient.cwiseAbs().maxCoeff(&free_axis);
  std::array<Eigen::Index, 2> plane_axes{{0, 0}};
  size_t n = 0;
  for (Eigen::Index a = 0; a != 3; ++a) {
    if (a != free_axis)
      plane_axes[n++] = a;
  }
  for (const default_type coord : coords) {
    for (ssize_t i0 = 0; i0 != out.size(plane_axes[0]); ++i0) {
      for (ssize_t i1 = 0; i1 != out.size(plane_axes[1]); ++i1) {
        const default_type numerator = coord - offset - (gradient[plane_axes[0]] * static_cast<default_type>(i0)) -
                                       (gradient[plane_axes[1]] * static_cast<default_type>(i1));
        const auto free_index = static_cast<ssize_t>(std::round(numerator / gradient[free_axis]));
        if (free_index < 0 || free_index >= out.size(free_axis))
          continue;
        out.index(free_axis) = free_index;
        out.index(plane_axes[0]) = i0;
        out.index(plane_axes[1]) = i1;
        out.value() = value;
      }
    }
  }
}

//! Draw a sphere (-sphere option)
void apply_sphere(Image<float> &out, const ParsedOption &opt, const Space space, const Transform &transform) {
  if (out.ndim() != 3)
    throw Exception("-sphere option only works for 3D images");
  const Stencil ref = resolve(parse_floats(opt[0]), space, transform);
  const default_type radius = opt[1];
  const float value = opt[2];
  if (radius <= 0.0)
    throw Exception("-sphere radius must be positive");
  auto inside = [centre = ref.centre, radius](const Eigen::Vector3d &p) { return (p - centre).norm() < radius; };
  flood_fill(out, space, transform, {ref.seed}, inside, value);
}

//! Draw an ellipsoid (-ellipsoid option)
void apply_ellipsoid(Image<float> &out, const ParsedOption &opt, const Space space, const Transform &transform) {
  if (out.ndim() != 3)
    throw Exception("-ellipsoid option only works for 3D images");
  const Stencil ref = resolve(parse_floats(opt[0]), space, transform);
  const auto radii_in = parse_floats(opt[1]);
  Eigen::Vector3d radii;
  if (radii_in.size() == 1)
    radii.setConstant(radii_in[0]);
  else if (radii_in.size() == 3)
    radii = Eigen::Vector3d(radii_in[0], radii_in[1], radii_in[2]);
  else
    throw Exception("-ellipsoid radii must be either a single value or 3 comma-separated values");
  if ((radii.array() <= 0.0).any())
    throw Exception("-ellipsoid radii must be positive");
  const float value = opt[2];
  auto inside = [centre = ref.centre, radii](const Eigen::Vector3d &p) {
    return ((p - centre).array() / radii.array()).square().sum() < 1.0;
  };
  flood_fill(out, space, transform, {ref.seed}, inside, value);
}

//! Draw a rectangular cuboid (-cuboid option)
void apply_cuboid(Image<float> &out, const ParsedOption &opt, const Space space, const Transform &transform) {
  if (out.ndim() != 3)
    throw Exception("-cuboid option only works for 3D images");
  const Stencil ref = resolve(parse_floats(opt[0]), space, transform);
  const auto size_in = parse_floats(opt[1]);
  Eigen::Vector3d halfsize;
  if (size_in.size() == 1)
    halfsize.setConstant(0.5 * size_in[0]);
  else if (size_in.size() == 3)
    halfsize = 0.5 * Eigen::Vector3d(size_in[0], size_in[1], size_in[2]);
  else
    throw Exception("-cuboid size must be either a single value or 3 comma-separated values");
  if ((halfsize.array() <= 0.0).any())
    throw Exception("-cuboid side lengths must be positive");
  const float value = opt[2];
  auto inside = [centre = ref.centre, halfsize](const Eigen::Vector3d &p) {
    return ((p - centre).array().abs() <= halfsize.array()).all();
  };
  flood_fill(out, space, transform, {ref.seed}, inside, value);
}

//! Draw a single-voxel-thick line (-line option)
void apply_line(Image<float> &out, const ParsedOption &opt, const Space space, const Transform &transform) {
  if (out.ndim() != 3)
    throw Exception("-line option only works for 3D images");
  const Stencil first = resolve(parse_floats(opt[0]), space, transform);
  const Stencil second = resolve(parse_floats(opt[1]), space, transform);
  const float value = opt[2];
  draw_line(out, first.seed, second.seed, value);
}

//! Change the value within a single voxel (-voxel option)
void apply_voxel(Image<float> &out, const ParsedOption &opt, const Space space, const Transform &transform) {
  const auto position = parse_floats(opt[0]);
  const float value = opt[1];
  if (position.size() != out.ndim())
    throw Exception("Image has " + str(out.ndim()) + " dimensions, but -voxel option position " + opt[0].as_text() +
                    " provides " + str(position.size()) + " coordinates");
  if (space == Space::scanner) {
    const Eigen::Vector3d scanner_pos(position[0], position[1], position[2]);
    const Vox voxel(Eigen::Vector3d(transform.scanner2voxel * scanner_pos));
    for (size_t axis = 0; axis != 3; ++axis) {
      if (voxel[axis] < 0 || voxel[axis] >= out.size(axis))
        throw Exception("Scanner-space position " + opt[0].as_text() +
                        " provided to -voxel option does not lie within the image field of view");
      out.index(axis) = voxel[axis];
    }
    for (size_t axis = 3; axis != out.ndim(); ++axis) {
      if (std::round(position[axis]) != position[axis])
        throw Exception("Non-spatial coordinates provided using -voxel option must be provided as integers");
      out.index(axis) = static_cast<ssize_t>(position[axis]);
    }
  } else {
    for (size_t axis = 0; axis != out.ndim(); ++axis) {
      if (std::round(position[axis]) != position[axis])
        throw Exception("Voxel coordinates provided using -voxel option must be provided as integers");
      out.index(axis) = static_cast<ssize_t>(position[axis]);
    }
  }
  out.value() = value;
}

} // namespace

void run() {
  const bool inplace = (argument.size() == 1);
  auto H = Header::open(argument[0]);
  auto in = H.get_image<float>(std::nullopt, inplace); // Need to set read/write flag
  Image<float> out;
  if (inplace) {
    out = Image<float>(in);
  } else {
    if (static_cast<std::filesystem::path>(argument[0])
                .lexically_normal()
                .compare(static_cast<std::filesystem::path>(argument[1]).lexically_normal()) == 0 &&
        !is_dash(argument[0].as_text()))
      throw Exception("Do not provide same image as input and output;"
                      " instead specify image just once and it will be edited in-place");
    out = Image<float>::create(argument[1], H);
    copy(in, out);
  }

  const Transform transform(H);
  const bool scanner = !get_options("scanner").empty();
  if (scanner && H.ndim() < 3)
    throw Exception("Cannot specify scanner-space coordinates if image has less than 3 dimensions");
  const Space space = scanner ? Space::scanner : Space::voxel;

  // Editing options are applied in the exact order in which they appear on the command-line,
  //   so that wherever two stencils overlap the one provided later takes precedence.
  size_t operation_count = 0;
  for (const auto &opt : App::option) {
    if (opt == "plane")
      apply_plane(out, opt, space, transform);
    else if (opt == "sphere")
      apply_sphere(out, opt, space, transform);
    else if (opt == "ellipsoid")
      apply_ellipsoid(out, opt, space, transform);
    else if (opt == "cuboid")
      apply_cuboid(out, opt, space, transform);
    else if (opt == "line")
      apply_line(out, opt, space, transform);
    else if (opt == "voxel")
      apply_voxel(out, opt, space, transform);
    else
      continue;
    ++operation_count;
  }

  if (operation_count == 0U) {
    if (inplace) {
      WARN("No edits specified; image will be unaffected");
    } else {
      WARN("No edits specified; output image will be copy of input");
    }
  }
}
