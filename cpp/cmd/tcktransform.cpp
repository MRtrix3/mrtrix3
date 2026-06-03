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

#include "command.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "image.h"
#include "ordered_thread_queue.h"
#include "progressbar.h"
#include "transform.h"

#include "algo/loop.h"
#include "datatype.h"
#include "header.h"
#include "image_helpers.h"
#include "interp/cubic.h"
#include "registration/warp/extrapolate.h"
#include "registration/warp/validate.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>

using namespace MR;
using namespace MR::DWI;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Apply a spatial transformation to a tracks file";

  DESCRIPTION
  + "Unlike the non-linear transformation of image data,"
    " where the value of the deformation field in a destination voxel position"
    " defines the location in space from which to \"pull\" image data into that voxel,"
    " the non-linear transformation of streamlines data"
    " involves sampling the deformation field at each streamline vertex location"
    " to determine the new spatial location to which to \"push\" that vertex."
    " As such, the appropriate deformation field to apply to streamlines data"
    " is the inverse of what would be applied to image data."
    " So for instance, this may involve the utilisation of a template-to-subject warp field"
    " in order to transform streamlines from subject to template space."

  + "Valid warp information is guaranteed to be available at every streamline vertex,"
    " however far it lies beyond the region over which the input warp field was estimated:"
    " the field is cubically interpolated against a buffer in which all voxels lacking"
    " finite input data have been extrapolated by local low-order polynomial fitting"
    " (the warp being a smooth, locally affine or quadratic coordinate map),"
    " and that buffer is grown on demand to encompass any streamline reaching beyond it."
    " A warning is issued if any such growth beyond the input data occurred.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_tracks_in()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output track file").type_tracks_out();

}
// clang-format on

using value_type = float;
using TrackType = Tractography::Streamline<value_type>;

namespace {

//! the cubic margin: the kernel of a sample at voxel-space position P reads voxels
//!   floor(P)-1 .. floor(P)+2, so a sample is interpolable without recourse to the
//!   interpolator's clamp() (i.e. without reading beyond the field of view) only where
//!   floor(P) lies at least this many voxels from either edge along every spatial axis.
constexpr ssize_t margin = 2;

//! the polynomial degree policy used to extrapolate beyond the valid region; Adaptive
//!   matches the default of Interp::Warp, carrying the boundary gradient and curvature
//!   into the extrapolated region.
constexpr Registration::Warp::ExtrapolateDegree extrapolate_degree = Registration::Warp::ExtrapolateDegree::Adaptive;

//! shift the scanner-space origin of \a header so that voxel index \a origin_voxel along
//!   each axis maps to the scanner position formerly held by voxel index 0
/*! The voxel grid (orientation and spacing) is unchanged; only the translation moves, so
 *  every retained voxel keeps its scanner-space position. \a origin_voxel may be negative
 *  (the grid extends below the former origin) or positive (the grid is shrink-wrapped). */
void shift_origin(Header &header, const std::array<ssize_t, 3> &origin_voxel) {
  for (ssize_t axis = 0; axis != 3; ++axis)
    for (ssize_t i = 0; i != 3; ++i)
      header.transform()(i, 3) +=
          header.spacing(axis) * header.transform()(i, axis) * static_cast<default_type>(origin_voxel[axis]);
}

//! build the initial fully-populated warp buffer from the input field
/*! A bitwise validity mask is formed (a voxel valid only where all three components are
 *  finite) and the bounding box of the valid region accumulated. The working grid is
 *  shrink-wrapped to that bounding box plus a \c margin-voxel border, the valid voxels are
 *  copied onto it, and every remaining (invalid) voxel is filled by rank-ordered local
 *  polynomial extrapolation. The returned buffer therefore holds finite data at every
 *  voxel: empirical warp where valid, extrapolated elsewhere. */
Image<value_type> build_initial_field(Image<value_type> field) {
  Header input_mask_header(field);
  input_mask_header.ndim() = 3;
  input_mask_header.datatype() = DataType::Bit;
  Image<bool> input_validity(Image<bool>::scratch(input_mask_header, "warp field validity mask"));
  std::array<ssize_t, 3> bbox_min{field.size(0), field.size(1), field.size(2)};
  std::array<ssize_t, 3> bbox_max{-1, -1, -1};
  for (auto l = Loop(input_validity)(input_validity); l; ++l) {
    assign_pos_of(input_validity, 0, 3).to(field);
    bool valid = true;
    for (ssize_t component = 0; component != 3; ++component) {
      field.index(3) = component;
      if (!std::isfinite(field.value())) {
        valid = false;
        break;
      }
    }
    input_validity.value() = valid;
    if (valid) {
      for (ssize_t axis = 0; axis != 3; ++axis) {
        const ssize_t pos = input_validity.index(axis);
        bbox_min[axis] = std::min(bbox_min[axis], pos);
        bbox_max[axis] = std::max(bbox_max[axis], pos);
      }
    }
  }
  if (bbox_max[0] < 0)
    throw Exception("Warp field \"" + field.name() + "\" contains no finite voxels");

  // Shrink-wrap the working grid to the valid bounding box plus a margin border. `shift`
  //   maps an input voxel index onto the working grid (input voxel `bbox_min - margin`
  //   becomes working voxel 0).
  std::array<ssize_t, 3> shift{0, 0, 0};
  std::array<ssize_t, 3> origin_voxel{0, 0, 0};
  Header grid(field);
  for (ssize_t axis = 0; axis != 3; ++axis) {
    origin_voxel[axis] = bbox_min[axis] - margin;
    shift[axis] = -origin_voxel[axis];
    grid.size(axis) = (bbox_max[axis] - bbox_min[axis] + 1) + 2 * margin;
  }
  shift_origin(grid, origin_voxel);

  Image<value_type> buffer(Image<value_type>::scratch(grid, "imputed warp field"));
  for (auto l = Loop(field)(field); l; ++l) {
    bool in_grid = true;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      const ssize_t w = field.index(axis) + shift[axis];
      if (w < 0 || w >= buffer.size(axis)) {
        in_grid = false;
        break;
      }
      buffer.index(axis) = w;
    }
    if (!in_grid)
      continue;
    buffer.index(3) = field.index(3);
    buffer.value() = field.value();
  }

