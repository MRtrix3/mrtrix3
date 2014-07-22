/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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


#include "dwi/tractography/mapping/mapper.h"

#include "math/matrix.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




void TrackMapperBase::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{
  Voxel vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
    vox = round (transform.scanner2voxel (*i));
    if (check (vox, info))
      voxels.insert (vox);
  }
}



void TrackMapperBase::voxelise (const std::vector< Point<float> >& tck, SetVoxelDEC& voxels) const
{

  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDEC vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (transform.scanner2voxel (*i));
    if (check (vox, info)) {
      SetVoxelDEC::iterator existing_vox = voxels.find (vox);
      if (existing_vox == voxels.end()) {
        vox.set_dir (*(i+1) - *prev);
        voxels.insert (vox);
      } else {
        existing_vox->add_dir (*(i+1) - *prev);
      }
    }
    prev = i;
  }

  vox = round (transform.scanner2voxel (*last));
  if (check (vox, info)) {
    SetVoxelDEC::iterator existing_vox = voxels.find (vox);
    if (existing_vox == voxels.end()) {
      vox.set_dir (*last - *prev);
      voxels.insert (vox);
    } else {
      existing_vox->add_dir (*last - *prev);
    }
  }

  for (SetVoxelDEC::iterator i = voxels.begin(); i != voxels.end(); ++i)
    i->norm_dir();

}




void TrackMapperBase::voxelise (const std::vector< Point<float> >& tck, SetVoxelDir& voxels) const
{

  typedef Point<float> PointF;

  static const float accuracy = Math::pow2 (0.005 * minvalue (info.vox (0), info.vox (1), info.vox (2)));

  if (tck.size() < 2)
    return;

  Math::Hermite<float> hermite (0.1);

  const PointF tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
  const PointF tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

  unsigned int p = 0;
  PointF p_voxel_exit = tck.front();
  float mu = 0.0;
  bool end_track = false;
  Voxel next_voxel (round (transform.scanner2voxel (tck.front())));

  do {

    const PointF p_voxel_entry (p_voxel_exit);
    PointF p_prev (p_voxel_entry);
    float length = 0.0;
    const Voxel this_voxel = next_voxel;

    while ((p != tck.size()) && ((next_voxel = round (transform.scanner2voxel (tck[p]))) == this_voxel)) {
      length += dist (p_prev, tck[p]);
      p_prev = tck[p];
      ++p;
      mu = 0.0;
    }

    if (p == tck.size()) {
      p_voxel_exit = tck.back();
      end_track = true;
    } else {

      float mu_min = mu;
      float mu_max = 1.0;

      const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
      const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

      PointF p_min = p_prev;
      PointF p_max = tck[p];

      while (dist2 (p_min, p_max) > accuracy) {

        mu = 0.5 * (mu_min + mu_max);
        hermite.set (mu);
        const PointF p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);
        const Voxel mu_voxel = round (transform.scanner2voxel (p_mu));

        if (mu_voxel == this_voxel) {
          mu_min = mu;
          p_min = p_mu;
        } else {
          mu_max = mu;
          p_max = p_mu;
          next_voxel = mu_voxel;
        }

      }
      p_voxel_exit = p_max;

    }

    length += dist (p_prev, p_voxel_exit);
    PointF traversal_vector (p_voxel_exit - p_voxel_entry);
    if (traversal_vector.normalise().valid() && check (this_voxel, info)) {
      VoxelDir voxel_dir (this_voxel, traversal_vector, length);
      voxels.insert (voxel_dir);
    }

  } while (!end_track);

}




