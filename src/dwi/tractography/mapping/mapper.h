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

#ifndef __dwi_tractography_mapping_mapper_h__
#define __dwi_tractography_mapping_mapper_h__



#include <vector>

#include "point.h"

#include "image/header.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "thread/queue.h"

#include "dataset/interp/linear.h"

#include "dwi/tractography/file.h"

#include "dwi/tractography/mapping/common.h"
#include "dwi/tractography/mapping/resampler.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"


// Didn't bother making this a command-line option, since curvature contrast results were very poor regardless of smoothing
#define CURVATURE_TRACK_SMOOTHING_FWHM 10.0 // In mm

// For getting the FMRI-based contrasts - how finely to sample the image, and how far to extrapolate +/- of the actual track termination
#define FMRI_CONTRAST_STEP 0.25
#define FMRI_CONTRAST_EXTRAP_LENGTH 4.0



namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




typedef DataSet::Interp::Base<const Image::Header> HeaderInterp;




template <class Cont>
class TrackMapperBase
{

  public:
    TrackMapperBase (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& output_header, const Math::Matrix<float>& interp_matrix) :
      reader     (input),
      writer     (output),
      R          (interp_matrix, 3),
      H_out      (output_header),
      interp_out (H_out),
      os_factor  (interp_matrix.rows() + 1)
    {
      //if (R.valid()) {
        //assert (interp_matrix.is_set());
        //assert (interp_matrix.rows());
        //data.allocate (interp_matrix.rows(), 3);
      //}
    }

    TrackMapperBase (const TrackMapperBase& that) :
      reader     (that.reader),
      writer     (that.writer),
      R          (that.R),
      //data       (that.data),
      H_out      (that.H_out),
      interp_out (H_out),
      os_factor  (that.os_factor)
    { }

    virtual ~TrackMapperBase() { }


    void execute ()
    {
      TrackQueue::Reader::Item in (reader);
      typename Thread::Queue<Cont>::Writer::Item out (writer);

      while (in.read()) {
        out->clear();
        out->index = in->index;
        // In the case of contrasts which vary along the track length, want to get the
        // statistical value along the fibre length BEFORE streamline interpolation
        if (preprocess (in->tck, *out)) {
          if (R.valid())
            R.interpolate (in->tck);
            //interp_track (in->tck, data);
          voxelise (in->tck, *out);
          postprocess (in->tck, *out);
          if (!out.write())
            throw Exception ("error writing to write-back queue");
        }
      }
    }


  private:

    TrackQueue::Reader reader;
    typename Thread::Queue<Cont>::Writer writer;
    Resampler< Point<float>, float > R;
    //Math::Matrix<float> data;


  protected:
    const Image::Header& H_out;
    HeaderInterp interp_out;
    const size_t os_factor;


    virtual void voxelise    (const std::vector< Point<float> >&, Cont&) const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise()"); }
    virtual bool preprocess  (const std::vector< Point<float> >&, Cont&)       { return true; }
    virtual void postprocess (const std::vector< Point<float> >&, Cont&) const { }


};



template <>
void TrackMapperBase<SetVoxel>::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  Voxel vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
    vox = round (interp_out.scanner2voxel (*i));
    if (check (vox, H_out))
      voxels.insert (vox);
  }

}


template <>
void TrackMapperBase<SetVoxelDir>::voxelise (const std::vector< Point<float> >& tck, SetVoxelDir& voxels) const
{

  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDir vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (interp_out.scanner2voxel (*i));
    if (check (vox, H_out)) {
      vox.dir = *(i+1) - *prev;
      SetVoxelDir::iterator existing_vox = voxels.find (vox);
      if (existing_vox == voxels.end()) {
        voxels.insert (vox);
      } else {
        VoxelDir new_vox (*existing_vox);
        new_vox.dir += vox.dir;
        voxels.erase (existing_vox);
        voxels.insert (new_vox);
      }
      prev = i;
    }
  }
  vox = round (interp_out.scanner2voxel (*last));
  if (check (vox, H_out)) {
    vox.dir = *last - *prev;
    SetVoxelDir::iterator existing_vox = voxels.find (vox);
    if (existing_vox == voxels.end()) {
      voxels.insert (vox);
    } else {
      VoxelDir new_vox (*existing_vox);
      new_vox.dir += vox.dir;
      voxels.erase (existing_vox);
      voxels.insert (new_vox);
    }
  }

}






