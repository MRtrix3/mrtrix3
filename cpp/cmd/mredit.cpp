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

#include <algorithm>
#include <array>
#include <set>
#include <vector>

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "transform.h"
#include "types.h"

#include "algo/copy.h"

#include <filesystem>

using namespace MR;
using namespace App;

// TODO:
// * Operate on mask images rather than arbitrary images?
// * Remove capability to edit in-place - just deal with image swapping in the script?
// * Tests
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

  + "The geometric primitives -sphere, -ellipsoid, -cuboid and -line"
    " are by default defined with respect to the image voxel grid:"
    " positions are voxel indices,"
    " sizes (radii and side lengths) are measured in voxels,"
    " and the principal axes of each shape are aligned with the image axes."
    " If the -scanner option is used,"
    " these quantities are instead defined with respect to scanner space:"
    " positions are scanner-space coordinates in millimetres,"
    " sizes are measured in millimetres,"
    " and the principal axes of each shape are aligned with the scanner axes."
    " For an image whose voxel axes are not aligned with the scanner axes,"
    " or whose voxels are not isotropic,"
    " the two interpretations can yield substantially different results.";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in()
  + Argument ("output", "the (optional) output image").type_image_out().optional();

  OPTIONS
  + Option ("plane", "fill one or more planes on a particular image axis").allow_multiple()
    + Argument ("axis").type_integer (0, 2)
    + Argument ("coord").type_sequence_int()
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

  + Option ("line", "draw a straight line of the specified radius between two points"
                    " (i.e. a cylinder with hemispherical caps)").allow_multiple()
    + Argument ("first").type_sequence_float()
    + Argument ("second").type_sequence_float()
    + Argument ("radius").type_float()
    + Argument ("value").type_float()

  + Option ("voxel", "change the image value within a single voxel").allow_multiple()
    + Argument ("position").type_sequence_float()
    + Argument ("value").type_float()

  + Option ("scanner", "interpret all stencil positions, sizes and orientations in scanner space (mm),"
                       " rather than with respect to the voxel grid");

}
// clang-format on

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
                Predicate inside,
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