  Header mask_header(grid);
  mask_header.ndim() = 3;
  mask_header.datatype() = DataType::Bit;
  Image<bool> validity(Image<bool>::scratch(mask_header, "warp field validity mask"));
  for (auto l = Loop(input_validity)(input_validity); l; ++l) {
    bool in_grid = true;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      const ssize_t w = input_validity.index(axis) + shift[axis];
      if (w < 0 || w >= validity.size(axis)) {
        in_grid = false;
        break;
      }
      validity.index(axis) = w;
    }
    if (in_grid)
      validity.value() = input_validity.value();
  }

  // Extrapolate every invalid voxel of the working grid (not merely a dilation halo).
  Image<bool> halo(Image<bool>::scratch(mask_header, "extrapolation halo mask"));
  for (auto l = Loop(halo)(halo, validity); l; ++l)
    halo.value() = !validity.value();
  Registration::Warp::extrapolate_warp_halo(buffer, validity, halo, extrapolate_degree);

  return buffer;
}

//! grow the warp buffer so it covers the target voxel range, extrapolating the new band
/*! \param old        the current fully-populated buffer.
 *  \param target_min the lowest voxel index (in \a old's grid) that must be covered.
 *  \param target_max the highest voxel index (in \a old's grid) that must be covered.
 *  The new grid spans the union of \a old's extent and the target range; \a old is copied
 *  onto it and the added band is filled by the same rank-ordered polynomial extrapolation,
 *  seeded from the (entirely populated) old region. */
