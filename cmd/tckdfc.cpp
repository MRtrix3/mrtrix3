/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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



const char* windows[] = { "rectangle", "triangle", "cosine", "hann", "hamming", "lanczos", nullptr };


void usage () {

AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

SYNOPSIS = "Perform the Track-Weighted Dynamic Functional Connectivity (TW-dFC) method";

DESCRIPTION
  + "This command generates a Track-Weighted Image (TWI), where the "
    "contribution from each streamline to the image is the Pearson "
    "correlation between the fMRI time series at the streamline endpoints."

  + "The output image can be generated in one of two ways "
    "(note that one of these two command-line options MUST be provided): "

  + "- \"Static\" functional connectivity (-static option): "
    "Each streamline contributes to a static 3D output image based on the "
    "correlation between the signals at the streamline endpoints using the "
    "entirety of the input time series."

  + "- \"Dynamic\" functional connectivity (-dynamic option): "
    "The output image is a 4D image, with the same number of volumes as "
    "the input fMRI time series. For each volume, the contribution from "
    "each streamline is calculated based on a finite-width sliding time "
    "window, centred at the timepoint corresponding to that volume."

  + "Note that the -backtrack option in this command is similar, but not precisely "
    "equivalent, to back-tracking as can be used with Anatomically-Constrained "
    "Tractography (ACT) in the tckgen command. However, here the feature does not "
    "change the streamlines trajectories in any way; it simply enables detection of "
    "the fact that the input fMRI image may not contain a valid timeseries underneath "
    "the streamline endpoint, and where this occurs, searches from the streamline "
    "endpoint inwards along the streamline trajectory in search of a valid "
    "timeseries to sample from the input image.";

ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file_in()
  + Argument ("fmri",   "the pre-processed fMRI time series").type_image_in()
  + Argument ("output", "the output TW-dFC image").type_image_out();

OPTIONS
  + OptionGroup ("Options for toggling between static and dynamic TW-dFC methods; "
                 "note that one of these options MUST be provided")

  + Option ("static", "generate a \"static\" (3D) output image.")

  + Option ("dynamic", "generate a \"dynamic\" (4D) output image; "
                       "must additionally provide the shape and width (in volumes) of the sliding window.")
    + Argument ("shape").type_choice (windows)
    + Argument ("width").type_integer (3)


  + OptionGroup ("Options for setting the properties of the output image")

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


  + OptionGroup ("Other options for affecting the streamline sampling & mapping behaviour")

  + Option ("backtrack",
            "if no valid timeseries is found at the streamline endpoint, back-track along "
            "the streamline trajectory until a valid timeseries is found")

  + Option ("upsample",
      "upsample the tracks by some ratio using Hermite interpolation before mappipng \n"
      "(if omitted, an appropriate ratio will be determined automatically)")
    + Argument ("factor").type_integer (1);


  REFERENCES
  + "Calamante, F.; Smith, R.E.; Liang, X.; Zalesky, A.; Connelly, A " // Internal
    "Track-weighted dynamic functional connectivity (TW-dFC): a new method to study time-resolved functional connectivity. "
    "Brain Struct Funct, 2017, doi: 10.1007/s00429-017-1431-1";

}





// This class is similar to Mapping::MapWriter, but doesn't write to a HDD file on close
// Instead, the one timepoint volume generated during this iteration is written
//   into the one large buffer that contains the entire TW-dFC time series
class Receiver
{ MEMALIGN(Receiver)

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
{ MEMALIGN(Count_receiver)
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





void run ()
{
  bool is_static = get_options ("static").size();
  vector<float> window;

  auto opt = get_options ("dynamic");
  if (opt.size()) {
    if (is_static)
      throw Exception ("Do not specify both -static and -dynamic options");

    // Generate the window filter
    const int window_shape = opt[0][0];
    const ssize_t window_width = opt[0][1];
    if (!(window_width % 2))
      throw Exception ("Width of sliding time window must be an odd integer");

    window.resize (window_width);
    const ssize_t halfwidth = (window_width+1) / 2;
    const ssize_t centre = (window_width-1) / 2; // Element at centre of the window

    switch (window_shape) {

      case 0: // rectangular
        window.assign (window_width, 1.0);
        break;

      case 1: // triangle
        for (ssize_t i = 0; i != window_width; ++i)
          window[i] = 1.0 - (abs (i - centre) / default_type(halfwidth));
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
          const default_type v = 2.0 * Math::pi * abs (i - centre) / default_type(window_width - 1);
          window[i] = v ? std::max (0.0, (std::sin (v) / v)) : 1.0;
        }
        break;

      default:
        throw Exception ("Unsupported sliding window shape");
    }

  } else if (!is_static) {
    throw Exception ("Either the -static or -dynamic option must be provided");
  }

  const std::string tck_path = argument[0];
  Tractography::Properties properties;
  {
    // Just get the properties for now; will re-instantiate the reader multiple times later
    // TODO Constructor for properties using the file path?
    Tractography::Reader<float> tck_file (tck_path, properties);
  }
  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);

  Image<float> fmri_image (Image<float>::open (argument[1]).with_direct_io(3));

  vector<default_type> voxel_size;
  opt = get_options("vox");
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
  if (is_static) {
    header.ndim() = 3;
  } else {
    header.ndim() = 4;
    header.size(3) = fmri_image.size(3);
  }
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

  Header H_3D (header);
  H_3D.ndim() = 3;

  if (is_static) {

    Tractography::Reader<float> tck_file (tck_path, properties);
    Mapping::TrackLoader loader (tck_file, num_tracks, "Generating (static) TW-dFC image");
    Mapping::TrackMapperTWI mapper (H_3D, SCALAR_MAP, ENDS_CORR);
    mapper.set_upsample_ratio (upsample_ratio);
    mapper.add_twdfc_static_image (fmri_image);
    Mapping::MapWriter<float> writer (header, argument[2], stat_vox);
    Thread::run_queue (loader, Thread::batch (Tractography::Streamline<>()), Thread::multi (mapper), Thread::batch (Mapping::SetVoxel()), writer);

  } else {

    Image<uint32_t> counts;
    if (stat_vox == V_MEAN) {
      counts = Image<uint32_t>::scratch (H_3D, "Track count scratch buffer");
      Tractography::Reader<float> tck_file (tck_path, properties);
      Mapping::TrackLoader loader (tck_file, num_tracks, "Calculating initial TDI");
      Mapping::TrackMapperBase mapper (H_3D);
      mapper.set_upsample_ratio (upsample_ratio);
      Count_receiver receiver (counts);
      Thread::run_queue (loader, Thread::batch (Tractography::Streamline<>()), Thread::multi (mapper), Thread::batch (Mapping::SetVoxel()), receiver);
    }

    Image<float> out_image (Image<float>::create (argument[2], header));
    ProgressBar progress ("Generating TW-dFC image", header.size(3));
    for (ssize_t timepoint = 0; timepoint != header.size(3); ++timepoint) {

      {
        LogLevelLatch latch (0);
        Tractography::Reader<float> tck_file (tck_path, properties);
        Mapping::TrackLoader loader (tck_file);
        Mapping::TrackMapperTWI mapper (H_3D, SCALAR_MAP, ENDS_CORR);
        mapper.set_upsample_ratio (upsample_ratio);
        mapper.add_twdfc_dynamic_image (fmri_image, window, timepoint);
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
}


