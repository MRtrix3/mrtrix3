/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "dwi/sdeconv/rf_estimation.h"



namespace MR {
namespace DWI {
namespace RF {




bool FODSegResult::is_sf (const SFThresholds& thresholds) const
{
  if (volume_ratio > thresholds.get_volume_ratio())
    return false;
  if (integral     < thresholds.get_min_integral())
    return false;
  if (integral     > thresholds.get_max_integral())
    return false;
  if (dispersion   > thresholds.get_max_dispersion())
    return false;
  return true;
}




void SFThresholds::update (const std::vector<FODSegResult>& data, const default_type dispersion_multiplier, const default_type integral_multiplier, const size_t iter)
{

/*
  // Simple update based on multiples of the minimum dispersion / maximum integral
  float min_lobe_dispersion = std::numeric_limits<float>::max();
  float max_lobe_integral = 0.0;
  for (std::vector<FODSegResult>::const_iterator i = data.begin(); i != data.end(); ++i) {
    if (i->get_volume_ratio() < volume_ratio) {
      if (i->get_integral()   > max_lobe_integral)   max_lobe_integral   = i->get_integral();
      if (i->get_dispersion() < min_lobe_dispersion) min_lobe_dispersion = i->get_dispersion();
    }
  }
  min_integral   = integral_multiplier   * max_lobe_integral;
  max_dispersion = dispersion_multiplier * min_lobe_dispersion;
*/

/*
  // Output dispersion & integral values to text file
  File::OFStream out_integrals   (std::string("iter_" + str(iter) + "_integrals.txt")  , std::ios_base::out | std::ios_base::trunc);
  File::OFStream out_dispersions (std::string("iter_" + str(iter) + "_dispersions.txt"), std::ios_base::out | std::ios_base::trunc);
  for (std::vector<FODSegResult>::const_iterator i = data.begin(); i != data.end(); ++i) {
    if (i->get_volume_ratio() < get_volume_ratio()) {
      out_integrals   << str(i->get_integral())   << " ";
      out_dispersions << str(i->get_dispersion()) << " ";
    }
  }
  out_integrals   << "\n";
  out_dispersions << "\n";
  out_integrals  .close();
  out_dispersions.close();
*/

  // TODO Output images showing the dispersion & integral values?

  // New threshold updates based on observing histograms:
  // - Integral to be +- 'x' standard deviations (either side; also reject voxels where FOD is abnormally large)
  // - Dispersion limit to be defined using some multiple of the difference between the minimum (sharpest peak) and the mean

  default_type dispersion_sum = 0.0;
  default_type dispersion_min = std::numeric_limits<default_type>::max();
  default_type integral_sum = 0.0, integral_sq_sum = 0.0;
  size_t count = 0;
  for (std::vector<FODSegResult>::const_iterator i = data.begin(); i != data.end(); ++i) {
    if (i->get_volume_ratio() < volume_ratio) {
      dispersion_sum += i->get_dispersion();
      dispersion_min = std::min (dispersion_min, i->get_dispersion());
      integral_sum += i->get_integral();
      integral_sq_sum += Math::pow2 (i->get_integral());
      ++count;
    }
  }
  const default_type dispersion_mean  = dispersion_sum / default_type(count);
  const default_type integral_mean  = integral_sum / default_type(count);
  const default_type integral_stdev = std::sqrt ((integral_sq_sum / default_type(count)) - Math::pow2 (integral_mean));

  min_integral = integral_mean - (integral_multiplier * integral_stdev);
  max_integral = integral_mean + (integral_multiplier * integral_stdev);
  max_dispersion = dispersion_mean + (dispersion_multiplier * (dispersion_mean - dispersion_min));

  DEBUG ("Updated thresholds: volume ratio " + str (volume_ratio) + ", min integral " + str(min_integral) + ", max integral " + str(max_integral) + ", maximum dispersion " + str (max_dispersion));

}





bool FODCalcAndSeg::operator() (const Iterator& pos)
{
  assign_pos_of (pos).to (mask);
  if (!mask.value())
    return true;

  assign_pos_of (pos, 0, 3).to (in);

  // Load the raw DWI data
  Eigen::Matrix<default_type, Eigen::Dynamic, 1> dwi_data (csd.shared.dwis.size());
  for (size_t n = 0; n < csd.shared.dwis.size(); n++) {
    in.index(3) = csd.shared.dwis[n];
    dwi_data[n] = in.value();
    if (!std::isfinite (dwi_data[n]))
      return true;
    if (dwi_data[n] < 0.0)
      dwi_data[n] = 0.0;
  }
  csd.set (dwi_data);

  // Perform CSD
  try {
    size_t n;
    for (n = 0; n < csd.shared.niter; n++)
      if (csd.iterate())
        break;
    if (n == csd.shared.niter)
      return true;
  } catch (...) {
    return true;
  }

  // Perform FOD segmentation
  DWI::FMLS::SH_coefs coefs (csd.FOD());
  coefs.vox[0] = pos.index(0); coefs.vox[1] = pos.index(1); coefs.vox[2] = pos.index(2);
  DWI::FMLS::FOD_lobes lobes;
  (*fmls) (coefs, lobes);
  if (lobes.empty())
    return true;

  // Summarise the results of FOD segmentation and store
  const FODSegResult result (lobes);
  {
    std::lock_guard<std::mutex> lock (*mutex);
    output.push_back (result);
  }

  return true;
}




bool SFSelector::operator() (FODSegResult& out)
{
  while (it != input.end()) {
    if (it->is_sf (thresholds)) {
      out = *it;
      assign_pos_of (it->get_vox()).to (output);
      output.value() = true;
      ++it;
      return true;
    }
    ++it;
  }
  return false;
}




bool ResponseEstimator::operator() (const FODSegResult& in)
{
  assign_pos_of (in.get_vox(), 0, 3).to (dwi);

  // Load the raw DWI data
  Eigen::Matrix<default_type, Eigen::Dynamic, 1> dwi_data (shared.dwis.size());
  for (size_t n = 0; n < shared.dwis.size(); n++) {
    dwi.index(3) = shared.dwis[n];
    dwi_data[n] = dwi.value();
    if (dwi_data[n] < 0.0)
      dwi_data[n] = 0.0;
  }

  // Rotate the diffusion gradient orientations into a new reference frame,
  //   where the Z direction is defined by the FOD peak
  Eigen::Matrix<default_type, 3, 3> R = gen_rotation_matrix (in.get_peak_dir().cast<default_type>());
  Eigen::Matrix<default_type, Eigen::Dynamic, 4> rotated_grad (shared.grad.rows(), 4);
  Eigen::Vector3 vec (3), rot (3);
  for (size_t row = 0; row != size_t(shared.grad.rows()); ++row) {
    vec[0] = shared.grad (row, 0);
    vec[1] = shared.grad (row, 1);
    vec[2] = shared.grad (row, 2);
    rot = R * vec;
    rotated_grad (row, 0) = rot[0];
    rotated_grad (row, 1) = rot[1];
    rotated_grad (row, 2) = rot[2];
    rotated_grad (row, 3) = shared.grad (row, 3);
  }

  // Convert directions from Euclidean space to azimuth/elevation pairs
  const Eigen::MatrixXd dirs = DWI::gen_direction_matrix (rotated_grad, shared.dwis);

  try {

    // Convert the DWI signal to spherical harmonics in the new reference frame
    Math::SH::Transform<default_type> transform (dirs, lmax);
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> SH;
    transform.A2SH (SH, dwi_data);

    // Extract the m=0 components and save
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> response (lmax/2+1);
    for (size_t l = 0; l <= lmax; l += 2)
      response[l/2] = SH[Math::SH::index (l, 0)];

    std::lock_guard<std::mutex> lock (*mutex);
    output += response;

  } catch (...) {
    WARN ("Invalid rotated-gradient SH transformation in voxel " + str(in.get_vox()));
  }

  return true;
}



Eigen::Matrix<default_type, 3, 3> ResponseEstimator::gen_rotation_matrix (const Eigen::Vector3& dir) const
{
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




}
}
}