template <class Cont>
class TrackMapperTWI : public TrackMapperBase<Cont>
{
  public:
    TrackMapperTWI (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& output_header, const Math::Matrix<float>& interp_matrix, const float step, const contrast_t c, const stat_t s, const float denom = 0.0) :
      TrackMapperBase<Cont> (input, output, output_header, interp_matrix),
      contrast              (c),
      track_statistic       (s),
      step_size             (step),
      gaussian_denominator  (denom) { }

    TrackMapperTWI (const TrackMapperTWI& that) :
      TrackMapperBase<Cont> (that),
      contrast              (that.contrast),
      track_statistic       (that.track_statistic),
      step_size             (that.step_size),
      gaussian_denominator  (that.gaussian_denominator) { }

    virtual ~TrackMapperTWI() { }


  protected:
    virtual void load_values (const std::vector< Point<float> >&, std::vector<float>&);
    const contrast_t contrast;
    const stat_t track_statistic;


  private:
    const float step_size;

    // Members for when the contribution of a track is not a constant
    const float gaussian_denominator;
    std::vector<float> factors;

    void set_factor (const std::vector< Point<float> >&, Cont&);

    // Call the inheited virtual function unless a specialisation for this class exists
    void voxelise (const std::vector< Point<float> >& tck, Cont& voxels) const { TrackMapperBase<Cont>::voxelise (tck, voxels); }

    // Overload virtual function
    // Change here: only contribute to the image if factor is not 0 (so it doesn't affect sums / means)
    bool preprocess (const std::vector< Point<float> >& tck, Cont& out) { set_factor (tck, out); return out.factor; }

};


template <>
void TrackMapperTWI<SetVoxel>::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  if (contrast == ENDPOINT) {

    Voxel vox;
    const size_t os_ratio = TrackMapperBase<SetVoxel>::os_factor;
    for (size_t i = os_ratio; i != (2 * os_ratio) + 1; ++i) {
      vox = round (interp_out.scanner2voxel (tck[1]));
      if (check (vox, H_out))
        voxels.insert (vox);
    }
    for (size_t i = tck.size() - os_ratio - 1; i != tck.size() - (2 * os_ratio) - 2; --i) {
      vox = round (interp_out.scanner2voxel (tck[1]));
      if (check (vox, H_out))
        voxels.insert (vox);
    }

  } else {

    TrackMapperBase<SetVoxel>::voxelise (tck, voxels);

  }

}



template <>
void TrackMapperTWI<SetVoxelFactor>::voxelise (const std::vector< Point<float> >& tck, SetVoxelFactor& voxels) const
{

  VoxelFactor vox;
  for (size_t i = 0; i != tck.size(); ++i) {
    vox = round (interp_out.scanner2voxel (tck[i]));
    if (check (vox, H_out)) {

      // Get a linearly-interpolated value from factors[] based upon factors[] being
      //   generated with non-interpolated data, and index 'i' here representing interpolated data
      const float ideal_index = float(i) / float(os_factor);
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




template <class Cont>
void TrackMapperTWI<Cont>::load_values (const std::vector< Point<float> >& tck, std::vector<float>& values)
{

  if (contrast != CURVATURE)
    throw Exception ("Function TrackMapper::load_values() only works with curvature contrast");

  std::vector< Point<float> > tangents;
  tangents.reserve (tck.size());

  // Would like to be able to manipulate the length over which the tangent calculation is affected
  // However don't want to just take a pair of distant points and get the tangent that way; would rather
  //   find a way to 'smooth' the curvature in a non-scalar fashion i.e. inverted curvature cancels
  // Ideally would like to get a curvature measurement & azimuth at each point; these can be averaged
  //   using a Gaussian kernel
  // But how to define azimuth & make it consistent between points?
  // Average principal normal vectors using a gaussian kernel, re-determine the curvature

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
  }

  // For those tangents which are invalid, fill with valid values from neighbours
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
      const float distance = step_size * Math::abs ((int)i - (int)j);
      const float this_weight = exp (-distance * distance / gaussian_denominator);
      this_tangent += (tangents[j] * this_weight);
    }

    smoothed_tangents.push_back (this_tangent.normalise());

  }

  values.reserve (tck.size());
  for (size_t i = 0; i != tck.size(); ++i) {

    // Risk of acos() returning NAN if the dot product is greater than 1.0
    float this_value;
    if (i == 0)
      this_value = acos (smoothed_tangents[ 1 ].dot (smoothed_tangents[ 0 ]))       / step_size;
    else if (i == tck.size() - 1)
      this_value = acos (smoothed_tangents[ i ].dot (smoothed_tangents[i-1]))       / step_size;
    else
      this_value = acos (smoothed_tangents[i+1].dot (smoothed_tangents[i-1])) * 0.5 / step_size;

    if (isnan (this_value))
      values.push_back (0.0);
    else
      values.push_back (this_value);

  }

}



