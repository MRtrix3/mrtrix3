/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith and J-Donald Tournier, 2011.

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

#include <fstream>
#include <vector>
#include <set>

#include "app.h"
#include "progressbar.h"
#include "point.h"
#include "image/voxel.h"
#include "dataset/misc.h"
#include "dataset/buffer.h"
#include "dataset/interp/linear.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "math/hermite.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;


// TODO Consider generalising colour processing such that it can be used with any contrast mechanism


enum contrast_t { TDI, DECTDI, ENDPOINT, MEAN_DIR, LENGTH, INVLENGTH, SCALAR_MAP, FOD_AMP, CURVATURE };
const char* contrasts[] = { "tdi", "dectdi", "endpoint", "mean_dir", "length", "invlength", "scalar_map", "fod_amp", "curvature", NULL };

enum stat_t { SUM, MIN, MEAN, MEDIAN, MAX, GAUSSIAN };
const char* statistics[] = { "sum", "min", "mean", "median", "max", "gaussian", NULL };


void usage () {
AUTHOR = "Robert E. Smith (r.smith@brain.org.au) and J-Donald Tournier (d.tournier@brain.org.au)";

DESCRIPTION 
  + "Use track data as a form of contrast for producing a high-resolution image.";

ARGUMENTS 
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("output", "the output track density image").type_image_out();

OPTIONS
  + Option ("template",
      "an image file to be used as a template for the output (the output image "
      "will have the same transform and field of view).")
    + Argument ("image").type_image_in()

  + Option ("vox", 
      "provide either an isotropic voxel size (in mm), or comma-separated list "
      "of 3 voxel dimensions.")
    + Argument ("size").type_sequence_float()

  + Option ("contrast",
      "define the desired form of contrast for the output image\n"
      "Options are: tdi, dectdi, endpoint, mean_dir, length, invlength, scalar_map, fod_amp, curvature (default: tdi)")
    + Argument ("type").type_choice (contrasts)

  + Option ("image",
      "provide the scalar image map for generating images with 'scalar_map' contrast, or the spherical harmonics image for 'fod_amp' contrast")
    + Argument ("image").type_image_in()

  + Option ("stat_vox",
      "define the statistic for choosing the final voxel intensities for a given contrast "
      "type given the individual values from the tracks passing through each voxel\n"
      "Options are: sum, min, mean, max (default: sum)")
    + Argument ("type").type_choice (statistics)

  + Option ("stat_tck",
      "define the statistic for choosing the contribution to be made by each streamline as a "
      "function of the samples taken along their lengths\n"
      "Only has an effect for 'scalar_map', 'fod_amp' and 'curvature' contrast types\n"
      "Options are: sum, min, mean, median, max, gaussian (default: mean)")
    + Argument ("type").type_choice (statistics)

  + Option ("fwhm_tck",
      "when using gaussian-smoothed per-track statistic, specify the desired full-width "
      "half-maximum of the Gaussian kernel (in mm)")
    + Argument ("value").type_float (1e-6, 10.0, 1e6)

  + Option ("datatype", 
      "specify output image data type.")
    + Argument ("spec").type_choice (DataType::identifiers)

  + Option ("resample", 
      "resample the tracks at regular intervals using Hermite interpolation\n"
      "(If omitted, an appropriate interpolation will be determined automatically)")
    + Argument ("factor").type_integer (1, 1, std::numeric_limits<int>::max());
}




using namespace MR::DWI;


#define HERMITE_TENSION 0.1
#define MAX_TRACKS_READ_FOR_HEADER 1000000
#define MAX_VOXEL_STEP_RATIO 0.333

// Didn't bother making this a command-line option, since curvature contrast results were very poor regardless of smoothing
#define CURVATURE_TRACK_SMOOTHING_FWHM 10.0 // In mm


typedef DataSet::Interp::Linear<const Image::Header>  HeaderInterp;
typedef Thread::Queue< std::vector< Point<float> > > TrackQueue;


class Voxel : public Point<int>
{
  public:
  Voxel (const int x, const int y, const int z) { p[0] = x; p[1] = y; p[2] = z; }
  Voxel () { memset (p, 0x00, 3 * sizeof(int)); }
  bool operator< (const Voxel& V) const { return ((p[2] == V.p[2]) ? ((p[1] == V.p[1]) ? (p[0] < V.p[0]) : (p[1] < V.p[1])) : (p[2] < V.p[2])); }
};


