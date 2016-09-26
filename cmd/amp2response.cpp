/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier 2014

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Eigen/Dense>

#include "command.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "types.h"

#include "math/constrained_least_squares.h"
#include "math/rng.h"
#include "math/sphere.h"
#include "math/SH.h"
#include "math/ZSH.h"

#include "dwi/gradient.h"
#include "dwi/shells.h"



using namespace MR;
using namespace App;



#define AMP2RESPONSE_DEBUG
#define AMP2RESPONSE_PERVOXEL_IMAGES



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au";

  DESCRIPTION 
    + "test suite for new mechanisms for estimating spherical deconvolution response functions";

  ARGUMENTS
    + Argument ("amps", "the amplitudes image").type_image_in()
    + Argument ("mask", "the mask containing the voxels from which to estimate the response function").type_image_in()
    + Argument ("directions", "a 4D image containing the estimated fibre directions").type_image_in()
    + Argument ("response", "the output zonal spherical harmonic coefficients").type_file_out();

  OPTIONS
    + Option ("directions", "provide an external text file containing the directions along which the amplitudes are sampled")
      + Argument("path").type_file_in()

    + DWI::ShellOption

    + Option ("lmax", "specify the maximum harmonic degree of the response function to estimate")
      + Argument ("value").type_integer (0, 20);
}



Eigen::Matrix<default_type, 3, 3> gen_rotation_matrix (const Eigen::Vector3& dir)
{
  static Math::RNG::Normal<default_type> rng;
  // Generates a matrix that will rotate a unit vector into a new frame of reference,
  //   where the peak direction of the FOD is aligned in Z (3rd dimension)
  // Previously this was done using the tensor eigenvectors
  // Here the other two axes are determined at random (but both are orthogonal to the FOD peak direction)
  Eigen::Matrix<default_type, 3, 3> R;
  R (2, 0) = dir[0]; R (2, 1) = dir[1]; R (2, 2) = dir[2];
  Eigen::Vector3 vec2 (rng(), rng(), rng());
  vec2 = dir.cross (vec2);
  vec2.normalize();
  R (0, 0) = vec2[0]; R (0, 1) = vec2[1]; R (0, 2) = vec2[2];
  Eigen::Vector3 vec3 = dir.cross (vec2);
  vec3.normalize();
  R (1, 0) = vec3[0]; R (1, 1) = vec3[1]; R (1, 2) = vec3[2];
  return R;
}


std::vector<size_t> all_volumes (const size_t num)
{
  std::vector<size_t> result;
  result.reserve (num);
  for (size_t i = 0; i != num; ++i)
    result.push_back (i);
  return result;
}


