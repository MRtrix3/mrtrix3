/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#include "dwi/sdeconv/rf_estimation.h"



namespace MR {
namespace DWI {
namespace RF {




bool FODSegResult::is_sf (const SFThresholds& thresholds) const
{
  if (volume_ratio > thresholds.get_volume_ratio())
    return false;
  if (integral < thresholds.get_min_integral())
    return false;
  if (integral > thresholds.get_max_integral())
    return false;
  if (dispersion > thresholds.get_max_dispersion())
    return false;
  return true;
}




void SFThresholds::update (const std::vector<FODSegResult>& data, const float dispersion_multiplier, const float integral_multiplier, const size_t iter)
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
  std::ofstream out_integrals   (std::string("iter_" + str(iter) + "_integrals.txt")  .c_str(), std::ios_base::trunc);
  std::ofstream out_dispersions (std::string("iter_" + str(iter) + "_dispersions.txt").c_str(), std::ios_base::trunc);
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

  double dispersion_sum = 0.0;
  float dispersion_min = std::numeric_limits<float>::max();
  double integral_sum = 0.0, integral_sq_sum = 0.0;
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
  const float dispersion_mean  = dispersion_sum / double(count);
  const float integral_mean  = integral_sum / double(count);
  const float integral_stdev = Math::sqrt ((integral_sq_sum / double(count)) - Math::pow2 (integral_mean));

  min_integral = integral_mean - (integral_multiplier * integral_stdev);
  max_integral = integral_mean + (integral_multiplier * integral_stdev);
  max_dispersion = dispersion_mean + (dispersion_multiplier * (dispersion_mean - dispersion_min));

  DEBUG ("Updated thresholds: volume ratio " + str (volume_ratio) + ", min integral " + str(min_integral) + ", max integral " + str(max_integral) + ", maximum dispersion " + str (max_dispersion));

}





bool FODCalcAndSeg::operator() (const Image::Iterator& pos)
{
  if (!Image::Nav::get_value_at_pos (mask, pos))
    return true;

  Image::Nav::set_pos (in, pos, 0, 3);

  // Load the raw DWI data
  Math::Vector<float> dwi_data (csd.shared.dwis.size());
  for (size_t n = 0; n < csd.shared.dwis.size(); n++) {
    in[3] = csd.shared.dwis[n];
    dwi_data[n] = in.value();
    if (!std::isfinite (dwi_data[n]))
      return true;
    if (dwi_data[n] < 0.0)
      dwi_data[n] = 0.0;
  }
  csd.set (dwi_data);

  // Perform CSD
  size_t n;
  for (n = 0; n < csd.shared.niter; n++)
    if (csd.iterate())
      break;
  if (n == csd.shared.niter)
    return true;

  // Perform FOD segmentation
  DWI::FMLS::SH_coefs coefs (csd.FOD());
  coefs.vox[0] = pos[0]; coefs.vox[1] = pos[1]; coefs.vox[2] = pos[2];
  DWI::FMLS::FOD_lobes lobes;
  (*fmls) (coefs, lobes);
  if (lobes.empty())
    return true;

  // Summarise the results of FOD segmentation and store
  const FODSegResult result (lobes);
  {
    Thread::Mutex::Lock lock (*mutex);
    output.push_back (result);
  }

  return true;
}




bool SFSelector::operator() (FODSegResult& out)
{
  while (it != input.end()) {
    if (it->is_sf (thresholds)) {
      out = *it;
      Image::Nav::set_value_at_pos (output, it->get_vox(), true);
      ++it;
      return true;
    }
    ++it;
  }
  return false;
}




bool ResponseEstimator::operator() (const FODSegResult& in)
{
  Image::Nav::set_pos (dwi, in.get_vox(), 0, 3);

  // Load the raw DWI data
  Math::Vector<float> dwi_data (shared.dwis.size());
  for (size_t n = 0; n < shared.dwis.size(); n++) {
    dwi[3] = shared.dwis[n];
    dwi_data[n] = dwi.value();
    if (dwi_data[n] < 0.0)
      dwi_data[n] = 0.0;
  }

  // Rotate the diffusion gradient orientations into a new reference frame,
  //   where the Z direction is defined by the FOD peak
  Math::Matrix<float> R = gen_rotation_matrix (in.get_peak_dir());
  Math::Matrix<float> rotated_grad (shared.grad.rows(), 4);
  Math::Vector<float> vec (3), rot (3);
  for (size_t row = 0; row != shared.grad.rows(); ++row) {
    vec[0] = shared.grad (row, 0);
    vec[1] = shared.grad (row, 1);
    vec[2] = shared.grad (row, 2);
    Math::mult (rot, R, vec);
    rotated_grad (row, 0) = rot[0];
    rotated_grad (row, 1) = rot[1];
    rotated_grad (row, 2) = rot[2];
    rotated_grad (row, 3) = shared.grad (row, 3);
  }

  // Convert directions from Euclidean space to azimuth/elevation pairs
  Math::Matrix<float> dirs;
  DWI::gen_direction_matrix (dirs, rotated_grad, shared.dwis);

  // Convert the DWI signal to spherical harmonics in the new reference frame
  Math::SH::Transform<float> transform (dirs, lmax);
  Math::Vector<float> SH;
  transform.A2SH (SH, dwi_data);

  // Extract the m=0 components and save
  Math::Vector<float> response (lmax/2+1);
  for (size_t l = 0; l <= lmax; l += 2)
    response[l/2] = SH[Math::SH::index (l, 0)];
  {
    Thread::Mutex::Lock lock (*mutex);
    output += response;
  }

  return true;
}



Math::Matrix<float> ResponseEstimator::gen_rotation_matrix (const Point<float>& dir) const
{
  // Generates a matrix that will rotate a unit vector into a new frame of reference,
  //   where the peak direction of the FOD is aligned in Z (3rd dimension)
  // Previously this was done using the tensor eigenvectors
  // Here the other two axes are determined at random (but both are orthogonal to the FOD peak direction)
  Math::Matrix<float> R (3, 3);
  R (2, 0) = dir[0]; R (2, 1) = dir[1]; R (2, 2) = dir[2];
  Point<float> vec2 (rng.uniform(), rng.uniform(), rng.uniform());
  vec2 = dir.cross (vec2);
  vec2.normalise();
  R (0, 0) = vec2[0]; R (0, 1) = vec2[1]; R (0, 2) = vec2[2];
  const Point<float> vec3 ((dir.cross (vec2)).normalise());
  R (1, 0) = vec3[0]; R (1, 1) = vec3[1]; R (1, 2) = vec3[2];
  return R;
}




}
}
}