Voxel round (const Point<float>& p) 
{ 
  assert (finite (p[0]) && finite (p[1]) && finite (p[2]));
  return (Voxel (Math::round<int> (p[0]), Math::round<int> (p[1]), Math::round<int> (p[2])));
}
bool check (const Voxel& V, const Image::Header& H) 
{
  return (V[0] >= 0 && V[0] < H.dim(0) && V[1] >= 0 && V[1] < H.dim(1) && V[2] >= 0 && V[2] < H.dim(2));
}
Point<float> abs (const Point<float>& d)
{
  return (Point<float> (Math::abs(d[0]), Math::abs(d[1]), Math::abs(d[2])));
}

class VoxelDir : public Voxel 
{
  public:
    VoxelDir () :
      dir (Point<float> (0.0, 0.0, 0.0))
    {
      memset (p, 0x00, 3 * sizeof(int));
    }
    VoxelDir (const Voxel& V) :
      dir (Point<float> (0.0, 0.0, 0.0))
    {
      memcpy (p, V, 3 * sizeof(int));
    }
    Point<float> dir;
    VoxelDir& operator= (const Voxel& V)          { Voxel::operator= (V); return (*this); }
    bool      operator< (const VoxelDir& V) const { return Voxel::operator< (V); }
};


class VoxelFactor : public Voxel
{
  public:
    VoxelFactor () :
      sum (0.0),
      contributions (0) { }

    VoxelFactor (const int x, const int y, const int z, const float factor) :
      Voxel (x, y, z),
      sum (factor),
      contributions (1) { }

    VoxelFactor (const Voxel& v) :
      Voxel (v),
      sum (0.0),
      contributions (0) { }

    VoxelFactor (const VoxelFactor& v) :
      Voxel (v),
      sum (v.sum),
      contributions (v.contributions) { }

    void add_contribution (const float factor) {
      sum += factor;
      ++contributions;
    }

    void set_factor (const float i) { sum = i; contributions = 1; }
    float get_factor() const { return (sum / float(contributions)); }
    size_t get_contribution_count() const { return contributions; }

    VoxelFactor& operator= (const Voxel& V)             { Voxel::operator= (V); return (*this); }
    bool         operator< (const VoxelFactor& V) const { return Voxel::operator< (V); }

  private:
    float sum;
    size_t contributions;
};



class SetVoxel       : public std::set<Voxel>       { public: float factor; };
class SetVoxelDir    : public std::set<VoxelDir>    { public: float factor; };
class SetVoxelFactor : public std::set<VoxelFactor> { };


template <typename T>
class Resampler
{
  public:
    Resampler (const Math::Matrix<T>& interp_matrix, const size_t c) :
      M (interp_matrix),
      columns (c),
      data (4, c) { }

    ~Resampler() { }

    size_t get_columns () const { return (columns); }
    bool valid () const { return (M.is_set()); }

    void init (const T* a, const T* b, const T* c)
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = 0.0;
        data(1,i) = a[i];
        data(2,i) = b[i];
        data(3,i) = c[i];
      }
    }

    void increment (const T* a)
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = data(1,i);
        data(1,i) = data(2,i);
        data(2,i) = data(3,i);
        data(3,i) = a[i];
      }
    }

    MR::Math::Matrix<T>& interpolate (Math::Matrix<T>& result) const 
    {
      return (Math::mult (result, M, data));
    }

  private:
    const Math::Matrix<T>& M;
    size_t columns;
    Math::Matrix<T> data;
};



class TrackLoader
{
  public:
    TrackLoader (TrackQueue& queue, DWI::Tractography::Reader<float>& file, size_t count) :
      writer (queue),
      reader (file),
      total_count (count) { }

    void execute ()
    {
      TrackQueue::Writer::Item item (writer);

      ProgressBar progress ("mapping tracks to image...", total_count);
      while (reader.next (*item)) {
        if (!item.write())
          throw Exception ("error writing to track-mapping queue");
        ++progress;
      }
    }

  private:
    TrackQueue::Writer writer;
    DWI::Tractography::Reader<float>& reader;
    size_t total_count;

};






