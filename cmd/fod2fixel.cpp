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
#include "progressbar.h"

#include "image.h"

#include "fixel/keys.h"
#include "fixel/helpers.h"

#include "math/SH.h"

#include "thread_queue.h"

#include "dwi/fmls.h"
#include "dwi/directions/set.h"

#include "file/path.h"




using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;



const OptionGroup OutputOptions = OptionGroup ("Metric values for fixel-based sparse output images")

  + Option ("afd",
            "output the total Apparent Fibre Density per fixel (integral of FOD lobe)")
    + Argument ("image").type_image_out()

  + Option ("peak",
            "output the peak FOD amplitude per fixel")
    + Argument ("image").type_image_out()

  + Option ("disp",
            "output a measure of dispersion per fixel as the ratio between FOD lobe integral and peak amplitude")
    + Argument ("image").type_image_out();




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Perform segmentation of continuous Fibre Orientation Distributions (FODs) to produce discrete fixels";

  REFERENCES
    + "* Reference for the FOD segmentation method:\n"
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312 (Appendix 2)"

    + "* Reference for Apparent Fibre Density (AFD):\n"
    "Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A. " // Internal
    "Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images."
    "Neuroimage, 2012, 15;59(4), 3976-94.";

  ARGUMENTS
  + Argument ("fod", "the input fod image.").type_image_in ()
  + Argument ("fixel_directory", "the output fixel directory").type_directory_out();


  OPTIONS

  + OutputOptions

  + FMLSSegmentOption

  + OptionGroup ("Other options for fod2fixel")

  + Option ("mask",
                "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in()

  + Option ("maxnum", "maximum number of fixels to output for any particular voxel (default: no limit)")
    + Argument ("number").type_integer(1)

  + Option ("nii", "output the directions and index file in nii format (instead of the default mif)")

  + Option ("dirpeak", "define the fixel direction as the peak lobe direction as opposed to the lobe mean");

}



class Segmented_FOD_receiver { MEMALIGN(Segmented_FOD_receiver)

  public:
    Segmented_FOD_receiver (const Header& header, const uint32_t maxnum = 0, bool dir_as_peak = false) :
        H (header), fixel_count (0), max_per_voxel (maxnum), dir_as_peak (dir_as_peak) { }

    void commit ();

    void set_fixel_directory_output (const std::string& path) { fixel_directory_path = path; }
    void set_index_output (const std::string& path) { index_path = path; }
    void set_directions_output (const std::string& path) { dir_path = path; }
    void set_afd_output (const std::string& path) { afd_path = path; }
    void set_peak_output (const std::string& path) { peak_path = path; }
    void set_disp_output (const std::string& path) { disp_path = path; }

    bool operator() (const FOD_lobes&);


  private:

    struct Primitive_FOD_lobe { MEMALIGN (Primitive_FOD_lobe)
      Eigen::Vector3f dir;
      float integral;
      float peak_value;
      Primitive_FOD_lobe (Eigen::Vector3f dir, float integral, float peak_value) :
          dir (dir), integral (integral), peak_value (peak_value) {}
    };


    class Primitive_FOD_lobes : public vector<Primitive_FOD_lobe> { MEMALIGN (Primitive_FOD_lobes)
      public:
        Primitive_FOD_lobes (const FOD_lobes& in, const uint32_t maxcount, bool use_peak_dir) :
            vox (in.vox)
        {
          const uint32_t N = maxcount ? std::min (uint32_t(in.size()), maxcount) : in.size();
          for (uint32_t i = 0; i != N; ++i) {
            const FOD_lobe& lobe (in[i]);
            if (use_peak_dir)
              this->emplace_back (lobe.get_peak_dir(0).cast<float>(), lobe.get_integral(), lobe.get_max_peak_value());
            else
              this->emplace_back (lobe.get_mean_dir().cast<float>(), lobe.get_integral(), lobe.get_max_peak_value());
          }
        }
        Eigen::Array3i vox;
    };

    Header H;
    std::string fixel_directory_path, index_path, dir_path, afd_path, peak_path, disp_path;
    vector<Primitive_FOD_lobes> lobes;
    uint32_t fixel_count;
    uint32_t max_per_voxel;
    bool dir_as_peak;
};




bool Segmented_FOD_receiver::operator() (const FOD_lobes& in)
{
  if (in.size()) {
    lobes.emplace_back (in, max_per_voxel, dir_as_peak);
    fixel_count += lobes.back().size();
  }
  return true;
}