void run () 
{

  // Get directions from either selecting a b-value shell, or the header, or external file
  auto header = Header::open (argument[0]);

  Eigen::MatrixXd dirs_azel;
  std::vector<size_t> volumes;

  auto opt = get_options ("directions");
  if (opt.size()) {
    dirs_azel = load_matrix (opt[0][0]);
    volumes = all_volumes (dirs_azel.rows());
  } else {
    auto hit = header.keyval().find ("directions");
    if (hit != header.keyval().end()) {
      std::vector<default_type> dir_vector;
      for (auto line : split_lines (hit->second)) {
        auto v = parse_floats (line);
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      dirs_azel.resize (dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        dirs_azel (i/2, 0) = dir_vector[i];
        dirs_azel (i/2, 1) = dir_vector[i+1];
      }
      volumes = all_volumes (dirs_azel.rows());
    } else {
      auto grad = DWI::get_valid_DW_scheme (header);
      DWI::Shells shells (grad);
      shells.select_shells (false, true);
      volumes = shells.largest().get_volumes();
      dirs_azel = DWI::gen_direction_matrix (grad, volumes);
    }
  }

  Eigen::MatrixXd dirs_cartesian = Math::Sphere::spherical2cartesian (dirs_azel);

  // Because the amp->SH transform doesn't need to be applied per voxel,
  //   lmax is effectively unconstrained. Therefore generate response at
  //   lmax=8 regardless of number of input volumes, unless user
  //   explicitly requests something else
  const size_t lmax = get_option_value ("lmax", 8);

  auto image = header.get_image<float>();
  auto mask = Image<bool>::open (argument[1]);
  check_dimensions (image, mask, 0, 3);
  auto dir_image = Image<float>::open (argument[2]);
  if (dir_image.ndim() < 4 || dir_image.size(3) < 3)
    throw Exception ("input direction image \"" + std::string (argument[2]) + "\" does not have expected dimensions");
  check_dimensions (image, dir_image, 0, 3);

  // All directions from all SF voxels get concatenated into a single large matrix
  Eigen::MatrixXd cat_transforms;
  Eigen::VectorXd cat_data;

#ifdef AMP2RESPONSE_DEBUG
  // To make sure we've got our data rotated correctly, let's generate a scatterplot of
  //   elevation vs. amplitude
  Eigen::MatrixXd scatter;
#endif

  size_t sf_counter = 0;
  for (auto l = Loop (mask) (image, mask, dir_image); l; ++l) {
    if (mask.value()) {

      // Grab the image data
      Eigen::VectorXd data (dirs_azel.rows());
      for (size_t i = 0; i != volumes.size(); ++i) {
        image.index(3) = volumes[i];
        data[i] = image.value();
      }

      // Grab the fibre direction
      // Eventually, it might be possible to optimise these fibre directions
      //   during the response function fit; i.e. optimise (az,el) in each voxel
      //   to minimise SSE compared to the current RF estimate
      Eigen::Vector3 fibre_dir;
      for (dir_image.index(3) = 0; dir_image.index(3) != 3; ++dir_image.index(3))
        fibre_dir[dir_image.index(3)] = dir_image.value();
      fibre_dir.normalize();

      // Rotate the directions into a new reference frame,
      //   where the Z axis is defined by the specified direction
      Eigen::Matrix<default_type, 3, 3> R = gen_rotation_matrix (fibre_dir);
      Eigen::Matrix<default_type, Eigen::Dynamic, 3> rotated_dirs_cartesian (dirs_cartesian.rows(), 3);
      Eigen::Vector3 vec (3), rot (3);
      for (ssize_t row = 0; row != dirs_azel.rows(); ++row) {
        vec = dirs_cartesian.row (row);
        rot = R * vec;
        rotated_dirs_cartesian.row (row) = rot;
      }

      // Convert directions from Euclidean space to azimuth/elevation pairs
      Eigen::MatrixXd rotated_dirs_azel = Math::Sphere::cartesian2spherical (rotated_dirs_cartesian);

      // Constrain elevations to between 0 and pi/2
      for (ssize_t i = 0; i != rotated_dirs_azel.rows(); ++i) {
        if (rotated_dirs_azel (i, 1) > Math::pi_2) {
          if (rotated_dirs_azel (i, 0) > Math::pi)
            rotated_dirs_azel (i, 0) -= Math::pi;
          else
            rotated_dirs_azel (i, 0) += Math::pi;
          rotated_dirs_azel (i, 1) = Math::pi - rotated_dirs_azel (i, 1);
        }
      }

#ifdef AMP2RESPONSE_PERVOXEL_IMAGES
      // For the sake of generating a figure, output the original and rotated signals to a dixel ODF image
      Header rotated_header (header);
      rotated_header.size(0) = rotated_header.size(1) = rotated_header.size(2) = 1;
      rotated_header.size(3) = volumes.size();
      Header nonrotated_header (rotated_header);
      nonrotated_header.size(3) = header.size(3);
      Eigen::MatrixXd rotated_grad (volumes.size(), 4);
      for (size_t i = 0; i != volumes.size(); ++i) {
        rotated_grad.block<1,3>(i, 0) = rotated_dirs_cartesian.row(i);
        rotated_grad(i, 3) = 1000.0;
      }
      DWI::set_DW_scheme (rotated_header, rotated_grad);
      Image<float> out_rotated = Image<float>::create ("rotated_amps_" + str(sf_counter) + ".mif", rotated_header);
      Image<float> out_nonrotated = Image<float>::create ("nonrotated_amps_" + str(sf_counter) + ".mif", nonrotated_header);
      out_rotated.index(0) = out_rotated.index(1) = out_rotated.index(2) = 0;
      out_nonrotated.index(0) = out_nonrotated.index(1) = out_nonrotated.index(2) = 0;
      for (size_t i = 0; i != volumes.size(); ++i) {
        image.index(3) = volumes[i];
        out_rotated.index(3) = i;
        out_rotated.value() = image.value();
      }
      for (ssize_t i = 0; i != header.size(3); ++i) {
        image.index(3) = out_nonrotated.index(3) = i;
        out_nonrotated.value() = image.value();
      }
#endif

      // Generate the ZSH -> amplitude transform
      Eigen::MatrixXd transform = Math::ZSH::init_amp_transform<default_type> (rotated_dirs_azel.col(1), lmax);

      // Concatenate these data to the ICLS matrices
      const size_t old_rows = cat_transforms.rows();
      cat_transforms.conservativeResize (old_rows + transform.rows(), transform.cols());
      cat_transforms.block (old_rows, 0, transform.rows(), transform.cols()) = transform;
      cat_data.conservativeResize (old_rows + data.size());
      cat_data.tail (data.size()) = data;

#ifdef AMP2RESPONSE_DEBUG
      scatter.conservativeResize (cat_data.size(), 2);
      scatter.block (old_rows, 0, data.size(), 1) = rotated_dirs_azel.col(1);
      scatter.block (old_rows, 1, data.size(), 1) = data;
#endif

      ++sf_counter;

    }
  }

#ifdef AMP2RESPONSE_DEBUG
  save_matrix (scatter, "scatter.csv");
#endif

  // Generate the constraint matrix
  // We are going to both constrain the amplitudes to be non-negative, and constrain the derivatives to be non-negative
  const size_t num_angles_constraint = 90;
  Eigen::VectorXd els;
  els.resize (num_angles_constraint+1);
  for (size_t i = 0; i <= num_angles_constraint; ++i)
    els[i] = default_type(i) * Math::pi / 180.0;
  Eigen::MatrixXd amp_transform   = Math::ZSH::init_amp_transform  <default_type> (els, lmax);
  Eigen::MatrixXd deriv_transform = Math::ZSH::init_deriv_transform<default_type> (els, lmax);

  Eigen::MatrixXd constraints;
  constraints.resize (amp_transform.rows() + deriv_transform.rows(), amp_transform.cols());
  constraints.block (0, 0, amp_transform.rows(), amp_transform.cols()) = amp_transform;
  constraints.block (amp_transform.rows(), 0, deriv_transform.rows(), deriv_transform.cols()) = deriv_transform;

  // Initialise the problem solver
  auto problem = Math::ICLS::Problem<default_type> (cat_transforms, constraints, 1e-10, 1e-10);
  auto solver  = Math::ICLS::Solver <default_type> (problem);

  // Estimate the solution
  Eigen::VectorXd rf;
  const size_t niter = solver (rf, cat_data);

  INFO ("Response function [" + str(rf.transpose()) + " ] solved after " + str(niter) + " iterations from " + str(sf_counter) + " voxels");

  save_vector (rf, argument[3]);
}
