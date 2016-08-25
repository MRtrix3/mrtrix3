/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

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

#include <vector>

#include "command.h"
#include "exception.h"
#include "point.h"
#include "progressbar.h"
#include "thread.h"
#include "thread_queue.h"

#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/transform.h"
#include "image/voxel.h"
#include "math/matrix.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/writer.h"



#define MAX_TRACKS_READ_FOR_HEADER 1000000
#define MAX_VOXEL_STEP_RATIO 0.333




using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;



#define DEFAULT_SLIDING_WINDOW_WIDTH 15
const char* windows[] = { "rectangle", "triangle", "cosine", "hann", "hamming", "lanczos", NULL };


void usage () {

AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

DESCRIPTION
  + "Generate a sliding-window TWI where the contribution from each streamline is the Pearson correlation between its endpoints in an fMRI time series within that sliding window";

ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file_in()
  + Argument ("fmri",   "the fMRI time series").type_image_in()
  + Argument ("output", "the output sliding-window TWI image").type_image_out();

OPTIONS
  + Option ("template",
      "an image file to be used as a template for the output (the output image "
      "will have the same transform and field of view).")
    + Argument ("image").type_image_in()

  + Option ("vox",
      "provide either an isotropic voxel size (in mm), or comma-separated list "
      "of 3 voxel dimensions.")
    + Argument ("size").type_sequence_float()

  + Option ("stat_vox",
      "define the statistic for choosing the final voxel intensities for a given contrast "
      "type given the individual values from the tracks passing through each voxel\n"
      "Options are: sum, min, mean, max (default: mean)")
    + Argument ("type").type_choice (voxel_statistics)

  + Option ("window_shape", "specify the shape of the sliding window weighting function. "
                            "Options are: rectangle, triangle, cosine, hann, hamming, lanczos (default = rectangle)")
    + Argument ("shape").type_choice (windows, 0)

  + Option ("window_width", "set the full width of the sliding window (in volumes, not time) (must be an odd number)")
    + Argument ("value").type_integer (3, 15, 1e6-1)

  + Option ("backtrack",
            "if no valid timeseries is found at the streamline endpoint, backtrack along "
            "the streamline trajectory until a valid timeseries is found")

  + Option ("resample",
      "resample the tracks at regular intervals using Hermite interpolation\n"
      "(If omitted, an appropriate interpolation will be determined automatically)")
    + Argument ("factor").type_integer (1, 1, std::numeric_limits<int>::max());

}








class Mapper : public Mapping::TrackMapperBase
{

    typedef Image::BufferPreload<float>::voxel_type input_voxel_type;
    typedef Image::Interp::Linear<input_voxel_type> interp_type;

  public:
    Mapper (const Image::Header& header, const size_t upsample_ratio, Image::BufferPreload<float>& input_image, const std::vector<float>& windowing_function, const int timepoint, std::unique_ptr<Image::BufferScratch<bool>>& mask) :
        Mapping::TrackMapperBase (header),
        vox (input_image),
        interp (vox),
        fmri_transform (input_image),
        kernel (windowing_function),
        kernel_centre (kernel.size() / 2),
        sample_centre (timepoint),
        v_mask (mask ? new Image::BufferScratch<bool>::voxel_type (*mask) : nullptr)
    {
      Mapping::TrackMapperBase::set_upsample_ratio (upsample_ratio);
    }


  private:
    input_voxel_type vox;
    mutable interp_type interp;
    const Image::Transform fmri_transform;
    const std::vector<float>& kernel;
    const int kernel_centre, sample_centre;

    // Build a binary mask of voxels that contain valid time series
    //   (only used if backtracking is enabled)
    MR::copy_ptr<Image::BufferScratch<bool>::voxel_type> v_mask;

    // This is where the windowed Pearson correlation coefficient for the streamline is determined
    // By overloading this function, the standard mapping functionalities of TrackMapperBase are utilised;
    //   it's only the per-factor streamline that changes
    bool preprocess (const Streamline<>&, SetVoxelExtras&) const override;

    const Point<float> get_end_point (const std::vector< Point<float> >&, const bool) const;

};