template <typename Cont>
class TrackMapper
{
  public:
    TrackMapper (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& output_header, const Math::Matrix<float>& interp_matrix, const float step, const contrast_t c, const stat_t s, const float denom = 0.0) :
      contrast             (c),
      step_size            (step),
      track_statistic      (s),
      reader               (input),
      writer               (output),
      H_out                (output_header),
      interp_out           (H_out),
      resample_matrix      (interp_matrix),
      gaussian_denominator (denom),
      os_factor            (interp_matrix.rows() + 1) { }

    TrackMapper (const TrackMapper& that) :
      contrast             (that.contrast),
      step_size            (that.step_size),
      track_statistic      (that.track_statistic),
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
        if (R.valid())
          interp_track (*in, R, data);
        voxelise (*out, *in, interp_out);
        if (!out.write())
          throw Exception ("error writing to write-back queue");
      }
    }


  protected:
    virtual void load_values (const std::vector< Point<float> >&, std::vector<float>&);
    const contrast_t contrast;


  private:
    const float step_size;
    const stat_t track_statistic;
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
    void voxelise   (Cont&, const std::vector< Point<float> >&, const HeaderInterp&) const;


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

      const float distance = step_size * abs (i - j);
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
  double factor;

  switch (contrast) {

    case TDI:       factor = 1.0; break;
    case DECTDI:    factor = 1.0; break;
    case ENDPOINT:  factor = 1.0; break;
    case LENGTH:    factor = (step_size * (tck.size() - 1)); break;
    case INVLENGTH: factor = (step_size / (tck.size() - 1)); break;

    case SCALAR_MAP:
    case FOD_AMP:
    case CURVATURE:

      values.reserve (tck.size());
      load_values (tck, values); // This should call the overloaded virtual function for TrackMapperImage

      factor = 0.0;
      switch (track_statistic) {

        case SUM:
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i)
            factor += *i;
          break;

        case MIN:
          factor = INFINITY;
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i)
            factor = MIN (factor, *i);
          break;

        case MEAN:
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i)
            factor += *i;
          factor = (values.size() ? factor / float(values.size()) : 0.0);
          break;

        case MAX:
          factor = -INFINITY;
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i)
            factor = MAX (factor, *i);
          break;

        case MEDIAN:
          if (values.empty()) {
            factor = 0.0;
          } else {
            nth_element (values.begin(), values.begin() + (values.size() / 2), values.end());
            factor = *(values.begin() + (values.size() / 2));
          }
          break;

        default:
          throw Exception ("Undefined / unsupported track statistic in TrackMapper<SetVoxel>::set_factor()");

      }
      break;

    default:
      throw Exception ("Undefined / unsupported contrast mechanism in TrackMapper<SetVoxel>::set_factor()");

  }

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
void TrackMapper<SetVoxel>::voxelise (SetVoxel& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{

  Voxel vox;
  if (contrast == ENDPOINT) {

    static const size_t os_ratio = resample_matrix.rows() + 1;

    for (size_t i = os_ratio; i != (2 * os_ratio) + 1; ++i) {
      vox = round (interp.scanner2voxel (tck[1]));
      if (check (vox, H_out))
        voxels.insert (vox);
    }
    for (size_t i = tck.size() - os_ratio - 1; i != tck.size() - (2 * os_ratio) - 2; --i) {
      vox = round (interp.scanner2voxel (tck[1]));
      if (check (vox, H_out))
        voxels.insert (vox);
    }

  } else {

    for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
      vox = round (interp.scanner2voxel (*i));
      if (check (vox, H_out))
        voxels.insert (vox);
    }

  }

}

template <>
void TrackMapper<SetVoxelDir>::voxelise (SetVoxelDir& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{

  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDir vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (interp.scanner2voxel (*i));
    if (check (vox, H_out)) {
      vox.dir = (*(i+1) - *prev).normalise();
      voxels.insert (vox);
      prev = i;
    }
  }
  vox = round (interp.scanner2voxel (*last));
  if (check (vox, H_out)) {
    vox.dir = (*last - *prev).normalise();
    voxels.insert (vox);
  }

}