Image<value_type> regrid_and_extrapolate(const Image<value_type> &old,
                                         const std::array<ssize_t, 3> &target_min,
                                         const std::array<ssize_t, 3> &target_max) {
  std::array<ssize_t, 3> shift{0, 0, 0};
  std::array<ssize_t, 3> origin_voxel{0, 0, 0};
  Header grid(old);
  for (ssize_t axis = 0; axis != 3; ++axis) {
    const ssize_t lo = std::min<ssize_t>(0, target_min[axis]);
    const ssize_t hi = std::max<ssize_t>(old.size(axis) - 1, target_max[axis]);
    origin_voxel[axis] = lo;
    shift[axis] = -lo;
    grid.size(axis) = hi - lo + 1;
  }
  shift_origin(grid, origin_voxel);

  Image<value_type> buffer(Image<value_type>::scratch(grid, "imputed warp field"));
  Image<value_type> source(old);
  for (auto l = Loop(source)(source); l; ++l) {
    for (ssize_t axis = 0; axis != 3; ++axis)
      buffer.index(axis) = source.index(axis) + shift[axis];
    buffer.index(3) = source.index(3);
    buffer.value() = source.value();
  }

  // The copied old region (entirely populated) seeds the extrapolation; the new band is
  //   everything outside it.
  Header mask_header(grid);
  mask_header.ndim() = 3;
  mask_header.datatype() = DataType::Bit;
  Image<bool> seed(Image<bool>::scratch(mask_header, "warp field validity mask"));
  Image<bool> halo(Image<bool>::scratch(mask_header, "extrapolation halo mask"));
  for (auto l = Loop(seed)(seed, halo); l; ++l) {
    bool in_old = true;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      const ssize_t o = seed.index(axis) - shift[axis];
      if (o < 0 || o >= old.size(axis)) {
        in_old = false;
        break;
      }
    }
    seed.value() = in_old;
    halo.value() = !in_old;
  }
  Registration::Warp::extrapolate_warp_halo(buffer, seed, halo, extrapolate_degree);

  return buffer;
}

//! the integer (floored) voxel-index bounding box of a streamline's finite vertices
struct VoxelBox {
  std::array<ssize_t, 3> lo;
  std::array<ssize_t, 3> hi;
};

//! map every finite vertex of \a tck through \a scanner2voxel and floor to a voxel box
VoxelBox voxel_box(const transform_type &scanner2voxel, const TrackType &tck) {
  Eigen::Vector3d vmin = Eigen::Vector3d::Constant(std::numeric_limits<double>::infinity());
  Eigen::Vector3d vmax = Eigen::Vector3d::Constant(-std::numeric_limits<double>::infinity());
  for (size_t n = 0; n != tck.size(); ++n) {
    if (!tck[n].allFinite())
      continue;
    const Eigen::Vector3d p = scanner2voxel * tck[n].cast<default_type>();
    vmin = vmin.cwiseMin(p);
    vmax = vmax.cwiseMax(p);
  }
  VoxelBox box;
  for (ssize_t axis = 0; axis != 3; ++axis) {
    box.lo[axis] = static_cast<ssize_t>(std::floor(vmin[axis]));
    box.hi[axis] = static_cast<ssize_t>(std::floor(vmax[axis]));
  }
  return box;
}

//! true if every sample within \a box is cubically interpolable in \a buffer without clamp()
bool is_covered(const VoxelBox &box, const Image<value_type> &buffer) {
  for (ssize_t axis = 0; axis != 3; ++axis) {
    if (box.lo[axis] < margin || box.hi[axis] > buffer.size(axis) - 1 - margin)
      return false;
  }
  return true;
}

} // namespace

class Loader {
public:
  Loader(const std::filesystem::path &path) : reader(path, properties) {}

  bool operator()(TrackType &item) { return reader(item); }

  Tractography::Properties properties;

protected:
  Tractography::Reader<value_type> reader;
};

//! the warp buffer shared across worker threads, grown on demand under exclusive access
/*! A single instance is shared (via std::shared_ptr) by every copy of the Warper functor
 *  that Thread::multi spawns. \c scanner2voxel is a property of the voxel grid and is
 *  therefore held here, recomputed only when \c buffer is grown. \c generation is bumped on
 *  each growth, both to invalidate the per-thread interpolator snapshots and, being
 *  non-zero, to record that extrapolation beyond the input data took place. */
struct SharedField {
  std::shared_mutex mutex;
  Image<value_type> buffer;
  transform_type scanner2voxel;
  size_t generation = 0;
};

class Warper {
public:
  Warper(std::shared_ptr<SharedField> shared) : shared(std::move(shared)) {}