bool Mapper::preprocess (const Streamline<>& tck, SetVoxelExtras& out) const
{
  out.factor = 0.0;

  // Use trilinear interpolation
  // Store values into local vectors, since it's a two-pass operation
  std::vector<double> values[2];
  for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
    const Point<float> endpoint = get_end_point (tck, tck_end_index);
    if (!endpoint.valid())
      return true;
    values[tck_end_index].reserve (kernel.size());
    for (size_t i = 0; i != kernel.size(); ++i) {
      interp[3] = sample_centre - kernel_centre + int(i);
      if (interp[3] >= 0 && interp[3] < interp.dim(3))
        values[tck_end_index].push_back (interp.value());
      else
        values[tck_end_index].push_back (NAN);
    }
  }

  // Calculate the Pearson correlation coefficient within the kernel window
  double sums[2] = { 0.0, 0.0 };
  double kernel_sum = 0.0, kernel_sq_sum = 0.0;
  for (size_t i = 0; i != kernel.size(); ++i) {
    if (std::isfinite (values[0][i])) {
      sums[0] += kernel[i] * values[0][i];
      sums[1] += kernel[i] * values[1][i];
      kernel_sum += kernel[i];
      kernel_sq_sum += Math::pow2 (kernel[i]);
    }
  }
  const double means[2] = { sums[0] / kernel_sum, sums[1] / kernel_sum };
  const double denom = kernel_sum - (kernel_sq_sum / kernel_sum);

  double corr = 0.0, start_variance = 0.0, end_variance = 0.0;
  for (size_t i = 0; i != kernel.size(); ++i) {
    if (std::isfinite (values[0][i])) {
      corr           += kernel[i] * (values[0][i] - means[0]) * (values[1][i] - means[1]);
      start_variance += kernel[i] * Math::pow2 (values[0][i] - means[0]);
      end_variance   += kernel[i] * Math::pow2 (values[1][i] - means[1]);
    }
  }
  corr           /= denom;
  start_variance /= denom;
  end_variance   /= denom;

  if (start_variance && end_variance)
    out.factor = corr / std::sqrt (start_variance * end_variance);
  return true;
}



// This function selects a streamline position to sample for this streamline endpoint. If backtracking
//   is enabled, and the endpoint voxel is either outside the FoV or doesn't contain a valid
//   time series, trace back along the length of the streamline until a voxel with a valid
//   time series is found.
// Note that because trilinear interpolation is used, theoretically a valid time series could be
//   obtained for a point within a voxel with no time series; nevertheless, from an implementation
//   perspective, it's easier to just require that the voxel in which the sample point resides
//   has a valid time series
const Point<float> Mapper::get_end_point (const std::vector< Point<float> >& tck, const bool end) const
{
  int index = end ? tck.size() - 1 : 0;
  if (v_mask) {

    // Do backtracking
    const int step = end ? -1 : 1;
    for (; index >= 0 && index <= int(tck.size() - 1); index += step) {
      const Point<float> p = fmri_transform.scanner2voxel (tck[index]);
      const Point<int> v (std::round (p[0]), std::round (p[1]), std::round (p[2]));
      // Is this point both within the FoV, and contains a valid time series?
      if (Image::Nav::within_bounds (vox, v) && Image::Nav::get_value_at_pos (*v_mask, v))
        return tck[index];
    }
    return Point<float>();

  } else {

    const Point<float> p = fmri_transform.scanner2voxel (tck[index]);
    const Point<int> v (std::round (p[0]), std::round (p[1]), std::round (p[2]));
    if (Image::Nav::within_bounds (vox, v))
      return tck[index];
    return Point<float>();

  }

}




// This class is similar to Mapping::MapWriter, but doesn't write to a HDD file on close
class Receiver
{

  public:
    Receiver (const Image::Header& header, const vox_stat_t stat_vox) :
        buffer (header),
        v_buffer (buffer),
        vox_stat (stat_vox)
    {
      Image::Loop loop;
      if (vox_stat == V_MIN) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = std::numeric_limits<float>::max();
      } else if (vox_stat == V_MAX) {
        for (loop.start (v_buffer); loop.ok(); loop.next (v_buffer))
          v_buffer.value() = -std::numeric_limits<float>::max();
      }
    }


    bool operator() (const Mapping::SetVoxel&);
    void scale_by_count (Image::BufferScratch<uint32_t>&);
    void write (Image::Buffer<float>::voxel_type&);


  private:
    Image::BufferScratch<float> buffer;
    Image::BufferScratch<float>::voxel_type v_buffer;
    const vox_stat_t vox_stat;

};



