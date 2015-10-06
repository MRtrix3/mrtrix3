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

  + Option ("resample",
      "resample the tracks at regular intervals using Hermite interpolation\n"
      "(If omitted, an appropriate interpolation will be determined automatically)")
    + Argument ("factor").type_integer (1, 1, std::numeric_limits<int>::max());

}








class Mapper : public Mapping::TrackMapperBase
{

    typedef Image::BufferPreload<float>::voxel_type input_voxel_type;

  public:
    Mapper (const Image::Header& header, const size_t upsample_ratio, Image::BufferPreload<float>& input_image, const std::vector<float>& windowing_function, const int timepoint) :
        Mapping::TrackMapperBase (header),
        in (input_image),
        fmri_transform (input_image),
        kernel (windowing_function),
        kernel_centre (kernel.size() / 2),
        sample_centre (timepoint)
    {
      Mapping::TrackMapperBase::set_upsample_ratio (upsample_ratio);
    }


  private:
    const input_voxel_type in; // Copy-construct from this rather than using it directly (one for each streamline endpoint)
    const Image::Transform fmri_transform;
    const std::vector<float>& kernel;
    const int kernel_centre, sample_centre;

    // This is where the windowed Pearson correlation coefficient for the streamline is determined
    // By overloading this function, the standard mapping functionalities of TrackMapperBase are utilised;
    //   it's only the per-factor streamline that changes
    bool preprocess (const Streamline<>&, SetVoxelExtras&) const override;

    const Point<int> get_last_voxel_in_fov (const std::vector< Point<float> >&, const bool) const;

};



bool Mapper::preprocess (const Streamline<>& tck, SetVoxelExtras& out) const
{

  out.factor = 0.0;

  input_voxel_type start_voxel (in), end_voxel (in);
  const Point<int> v_start (get_last_voxel_in_fov (tck, false));
  if (v_start == Point<int> (-1, -1, -1))
    return false;
  Image::Nav::set_pos (start_voxel, v_start);

  const Point<int> v_end (get_last_voxel_in_fov (tck, true));
  if (v_end == Point<int> (-1, -1, -1))
    return false;
  Image::Nav::set_pos (end_voxel, v_end);

  double start_mean = 0.0, end_mean = 0.0, kernel_sum = 0.0, kernel_sq_sum = 0.0;
  for (size_t i = 0; i != kernel.size(); ++i) {
    start_voxel[3] = end_voxel[3] = sample_centre - kernel_centre + int(i);
    if (start_voxel[3] >= 0 && start_voxel[3] < start_voxel.dim(3)) {
      start_mean += kernel[i] * start_voxel.value();
      end_mean   += kernel[i] * end_voxel  .value();
      kernel_sum += kernel[i];
      kernel_sq_sum += Math::pow2 (kernel[i]);
    }
  }
  start_mean /= kernel_sum;
  end_mean   /= kernel_sum;

  const double denom = kernel_sum - (kernel_sq_sum / kernel_sum);

  double corr = 0.0, start_variance = 0.0, end_variance = 0.0;
  for (size_t i = 0; i != kernel.size(); ++i) {
    start_voxel[3] = end_voxel[3] = sample_centre - kernel_centre + int(i);
    if (start_voxel[3] >= 0 && start_voxel[3] < start_voxel.dim(3)) {
      corr           += kernel[i] * (start_voxel.value() - start_mean) * (end_voxel.value() - end_mean);
      start_variance += kernel[i] * Math::pow2 (start_voxel.value() - start_mean);
      end_variance   += kernel[i] * Math::pow2 (end_voxel  .value() - end_mean  );
    }
  }
  corr           /= denom;
  start_variance /= denom;
  end_variance   /= denom;

  out.factor = corr / std::sqrt (start_variance * end_variance);
  return true;

}



// Slightly different to get_last_point_in_fov() provided in the TrackMapperTWIImage:
//   want the last voxel traversed by the streamline before exiting the FoV, rather than the last
//   point for which a valid tri-linear interpolation can be performed
const Point<int> Mapper::get_last_voxel_in_fov (const std::vector< Point<float> >& tck, const bool end) const
{

  int index = end ? tck.size() - 1 : 0;
  const int step  = end ? -1 : 1;
  do {
    const Point<float>& p = fmri_transform.scanner2voxel (tck[index]);
    const Point<int> v (std::round (p[0]), std::round (p[1]), std::round (p[2]));
    if (Image::Nav::within_bounds (in, v))
      return v;
    index += step;
  } while (index >= 0 && index < int(tck.size()));

  return Point<int> (-1, -1, -1);

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
  else if (step_size && finite (step_size)) {
    resample_factor = std::ceil (step_size / (minvalue (header.vox(0), header.vox(1), header.vox(2)) * MAX_VOXEL_STEP_RATIO));
    INFO ("track interpolation factor automatically set to " + str(resample_factor));
  }
  else {
    resample_factor = 1;
    WARN ("track interpolation off; track step size information in header is absent or malformed");
  }

  //Math::Matrix<float> interp_matrix (resample_factor > 1 ? gen_interp_matrix<float> (resample_factor) : Math::Matrix<float> ());

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
      Mapper mapper (H_3d, resample_factor, in_image, window, timepoint);
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