  bool operator()(const TrackType &in, TrackType &out) {
    out.clear();
    out.set_index(in.get_index());
    out.weight = in.weight;
    if (in.size() == 0)
      return true;

    while (true) {
      {
        std::shared_lock<std::shared_mutex> lock(shared->mutex);
        if (local_generation != shared->generation) {
          interp.emplace(shared->buffer);
          local_generation = shared->generation;
        }
        if (is_covered(voxel_box(shared->scanner2voxel, in), shared->buffer)) {
          for (size_t n = 0; n != in.size(); ++n) {
            const auto vertex = pos(in[n]);
            if (vertex.allFinite())
              out.push_back(vertex);
          }
          return true;
        }
      }
      // The streamline reaches beyond the buffer: grow it under exclusive access. A
      //   shared_mutex cannot be upgraded, so the shared lock above is released first.
      grow_to_cover(in);
    }
  }

protected:
  std::shared_ptr<SharedField> shared;
  std::optional<Interp::Cubic<Image<value_type>>> interp;
  size_t local_generation = std::numeric_limits<size_t>::max();

  //! sample the cubic-interpolated warp at scanner-space position \a x
  Eigen::Matrix<value_type, 3, 1> pos(const Eigen::Matrix<value_type, 3, 1> &x) {
    Eigen::Matrix<value_type, 3, 1> p;
    p.setConstant(std::numeric_limits<value_type>::quiet_NaN());
    if (interp->scanner(x)) {
      interp->index(3) = 0;
      p[0] = interp->value();
      interp->index(3) = 1;
      p[1] = interp->value();
      interp->index(3) = 2;
      p[2] = interp->value();
    }
    return p;
  }

  //! enlarge the shared buffer so that \a in becomes interpolable, then return
  void grow_to_cover(const TrackType &in) {
    std::unique_lock<std::shared_mutex> lock(shared->mutex);
    // Re-check under exclusive access: another thread may have already grown the buffer to
    //   cover this streamline while this thread waited for the lock.
    const VoxelBox box = voxel_box(shared->scanner2voxel, in);
    if (is_covered(box, shared->buffer))
      return;
    std::array<ssize_t, 3> target_min;
    std::array<ssize_t, 3> target_max;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      target_min[axis] = box.lo[axis] - margin;
      target_max[axis] = box.hi[axis] + margin;
    }
    shared->buffer = regrid_and_extrapolate(shared->buffer, target_min, target_max);
    shared->scanner2voxel = Transform(shared->buffer).scanner2voxel;
    ++shared->generation;
  }
};

class Writer {
public:
  Writer(const std::filesystem::path &path, const Tractography::Properties &properties)
      : progress("applying spatial transformation to tracks",
                 properties.find("count") == properties.end() ? 0 : to<size_t>(properties.find("count")->second)),
        writer(path, properties) {}

  bool operator()(const TrackType &item) {
    writer(item);
    ++progress;
    return true;
  }

protected:
  ProgressBar progress;
  Tractography::Properties properties;
  Tractography::Writer<value_type> writer;
};

void run() {
  Header H_warp = Header::open(argument[1]);
  auto warp_format = Registration::Warp::validate_header(H_warp);
  if (warp_format != Registration::Warp::WarpFormat::Simple)
    throw Exception("Command is only compatible with 4D deformation warp fields,"
                    " not the 5D \"full\" warp format"
                    " (see eg. command \"warpconvert\")");
  auto data = H_warp.get_image<value_type>(DirectIO{3});
  Registration::Warp::debug_validate_image(data);

  auto shared = std::make_shared<SharedField>();
  shared->buffer = build_initial_field(data);
  shared->scanner2voxel = Transform(shared->buffer).scanner2voxel;

  Loader loader(argument[0]);
  Warper warper(shared);
  Writer writer(argument[2], loader.properties);

  Thread::run_ordered_queue(
      loader, Thread::batch(TrackType(), 1024), Thread::multi(warper), Thread::batch(TrackType(), 1024), writer);

  if (shared->generation > 0) {
    WARN("some streamline transformation was based on extrapolation"
         " of the non-linear warp field beyond the input data");
  }
}