bool Receiver::operator() (const Mapping::SetVoxel& in)
{
  const float factor = in.factor;
  for (Mapping::SetVoxel::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v_buffer, *i);
    switch (vox_stat) {
      case V_SUM:  v_buffer.value() += factor; break;
      case V_MIN:  v_buffer.value() = std::min (float(v_buffer.value()), factor); break;
      case V_MAX:  v_buffer.value() = std::max (float(v_buffer.value()), factor); break;
      case V_MEAN: v_buffer.value() += factor; break;
      // Unlike Mapping::MapWriter, don't need to deal with counts here
    }
  }
  return true;
}

void Receiver::scale_by_count (Image::BufferScratch<uint32_t>& counts)
{
  Image::BufferScratch<uint32_t>::voxel_type v_counts (counts);
  Image::Loop loop;
  for (loop.start (v_buffer, v_counts); loop.ok(); loop.next (v_buffer, v_counts)) {
    if (v_counts.value())
      v_buffer.value() /= float(v_counts.value());
    else
      v_buffer.value() = 0.0f;
  }
}

void Receiver::write (Image::Buffer<float>::voxel_type& v_out)
{
  Image::LoopInOrder loop (v_out, 0, 3);
  for (loop.start (v_buffer, v_out); loop.ok(); loop.next (v_buffer, v_out))
    v_out.value() = v_buffer.value();
}







// Separate class for generating TDI i.e. receive SetVoxel & write directly to counts
class Count_receiver
{

  public:
    Count_receiver (Image::BufferScratch<uint32_t>& out) :
      v (out) { }

    bool operator() (const Mapping::SetVoxel&);

  private:
    Image::BufferScratch<uint32_t>::voxel_type v;

};



bool Count_receiver::operator() (const Mapping::SetVoxel& in)
{
  for (Mapping::SetVoxel::const_iterator i = in.begin(); i != in.end(); ++i) {
    Image::Nav::set_pos (v, *i);
    //if (Image::Nav::within_bounds (v)) // TrackMapperBase checks this
      v.value() = v.value() + 1;
  }
  return true;
}