void TrackMapperDixel::voxelise (const std::vector< Point<float> >& tck, SetDixel& dixels) const
{

  typedef Point<float> PointF;

  static const float accuracy = Math::pow2 (0.005 * minvalue (info.vox (0), info.vox (1), info.vox (2)));

  if (tck.size() < 2)
    return;

  Math::Hermite<float> hermite (0.1);

  const PointF tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
  const PointF tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

  unsigned int p = 0;
  PointF p_voxel_exit = tck.front();
  float mu = 0.0;
  bool end_track = false;
  Voxel next_voxel (round (transform.scanner2voxel (tck.front())));

  do {

    const PointF p_voxel_entry (p_voxel_exit);
    PointF p_prev (p_voxel_entry);
    float length = 0.0;
    const Voxel this_voxel = next_voxel;

    while ((p != tck.size()) && ((next_voxel = round (transform.scanner2voxel (tck[p]))) == this_voxel)) {
      length += dist (p_prev, tck[p]);
      p_prev = tck[p];
      ++p;
      mu = 0.0;
    }

    if (p == tck.size()) {
      p_voxel_exit = tck.back();
      end_track = true;
    } else {

      float mu_min = mu;
      float mu_max = 1.0;

      const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
      const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

      PointF p_min = p_prev;
      PointF p_max = tck[p];

      while (dist2 (p_min, p_max) > accuracy) {

        mu = 0.5 * (mu_min + mu_max);
        hermite.set (mu);
        const PointF p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);
        const Voxel mu_voxel = round (transform.scanner2voxel (p_mu));

        if (mu_voxel == this_voxel) {
          mu_min = mu;
          p_min = p_mu;
        } else {
          mu_max = mu;
          p_max = p_mu;
          next_voxel = mu_voxel;
        }

      }
      p_voxel_exit = p_max;

    }

    length += dist (p_prev, p_voxel_exit);
    PointF traversal_vector (p_voxel_exit - p_voxel_entry);
    if (traversal_vector.normalise().valid() && check (this_voxel, info)) {

      // Only here does this voxelise() function differ from TrackMapperBase<SetVoxelDir>::voxelise()
      // Map to the appropriate direction & add to the set; only add to an existing dixel if
      //   both the voxel and direction match; otherwise, a single streamline may contribute to
      //   multiple dixels within a voxel)
      // Although it would be possible to functionalise TrackMapperBase<SetVoxelDir>::voxelise(),
      //   call this function, then map individual VoxelDirs to dixels, this would require
      //   twice as many memory requests and potentially more cache misses. So I'm sticking to the
      //   code duplication solution.
      const size_t bin = dirs.select_direction (traversal_vector);
      Dixel this_dixel (this_voxel, bin, length);

      SetDixel::iterator existing_dixel = dixels.find (this_dixel);
      if (existing_dixel == dixels.end())
        dixels.insert (this_dixel);
      else
        existing_dixel->set_value (existing_dixel->get_value() + length);

    }

  } while (!end_track);

}






void TrackMapperTWI::set_factor (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const
{

  size_t count = 0;

  switch (contrast) {

    case TDI:         out.factor = 1.0; break;
    case PRECISE_TDI: out.factor = 1.0; break;
    case ENDPOINT:    out.factor = 1.0; break;
    case LENGTH:
    case INVLENGTH:
      out.factor = 0.0;
      for (size_t i = 1; i != tck.size(); ++i)
        out.factor += dist(tck[i], tck[i-1]);
      if (contrast == INVLENGTH)
        out.factor = 1.0f / out.factor;
      break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
    case FOD_AMP:
    case CURVATURE:

      factors.clear();
      factors.reserve (tck.size());
      load_factors (tck); // This should call the overloaded virtual function for TrackMapperImage

      switch (track_statistic) {

        case T_SUM:
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i))
              out.factor += *i;
          }
          break;

        case T_MIN:
          out.factor = INFINITY;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i))
              out.factor = MIN (out.factor, *i);
          }
          break;

        case T_MEAN:
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i)) {
              out.factor += *i;
              ++count;
            }
          }
          out.factor = (count ? (out.factor / float(count)) : 0.0);
          break;

        case T_MAX:
          out.factor = -INFINITY;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i))
              out.factor = MAX (out.factor, *i);
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
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i) && *i) {
              out.factor += *i;
              ++count;
            }
          }
          out.factor = (count ? (out.factor / float(count)) : 0.0);
          break;

        case GAUSSIAN:
          gaussian_smooth_factors (tck);
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (*i) {
              out.factor = 1.0;
              break;
            }
          }
          break;

        case ENDS_MIN:
          assert (factors.size() == 2);
          out.factor = (Math::abs(factors[0]) < Math::abs(factors[1])) ? factors[0] : factors[1];
          break;

        case ENDS_MEAN:
          assert (factors.size() == 2);
          out.factor = 0.5 * (factors[0] + factors[1]);
          break;

        case ENDS_MAX:
          assert (factors.size() == 2);
          out.factor = (Math::abs(factors[0]) > Math::abs(factors[1])) ? factors[0] : factors[1];
          break;

        case ENDS_PROD:
          assert (factors.size() == 2);
          if ((factors[0] < 0.0 && factors[1] < 0.0) || (factors[0] > 0.0 && factors[1] > 0.0))
            out.factor = factors[0] * factors[1];
          else
            out.factor = 0.0;
          break;

        default:
          throw Exception ("FIXME: Undefined / unsupported track statistic in TrackMapperTWI::get_factor()");

      }
      break;

    default:
      throw Exception ("FIXME: Undefined / unsupported contrast mechanism in TrackMapperTWI::get_factor()");

  }

  if (contrast == SCALAR_MAP_COUNT)
    out.factor = (out.factor ? 1.0 : 0.0);

  if (!std::isfinite (out.factor))
    out.factor = 0.0;

}