void Segmented_FOD_receiver::commit ()
{
  if (!lobes.size() || !fixel_count)
    return;

  using DataImage = Image<float>;
  using IndexImage = Image<uint32_t>;

  const auto index_filepath = Path::join (fixel_directory_path, index_path);

  std::unique_ptr<IndexImage> index_image;
  std::unique_ptr<DataImage> dir_image;
  std::unique_ptr<DataImage> afd_image;
  std::unique_ptr<DataImage> peak_image;
  std::unique_ptr<DataImage> disp_image;

  auto index_header (H);
  index_header.keyval()[Fixel::n_fixels_key] = str(fixel_count);
  index_header.ndim() = 4;
  index_header.size(3) = 2;
  index_header.datatype() = DataType::from<uint32_t>();
  index_header.datatype().set_byte_order_native();
  index_image = make_unique<IndexImage> (IndexImage::create (index_filepath, index_header));

  auto fixel_data_header (H);
  fixel_data_header.ndim() = 3;
  fixel_data_header.size(0) = fixel_count;
  fixel_data_header.size(2) = 1;
  fixel_data_header.datatype() = DataType::Float32;
  fixel_data_header.datatype().set_byte_order_native();

  if (dir_path.size()) {
    auto dir_header (fixel_data_header);
    dir_header.size(1) = 3;
    dir_image = make_unique<DataImage> (DataImage::create (Path::join(fixel_directory_path, dir_path), dir_header));
    dir_image->index(1) = 0;
    Fixel::check_fixel_size (*index_image, *dir_image);
  }

  if (afd_path.size()) {
    auto afd_header (fixel_data_header);
    afd_header.size(1) = 1;
    afd_image = make_unique<DataImage> (DataImage::create (Path::join(fixel_directory_path, afd_path), afd_header));
    afd_image->index(1) = 0;
    Fixel::check_fixel_size (*index_image, *afd_image);
  }

  if (peak_path.size()) {
    auto peak_header(fixel_data_header);
    peak_header.size(1) = 1;
    peak_image = make_unique<DataImage> (DataImage::create (Path::join(fixel_directory_path, peak_path), peak_header));
    peak_image->index(1) = 0;
    Fixel::check_fixel_size (*index_image, *peak_image);
  }

  if (disp_path.size()) {
    auto disp_header (fixel_data_header);
    disp_header.size(1) = 1;
    disp_image = make_unique<DataImage> (DataImage::create (Path::join(fixel_directory_path, disp_path), disp_header));
    disp_image->index(1) = 0;
    Fixel::check_fixel_size (*index_image, *disp_image);
  }

  size_t offset (0), lobe_index (0);


  for (const auto& vox_fixels : lobes) {
    size_t n_vox_fixels = vox_fixels.size();

    assign_pos_of (vox_fixels.vox).to (*index_image);

    index_image->index(3) = 0;
    index_image->value () = n_vox_fixels;

    index_image->index(3) = 1;
    index_image->value() = offset;

    if (dir_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        dir_image->index(0) = offset + i;
        dir_image->row(1) = vox_fixels[i].dir;
      }
    }

    if (afd_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        afd_image->index(0) = offset + i;
        afd_image->value() = vox_fixels[i].integral;
      }
    }

    if (peak_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        peak_image->index(0) = offset + i;
        peak_image->value() = vox_fixels[i].peak_value;
      }
    }

    if (disp_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        disp_image->index(0) = offset + i;
        disp_image->value() = vox_fixels[i].integral / vox_fixels[i].peak_value;
      }
    }

    offset += n_vox_fixels;
    lobe_index ++;
  }

  assert (offset == fixel_count);
}




void run ()
{
  Header H = Header::open (argument[0]);
  Math::SH::check (H);
  auto fod_data = H.get_image<float>();

  const bool dir_as_peak = get_options ("dirpeak").size();
  const uint32_t maxnum = get_option_value ("maxnum", 0);

  Segmented_FOD_receiver receiver (H, maxnum, dir_as_peak);

  auto& fixel_directory_path  = argument[1];
  receiver.set_fixel_directory_output (fixel_directory_path);

  std::string file_extension (".mif");
  if (get_options ("nii").size())
    file_extension = ".nii";

  static const std::string default_index_filename ("index" + file_extension);
  static const std::string default_directions_filename ("directions" + file_extension);
  receiver.set_index_output (default_index_filename);
  receiver.set_directions_output (default_directions_filename);

  auto
  opt = get_options ("afd");  if (opt.size()) receiver.set_afd_output  (opt[0][0]);
  opt = get_options ("peak"); if (opt.size()) receiver.set_peak_output (opt[0][0]);
  opt = get_options ("disp"); if (opt.size()) receiver.set_disp_output (opt[0][0]);

  opt = get_options ("mask");
  Image<float> mask;
  if (opt.size()) {
    mask = Image<float>::open (std::string (opt[0][0]));
    if (!dimensions_match (fod_data, mask, 0, 3))
      throw Exception ("Cannot use image \"" + str(opt[0][0]) + "\" as mask image; dimensions do not match FOD image");
  }

  Fixel::check_fixel_directory (fixel_directory_path, true, true);

  FMLS::FODQueueWriter writer (fod_data, mask);

  const DWI::Directions::FastLookupSet dirs (1281);
  Segmenter fmls (dirs, Math::SH::LforN (H.size(3)));
  load_fmls_thresholds (fmls);

  Thread::run_queue (writer, Thread::batch (SH_coefs()), Thread::multi (fmls), Thread::batch (FOD_lobes()), receiver);
  receiver.commit ();
}