template <>
void TrackMapper<SetVoxelFactor>::voxelise (SetVoxelFactor& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{

  VoxelFactor vox;
  for (size_t i = 0; i != tck.size(); ++i) {
    vox = round (interp.scanner2voxel (tck[i]));
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

  switch (TrackMapper<Cont>::contrast) {

    case SCALAR_MAP:
      for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
        if (!interp.scanner (*i))
          values.push_back (interp.value());
        else
          values.push_back (NAN);
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












template <typename Cont>
class MapWriterBase
{
  public:
    MapWriterBase (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t s) :
      reader (queue),
      H (header),
      voxel_statistic (s),
      counts ((s == MEAN) ? (new DataSet::Buffer<uint32_t>(header, 3, "counts")) : NULL)
    { }

    virtual void execute () = 0;

   protected:
    typename Thread::Queue<Cont>::Reader reader;
    Image::Header& H;
    const stat_t voxel_statistic;
    Ptr< DataSet::Buffer<uint32_t> > counts;

};


template <typename Cont>
class MapWriterScalar : public MapWriterBase<Cont>
{
  public:
    MapWriterScalar (Thread::Queue<Cont>& queue, Image::Header& header, const stat_t voxel_statistic) :
      MapWriterBase<Cont> (queue, header, voxel_statistic),
      buffer (MapWriterBase<Cont>::H, 3, "buffer")
    {
    	DataSet::Loop loop;
    	if (voxel_statistic == MIN) {
    		for (loop.start (buffer); loop.ok(); loop.next (buffer))
    			buffer.value() =  INFINITY;
    	} else if (voxel_statistic == MAX) {
    		for (loop.start (buffer); loop.ok(); loop.next (buffer))
    			buffer.value() = -INFINITY;
    	}
    }

    ~MapWriterScalar ()
    {

    	DataSet::Loop loop;

      switch (MapWriterBase<Cont>::voxel_statistic) {

        case SUM: break;

        case MIN:
        	for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        		if (buffer.value() ==  INFINITY)
        		  buffer.value() = 0.0;
        	}
        	break;

        case MEAN:
        	for (loop.start (buffer, *MapWriterBase<Cont>::counts); loop.ok(); loop.next (buffer, *MapWriterBase<Cont>::counts)) {
        		if ((*MapWriterBase<Cont>::counts).value())
        			buffer.value() /= float(MapWriterBase<Cont>::counts->value());
        	}
        	break;

        case MAX:
        	for (loop.start (buffer); loop.ok(); loop.next (buffer)) {
        		if (buffer.value() == -INFINITY)
        			buffer.value() = 0.0;
        	}
        	break;

        default:
          throw Exception ("Unknown / unhandled voxel statistic in ~MapWriterScalar()");

      }
      Image::Voxel<float> vox (MapWriterBase<Cont>::H);
      DataSet::LoopInOrder loopinorder (vox, "writing image to file...", 0, 3);
      for (loopinorder.start (vox, buffer); loopinorder.ok(); loopinorder.next (vox, buffer))
        vox.value() = buffer.value();
    }

    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = (*i)[0];
          buffer[1] = (*i)[1];
          buffer[2] = (*i)[2];
          const float factor = get_factor (*item, i);
          switch (MapWriterBase<Cont>::voxel_statistic) {
            case SUM:  buffer.value() += factor; break;
            case MIN:  buffer.value() = MIN(buffer.value(), factor); break;
            case MAX:  buffer.value() = MAX(buffer.value(), factor); break;
            case MEAN:
              // Only increment counts[] if it is necessary to do so given the chosen statistic
              buffer.value() += factor;
              (*MapWriterBase<Cont>::counts)[0] = (*i)[0];
              (*MapWriterBase<Cont>::counts)[1] = (*i)[1];
              (*MapWriterBase<Cont>::counts)[2] = (*i)[2];
              (*MapWriterBase<Cont>::counts).value() += 1;
              break;
            default:
              throw Exception ("Unknown / unhandled voxel statistic in MapWriterScalar::execute()");
          }
        }
      }
    }


  private:
    DataSet::Buffer<float> buffer;

    float get_factor (const Cont&, const typename Cont::const_iterator) const;


};

template <> float MapWriterScalar<SetVoxel>      ::get_factor (const SetVoxel&       set, const SetVoxel      ::const_iterator item) const { return set.factor; }
template <> float MapWriterScalar<SetVoxelFactor>::get_factor (const SetVoxelFactor& set, const SetVoxelFactor::const_iterator item) const { return item->get_factor(); }