void TrackMapperTWI::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  if (contrast == ENDPOINT) {

    Voxel vox = round (transform.scanner2voxel (tck.front()));
    if (check (vox, info))
      voxels.insert (vox);

    vox = round (transform.scanner2voxel (tck.back()));
    if (check (vox, info))
      voxels.insert (vox);

  } else {

    TrackMapperBase::voxelise (tck, voxels);

  }

}



void TrackMapperTWI::voxelise (const std::vector< Point<float> >& tck, SetVoxelDEC& voxels) const
{

  if (contrast == ENDPOINT) {

    VoxelDEC vox = round (transform.scanner2voxel (tck.front()));
    if (check (vox, info)) {
      vox.set_dir (tck[0] - tck[1]);
      voxels.insert (vox);
    }

    vox = round (transform.scanner2voxel (tck.back()));
    if (check (vox, info)) {
      vox.set_dir (tck[tck.size() - 1] - tck[tck.size() - 2]);
      voxels.insert (vox);
    }

  } else {

    TrackMapperBase::voxelise (tck, voxels);

  }

}




void TrackMapperTWI::voxelise (const std::vector< Point<float> >& tck, SetVoxelFactor& voxels) const
{

  VoxelFactor vox;
  for (size_t i = 0; i != tck.size(); ++i) {
    vox = round (transform.scanner2voxel (tck[i]));
    if (check (vox, info)) {

      // Get a linearly-interpolated value from factors[] based upon factors[] being
      //   generated with non-interpolated data, and index 'i' here representing interpolated data
      const float ideal_index = float(i) / float(get_upsample_factor());
      const size_t lower_index = MAX(floor (ideal_index), 0);
      const size_t upper_index = MIN(ceil  (ideal_index), tck.size() - 1);
      const float mu = ideal_index - lower_index;
      const float factor = (mu * factors[upper_index]) + ((1.0 - mu) * factors[lower_index]);

      // Change here from base classes: need to explicitly check whether this voxel has been visited
      SetVoxelFactor::iterator v = voxels.find (vox);
      if (v == voxels.end()) {
        vox.set_factor (factor);
        voxels.insert (vox);
      } else {
        vox = *v;
        vox.add_contribution (factor);
        voxels.erase (v);
        voxels.insert (vox);
      }

    }
  }

}