void run () {

  const std::string tck_path = argument[0];
  Tractography::Properties properties;
  {
    // Just get the properties for now; will re-instantiate the reader multiple times later
    // TODO Constructor for properties using the file path?
    Tractography::Reader<float> tck_file (tck_path, properties);
  }

  Image::Stride::List strides (4, 0);
  strides[0] = 2; strides[1] = 3; strides[2] = 4; strides[3] = 1;
  Image::BufferPreload<float> in_image (argument[1], strides);

  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);

  float step_size = 0.0;
  if (properties.find ("output_step_size") != properties.end())
    step_size = to<float> (properties["output_step_size"]);
  else
    step_size = to<float> (properties["step_size"]);

  std::vector<float> voxel_size;
  Options opt = get_options("vox");
  if (opt.size())
    voxel_size = opt[0][0];

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    INFO ("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Image::Header header;
  header.datatype() = DataType::Float32;
  header.datatype().set_byte_order_native();
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
    generate_header (header, argument[0], voxel_size);
  }

  header.set_ndim (4);
  header.dim(3) = in_image.dim(3);
  header.comments().push_back ("Sliding window track-weighted image");

  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i)
    header.comments().push_back (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments().push_back ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments().push_back ("comment: " + *i);

  size_t resample_factor;
  opt = get_options ("resample");
  if (opt.size()) {
    resample_factor = opt[0][0];
    INFO ("track interpolation factor manually set to " + str(resample_factor));
  }
  else if (step_size && std::isfinite (step_size)) {
    resample_factor = std::ceil (step_size / (minvalue (header.vox(0), header.vox(1), header.vox(2)) * MAX_VOXEL_STEP_RATIO));
    INFO ("track interpolation factor automatically set to " + str(resample_factor));
  }
  else {
    resample_factor = 1;
    WARN ("track interpolation off; track step size information in header is absent or malformed");
  }

  opt = get_options ("stat_vox");
  vox_stat_t stat_vox = opt.size() ? vox_stat_t(int(opt[0][0])) : V_MEAN;

  // Generate the window filter
  opt = get_options ("window_width");
  const int window_width = opt.size() ? to<int>(opt[0][0]) : DEFAULT_SLIDING_WINDOW_WIDTH;
  if (!(window_width % 2))
    throw Exception ("Width of sliding time window must be an odd integer");
  opt = get_options ("window_shape");
  const int window_shape = opt.size() ? opt[0][0] : 0; // default = rectangular

  std::vector<float> window (window_width, 0.0);
  const int halfwidth = (window_width+1) / 2;
  const int centre = (window_width-1) / 2; // Element at centre of the window

  switch (window_shape) {

    case 0: // rectangular
      window.assign (window_width, 1.0);
      break;

    case 1: // triangle
      for (int i = 0; i != window_width; ++i)
        window[i] = 1.0 - (std::abs (i - centre) / float(halfwidth));
      break;

    case 2: // cosine
      for (int i = 0; i != window_width; ++i)
        window[i] = std::sin (i * M_PI / float(window_width - 1));
      break;

    case 3: // hann
      for (int i = 0; i != window_width; ++i)
        window[i] = 0.5 * (1.0 - std::cos (2.0 * M_PI * i / float(window_width - 1)));
      break;

    case 4: // hamming
      for (int i = 0; i != window_width; ++i)
        window[i] = 0.53836 - (0.46164 * std::cos (2.0 * M_PI * i / float(window_width - 1)));
      break;

    case 5: // lanczos
      for (int i = 0; i != window_width; ++i) {
        const double v = 2.0 * M_PI * std::abs (i - centre) / float(window_width - 1);
        window[i] = v ? std::max (0.0, (std::sin (v) / v)) : 1.0;
      }
      break;

  }

  // If necessary, generate mask of fMRI voxels with valid timeseries
  //   (use ptr value as flag for backtracking activation also)
  std::unique_ptr<Image::BufferScratch<bool>> mask;
  if (get_options ("backtrack").size()) {
    Image::Header H_mask (header);
    H_mask.set_ndim (3);
    H_mask.datatype() = DataType::Bit;
    mask.reset (new Image::BufferScratch<bool> (H_mask));
    Image::BufferScratch<bool>::voxel_type v_mask (*mask);
    Image::BufferPreload<float>::voxel_type v_in (in_image);

    auto f = [] (Image::BufferPreload<float>::voxel_type& in, Image::BufferScratch<bool>::voxel_type& mask) {
      for (in[3] = 0; in[3] != in.dim(3); ++in[3]) {
        if (std::isfinite (in.value()) && in.value()) {
          mask.value() = true;
          return true;
        }
      }
      mask.value() = false;
      return true;
    };

    Image::ThreadedLoop ("pre-calculating mask of valid time-series voxels...", v_mask)
        .run (f, v_in, v_mask);
  }

  // TODO Reconsider pre-calculating & storing SetVoxel for each streamline
  // - Faster, but ups RAM requirements, may become prohibitive with super-resolution
  // TODO Add voxel-wise statistic V_MEAN_ABS - mean absolute value?

  Image::Buffer<float> out_image (argument[2], header);
  Image::Header H_3d (header);
  H_3d.set_ndim (3);

  std::unique_ptr< Image::BufferScratch<uint32_t> > counts;
  if (stat_vox == V_MEAN) {
    counts.reset (new Image::BufferScratch<uint32_t> (H_3d));
    Tractography::Reader<float> tck_file (tck_path, properties);
    Mapping::TrackLoader loader (tck_file, num_tracks, "Calculating initial TDI... ");
    Mapping::TrackMapperBase mapper (H_3d);
    mapper.set_upsample_ratio (resample_factor);
    Count_receiver receiver (*counts);
    Thread::run_queue (loader, Thread::batch (Tractography::Streamline<>()), Thread::multi (mapper), Thread::batch (Mapping::SetVoxel()), receiver);
  }


  ProgressBar progress ("Generating sliding time-window TWI... ", header.dim(3));
  for (int timepoint = 0; timepoint != header.dim(3); ++timepoint) {

    {
      LogLevelLatch latch (0);
      Tractography::Reader<float> tck_file (tck_path, properties);
      Mapping::TrackLoader loader (tck_file);
      Mapper mapper (H_3d, resample_factor, in_image, window, timepoint, mask);
      Receiver receiver (H_3d, stat_vox);
      Thread::run_queue (loader, Thread::batch (Tractography::Streamline<>()), Thread::multi (mapper), Thread::batch (Mapping::SetVoxel()), receiver);

      if (stat_vox == V_MEAN)
        receiver.scale_by_count (*counts);

      Image::Buffer<float>::voxel_type v_out (out_image);
      v_out[3] = timepoint;
      receiver.write (v_out);
    }

    ++progress;
  }

}


