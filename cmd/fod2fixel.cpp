/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "progressbar.h"

#include "image.h"

#include "fixel_format/keys.h"
#include "fixel_format/helpers.h"

#include "math/SH.h"

#include "thread_queue.h"

#include "dwi/fmls.h"
#include "dwi/directions/set.h"

#include "gui/dialog/progress.h"
#include "file/path.h"




using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;



const OptionGroup OutputOptions = OptionGroup ("Metric values for fixel-based sparse output images")

  + Option ("index",
            "store the index file")
    + Argument ("image").type_image_out()

  + Option ("dir",
            "store the fixel directions")
    + Argument ("image").type_image_out()

  + Option ("afd",
            "store the total Apparent Fibre Density per fixel (integral of FOD lobe)")
    + Argument ("image").type_image_out()

  + Option ("peak",
            "store the peak FOD amplitude per fixel")
    + Argument ("image").type_image_out()

  + Option ("disp",
            "store a measure of dispersion per fixel as the ratio between FOD lobe integral and peak amplitude")
    + Argument ("image").type_image_out();




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "use a fast-marching level-set method to segment fibre orientation distributions, and save parameters of interest as fixel images";

  REFERENCES 
    + "* Reference for the FOD segmentation method:\n"
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312 (Appendix 2)"

    + "* Reference for Apparent Fibre Density:\n"
    "Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A. " // Internal
    "Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images."
    "Neuroimage, 2012, 15;59(4), 3976-94.";

  ARGUMENTS
  + Argument ("fod", "the input fod image.").type_image_in ()
  + Argument ("fixel_folder", "the output fixel folder").type_text();


  OPTIONS

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in()

  + OutputOptions

  + FMLSSegmentOption;

}







class Segmented_FOD_receiver
{

  public:
    Segmented_FOD_receiver (const Header& header) :
        H (header), n_fixels (0), output_index_file (true)
    {
    }

    void commit ();

    void set_output_index_file (bool flag) { output_index_file = flag; }
    void set_fixel_folder_output (const std::string& path) { fixel_folder_path = path; }
    void set_index_output (const std::string& path) { index_path = path; }
    std::string get_index_output () { return index_path; }
    void set_directions_output (const std::string& path) { dir_path = path; }
    void set_afd_output  (const std::string& path) { afd_path = path; }
    void set_peak_output (const std::string& path) { peak_path = path; }
    void set_disp_output (const std::string& path) { disp_path = path; }

    size_t num_outputs() const;

    bool operator() (const FOD_lobes&);


  private:
    template<typename UIntType> void primitive_commit ();

    Header H;
    std::string fixel_folder_path, index_path, dir_path, afd_path, peak_path, disp_path;
    std::vector<FOD_lobes> lobes;
    uint64_t n_fixels;
    bool output_index_file;
};



size_t Segmented_FOD_receiver::num_outputs() const
{
  size_t count = output_index_file ? 1 : 0;
  if (dir_path.size()) ++count;
  if (afd_path.size())  ++count;
  if (peak_path.size()) ++count;
  if (disp_path.size()) ++count;
  return count;
}




bool Segmented_FOD_receiver::operator() (const FOD_lobes& in)
{

  if (size_t n = in.size()) {
    lobes.emplace_back (in);
    n_fixels += n;
  }

  return true;
}



void Segmented_FOD_receiver::commit ()
{
  if (!lobes.size() || !n_fixels || !num_outputs())
    return;

  if (n_fixels < (uint64_t)std::numeric_limits<uint32_t>::max ())
    primitive_commit<uint32_t> ();
  else
    primitive_commit<uint64_t> ();
}