template <typename Cont>
class MapWriterColour : public MapWriterBase<Cont>
{

  public:
    MapWriterColour(Thread::Queue<Cont>& queue, Image::Header& header, const stat_t voxel_statistic) :
      MapWriterBase<Cont>(queue, header, voxel_statistic),
      buffer (MapWriterBase<Cont>::H, 3, "directional_buffer")
    {
      if (voxel_statistic != SUM && voxel_statistic != MEAN)
        throw Exception ("Attempting to create MapWriterColour with a voxel statistic other than sum or mean");
    }

    ~MapWriterColour ()
    {
      Image::Voxel<float> vox (MapWriterBase<Cont>::H);
      DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
      for (loop.start (vox, buffer); loop.ok(); loop.next (vox, buffer)) {
        Point<float> temp = buffer.value();
        // Mean direction - no need to track the number of contribution
        if ((MapWriterBase<Cont>::voxel_statistic == MEAN) && (temp != Point<float> (0.0, 0.0, 0.0)))
          temp.normalise();
        vox[3] = 0; vox.value() = temp[0];
        vox[3] = 1; vox.value() = temp[1];
        vox[3] = 2; vox.value() = temp[2];
      }
    }

    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = (*i)[0];
          buffer[1] = (*i)[1];
          buffer[2] = (*i)[2];
          // Voxel statistic for DECTDI is always sum
          // This also means that there is no need to increment counts[]
          // If this is being used for MEAN_DIR contrast, the direction can just be normalised; again, don't need counts
          buffer.value() += abs (i->dir);
        }
      }
    }


  private:
    DataSet::Buffer< Point<float> > buffer;

};