template <>
void TrackMapperTWI<SetVoxel>::set_factor (const std::vector< Point<float> >& tck, SetVoxel& out)
{

  std::vector<float> values;
  double factor = 0.0;
  size_t count = 0;

  switch (contrast) {

    case TDI:       factor = 1.0; break;
    case DECTDI:    factor = 1.0; break;
    case ENDPOINT:  factor = 1.0; break;
    case LENGTH:    factor = (step_size * (tck.size() - 1)); break;
    case INVLENGTH: factor = (step_size / (tck.size() - 1)); break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
    case FOD_AMP:
    case CURVATURE:

      values.reserve (tck.size());
      load_values (tck, values); // This should call the overloaded virtual function for TrackMapperImage

      switch (track_statistic) {

        case SUM:
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (*i))
              factor += *i;
          }
          break;

        case MIN:
          factor = INFINITY;
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (*i))
              factor = MIN (factor, *i);
          }
          break;

        case MEAN:
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (*i)) {
              factor += *i;
              ++count;
            }
          }
          factor = (count ? (factor / float(count)) : 0.0);
          break;

        case MAX:
          factor = -INFINITY;
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (factor))
              factor = MAX (factor, *i);
          }
          break;

        case MEDIAN:
          if (values.empty()) {
            factor = 0.0;
          } else {
            nth_element (values.begin(), values.begin() + (values.size() / 2), values.end());
            factor = *(values.begin() + (values.size() / 2));
          }
          break;

        case FMRI_MIN:
          factor = MIN(values[0], values[1]);
          break;

        case FMRI_MEAN:
          factor = 0.5 * (values[0] + values[1]);
          break;

        case FMRI_MAX:
          factor = MAX(values[0], values[1]);
          break;

        default:
          throw Exception ("Undefined / unsupported track statistic in TrackMapper<SetVoxel>::set_factor()");

      }
      break;

    default:
      throw Exception ("Undefined / unsupported contrast mechanism in TrackMapper<SetVoxel>::set_factor()");

  }

  if (contrast == SCALAR_MAP_COUNT)
    factor = (factor ? 1.0 : 0.0);

  if (!finite (factor))
    factor = 0.0;

  out.factor = factor;

}


template <>
void TrackMapperTWI<SetVoxelDir>::set_factor (const std::vector< Point<float> >& tck, SetVoxelDir& out)
{
  out.factor = 1.0;
}


template <>
void TrackMapperTWI<SetVoxelFactor>::set_factor (const std::vector< Point<float> >& tck, SetVoxelFactor& /*unused*/)
{

  // Firstly, get the interpolated value at each point
  std::vector<float> values;
  values.reserve (tck.size());
  load_values (tck, values); // This should call the overloaded virtual function for TrackMapperImage

  // From each point, get an estimate of the appropriate factor based upon its distance to all other points
  // Distances accumulate along length of track
  factors.clear();
  factors.reserve (tck.size());

  for (size_t i = 0; i != tck.size(); ++i) {

    double sum = 0.0, norm = 0.0;

    if (values[i] != INFINITY) {
      sum  = values[i];
      norm = 1.0; // Gaussian is unnormalised -> e^0 = 1
    }

    float distance = 0.0;
    for (size_t j = i; j--; ) { // Decrement AFTER null test, so loop runs with j = 0
      distance += step_size;
      if (values[j] != INFINITY) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * values[j];
      }
    }
    distance = 0.0;
    for (size_t j = i + 1; j < tck.size(); ++j) {
      distance += step_size;
      if (values[j] != INFINITY) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * values[j];
      }
    }

    if (norm)
      factors.push_back (sum / norm);
    else
      factors.push_back (0.0);

  }

}



template <class Cont>
class TrackMapperTWIImage : public TrackMapperTWI<Cont>
{