template<typename UIntType>
void Segmented_FOD_receiver::primitive_commit ()
{
  using DataImage = Image<float>;
  using IndexImage = Image<UIntType>;

  const auto index_filepath = Path::join (fixel_folder_path, index_path);

  std::unique_ptr<Image<UIntType>> index_image (nullptr);
  std::unique_ptr<DataImage> dir_image (nullptr);
  std::unique_ptr<DataImage> afd_image (nullptr);
  std::unique_ptr<DataImage> peak_image (nullptr);
  std::unique_ptr<DataImage> disp_image (nullptr);


  if (output_index_file) {
    auto index_header (H);
    index_header.keyval()[FixelFormat::n_fixels_key] = str(n_fixels);
    index_header.ndim () = 4;
    index_header.size (3) = 2;
    index_header.datatype () = DataType::from<UIntType> ();
    index_header.datatype ().set_byte_order_native ();
    index_image = std::unique_ptr<IndexImage> (new IndexImage (IndexImage::create (index_filepath, index_header)));
  } else {
    index_image = std::unique_ptr<IndexImage> (new IndexImage (IndexImage::open (index_filepath)));
  }

  auto fixel_data_header (H);
  fixel_data_header.ndim () = 3;
  fixel_data_header.size (0) = n_fixels;
  fixel_data_header.size (2) = 1;
  fixel_data_header.datatype () = DataType::Float32;
  fixel_data_header.datatype ().set_byte_order_native();

  if (dir_path.size ()) {
    auto dir_header (fixel_data_header);
    dir_header.size (1) = 3;
    dir_image = std::unique_ptr<DataImage> (new DataImage (DataImage::create (Path::join(fixel_folder_path, dir_path), dir_header)));
    dir_image->index (1) = 0;
    FixelFormat::check_fixel_size (*index_image, *dir_image);
  }

  if (afd_path.size ()) {
    auto afd_header (fixel_data_header);
    afd_header.size (1) = 1;
    afd_image = std::unique_ptr<DataImage> (new DataImage (DataImage::create (Path::join(fixel_folder_path, afd_path), afd_header)));
    afd_image->index (1) = 0;
    FixelFormat::check_fixel_size (*index_image, *afd_image);
  }

  if (peak_path.size ()) {
    auto peak_header (fixel_data_header);
    peak_header.size (1) = 1;
    peak_image = std::unique_ptr<DataImage> (new DataImage (DataImage::create (Path::join(fixel_folder_path, peak_path), peak_header)));
    peak_image->index (1) = 0;
    FixelFormat::check_fixel_size (*index_image, *peak_image);
  }

  if (disp_path.size ()) {
    auto disp_header (fixel_data_header);
    disp_header.size (1) = 1;
    disp_image = std::unique_ptr<DataImage> (new DataImage (DataImage::create (Path::join(fixel_folder_path, disp_path), disp_header)));
    disp_image->index (1) = 0;
    FixelFormat::check_fixel_size (*index_image, *disp_image);
  }

  size_t offset (0), lobe_index (0);
  const size_t nlobes (lobes.size ());

  MR::ProgressBar progress("Generating fixel files", nlobes);
  auto display_func = [&lobe_index, nlobes, &offset]() {
    return str (lobe_index) + " of " + str (nlobes) + " FOD lobes read. " + str (offset) + " fixels found.";
  };

  for (const auto& vox_fixels : lobes) {
    size_t n_vox_fixels = vox_fixels.size();

    if (output_index_file) {
      assign_pos_of (vox_fixels.vox).to (*index_image);

      index_image->index (3) = 0;
      index_image->value () = n_vox_fixels;

      index_image->index (3) = 1;
      index_image->value () = offset;
    }

    if (dir_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        dir_image->index (0) = offset + i;
        dir_image->row (1) = vox_fixels[i].get_mean_dir ();
      }
    }

    if (afd_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        afd_image->index (0) = offset + i;
        afd_image->value () = vox_fixels[i].get_integral ();
      }
    }

    if (peak_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        peak_image->index (0) = offset + i;
        peak_image->value () = vox_fixels[i].get_peak_value ();
      }
    }

    if (disp_image) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        disp_image->index (0) = offset + i;
        disp_image->value () = vox_fixels[i].get_integral () / vox_fixels[i].get_peak_value ();
      }
    }


    offset += n_vox_fixels;
    lobe_index ++;

    progress.update (display_func);
  }

  progress.done ();

  assert (offset == n_fixels);
}




void run ()
{
  Header H = Header::open (argument[0]);
  Math::SH::check (H);
  auto fod_data = H.get_image<float>();

  Segmented_FOD_receiver receiver (H);

  auto& fixel_folder_path  = argument[1];
  receiver.set_fixel_folder_output (fixel_folder_path);

  static const std::string default_index_filename = "index.mif";
  receiver.set_index_output (default_index_filename);

  auto
  opt = get_options ("index"); if (opt.size()) receiver.set_index_output (opt[0][0]);
  opt = get_options ("dir"); if (opt.size()) receiver.set_directions_output (opt[0][0]);
  opt = get_options ("afd");  if (opt.size()) receiver.set_afd_output (opt[0][0]);
  opt = get_options ("peak"); if (opt.size()) receiver.set_peak_output (opt[0][0]);
  opt = get_options ("disp"); if (opt.size()) receiver.set_disp_output (opt[0][0]);

  opt = get_options ("mask");
  Image<float> mask;
  if (opt.size()) {
    mask = Image<float>::open (std::string (opt[0][0]));
    if (!dimensions_match (fod_data, mask, 0, 3))
      throw Exception ("Cannot use image \"" + str(opt[0][0]) + "\" as mask image; dimensions do not match FOD image");
  }


  if (!Path::exists (fixel_folder_path))
    File::mkdir (fixel_folder_path);
  else if (!Path::is_dir (fixel_folder_path))
    throw Exception (str(fixel_folder_path) + " is not a directory");
  else if (Path::exists (Path::join (fixel_folder_path, receiver.get_index_output ()))) {
    receiver.set_output_index_file (false);

    if (!receiver.num_outputs ())
      throw Exception ("Nothing to do; please specify at least one output image type");
  }


  FMLS::FODQueueWriter writer (fod_data, mask);

  const DWI::Directions::Set dirs (1281);
  Segmenter fmls (dirs, Math::SH::LforN (H.size(3)));
  load_fmls_thresholds (fmls);

  Thread::run_queue (writer, Thread::batch (SH_coefs()), Thread::multi (fmls), Thread::batch (FOD_lobes()), receiver);

  receiver.commit ();
}

