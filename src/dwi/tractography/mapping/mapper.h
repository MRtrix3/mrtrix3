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



enum contrast_t { TDI, DECTDI, ENDPOINT, MEAN_DIR, LENGTH, INVLENGTH, SCALAR_MAP, SCALAR_MAP_COUNT, FOD_AMP, CURVATURE };
const char* contrasts[] = { "tdi", "dectdi", "endpoint", "mean_dir", "length", "invlength", "scalar_map", "scalar_map_count", "fod_amp", "curvature", NULL };


typedef DataSet::Interp::Base<const Image::Header>  HeaderInterp;
typedef Thread::Queue< std::vector< Point<float> > > TrackQueue;



template <typename Cont>
class TrackMapper
{
  public:
    TrackMapper (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& output_header, const Math::Matrix<float>& interp_matrix, const float step, const contrast_t c, const stat_t s, const float denom = 0.0) :
      contrast             (c),
      track_statistic      (s),
      step_size            (step),
      reader               (input),
      writer               (output),
      H_out                (output_header),
      interp_out           (H_out),
      resample_matrix      (interp_matrix),
      gaussian_denominator (denom),
      os_factor            (interp_matrix.rows() + 1) { }

    TrackMapper (const TrackMapper& that) :
      contrast             (that.contrast),
      track_statistic      (that.track_statistic),
      step_size            (that.step_size),
      reader               (that.reader),
      writer               (that.writer),
      H_out                (that.H_out),
      interp_out           (H_out),
      resample_matrix      (that.resample_matrix),
      gaussian_denominator (that.gaussian_denominator),
      os_factor            (that.os_factor) { }

    virtual ~TrackMapper() { }

    void execute ()
    {
      TrackQueue::Reader::Item in (reader);
      typename Thread::Queue<Cont>::Writer::Item out (writer);
      Resampler<float> R (resample_matrix, 3);
      Math::Matrix<float> data;
      if (R.valid()) {
        assert (resample_matrix.is_set());
        assert (resample_matrix.rows());
        data.allocate (resample_matrix.rows(), 3);
      }

      while (in.read()) {
        out->clear();
        // In the case of contrasts which vary along the track length, want to get the
        // statistical value along the fibre length BEFORE streamline interpolation
        set_factor (*in, *out);
        if (out->factor) { // Change here: only contribute to the image if factor is not 0 (so it doesn't affect sums / means)
          if (R.valid())
            interp_track (*in, R, data);
          voxelise (*out, *in);
          if (!out.write())
            throw Exception ("error writing to write-back queue");
        }
      }
    }


  protected:
    virtual void load_values (const std::vector< Point<float> >&, std::vector<float>&);
    const contrast_t contrast;
    const stat_t track_statistic;


  private:
    const float step_size;
    TrackQueue::Reader reader;
    typename Thread::Queue<Cont>::Writer writer;
    const Image::Header& H_out;
    HeaderInterp interp_out;
    const Math::Matrix<float>& resample_matrix;

    // Members for when the contribution of a track is not a constant
    const float gaussian_denominator;
    const size_t os_factor;
    std::vector<float> factors;


    void interp_prepare (std::vector< Point<float> >& v)
    {
      const size_t s = v.size();
      if (s > 2) {
        v.insert    (v.begin(), v[ 0 ] + (float(2.0) * (v[ 0 ] - v[ 1 ])) - (v[ 1 ] - v[ 2 ]));
        v.push_back (           v[ s ] + (float(2.0) * (v[ s ] - v[s-1])) - (v[s-1] - v[s-2]));
      } else {
        v.push_back (           v[1] + (v[1] - v[0]));
        v.insert    (v.begin(), v[0] + (v[0] - v[1]));
      }
    }

    void interp_track (
        std::vector<Point<float> >& tck,
        Resampler<float>& R,
        Math::Matrix<float>& data)
    {
      std::vector<Point<float> > out;
      interp_prepare (tck);
      R.init (tck[0], tck[1], tck[2]);
      for (size_t i = 3; i < tck.size(); ++i) {
        out.push_back (tck[i-2]);
        R.increment (tck[i]);
        R.interpolate (data);
        for (size_t row = 0; row != data.rows(); ++row)
          out.push_back (Point<float> (data(row,0), data(row,1), data(row,2)));
      }
      out.push_back (tck[tck.size() - 2]);
      out.swap (tck);
    }


    void set_factor (const std::vector< Point<float> >&, Cont&);
    void voxelise   (Cont&, const std::vector< Point<float> >&) const;


};




template <class Cont>
void TrackMapper<Cont>::load_values (const std::vector< Point<float> >& tck, std::vector<float>& values)
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