void generate_header (Image::Header& header, Tractography::Reader<float>& file, const std::vector<float>& voxel_size) 
{

  std::vector<Point<float> > tck;
  size_t track_counter = 0;

  Point<float> min_values ( INFINITY,  INFINITY,  INFINITY);
  Point<float> max_values (-INFINITY, -INFINITY, -INFINITY);

  {
    ProgressBar progress ("creating new template image...", 0);
    while (file.next(tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
      for (std::vector<Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
        min_values[0] = std::min (min_values[0], (*i)[0]);
        max_values[0] = std::max (max_values[0], (*i)[0]);
        min_values[1] = std::min (min_values[1], (*i)[1]);
        max_values[1] = std::max (max_values[1], (*i)[1]);
        min_values[2] = std::min (min_values[2], (*i)[2]);
        max_values[2] = std::max (max_values[2], (*i)[2]);
      }
      ++progress;
    }
  }

  min_values -= Point<float> (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);
  max_values += Point<float> (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);

  header.set_name ("tckmap image header");
  header.set_ndim (3);

  for (size_t i = 0; i != 3; ++i) {
    header.set_dim (i, Math::ceil((max_values[i] - min_values[i]) / voxel_size[i]));
    header.set_vox (i, voxel_size[i]);
    header.set_stride (i, i+1);
    header.set_units (i, Image::Axis::millimeters);
  }

  header.set_description (0, Image::Axis::left_to_right);
  header.set_description (1, Image::Axis::posterior_to_anterior);
  header.set_description (2, Image::Axis::inferior_to_superior);

  Math::Matrix<float>& M (header.get_transform());
  M.allocate (4,4);
  M.identity();
  M(0,3) = min_values[0];
  M(1,3) = min_values[1];
  M(2,3) = min_values[2];

}






template <typename T>
Math::Matrix<T> gen_interp_matrix (const size_t os_factor)
{
  Math::Matrix<T> M;
  if (os_factor > 1) {
    const size_t dim = os_factor - 1;
    Math::Hermite<T> interp (HERMITE_TENSION);
    M.allocate (dim, 4);
    for (size_t i = 0; i != dim; ++i) {
      interp.set ((i+1.0) / float(os_factor));
      for (size_t j = 0; j != 4; ++j)
        M(i,j) = interp.coef(j);
    }
  }
  return (M);
}





void oversample_header (Image::Header& header, const std::vector<float>& voxel_size) 
{
  info ("oversampling header...");

  for (size_t i = 0; i != 3; ++i) {
    header.get_transform()(i, 3) += 0.5 * (voxel_size[i] - header.vox(i));
    header.set_dim (i, Math::ceil(header.dim(i) * header.vox(i) / voxel_size[i]));
    header.set_vox (i, voxel_size[i]);
  }
}





void run () {

  Tractography::Properties properties;
  Tractography::Reader<float> file;
  file.open (argument[0], properties);

  const size_t num_tracks = properties["count"]    .empty() ? 0   : to<size_t> (properties["count"]);
  const float  step_size  = properties["step_size"].empty() ? 1.0 : to<float>  (properties["step_size"]);

  std::vector<float> voxel_size;
  Options opt = get_options("vox");
  if (opt.size())
    voxel_size = opt[0][0];

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    info("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Image::Header header;
  opt = get_options ("template");
  if (opt.size()) {
    Image::Header template_header (opt[0][0]);
    header = template_header;
    if (!voxel_size.empty())
      oversample_header (header, voxel_size);
  }
  else {
    if (voxel_size.empty())
      throw Exception ("please specify either a template image or the desired voxel size");
    generate_header (header, file, voxel_size);
    file.close();
    file.open (argument[0], properties);
  }

  header.set_ndim (3);

  opt = get_options ("contrast");
  contrast_t contrast = opt.size() ? contrast_t(int(opt[0][0])) : TDI;

  opt = get_options ("stat_vox");
  stat_t stat_vox = opt.size() ? stat_t(int(opt[0][0])) : SUM;

  opt = get_options ("stat_tck");
  stat_t stat_tck = opt.size() ? stat_t(int(opt[0][0])) : MEAN;

  if (stat_vox == MEDIAN)
    throw Exception ("Sorry, can't calculate median values for each voxel - would take too much memory");

  float gaussian_fwhm_tck = 0.0, gaussian_denominator_tck = 0.0; // gaussian_theta = 0.0;
  opt = get_options ("fwhm_tck");
  if (opt.size()) {
    if (stat_tck != GAUSSIAN) {
      info ("Overriding per-track statistic to Gaussian as a full-width half-maximum has been provided.");
      stat_tck = GAUSSIAN;
    }
    gaussian_fwhm_tck = opt[0][0];
    const float gaussian_theta_tck = gaussian_fwhm_tck / (2.0 * sqrt (2.0 * log (2.0)));
    gaussian_denominator_tck = 2.0 * gaussian_theta_tck * gaussian_theta_tck;
  } else if (stat_tck == GAUSSIAN) {
    throw Exception ("If using Gaussian per-streamline statistic, need to provide a full-width half-maximum for the Gaussian kernel using the -fwhm option");
  }

  // Deal with erroneous statistics & provide appropriate messages
  switch (contrast) {

    case TDI:
      if (stat_vox != SUM)
        info ("Cannot use voxel statistic other than 'sum' for TDI generation - ignoring");
      stat_vox = SUM;
      if (stat_tck != MEAN)
        info ("Cannot use track statistic other than default for TDI generation - ignoring");
      stat_tck = MEAN;
      break;

    case DECTDI:
      if (stat_vox != SUM)
        info ("Cannot use voxel statistic other than 'sum' for DECTDI generation - ignoring");
      stat_vox = SUM;
      if (stat_tck != MEAN)
        info ("Cannot use track statistic other than default for DECTDI generation - ignoring");
      stat_tck = MEAN;
      break;

      case ENDPOINT:
      if (stat_vox != SUM)
        info ("Cannot use voxel statistic other than 'sum' for endpoint map generation - ignoring");
      stat_vox = SUM;
      if (stat_tck != MEAN)
        info ("Cannot use track statistic other than default for endpoint map generation - ignoring");
      stat_tck = MEAN;
      break;

    case MEAN_DIR:
      if (stat_vox != SUM && stat_vox != MEAN)
        info ("Cannot use voxel statistic other than 'mean' for mean direction image - ignoring");
      stat_vox = MEAN;
      if (stat_tck != MEAN)
        info ("Cannot use track statistic other than default for mean direction image - ignoring");
      stat_tck = MEAN;

    case LENGTH:
      if (stat_tck != MEAN)
        info ("Cannot use track statistic other than default for length-weighted TDI generation - ignoring");
      stat_tck = MEAN;
      break;

    case INVLENGTH:
      if (stat_tck != MEAN)
        info ("Cannot use track statistic other than default for inverse-length-weighted TDI generation - ignoring");
      stat_tck = MEAN;
      break;

    case SCALAR_MAP:
      break;

    case FOD_AMP:
      break;

    case CURVATURE:
      //throw Exception ("Sorry; curvature contrast not yet working");
    	break;

    default:
      throw Exception ("Undefined contrast mechanism");

  }


  opt = get_options ("datatype");
  bool manual_datatype = false;
  if (opt.size()) {
    header.set_datatype (DataType::identifiers[int(opt[0][0])]);
    manual_datatype = true;
  }

  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i)
    header.add_comment (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.add_comment ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.add_comment ("comment: " + *i);

  size_t resample_factor;
  opt = get_options ("resample");
  if (opt.size()) {
    resample_factor = opt[0][0];
    info ("track interpolation factor manually set to " + str(resample_factor));
  }
  else if (step_size) {
    resample_factor = Math::ceil<size_t> (step_size / (minvalue (header.vox(0), header.vox(1), header.vox(2)) * MAX_VOXEL_STEP_RATIO));
    info ("track interpolation factor automatically set to " + str(resample_factor));
  }
  else {
    resample_factor = 1;
    info ("track interpolation off; no track step size information in header");
  }

  Math::Matrix<float> interp_matrix (gen_interp_matrix<float> (resample_factor));

  TrackQueue queue1 ("loaded tracks");
  TrackLoader loader (queue1, file, num_tracks);

  std::string msg = MR::App::NAME + ": Generating image with ";
  switch (contrast) {
    case TDI:        msg += "density";          break;
    case DECTDI:     msg += "coloured density"; break;
    case ENDPOINT:   msg += "endpoint density"; break;
    case MEAN_DIR:   msg += "mean direction";   break;
    case LENGTH:     msg += "length";           break;
    case INVLENGTH:  msg += "inverse length";   break;
    case SCALAR_MAP: msg += "scalar map";       break;
    case FOD_AMP:    msg += "FOD amplitude";    break;
    case CURVATURE:  msg += "curvature";        break;
    default:         msg += "ERROR";            break;
  }
  msg += " contrast";
  if (contrast == SCALAR_MAP || contrast == FOD_AMP || contrast == CURVATURE)
    msg += ", ";
  else
    msg += " and ";
  switch (stat_vox) {
    case SUM:    msg += "summed";  break;
    case MIN:    msg += "minimum"; break;
    case MEAN:   msg += "mean";    break;
    case MAX:    msg += "maximum"; break;
    default:     msg += "ERROR";   break;
  }
  msg += " per-voxel statistic";
  if (contrast == SCALAR_MAP || contrast == FOD_AMP || contrast == CURVATURE) {
    msg += " and ";
    switch (stat_tck) {
      case SUM:      msg += "summed";  break;
      case MIN:      msg += "minimum"; break;
      case MEAN:     msg += "mean";    break;
      case MEDIAN:   msg += "median";  break;
      case MAX:      msg += "maximum"; break;
      case GAUSSIAN: msg += "gaussian (FWHM " + str (gaussian_fwhm_tck) + "mm)"; break;
      default:       msg += "ERROR";   break;
    }
    msg += " per-track statistic";
  }
  std::cerr << msg << "\n";

  // Use a branching IF instead of a switch statement to permit scope
  if (contrast == TDI || contrast == ENDPOINT || contrast == LENGTH || contrast == INVLENGTH || contrast == CURVATURE) {

    if (!manual_datatype)
      header.set_datatype ((contrast == TDI || contrast == ENDPOINT) ? DataType::UInt32 : DataType::Float32);

    switch (contrast) {
      case TDI:        header.add_comment ("track density image"); break;
      case DECTDI:     header.add_comment ("directionally-encoded colour track density image"); break;
      case ENDPOINT:   header.add_comment ("track endpoint density image"); break;
      case MEAN_DIR:   header.add_comment ("mean direction coloured image"); break;
      case LENGTH:     header.add_comment ("track density image (weighted by track length)"); break;
      case INVLENGTH:  header.add_comment ("track density image (weighted by inverse track length)"); break;
      case SCALAR_MAP: header.add_comment ("track-weighted image (using scalar image)"); break;
      case FOD_AMP:    header.add_comment ("track-weighted image (using FOD amplitude)"); break;
      case CURVATURE:  header.add_comment ("track-weighted image (using track curvature)"); break;
      default:         break; // Just to shut up the compiler
    }

    header.create (argument[1]);

    if (stat_tck == GAUSSIAN) {

      Thread::Queue   <SetVoxelFactor> queue2 ("processed tracks");
      TrackMapper     <SetVoxelFactor> mapper (queue1, queue2, header, interp_matrix, step_size, contrast, stat_tck, gaussian_denominator_tck);
      MapWriterScalar <SetVoxelFactor> writer (queue2, header, stat_vox);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapper<SetVoxelFactor> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    } else {

      Thread::Queue   <SetVoxel> queue2 ("processed tracks");
      TrackMapper     <SetVoxel> mapper (queue1, queue2, header, interp_matrix, step_size, contrast, stat_tck);
      MapWriterScalar <SetVoxel> writer (queue2, header, stat_vox);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapper<SetVoxel> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    }

  } else if (contrast == DECTDI || contrast == MEAN_DIR) {

    if (!manual_datatype)
      header.set_datatype (DataType::Float32);

    header.set_ndim (4);
    header.set_dim (3, 3);
    header.set_stride (0, header.stride(0) + ( header.stride(0) > 0 ? 1 : -1));
    header.set_stride (1, header.stride(1) + ( header.stride(1) > 0 ? 1 : -1));
    header.set_stride (2, header.stride(2) + ( header.stride(2) > 0 ? 1 : -1));
    header.set_stride (3, 1);
    header.set_description (3, "direction");
    header.add_comment ("coloured track density map");

    header.create (argument[1]);

    Thread::Queue   <SetVoxelDir> queue2 ("processed tracks");
    TrackMapper     <SetVoxelDir> mapper (queue1, queue2, header, interp_matrix, step_size, contrast, stat_tck);
    MapWriterColour <SetVoxelDir> writer (queue2, header, stat_vox);

    Thread::Exec loader_thread (loader, "loader");
    Thread::Array< TrackMapper<SetVoxelDir> > mapper_list (mapper);
    Thread::Exec mapper_threads (mapper_list, "mapper");

    writer.execute();

  } else if (contrast == SCALAR_MAP || contrast == FOD_AMP) {

    opt = get_options ("image");
    if (!opt.size()) {
      if (contrast == SCALAR_MAP)
        throw Exception ("If using 'scalar_map' contrast, must provide the relevant scalar image using -image option");
      else
        throw Exception ("If using 'fod_amp' contrast, must provide the relevant spherical harmonic image using -image option");
    }

    Image::Header H_in (opt[0][0]);
    if (contrast == SCALAR_MAP && !(H_in.ndim() == 3 || (H_in.ndim() == 4 && H_in.dim(3) == 1)))
      throw Exception ("Use of 'scalar_map' contrast option requires a 3-dimensional image; your image is " + str(H_in.ndim()) + "D");
    if (contrast == FOD_AMP    &&   H_in.ndim() != 4)
      throw Exception ("Use of 'fod_amp' contrast option requires a 4-dimensional image; your image is " + str(H_in.ndim()) + "D");

    if (!manual_datatype && (H_in.datatype() != DataType::Bit))
      header.set_datatype (H_in.datatype());

    header.create (argument[1]);

    if (stat_tck == GAUSSIAN) {

      Thread::Queue          <SetVoxelFactor> queue2 ("processed tracks");
      TrackMapperImage       <SetVoxelFactor> mapper (queue1, queue2, header, interp_matrix, step_size, contrast, stat_tck, gaussian_denominator_tck, H_in);
      MapWriterScalar        <SetVoxelFactor> writer (queue2, header, stat_vox);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapperImage<SetVoxelFactor> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    } else {

      Thread::Queue    <SetVoxel> queue2 ("processed tracks");
      TrackMapperImage <SetVoxel> mapper (queue1, queue2, header, interp_matrix, step_size, contrast, stat_tck, gaussian_denominator_tck, H_in);
      MapWriterScalar  <SetVoxel> writer (queue2, header, stat_vox);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapperImage<SetVoxel> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    }

  } else {
    throw Exception ("Undefined contrast mechanism for output image");
  }

}

