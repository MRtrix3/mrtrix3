/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "dwi/tractography/mapping/mapper.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




void TrackMapperBase::voxelise (const Streamline<>& tck, SetVoxel& voxels) const
{
  Eigen::Vector3i vox;
  for (const auto& i : tck) {
    vox = round (scanner2voxel * i);
    if (check (vox, info))
      voxels.std::set<Voxel>::insert (vox);
  }
}





void TrackMapperTWI::set_factor (const Streamline<>& tck, SetVoxelExtras& out) const
{

  size_t count = 0;

  switch (contrast) {

    case TDI: out.factor = 1.0; break;
    case LENGTH: out.factor = tck.calc_length(); break;
    case INVLENGTH: out.factor = 1.0 / tck.calc_length(); break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
    case FOD_AMP:
    case CURVATURE:

      factors.clear();
      factors.reserve (tck.size());
      load_factors (tck);

      switch (track_statistic) {

        case T_SUM:
          out.factor = 0.0;
          for (const auto& i : factors)
            if (std::isfinite (i))
              out.factor += i;
          break;

        case T_MIN:
          out.factor = INFINITY;
          for (const auto& i : factors) {
            if (std::isfinite (i))
              out.factor = std::min (out.factor, i);
          }
          break;

        case T_MEAN:
          out.factor = 0.0;
          for (const auto& i : factors) {
            if (std::isfinite (i)) {
              out.factor += i;
              ++count;
            }
          }
          out.factor = (count ? (out.factor / default_type(count)) : 0.0);
          break;

        case T_MAX:
          out.factor = -INFINITY;
          for (const auto& i : factors) {
            if (std::isfinite (i))
              out.factor = std::max (out.factor, i);
          }
          break;

        case T_MEDIAN:
          if (factors.empty()) {
            out.factor = 0.0;
          } else {
            nth_element (factors.begin(), factors.begin() + (factors.size() / 2), factors.end());
            out.factor = *(factors.begin() + (factors.size() / 2));
          }
          break;

        case T_MEAN_NONZERO:
          out.factor = 0.0;
          for (const auto& i : factors) {
            if (std::isfinite (i) && i) {
              out.factor += i;
              ++count;
            }
          }
          out.factor = (count ? (out.factor / default_type(count)) : 0.0);
          break;

        case GAUSSIAN:
          throw Exception ("Gaussian track-wise statistic should not be used in TrackMapperTWI class; use Mapping::Gaussian::TrackMapper instead");

        case ENDS_MIN:
          assert (factors.size() == 2);
          out.factor = (std::abs(factors[0]) < std::abs(factors[1])) ? factors[0] : factors[1];
          break;

        case ENDS_MEAN:
          assert (factors.size() == 2);
          out.factor = 0.5 * (factors[0] + factors[1]);
          break;

        case ENDS_MAX:
          assert (factors.size() == 2);
          out.factor = (std::abs(factors[0]) > std::abs(factors[1])) ? factors[0] : factors[1];
          break;

        case ENDS_PROD:
          assert (factors.size() == 2);
          if ((factors[0] < 0.0 && factors[1] < 0.0) || (factors[0] > 0.0 && factors[1] > 0.0))
            out.factor = factors[0] * factors[1];
          else
            out.factor = 0.0;
          break;

        case ENDS_CORR:
          assert (factors.size() == 1);
          out.factor = factors.front();
          break;

        default:
          throw Exception ("FIXME: Undefined / unsupported track statistic in TrackMapperTWI::get_factor()");

      }
      break;

    case VECTOR_FILE:
      assert (vector_data);
      assert (tck.index < size_t(vector_data->size()));
      out.factor = (*vector_data)[tck.index];
      break;

    default:
      throw Exception ("FIXME: Undefined / unsupported contrast mechanism in TrackMapperTWI::get_factor()");

  }

  if (contrast == SCALAR_MAP_COUNT)
    out.factor = (out.factor ? 1.0 : 0.0);

  if (!std::isfinite (out.factor))
    out.factor = 0.0;

}




void TrackMapperTWI::add_scalar_image (const std::string& path)
{
  if (image_plugin)
    throw Exception ("Cannot add more than one associated image to TWI");
  if (contrast != SCALAR_MAP && contrast != SCALAR_MAP_COUNT)
    throw Exception ("Cannot add a scalar image to TWI unless the contrast depends on it");
  image_plugin.reset (new TWIScalarImagePlugin (path, track_statistic));
}

void TrackMapperTWI::set_backtrack()
{
  if (!image_plugin)
    throw Exception ("Cannot backtrack if no TWI associated image provided");
  const TWIImagePluginBase* const base = image_plugin.get();
  if (typeid(*base) != typeid(TWIScalarImagePlugin))
    throw Exception ("Backtracking is only applicable to scalar image TWI plugins");
  TWIScalarImagePlugin* const ptr = dynamic_cast<TWIScalarImagePlugin*>(image_plugin.get());
  ptr->set_backtrack();
}

void TrackMapperTWI::add_fod_image (const std::string& path)
{
  if (image_plugin)
    throw Exception ("Cannot add more than one associated image to TWI");
  if (contrast != FOD_AMP)
    throw Exception ("Cannot add an FOD image to TWI unless the FOD_AMP contrast is used");
  image_plugin.reset (new TWIFODImagePlugin (path, track_statistic));
}

void TrackMapperTWI::add_twdfc_image (Image<float>& image, const vector<float>& kernel, const ssize_t timepoint)
{
  if (image_plugin)
    throw Exception ("Cannot add more than one associated image to TWI");
  if (contrast != SCALAR_MAP)
    throw Exception ("For sliding time-window fMRI mapping, mapper must be set to SCALAR_MAP contrast");
  if (track_statistic != ENDS_CORR)
    throw Exception ("For sliding time-window fMRI mapping, only the endpoint correlation track-wise statistic is valid");
  image_plugin.reset (new TWDFCImagePlugin (image, kernel, timepoint));
}

