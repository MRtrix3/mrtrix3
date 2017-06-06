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

#include "command.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "progressbar.h"
#include "thread.h"
#include "thread_queue.h"
#include "transform.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/writer.h"



#define MAX_VOXEL_STEP_RATIO 0.333




using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;



#define DEFAULT_SLIDING_WINDOW_WIDTH 15
const char* windows[] = { "rectangle", "triangle", "cosine", "hann", "hamming", "lanczos", NULL };


void usage () {

AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

SYNOPSIS = "Perform the Track-Weighted Dynamic Functional Connectivity (TW-dFC) method.";

DESCRIPTION
  + "This command generates a sliding-window Track-Weighted Image (TWI), where "
    "the contribution from each streamline to the image at each timepoint is the "
    "Pearson correlation between the fMRI time series at the streamline endpoints, "
    "within a sliding temporal window centred at that timepoint.";

ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file_in()
  + Argument ("fmri",   "the pre-processed fMRI time series").type_image_in()
  + Argument ("output", "the output TW-dFC image").type_image_out();

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
      "Options are: " + join(voxel_statistics, ", ") + " (default: mean)")
    + Argument ("type").type_choice (voxel_statistics)

  + Option ("window_shape", "specify the shape of the sliding window weighting function. "
                            "Options are: rectangle, triangle, cosine, hann, hamming, lanczos (default = rectangle)")
    + Argument ("shape").type_choice (windows)

  + Option ("window_width", "set the full width of the sliding window (in volumes, not time) (must be an odd number) (default = " + str(DEFAULT_SLIDING_WINDOW_WIDTH) + ")")
    + Argument ("value").type_integer (3)

  + Option ("backtrack",
            "if no valid timeseries is found at the streamline endpoint, backtrack along "
            "the streamline trajectory until a valid timeseries is found")

  + Option ("upsample",
      "upsample the tracks by some ratio using Hermite interpolation before mappipng \n"
      "(if omitted, an appropriate ratio will be determined automatically)")
    + Argument ("factor").type_integer (1);

}





// This class is similar to Mapping::MapWriter, but doesn't write to a HDD file on close
// Instead, the one timepoint volume generated during this iteration is written
//   into the one large buffer that contains the entire TW-dFC time series
class Receiver
{

  public:
    Receiver (const Header& header, const vox_stat_t stat_vox) :
        buffer (Image<float>::scratch (header, "TW-dFC scratch buffer")),
        vox_stat (stat_vox)
    {
      if (vox_stat == V_MIN) {
        for (auto l = Loop(buffer) (buffer); l; ++l)
          buffer.value() = std::numeric_limits<float>::infinity();
      } else if (vox_stat == V_MAX) {
        for (auto l = Loop(buffer) (buffer); l; ++l)
          buffer.value() = -std::numeric_limits<float>::infinity();
      }
    }


    bool operator() (const Mapping::SetVoxel&);
    void scale_by_count (Image<uint32_t>&);
    void write (Image<float>&);


  private:
    Image<float> buffer;
    const vox_stat_t vox_stat;

};



bool Receiver::operator() (const Mapping::SetVoxel& in)
{
  const float factor = in.factor;
  for (const auto& i : in) {
    assign_pos_of (i, 0, 3).to (buffer);
    switch (vox_stat) {
      case V_SUM:  buffer.value() += factor; break;
      case V_MIN:  buffer.value() = std::min (float(buffer.value()), factor); break;
      case V_MAX:  buffer.value() = std::max (float(buffer.value()), factor); break;
      case V_MEAN: buffer.value() += factor; break;
      // Unlike Mapping::MapWriter, don't need to deal with counts here
    }
  }
  return true;
}

void Receiver::scale_by_count (Image<uint32_t>& counts)
{
  assert (dimensions_match (buffer, counts, 0, 3));
  for (auto l = Loop(buffer) (buffer, counts); l; ++l) {
    if (counts.value())
      buffer.value() /= float(counts.value());
    else
      buffer.value() = 0.0f;
  }
}

void Receiver::write (Image<float>& out)
{
  for (auto l = Loop(buffer) (buffer, out); l; ++l)
    out.value() = buffer.value();
}







// Separate class for generating TDI i.e. receive SetVoxel & write directly to counts
class Count_receiver
{
  public:
    Count_receiver (Image<uint32_t>& out) :
        v (out) { }
    bool operator() (const Mapping::SetVoxel& in) {
      for (const auto& i : in) {
        assign_pos_of (i, 0, 3).to (v);
        v.value() = v.value() + 1;
      }
      return true;
    }
  private:
    Image<uint32_t> v;
};





