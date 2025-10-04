/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "dwi/tractography/seeding/basic.h"
#include "adapter/subset.h"
#include "dwi/tractography/rng.h"

namespace MR::DWI::Tractography::Seeding {

Sphere::Sphere(const std::string &in)                         //
    : Base(in, "sphere", MAX_TRACKING_SEED_ATTEMPTS_RANDOM) { //
  auto F = parse_floats(in);
  if (F.size() != 4)
    throw Exception("Could not parse seed \"" + in + "\" as a spherical seed point;" +    //
                    " needs to be 4 comma-separated values (XYZ position, then radius)"); //
  pos = {float(F[0]), float(F[1]), float(F[2])};
  rad = F[3];
  Base::volume = 4.0 * Math::pi * Math::pow3(rad) / 3.0;
}

bool Sphere::get_seed(Eigen::Vector3f &p) const {
  std::uniform_real_distribution<float> uniform;
  do {
    p = {2.0f * uniform(rng) - 1.0f, 2.0f * uniform(rng) - 1.0f, 2.0f * uniform(rng) - 1.0f};
  } while (p.squaredNorm() > 1.0f);
  p = pos + rad * p;
  return true;
}

SeedMask::SeedMask(const std::string &in)                                 //
    : Base(in, "random seeding mask", MAX_TRACKING_SEED_ATTEMPTS_RANDOM), //
      mask(in) {                                                          //
  Base::volume = get_count(mask) * mask.spacing(0) * mask.spacing(1) * mask.spacing(2);
}

bool SeedMask::get_seed(Eigen::Vector3f &p) const {
  auto seed = mask;
  do {
    seed.index(0) = std::uniform_int_distribution<int>(0, mask.size(0) - 1)(rng);
    seed.index(1) = std::uniform_int_distribution<int>(0, mask.size(1) - 1)(rng);
    seed.index(2) = std::uniform_int_distribution<int>(0, mask.size(2) - 1)(rng);
  } while (!seed.value());
  std::uniform_real_distribution<float> uniform;
  p = {seed.index(0) + uniform(rng) - 0.5f, seed.index(1) + uniform(rng) - 0.5f, seed.index(2) + uniform(rng) - 0.5f};
  p = (*mask.voxel2scanner) * p;
  return true;
}

Random_per_voxel::Random_per_voxel(const std::string &in, const ssize_t num_per_voxel)
    : Base(in, "random per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
      mask(in),
      num(num_per_voxel),
      inc(0),
      expired(false) {
  Base::count = get_count(mask) * num_per_voxel;
  mask.index(0) = 0;
  mask.index(1) = 0;
  mask.index(2) = -1;
}

bool Random_per_voxel::get_seed(Eigen::Vector3f &p) const {

  std::lock_guard<std::mutex> lock(mutex);

  if (expired)
    return false;

  if (mask.index(2) < 0 || ++inc == num) {
    inc = 0;

    do {
      if (++mask.index(2) == mask.size(2)) {
        mask.index(2) = 0;
        if (++mask.index(1) == mask.size(1)) {
          mask.index(1) = 0;
          ++mask.index(0);
        }
      }
    } while (mask.index(0) != mask.size(0) && !mask.value());

    if (mask.index(0) == mask.size(0)) {
      expired = true;
      return false;
    }
  }

  std::uniform_real_distribution<float> uniform;
  p = {mask.index(0) + uniform(rng) - 0.5f, mask.index(1) + uniform(rng) - 0.5f, mask.index(2) + uniform(rng) - 0.5f};
  p = (*mask.voxel2scanner) * p;
  return true;
}

Grid_per_voxel::Grid_per_voxel(const std::string &in, const ssize_t os_factor)
    : Base(in, "grid per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
      mask(in),
      os(os_factor),
      pos(os, os, os),
      offset(-0.5 + (1.0 / (2 * os))),
      step(1.0 / os),
      expired(false) {
  Base::count = get_count(mask) * Math::pow3(os_factor);
}

bool Grid_per_voxel::get_seed(Eigen::Vector3f &p) const {

  std::lock_guard<std::mutex> lock(mutex);

  if (expired)
    return false;

  if (++pos[2] >= os) {
    pos[2] = 0;
    if (++pos[1] >= os) {
      pos[1] = 0;
      if (++pos[0] >= os) {
        pos[0] = 0;

        do {
          if (++mask.index(2) == mask.size(2)) {
            mask.index(2) = 0;
            if (++mask.index(1) == mask.size(1)) {
              mask.index(1) = 0;
              ++mask.index(0);
            }
          }
        } while (mask.index(0) != mask.size(0) && !mask.value());
        if (mask.index(0) == mask.size(0)) {
          expired = true;
          return false;
        }
      }
    }
  }

  p = {mask.index(0) + offset + (pos[0] * step),
       mask.index(1) + offset + (pos[1] * step),
       mask.index(2) + offset + (pos[2] * step)};
  p = (*mask.voxel2scanner) * p;
  return true;
}

Rejection::Rejection(const std::string &in)
    : Base(in, "rejection sampling", MAX_TRACKING_SEED_ATTEMPTS_RANDOM),
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
      interp(in),
#endif
      max(0.0) {
  auto vox = Image<float>::open(in);
  if (!(vox.ndim() == 3 || (vox.ndim() == 4 && vox.size(3) == 1)))
    throw Exception("Seed image must be a 3D image");
  std::vector<ssize_t> bottom(3, std::numeric_limits<ssize_t>::max());
  std::vector<ssize_t> top(3, 0);

  for (auto i = Loop(0, 3)(vox); i; ++i) {
    const float value = vox.value();
    if (value) {
      if (value < 0.0)
        throw Exception("Cannot have negative values in an image used for rejection sampling!");
      max = std::max(max, value);
      volume += value;
      if (vox.index(0) < bottom[0])
        bottom[0] = vox.index(0);
      if (vox.index(0) > top[0])
        top[0] = vox.index(0);
      if (vox.index(1) < bottom[1])
        bottom[1] = vox.index(1);
      if (vox.index(1) > top[1])
        top[1] = vox.index(1);
      if (vox.index(2) < bottom[2])
        bottom[2] = vox.index(2);
      if (vox.index(2) > top[2])
        top[2] = vox.index(2);
    }
  }

  if (!max)
    throw Exception("Cannot use image " + in + " for rejection sampling - image is empty");

  if (bottom[0])
    --bottom[0];
  if (bottom[1])
    --bottom[1];
  if (bottom[2])
    --bottom[2];

  top[0] = std::min(vox.size(0) - bottom[0], top[0] + 2 - bottom[0]);
  top[1] = std::min(vox.size(1) - bottom[1], top[1] + 2 - bottom[1]);
  top[2] = std::min(vox.size(2) - bottom[2], top[2] + 2 - bottom[2]);

  auto sub = Adapter::make<Adapter::Subset>(vox, bottom, top);
  Header header = sub;
  header.ndim() = 3;

  auto buf = Image<float>::scratch(header);
  volume *= buf.spacing(0) * buf.spacing(1) * buf.spacing(2);

  copy(sub, buf, 0, 3);
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
  interp = Interp::Linear<Image<float>>(buf);
#else
  image = buf;
  voxel2scanner = Transform(image).voxel2scanner.cast<float>();
#endif
}

bool Rejection::get_seed(Eigen::Vector3f &p) const {
  std::uniform_real_distribution<float> uniform;
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
  auto seed = interp;
  float selector;
  Eigen::Vector3f pos;
  do {
    pos = {
        uniform(rng) * (interp.size(0) - 1), uniform(rng) * (interp.size(1) - 1), uniform(rng) * (interp.size(2) - 1)};
    seed.voxel(pos);
    selector = rng->Uniform() * max;
  } while (seed.value() < selector);
  p = interp.voxel2scanner * pos;
#else
  auto seed = image;
  float selector;
  do {
    seed.index(0) = std::uniform_int_distribution<int>(0, image.size(0) - 1)(rng);
    seed.index(1) = std::uniform_int_distribution<int>(0, image.size(1) - 1)(rng);
    seed.index(2) = std::uniform_int_distribution<int>(0, image.size(2) - 1)(rng);
    selector = uniform(rng) * max;
  } while (seed.value() < selector);
  p = {seed.index(0) + uniform(rng) - 0.5f, seed.index(1) + uniform(rng) - 0.5f, seed.index(2) + uniform(rng) - 0.5f};
  p = voxel2scanner * p;
#endif
  return true;
}

CoordinatesLoader::CoordinatesLoader(const std::string &cds_path) //
    : coords(File::Matrix::load_matrix<float>(cds_path)) {        //
  switch (coords.cols()) {
  case 3:
    break;
  case 4: {
    weights = coords.col(3);
    coords.conservativeResize(coords.rows(), 3);
    if (weights.minCoeff() < 0.0F)
      throw Exception("Per-coordinate seeding weights must be non-negative");
    const float max_coeff = weights.maxCoeff();
    if (max_coeff == 0.0F)
      throw Exception("Per-coordinate seeds must have at least one non-zero value");
    // Normalise to max of 1.0: simplifies rejection seeding
    weights *= 1.0F / max_coeff;
  } break;
  default:
    throw Exception("Invalid number of columns (" + str(coords.cols()) + ") in seed coordinates file " + cds_path);
  }
}

Coordinates_fixed::Coordinates_fixed(const std::string &in, const ssize_t n_streamlines)
    : Base(in, "coordinate seeding fixed", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
      CoordinatesLoader(in),
      current_coord(0),
      num_at_coord(0),
      expired(false),
      streamlines_per_coordinate(n_streamlines) {
  if (have_weights())
    throw Exception("Seeding fixed # streamlines per coordinates"
                    " cannot also specify per-coordinate weights"
                    " (must be only 3 columns in input file)");
  Base::count = num_coordinates() * streamlines_per_coordinate;
}

bool Coordinates_fixed::get_seed(Eigen::Vector3f &p) const {
  std::lock_guard<std::mutex> lock(mutex);
  if (expired)
    return false;
  if (num_at_coord == streamlines_per_coordinate) {
    num_at_coord = 0;
    current_coord++;
  }
  if (current_coord == num_coordinates()) {
    expired = true;
    return false;
  }
  p = coords.row(current_coord);
  num_at_coord++;
  return true;
}

Coordinates_global::Coordinates_global(const std::string &in)                   //
    : Base(in, "coordinate seeding global", MAX_TRACKING_SEED_ATTEMPTS_RANDOM), //
      CoordinatesLoader(in) {}                                                  //

bool Coordinates_global::get_seed(Eigen::Vector3f &p) const {
  long coordinate_index = std::uniform_int_distribution<>(0, num_coordinates() - 1)(rng);
  if (have_weights()) {
    std::uniform_real_distribution<float> uniform;
    float selector = uniform(rng);
    do {
      coordinate_index = std::uniform_int_distribution<>(0, num_coordinates() - 1)(rng);
    } while (weights(coordinate_index) < selector);
  }
  p = coords.row(coordinate_index);
  return true;
}

} // namespace MR::DWI::Tractography::Seeding