void TrackMapperTWI::add_vector_data (const std::string& path)
{
  if (image_plugin)
    throw Exception ("Cannot add both an associated image and a vector data file to TWI");
  if (contrast != VECTOR_FILE)
    throw Exception ("Cannot add a vector data file to TWI unless the VECTOR_FILE contrast is used");
  vector_data.reset (new Eigen::VectorXf (load_vector<float> (path)));
}





void TrackMapperTWI::load_factors (const Streamline<>& tck) const
{

  if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT) {
    assert (image_plugin);
    image_plugin->load_factors (tck, factors);
    return;
  }
  if (contrast == FOD_AMP) {
    assert (image_plugin);
    image_plugin->load_factors (tck, factors);
    return;
  }
  if (contrast != CURVATURE)
    throw Exception ("Unsupported contrast in function TrackMapperTWI::load_factors()");

  vector<Eigen::Vector3> tangents;
  tangents.reserve (tck.size());

  // Would like to be able to manipulate the length over which the tangent calculation is affected
  // However don't want to just take a pair of distant points and get the tangent that way; would rather
  //   find a way to 'smooth' the curvature in a non-scalar fashion i.e. inverted curvature cancels
  // Ideally would like to get a curvature measurement & azimuth at each point; these can be averaged
  //   using a Gaussian kernel
  // But how to define azimuth & make it consistent between points?
  // Average principal normal vectors using a gaussian kernel, re-determine the curvature

  // Need to know the distance along the spline between every point and every other point
  // Start by logging the length of each step
  vector<default_type> step_sizes;
  step_sizes.reserve (tck.size());

  for (size_t i = 0; i != tck.size(); ++i) {
    Eigen::Vector3 this_tangent;
    if (i == 0)
      this_tangent = ((tck[1]   - tck[0]  ).cast<default_type>().normalized());
    else if (i == tck.size() - 1)
      this_tangent = ((tck[i]   - tck[i-1]).cast<default_type>().normalized());
    else
      this_tangent = ((tck[i+1] - tck[i-1]).cast<default_type>().normalized());
    if (this_tangent.allFinite())
      tangents.push_back (this_tangent);
    else
      tangents.push_back ({ 0.0, 0.0, 0.0 });
    if (i)
      step_sizes.push_back ((tck[i] - tck[i-1]).norm());
  }

  // For those tangents that are invalid, fill with valid factors from neighbours
  for (size_t i = 0; i != tangents.size(); ++i) {
    if (tangents[i].isZero()) {

      if (i == 0) {
        size_t j;
        for (j = 1; (j < tck.size() - 1) && !tangents[j].isZero(); ++j);
        tangents[i] = tangents[j];
      } else if (i == tangents.size() - 1) {
        size_t k;
        for (k = i - 1; k && !tangents[k].isZero(); --k);
        tangents[i] = tangents[k];
      } else {
        size_t j, k;
        for (j = 1; (j < tck.size() - 1) && !tangents[j].isZero(); ++j);
        for (k = i - 1; k && !tangents[k].isZero(); --k);
        tangents[i] = (tangents[j] + tangents[k]).normalized();
      }

    }
  }

  // Produce a matrix of spline distances between points
  Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> spline_distances = Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>::Zero (tck.size(), tck.size());
  for (size_t i = 0; i != tck.size(); ++i) {
    for (size_t j = 0; j <= i; ++j) {
      for (size_t k = i+1; k != tck.size(); ++k) {
        spline_distances (j, k) += step_sizes[i];
        spline_distances (k, j) += step_sizes[i];
      }
    }
  }

  // Smooth both the tangent vectors and the principal normal vectors according to a Gaussuan kernel
  // Remember: tangent vectors are unit length, but for principal normal vectors length must be preserved!

  vector<Eigen::Vector3> smoothed_tangents;
  smoothed_tangents.reserve (tangents.size());

  static const default_type gaussian_theta = CURVATURE_TRACK_SMOOTHING_FWHM / (2.0 * sqrt (2.0 * log (2.0)));
  static const default_type gaussian_denominator = 2.0 * gaussian_theta * gaussian_theta;

  for (size_t i = 0; i != tck.size(); ++i) {

    Eigen::Vector3 this_tangent (0.0, 0.0, 0.0);

    for (size_t j = 0; j != tck.size(); ++j) {
      const default_type distance = spline_distances (i, j);
      const default_type this_weight = exp (-distance * distance / gaussian_denominator);
      this_tangent += tangents[j] * this_weight;
    }

    smoothed_tangents.push_back (this_tangent.normalized());

  }

  for (size_t i = 0; i != tck.size(); ++i) {

    default_type tangent_dot_product, length;
    if (i == 0) {
      tangent_dot_product = smoothed_tangents[ 1 ].dot (smoothed_tangents[ 0 ]);
      length = spline_distances (0, 1);
    } else if (i == tck.size() - 1) {
      tangent_dot_product = smoothed_tangents[ i ].dot (smoothed_tangents[i-1]);
      length = spline_distances (i, i-1);
    } else {
      tangent_dot_product = smoothed_tangents[i+1].dot (smoothed_tangents[i-1]);
      length = spline_distances (i+1, i-1);
    }

    if (tangent_dot_product >= 1.0f)
      factors.push_back (0.0);
    else
      factors.push_back (std::acos (tangent_dot_product) / length);

  }

}




}
}
}
}