void run () {

  const std::string tck_path = argument[0];
  Tractography::Properties properties;
  {
    // Just get the properties for now; will re-instantiate the reader multiple times later
    // TODO Constructor for properties using the file path?
    Tractography::Reader<float> tck_file (tck_path, properties);
  }
  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);
  float step_size = 0.0;
  if (properties.find ("output_step_size") != properties.end())
    step_size = to<float> (properties["output_step_size"]);
  else
    step_size = to<float> (properties["step_size"]);

  Image<float> fmri_image (Image<float>::open (argument[1]).with_direct_io(3));

  vector<default_type> voxel_size;
  auto opt = get_options("vox");
  if (opt.size())
    voxel_size = parse_floats (opt[0][0]);

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    INFO ("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Header header;
  opt = get_options ("template");
  if (opt.size()) {
    header = Header::open (opt[0][0]);
    if (!voxel_size.empty())
      Mapping::oversample_header (header, voxel_size);
  } else {
    if (voxel_size.empty())
      throw Exception ("please specify either a template image using the -template option, or the desired voxel size using the -vox option");
    Mapping::generate_header (header, argument[0], voxel_size);
  }

  header.datatype() = DataType::Float32;
  header.datatype().set_byte_order_native();
  header.ndim() = 4;
  header.size(3) = fmri_image.size(3);
  add_line (header.keyval()["comments"], "TW-dFC image");

  size_t upsample_ratio;
  opt = get_options ("upsample");
  if (opt.size()) {
    upsample_ratio = opt[0][0];
    INFO ("track interpolation factor manually set to " + str(upsample_ratio));
  } else  {
    try {
      upsample_ratio = determine_upsample_ratio (header, properties, MAX_VOXEL_STEP_RATIO);
      INFO ("track interpolation factor automatically set to " + str(upsample_ratio));
    } catch (Exception& e) {
      e.push_back ("Try using -upsample option to explicitly set the streamline upsampling ratio;");
      e.push_back ("generally recommend a value of around (3 x step_size / voxel_size)");
      throw e;
    }
  }

  opt = get_options ("stat_vox");
  const vox_stat_t stat_vox = opt.size() ? vox_stat_t(int(opt[0][0])) : V_MEAN;

  // Generate the window filter
  const ssize_t window_width = get_option_value ("window_width", DEFAULT_SLIDING_WINDOW_WIDTH);
  if (!(window_width % 2))
    throw Exception ("Width of sliding time window must be an odd integer");
  const int window_shape = get_option_value ("window_shape", 0); // default = rectangular

  vector<float> window (window_width, 0.0);
  const ssize_t halfwidth = (window_width+1) / 2;
  const ssize_t centre = (window_width-1) / 2; // Element at centre of the window

  switch (window_shape) {

    case 0: // rectangular
      window.assign (window_width, 1.0);
      break;

    case 1: // triangle
      for (ssize_t i = 0; i != window_width; ++i)
        window[i] = 1.0 - (std::abs (i - centre) / default_type(halfwidth));
      break;

    case 2: // cosine
      for (ssize_t i = 0; i != window_width; ++i)
        window[i] = std::sin (i * Math::pi / default_type(window_width - 1));
      break;

    case 3: // hann
      for (ssize_t i = 0; i != window_width; ++i)
        window[i] = 0.5 * (1.0 - std::cos (2.0 * Math::pi * i / default_type(window_width - 1)));
      break;

    case 4: // hamming
      for (ssize_t i = 0; i != window_width; ++i)
        window[i] = 0.53836 - (0.46164 * std::cos (2.0 * Math::pi * i / default_type(window_width - 1)));
      break;

    case 5: // lanczos
      for (ssize_t i = 0; i != window_width; ++i) {
        const default_type v = 2.0 * Math::pi * std::abs (i - centre) / default_type(window_width - 1);
        window[i] = v ? std::max (0.0, (std::sin (v) / v)) : 1.0;
      }
      break;

    default:
      throw Exception ("Unsupported sliding window shape");
  }

  // TODO Reconsider pre-calculating & storing SetVoxel for each streamline
  // - Faster, but ups RAM requirements, may become prohibitive with super-resolution
  // TODO Add voxel-wise statistic V_MEAN_ABS - mean absolute value?

  Image<float> out_image (Image<float>::create (argument[2], header));
  Header H_3D (header);
  H_3D.ndim() = 3;

  Image<uint32_t> counts;
  if (stat_vox == V_MEAN) {
    counts = Image<uint32_t>::scratch (H_3D, "Track count scratch buffer");
    Tractography::Reader<float> tck_file (tck_path, properties);
    Mapping::TrackLoader loader (tck_file, num_tracks, "Calculating initial TDI... ");
    Mapping::TrackMapperBase mapper (H_3D);
    mapper.set_upsample_ratio (upsample_ratio);
    Count_receiver receiver (counts);
    Thread::run_queue (loader, Thread::batch (Tractography::Streamline<>()), Thread::multi (mapper), Thread::batch (Mapping::SetVoxel()), receiver);
  }

  ProgressBar progress ("Generating TW-dFC image", header.size(3));
  for (ssize_t timepoint = 0; timepoint != header.size(3); ++timepoint) {

    {
      LogLevelLatch latch (0);
      Tractography::Reader<float> tck_file (tck_path, properties);
      Mapping::TrackLoader loader (tck_file);
      Mapping::TrackMapperTWI mapper (H_3D, SCALAR_MAP, ENDS_CORR);
      mapper.set_upsample_ratio (upsample_ratio);
      mapper.add_twdfc_image (fmri_image, window, timepoint);
      Receiver receiver (H_3D, stat_vox);
      Thread::run_queue (loader, Thread::batch (Tractography::Streamline<>()), Thread::multi (mapper), Thread::batch (Mapping::SetVoxel()), receiver);

      if (stat_vox == V_MEAN)
        receiver.scale_by_count (counts);

      out_image.index(3) = timepoint;
      receiver.write (out_image);
    }

    ++progress;
  }

}


