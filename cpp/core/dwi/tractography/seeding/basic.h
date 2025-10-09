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

#pragma once

#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/base.h"
#include "dwi/tractography/seeding/seeding.h"
#include "file/matrix.h"

// By default, the rejection sampler will perform its sampling based on image intensity values,
//   and then randomly select a position within that voxel
// Use this flag to instead perform rejection sampling on the trilinear-interpolated value
//   at each trial seed point
// #define REJECTION_SAMPLING_USE_INTERPOLATION

namespace MR::DWI::Tractography::Seeding {

class Sphere : public Base {
public:
  Sphere(const std::string &in);
  virtual bool get_seed(Eigen::Vector3f &p) const override;

private:
  Eigen::Vector3f pos;
  float rad;
};

class SeedMask : public Base {
public:
  SeedMask(const std::string &in);
  virtual bool get_seed(Eigen::Vector3f &p) const override;

private:
  Mask mask;
};

class Random_per_voxel : public Base {
public:
  Random_per_voxel(const std::string &in, const size_t num_per_voxel);
  virtual bool get_seed(Eigen::Vector3f &p) const override;
  virtual ~Random_per_voxel() {}

private:
  mutable Mask mask;
  const ssize_t num;
  mutable ssize_t inc;
  mutable bool expired;
};

class Grid_per_voxel : public Base {
public:
  Grid_per_voxel(const std::string &in, const size_t os_factor);
  virtual ~Grid_per_voxel() {}
  virtual bool get_seed(Eigen::Vector3f &p) const override;

private:
  mutable Mask mask;
  const ssize_t os;
  mutable Eigen::Vector3i pos;
  const float offset, step;
  mutable bool expired;
};

class Rejection_per_voxel : public Base {
public:
  using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;
  Rejection_per_voxel(const std::string &);
  virtual bool get_seed(Eigen::Vector3f &p) const override;

private:
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
  Interp::Linear<Image<float>> interp;
#else
  Image<float> image;
  transform_type voxel2scanner;
#endif
  float max;
};

class CoordinatesLoader {
public:
  CoordinatesLoader(const std::string &cds_path);

protected:
  Eigen::MatrixXf coords;
  Eigen::VectorXf weights;
  ssize_t num_coordinates() const { return coords.rows(); }
  bool have_weights() const { return weights.size() > 0; }
};

class Count_per_coord : public Base, public CoordinatesLoader {
public:
  Count_per_coord(const std::string &in, const size_t streamlines_per_coord);
  virtual bool get_seed(Eigen::Vector3f &p) const override;

private:
  mutable ssize_t current_coord;
  mutable ssize_t num_at_coord;
  mutable bool expired;
  const ssize_t streamlines_per_coordinate;
};

class Coordinates : public Base, public CoordinatesLoader {
public:
  Coordinates(const std::string &in);
  virtual bool get_seed(Eigen::Vector3f &p) const override;
};

class Rejection_per_coord : public Base, public CoordinatesLoader {
public:
  Rejection_per_coord(const std::string &in);
  virtual bool get_seed(Eigen::Vector3f &p) const override;
};

} // namespace MR::DWI::Tractography::Seeding