  public:
    TrackMapperTWIImage (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& output_header, const Math::Matrix<float>& interp_matrix, const float step, const contrast_t c, const stat_t m, const float denom, Image::Header& input_header) :
      TrackMapperTWI<Cont> (input, output, output_header, interp_matrix, step, c, m, denom),
      voxel                (input_header),
      interp               (voxel),
      lmax                 (0),
      sh_coeffs            (NULL)
      {
        if (c == FOD_AMP) {
          lmax = Math::SH::LforN (voxel.dim(3));
          sh_coeffs = new float[voxel.dim(3)];
          precomputer.init (lmax);
        }
      }

    TrackMapperTWIImage (const TrackMapperTWIImage& that) :
      TrackMapperTWI<Cont> (that),
      voxel                (that.voxel),
      interp               (voxel),
      lmax                 (that.lmax),
      sh_coeffs            (NULL)
    {
      if (that.sh_coeffs) {
        sh_coeffs = new float[voxel.dim(3)];
        precomputer.init (lmax);
      }
    }

    ~TrackMapperTWIImage() {
      if (sh_coeffs) {
        delete[] sh_coeffs;
        sh_coeffs = NULL;
      }
    }


  private:
    Image::Voxel<float> voxel;
    DataSet::Interp::Linear< Image::Voxel<float> > interp;

    size_t lmax;
    float* sh_coeffs;
    Math::SH::PrecomputedAL<float> precomputer;

    void load_values (const std::vector< Point<float> >&, std::vector<float>&);

};


template <class Cont>
void TrackMapperTWIImage<Cont>::load_values (const std::vector< Point<float> >& tck, std::vector<float>& values)
{

  static const int fmri_contrast_extrap_points = Math::round (FMRI_CONTRAST_EXTRAP_LENGTH / FMRI_CONTRAST_STEP);

  switch (TrackMapperTWI<Cont>::contrast) {

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:

      if (TrackMapperTWI<Cont>::track_statistic == FMRI_MIN || TrackMapperTWI<Cont>::track_statistic == FMRI_MEAN || TrackMapperTWI<Cont>::track_statistic == FMRI_MAX) { // Only the track endpoints contribute

        // Want to extrapolate the track forwards & backwards at either end by some distance, take the
        //   maximum scalar value
        // The higher-level function TrackMapper::set_factor() will deal with taking the min/mean/max of the two
        for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
          Point<float> endpoint, tangent;
          if (tck_end_index) {
            endpoint = tck.back();
            tangent = (endpoint - tck[tck.size() - 1]).normalise();
          } else {
            endpoint = tck.front();
            tangent = (tck[1] - endpoint).normalise();
          }
          if (tangent.valid()) {

            // Build a vector of points to use when sampling the scalar image
            std::vector< Point<float> > to_test;
            for (int i = -fmri_contrast_extrap_points; i <= fmri_contrast_extrap_points; ++i)
              to_test.push_back (endpoint + (tangent * FMRI_CONTRAST_STEP * i));

            float max_value = 0.0;
            for (std::vector< Point<float> >::const_iterator p = to_test.begin(); p != to_test.end(); ++p) {
              if (!interp.scanner (*p))
                max_value = MAX (max_value, interp.value());
            }
            values.push_back (max_value);

          } else {
            // Could not compute a tangent; just take the value at the endpoint
            if (!interp.scanner (endpoint))
              values.push_back (interp.value());
            else
              values.push_back (NAN);
          }
        }

      } else { // The entire length of the track contributes

        for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
          if (!interp.scanner (*i))
            values.push_back (interp.value());
          else
            values.push_back (NAN);
        }

      }
      break;

    case FOD_AMP:
      for (size_t i = 0; i != tck.size(); ++i) {
        const Point<float>& p = tck[i];
        if (!interp.scanner (p)) {
          // Get the interpolated spherical harmonics at this point
          for (voxel[3] = 0; voxel[3] != voxel.dim(3); ++voxel[3])
            sh_coeffs[voxel[3]] = interp.value();
          const Point<float> dir = (tck[(i == tck.size() - 1) ? i : i + 1] - tck[i ? i - 1 : 0]).normalise();
          values.push_back (precomputer.value (sh_coeffs, dir));
        } else {
          values.push_back (NAN);
        }
      }
      break;

    default:
      throw Exception ("Undefined / unsupported contrast mechanism in TrackMapperImage::load_values()");

  }

}



}
}
}
}

#endif