void TrackMapperTWI::voxelise (const std::vector< Point<float> >& tck, SetVoxelDECFactor& voxels) const
{

  Point<float> prev = tck.front();
  const Point<float>& last = tck[tck.size() - 1];

  VoxelDECFactor vox;
  for (unsigned int i = 0; i != tck.size() - 1; ++i) {
    vox = round (transform.scanner2voxel (tck[i]));
    if (check (vox, info)) {

      const float ideal_index = float(i) / float(get_upsample_factor());
      const size_t lower_index = MAX(floor (ideal_index), 0);
      const size_t upper_index = MIN(ceil  (ideal_index), tck.size() - 1);
      const float mu = ideal_index - lower_index;
      const float factor = (mu * factors[upper_index]) + ((1.0 - mu) * factors[lower_index]);

      SetVoxelDECFactor::iterator existing_vox = voxels.find (vox);
      if (existing_vox == voxels.end()) {
        vox.set_factor (factor);
        vox.set_dir (tck[i+1] - prev);
        voxels.insert (vox);
      } else {
        existing_vox->add_dir (tck[i+1] - prev);
        existing_vox->add_contribution (factor);
      }
    }
    prev = tck[i];
  }

  vox = round (transform.scanner2voxel (last));
  if (check (vox, info)) {

    const float ideal_index = float(tck.size() - 1) / float(get_upsample_factor());
    const size_t lower_index = MAX(floor (ideal_index), 0);
    const size_t upper_index = MIN(ceil  (ideal_index), tck.size() - 1);
    const float mu = ideal_index - lower_index;
    const float factor = (mu * factors[upper_index]) + ((1.0 - mu) * factors[lower_index]);

    SetVoxelDECFactor::iterator existing_vox = voxels.find (vox);
    if (existing_vox == voxels.end()) {
      vox.set_dir (last - prev);
      vox.set_factor (factor);
      voxels.insert (vox);
    } else {
      existing_vox->add_dir (last - prev);
      existing_vox->add_contribution (factor);
    }
  }

  for (SetVoxelDECFactor::iterator i = voxels.begin(); i != voxels.end(); ++i)
    i->norm_dir();

}





void TrackMapperTWI::load_factors (const std::vector< Point<float> >& tck) const
{

  if (contrast != CURVATURE)
    throw Exception ("Function TrackMapperTWI::load_factors() only works with curvature contrast");

  std::vector< Point<float> > tangents;
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
  std::vector<float> step_sizes;
  step_sizes.reserve (tck.size());

  for (size_t i = 0; i != tck.size(); ++i) {
    Point<float> this_tangent;
    if (i == 0)
      this_tangent = ((tck[1]   - tck[0]  ).normalise());
    else if (i == tck.size() - 1)
      this_tangent = ((tck[i]   - tck[i-1]).normalise());
    else
      this_tangent = ((tck[i+1] - tck[i-1]).normalise());
    if (this_tangent.valid())
      tangents.push_back (this_tangent);
    else
      tangents.push_back (Point<float> (0.0, 0.0, 0.0));
    if (i)
      step_sizes.push_back (dist(tck[i], tck[i-1]));
  }

  // For those tangents that are invalid, fill with valid factors from neighbours
  for (size_t i = 0; i != tangents.size(); ++i) {
    if (tangents[i] == Point<float> (0.0, 0.0, 0.0)) {

      if (i == 0) {
        size_t j;
        for (j = 1; (j < tck.size() - 1) && (tangents[j] != Point<float> (0.0, 0.0, 0.0)); ++j);
        tangents[i] = tangents[j];
      } else if (i == tangents.size() - 1) {
        size_t k;
        for (k = i - 1; k && (tangents[k] != Point<float> (0.0, 0.0, 0.0)); --k);
        tangents[i] = tangents[k];
      } else {
        size_t j, k;
        for (j = 1; (j < tck.size() - 1) && (tangents[j] != Point<float> (0.0, 0.0, 0.0)); ++j);
        for (k = i - 1; k && (tangents[k] != Point<float> (0.0, 0.0, 0.0)); --k);
        tangents[i] = (tangents[j] + tangents[k]).normalise();
      }

    }
  }

  // Produce a matrix of spline distances between points
  Math::Matrix<float> spline_distances (tck.size(), tck.size());
  spline_distances = 0.0f;
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

  std::vector< Point<float> > smoothed_tangents;
  smoothed_tangents.reserve (tangents.size());

  static const float gaussian_theta = CURVATURE_TRACK_SMOOTHING_FWHM / (2.0 * sqrt (2.0 * log (2.0)));
  static const float gaussian_denominator = 2.0 * gaussian_theta * gaussian_theta;

  for (size_t i = 0; i != tck.size(); ++i) {

    Point<float> this_tangent (0.0, 0.0, 0.0);
    std::pair<float, Point<float> > this_normal (std::make_pair (0.0, Point<float>(0.0, 0.0, 0.0)));

    for (size_t j = 0; j != tck.size(); ++j) {
      const float distance = spline_distances (i, j);
      const float this_weight = exp (-distance * distance / gaussian_denominator);
      this_tangent += (tangents[j] * this_weight);
    }

    smoothed_tangents.push_back (this_tangent.normalise());

  }

  for (size_t i = 0; i != tck.size(); ++i) {

    float tangent_dot_product, length;
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
      factors.push_back (Math::acos (tangent_dot_product) / length);

  }

}