/*
  // Need one value for each point along the curve
  // Calculate for all but the first and last points, and just duplicate the first & last values
  for (size_t i = 1; i != tangents.size() - 1; ++i)
    values.push_back (0.5 * acos (tangents[i+1].dot (tangents[i-1])) / step_size);
  values.push_back (values.back());
  values.insert (values.begin(), values.front());
*/

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

  //std::vector< Point<float> > principal_normal_vectors;
  //principal_normal_vectors.reserve (tangents.size());

  //for (size_t i = 0; i != tangents.size(); ++i) {
  //  if (i == 0)
  //    principal_normal_vectors.push_back (tangents[1] - tangents[0]);
  //  else if (i == tangents.size() - 1)
  //    principal_normal_vectors.push_back (tangents[i] - tangents[i-1]);
  //  else
  //    principal_normal_vectors.push_back ((tangents[i+1] - tangents[i-1]) * 0.5);
  //}

  // Smooth both the tangent vectors and the principal normal vectors according to a Gaussuan kernel
  // Remember: tangent vectors are unit length, but for principal normal vectors length must be preserved!

  std::vector< Point<float> > smoothed_tangents;
  smoothed_tangents.reserve (tangents.size());
  //std::vector< Point<float> > smoothed_normals;
  //smoothed_normals.reserve (principal_normal_vectors.size());

  static const float gaussian_theta = CURVATURE_TRACK_SMOOTHING_FWHM / (2.0 * sqrt (2.0 * log (2.0)));
  static const float gaussian_denominator = 2.0 * gaussian_theta * gaussian_theta;

  for (size_t i = 0; i != tck.size(); ++i) {

    Point<float> this_tangent (0.0, 0.0, 0.0);
    std::pair<float, Point<float> > this_normal (std::make_pair (0.0, Point<float>(0.0, 0.0, 0.0)));

    for (size_t j = 0; j != tck.size(); ++j) {

      const float distance = step_size * Math::abs ((int)i - (int)j);
      const float this_weight = exp (-distance * distance / gaussian_denominator);
      this_tangent += (tangents[j] * this_weight);
      //this_normal.first += this_weight;
      //this_normal.second += (principal_normal_vectors[j] * this_weight);

    }

    smoothed_tangents.push_back (this_tangent.normalise());
    //smoothed_normals .push_back (this_normal.second * (1.0 / this_normal.first));

  }

  values.reserve (tck.size());
  for (size_t i = 0; i != tck.size(); ++i) {

    // ... Maybe I don't need the principal normal vectors after all?

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
void TrackMapper<SetVoxel>::set_factor (const std::vector< Point<float> >& tck, SetVoxel& out)
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
void TrackMapper<SetVoxelDir>::set_factor (const std::vector< Point<float> >& tck, SetVoxelDir& out)
{
  out.factor = 1.0;
}

template <>
void TrackMapper<SetVoxelFactor>::set_factor (const std::vector< Point<float> >& tck, SetVoxelFactor& /* unused */)
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






template <>
void TrackMapper<SetVoxel>::voxelise (SetVoxel& voxels, const std::vector< Point<float> >& tck) const
{

  Voxel vox;
  if (contrast == ENDPOINT) {

    static const size_t os_ratio = resample_matrix.rows() + 1;

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

    for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
      vox = round (interp_out.scanner2voxel (*i));
      if (check (vox, H_out))
        voxels.insert (vox);
    }

  }

}

template <>
void TrackMapper<SetVoxelDir>::voxelise (SetVoxelDir& voxels, const std::vector< Point<float> >& tck) const
{

  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDir vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (interp_out.scanner2voxel (*i));
    if (check (vox, H_out)) {
      vox.dir = (*(i+1) - *prev).normalise();
      voxels.insert (vox);
      prev = i;
    }
  }
  vox = round (interp_out.scanner2voxel (*last));
  if (check (vox, H_out)) {
    vox.dir = (*last - *prev).normalise();
    voxels.insert (vox);
  }

}


template <>
void TrackMapper<SetVoxelFactor>::voxelise (SetVoxelFactor& voxels, const std::vector< Point<float> >& tck) const
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
class TrackMapperImage : public TrackMapper<Cont>
{

  public:
    TrackMapperImage (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& output_header, const Math::Matrix<float>& interp_matrix, const float step, const contrast_t c, const stat_t m, const float denom, Image::Header& input_header) :
      TrackMapper<Cont> (input, output, output_header, interp_matrix, step, c, m, denom),
      voxel             (input_header),
      interp            (voxel),
      lmax              (0),
      sh_coeffs         (NULL)
      {
        if (c == FOD_AMP) {
          lmax = Math::SH::LforN (voxel.dim(3));
          sh_coeffs = new float[voxel.dim(3)];
          precomputer.init (lmax);
        }
      }

    TrackMapperImage (const TrackMapperImage& that) :
      TrackMapper<Cont> (that),
      voxel             (that.voxel),
      interp            (voxel),
      lmax              (that.lmax),
      sh_coeffs         (NULL)
    {
      if (that.sh_coeffs) {
        sh_coeffs = new float[voxel.dim(3)];
        precomputer.init (lmax);
      }
    }

    ~TrackMapperImage() {
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
void TrackMapperImage<Cont>::load_values (const std::vector< Point<float> >& tck, std::vector<float>& values)
{

  static const int fmri_contrast_extrap_points = Math::round (FMRI_CONTRAST_EXTRAP_LENGTH / FMRI_CONTRAST_STEP);

  switch (TrackMapper<Cont>::contrast) {

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:

      if (TrackMapper<Cont>::track_statistic == FMRI_MIN || TrackMapper<Cont>::track_statistic == FMRI_MEAN || TrackMapper<Cont>::track_statistic == FMRI_MAX) { // Only the track endpoints contribute

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