void run() {
  bool inplace = (argument.size() == 1);
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

  Transform transform(H);
  const bool scanner = !get_options("scanner").empty();
  if (scanner && H.ndim() < 3)
    throw Exception("Cannot specify scanner-space coordinates if image has less than 3 dimensions");
  const Space space = scanner ? Space::scanner : Space::voxel;

  size_t operation_count = 0;

  auto opt = get_options("plane");
  if (!opt.empty()) {
    if (H.ndim() != 3)
      throw Exception("-plane option only works for 3D images");
    if (scanner)
      throw Exception("-plane option cannot be used with scanner-space coordinates");
  }
  operation_count += opt.size();
  for (auto p : opt) {
    const size_t axis = p[0];
    const auto coords = parse_ints<uint32_t>(p[1]);
    const float value = p[2];
    const std::array<size_t, 2> loop_axes{{axis == 0 ? size_t(1) : size_t(0), axis == 2 ? size_t(1) : size_t(2)}};
    for (auto c : coords) {
      out.index(axis) = c;
      for (auto outer = Loop(loop_axes[0])(out); outer; ++outer) {
        for (auto inner = Loop(loop_axes[1])(out); inner; ++inner)
          out.value() = value;
      }
    }
  }

  opt = get_options("sphere");
  if (!opt.empty() && H.ndim() != 3)
    throw Exception("-sphere option only works for 3D images");
  operation_count += opt.size();
  for (auto s : opt) {
    const Stencil ref = resolve(parse_floats(s[0]), space, transform);
    const default_type radius = s[1];
    const float value = s[2];
    auto inside = [centre = ref.centre, radius](const Eigen::Vector3d &p) { return (p - centre).norm() < radius; };
    flood_fill(out, space, transform, {ref.seed}, inside, value);
  }

  opt = get_options("ellipsoid");
  if (!opt.empty() && H.ndim() != 3)
    throw Exception("-ellipsoid option only works for 3D images");
  operation_count += opt.size();
  for (auto e : opt) {
    const Stencil ref = resolve(parse_floats(e[0]), space, transform);
    const auto radii_in = parse_floats(e[1]);
    Eigen::Vector3d radii;
    if (radii_in.size() == 1)
      radii.setConstant(radii_in[0]);
    else if (radii_in.size() == 3)
      radii = Eigen::Vector3d(radii_in[0], radii_in[1], radii_in[2]);
    else
      throw Exception("-ellipsoid radii must be either a single value or 3 comma-separated values");
    if ((radii.array() <= 0.0).any())
      throw Exception("-ellipsoid radii must be positive");
    const float value = e[2];
    auto inside = [centre = ref.centre, radii](const Eigen::Vector3d &p) {
      return ((p - centre).array() / radii.array()).square().sum() < 1.0;
    };
    flood_fill(out, space, transform, {ref.seed}, inside, value);
  }

  opt = get_options("cuboid");
  if (!opt.empty() && H.ndim() != 3)
    throw Exception("-cuboid option only works for 3D images");
  operation_count += opt.size();
  for (auto c : opt) {
    const Stencil ref = resolve(parse_floats(c[0]), space, transform);
    const auto size_in = parse_floats(c[1]);
    Eigen::Vector3d halfsize;
    if (size_in.size() == 1)
      halfsize.setConstant(0.5 * size_in[0]);
    else if (size_in.size() == 3)
      halfsize = 0.5 * Eigen::Vector3d(size_in[0], size_in[1], size_in[2]);
    else
      throw Exception("-cuboid size must be either a single value or 3 comma-separated values");
    if ((halfsize.array() <= 0.0).any())
      throw Exception("-cuboid side lengths must be positive");
    const float value = c[2];
    auto inside = [centre = ref.centre, halfsize](const Eigen::Vector3d &p) {
      return ((p - centre).array().abs() <= halfsize.array()).all();
    };
    flood_fill(out, space, transform, {ref.seed}, inside, value);
  }

  opt = get_options("line");
  if (!opt.empty() && H.ndim() != 3)
    throw Exception("-line option only works for 3D images");
  operation_count += opt.size();
  for (auto l : opt) {
    const Stencil first = resolve(parse_floats(l[0]), space, transform);
    const Stencil second = resolve(parse_floats(l[1]), space, transform);
    const default_type radius = l[2];
    const float value = l[3];
    const Eigen::Vector3d axis = second.centre - first.centre;
    const default_type length_squared = axis.squaredNorm();
    auto inside = [start = first.centre, axis, length_squared, radius](const Eigen::Vector3d &p) {
      default_type t = length_squared > 0.0 ? (p - start).dot(axis) / length_squared : 0.0;
      t = std::clamp(t, 0.0, 1.0);
      const Eigen::Vector3d nearest = start + t * axis;
      return (p - nearest).norm() < radius;
    };
    const Eigen::Vector3d midpoint =
        0.5 * (first.seed.matrix().cast<default_type>() + second.seed.matrix().cast<default_type>());
    const std::vector<Vox> seeds{first.seed, Vox(midpoint), second.seed};
    flood_fill(out, space, transform, seeds, inside, value);
  }

  opt = get_options("voxel");
  operation_count += opt.size();
  for (auto v : opt) {
    const auto position = parse_floats(v[0]);
    const float value = v[1];
    if (position.size() != H.ndim())
      throw Exception("Image has " + str(H.ndim()) + " dimensions, but -voxel option position " + std::string(v[0]) +
                      " provides only " + str(position.size()) + " coordinates");
    if (scanner) {
      Eigen::Vector3d p(position[0], position[1], position[2]);
      p = transform.scanner2voxel * p;
      const Vox voxel(p);
      assign_pos_of(voxel).to(out);
      for (size_t axis = 3; axis != out.ndim(); ++axis) {
        if (std::round(position[axis]) != position[axis])
          throw Exception("Non-spatial coordinates provided using -voxel option must be provided as integers");
        out.index(axis) = position[axis];
      }
    } else {
      for (size_t axis = 0; axis != out.ndim(); ++axis) {
        if (std::round(position[axis]) != position[axis])
          throw Exception("Voxel coordinates provided using -voxel option must be provided as integers");
        out.index(axis) = position[axis];
      }
    }
    out.value() = value;
  }

  if (!operation_count) {
    if (inplace) {
      WARN("No edits specified; image will be unaffected");
    } else {
      WARN("No edits specified; output image will be copy of input");
    }
  }
}