void TrackMapperTWI::gaussian_smooth_factors (const std::vector< Point<float> >& tck) const
{

  std::vector<float> unsmoothed (factors);

  for (size_t i = 0; i != unsmoothed.size(); ++i) {

    double sum = 0.0, norm = 0.0;

    if (std::isfinite (unsmoothed[i])) {
      sum  = unsmoothed[i];
      norm = 1.0; // Gaussian is unnormalised -> e^0 = 1
    }

    float distance = 0.0;
    for (size_t j = i; j--; ) { // Decrement AFTER null test, so loop runs with j = 0
      distance += dist(tck[j], tck[j+1]);
      if (std::isfinite (unsmoothed[j])) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * unsmoothed[j];
      }
    }
    distance = 0.0;
    for (size_t j = i + 1; j < unsmoothed.size(); ++j) {
      distance += dist(tck[j], tck[j-1]);
      if (std::isfinite (unsmoothed[j])) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * unsmoothed[j];
      }
    }

    if (norm)
      factors[i] = (sum / norm);
    else
      factors[i] = 0.0;

  }

}




// TODO May be able to remove scope qualifiers
void TrackMapperTWIImage::load_factors (const std::vector< Point<float> >& tck) const
{

  switch (TrackMapperTWI::contrast) {

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:

      if (TrackMapperTWI::track_statistic == ENDS_MIN || TrackMapperTWI::track_statistic == ENDS_MEAN || TrackMapperTWI::track_statistic == ENDS_MAX || TrackMapperTWI::track_statistic == ENDS_PROD) { // Only the track endpoints contribute

        for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
          const Point<float> endpoint = get_last_point_in_fov (tck, tck_end_index);
          if (endpoint.valid())
            TrackMapperTWI::factors.push_back (interp.value());
          else
            TrackMapperTWI::factors.push_back (NAN);
        }

      } else { // The entire length of the track contributes

        for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
          if (!interp.scanner (*i))
            TrackMapperTWI::factors.push_back (interp.value());
          else
            TrackMapperTWI::factors.push_back (NAN);
        }

      }
      break;

    case FOD_AMP:
      for (size_t i = 0; i != tck.size(); ++i) {
        const Point<float>& p = tck[i];
        if (!interp.scanner (p)) {
          // Get the interpolated spherical harmonics at this point
          for (interp[3] = 0; interp[3] != interp.dim(3); ++interp[3])
            sh_coeffs[interp[3]] = interp.value();
          const Point<float> dir = (tck[(i == tck.size()-1) ? i : (i+1)] - tck[i ? (i-1) : 0]).normalise();
          TrackMapperTWI::factors.push_back (precomputer.value (sh_coeffs, dir));
        } else {
          TrackMapperTWI::factors.push_back (NAN);
        }
      }
      break;

    default:
      throw Exception ("FIXME: Undefined / unsupported contrast mechanism in TrackMapperTWIImage::load_factors()");

  }

}




const Point<float> TrackMapperTWIImage::get_last_point_in_fov (const std::vector< Point<float> >& tck, const bool end) const
{
  size_t index = end ? tck.size() - 1 : 0;
  const int step = end ? -1 : 1;
  while (interp.scanner (tck[index])) {
    index += step;
    if (index == tck.size())
      return Point<float>();
  }
  return tck[index];
}







}
}
}
}




