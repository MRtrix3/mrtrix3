/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "progressbar.h"

#include "image.h"

#include "fixel/fixel.h"
#include "fixel/helpers.h"

#include "math/SH.h"
#include "math/sphere.h"

#include "thread_queue.h"

#include "dwi/fmls.h"
#include "dwi/directions/set.h"

#include "file/path.h"


#define DEFAULT_DIRECTION_SET 1281


const char* fixel_direction_choices[] { "mean", "peak", "lsq", nullptr };
enum class fixel_dir_t { MEAN, PEAK, LSQ };


using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::FMLS;
using namespace App;

using Fixel::index_type;



const OptionGroup MetricOptions = OptionGroup ("Quantitative fixel-wise metric values to save as fixel data files")

  + Option ("afd",
            "output the total Apparent Fibre Density per fixel (integral of FOD lobe)")
    + Argument ("image").type_image_out()

  + Option ("peak_amp",
            "output the amplitude of the FOD at the maximal peak per fixel")
    + Argument ("image").type_image_out()

  + Option ("disp",
            "output a measure of dispersion per fixel as the ratio between FOD lobe integral and maximal peak amplitude")
    + Argument ("image").type_image_out()

  + Option ("lut",
            "output the voxel-wise directions lookup table from FMLS process")
    + Argument ("image").type_image_out();



const OptionGroup InputOptions = OptionGroup ("Input options for fod2fixel")

  + Option ("mask", "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in();

  // TODO Add option to specify a different direction set for segmentation



const OptionGroup OutputOptions = OptionGroup ("Options to modulate outputs of fod2fixel")

  + Option ("maxnum", "maximum number of fixels to output for any particular voxel (default: no limit)")
    + Argument ("number").type_integer(1)

  + Option ("nii", "output the directions and index file in NIfTI format (instead of the default .mif)")

  + Option ("fixel_dirs", "choose what will be used to define the direction of each fixel; "
                          "options are: " + join(fixel_direction_choices, ","))
    + Argument ("choice").type_choice (fixel_direction_choices)

  + Option ("nolookup", "do not export the lookup table image");



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Perform segmentation of continuous Fibre Orientation Distributions (FODs) to produce discrete fixels";

  DESCRIPTION
  + "The fixel directions can be determined in one of three ways. "
    "By default, the direction is calculated as the weighted mean of all amplitude samples "
    "that contributed to the formation of that fixel. "
    "One alternative is to instead use the direction of the maximal peak amplitude of the lobe "
    "using \"-fixel-dirs peak\". "
    "Another is to use a least-squares solution for the spherical average of the lobe "
    "(though this comes at a slight computational cost, and is typically not substantially "
    "different to the default mean direction) using \"-fixel_dirs lsq\"."

  + Fixel::format_description;

  REFERENCES
    + "* Reference for the FOD segmentation method:\n"
    "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312 (Appendix 2)"

    + "* Reference for Apparent Fibre Density (AFD):\n"
    "Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A. " // Internal
    "Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images."
    "Neuroimage, 2012, 15;59(4), 3976-94";

  ARGUMENTS
  + Argument ("fod", "the input fod image.").type_image_in ()
  + Argument ("fixel_directory", "the output fixel directory").type_directory_out();


  OPTIONS

  + MetricOptions

  + FMLSSegmentOption

  + InputOptions

  + OutputOptions;

}



class Segmented_FOD_receiver {

  public:
    Segmented_FOD_receiver (const Header& header,
                            const DWI::Directions::FastLookupSet& dirs,
                            const std::string& directory,
                            const fixel_dir_t fixel_directions,
                            bool save_as_nifti = false,
                            bool write_lookup = true) :
        H (header),
        dirs (dirs),
        fixel_directory_path (directory),
        fixel_directions (fixel_directions),
        save_as_nifti (save_as_nifti),
        write_lookup (write_lookup),
        fixel_count (0) { }

    void commit ();

    void set_fixel_directory_output (const std::string& path) { fixel_directory_path = path; }
    void set_afd_output (const std::string& path) { afd_path = path; }
    void set_peak_amp_output (const std::string& path) { peak_amp_path = path; }
    void set_disp_output (const std::string& path) { disp_path = path; }

    bool operator() (const FOD_lobes&);


  private:

    struct Primitive_FOD_lobe {
      Eigen::Vector3f dir;
      float integral;
      float max_peak_amp;
      Primitive_FOD_lobe (Eigen::Vector3f dir, float integral, float max_peak_amp) :
          dir (dir), integral (integral), max_peak_amp (max_peak_amp) {}
    };


    class Primitive_FOD_lobes : public vector<Primitive_FOD_lobe> {
      public:
        Primitive_FOD_lobes (const FOD_lobes& in, const fixel_dir_t fixel_directions) :
            vox (in.vox),
            lut (in.lut)
        {
          Eigen::Vector3f dir;
          for (const auto& lobe : in) {
            switch (fixel_directions) {
              case fixel_dir_t::MEAN: dir = lobe.get_mean_dir() .cast<float>(); break;
              case fixel_dir_t::PEAK: dir = lobe.get_peak_dir(0).cast<float>(); break;
              case fixel_dir_t::LSQ:  dir = lobe.get_lsq_dir () .cast<float>(); break;
            }
            this->emplace_back (dir, lobe.get_integral(), lobe.get_max_peak_value());
          }
        }
        Eigen::Array3i vox;
        FMLS::lookup_type lut;
    };

    Header H;
    const DWI::Directions::FastLookupSet& dirs;
    std::string fixel_directory_path, index_path, dir_path, lookup_path;
    std::string afd_path, peak_amp_path, disp_path;
    fixel_dir_t fixel_directions;
    bool save_as_nifti, write_lookup;

    vector<Primitive_FOD_lobes> lobes;
    index_type fixel_count;
};




bool Segmented_FOD_receiver::operator() (const FOD_lobes& in)
{
  assert (in.lut.size() == dirs.size());
  if (in.size()) {
    lobes.emplace_back (in, fixel_directions);
    fixel_count += lobes.back().size();
  }
  return true;
}



void Segmented_FOD_receiver::commit ()
{
  if (!lobes.size() || !fixel_count)
    return;

  using DataImage = Image<float>;
  using IndexImage = Image<index_type>;
  using LookupImage = Image<uint8_t>;

  const auto index_filepath = Path::join (fixel_directory_path, index_path);
  const std::string base_extension (save_as_nifti ? ".nii" : ".mif");

  IndexImage index_image;
  DataImage dir_image;
  LookupImage lookup_image;
  DataImage afd_image;
  DataImage peak_amp_image;
  DataImage disp_image;

  auto index_header (H);
  index_header.keyval()[Fixel::n_fixels_key] = str(fixel_count);
  index_header.ndim() = 4;
  index_header.size(3) = 2;
  index_header.datatype() = DataType::from<index_type>();
  index_header.datatype().set_byte_order_native();
  index_image = IndexImage::create (Path::join(fixel_directory_path, Fixel::basename_index + base_extension), index_header);

  auto fixel_data_header (H);
  fixel_data_header.ndim() = 3;
  fixel_data_header.size(0) = fixel_count;
  fixel_data_header.size(2) = 1;
  fixel_data_header.transform().setIdentity();
  fixel_data_header.spacing(0) = fixel_data_header.spacing(1) = fixel_data_header.spacing(2) = 1.0;
  fixel_data_header.datatype() = DataType::Float32;
  fixel_data_header.datatype().set_byte_order_native();

  auto dir_header (fixel_data_header);
  dir_header.size(1) = 3;
  dir_image = DataImage::create (Path::join(fixel_directory_path, Fixel::basename_directions + base_extension), dir_header);
  dir_image.index(1) = 0;

  if (write_lookup) {
    auto lookup_header (index_header);
    lookup_header.size(3) = dirs.size();
    lookup_header.datatype() = DataType::UInt8;
    lookup_header.stride(0) = 0;
    // TODO Option to write directions as spherical vs cartesian
    Eigen::Matrix<float, Eigen::Dynamic, 3> directions_matrix = Eigen::Matrix<float, Eigen::Dynamic, 3>::Zero (dirs.size(), 3);
    for (size_t i = 0; i != dirs.size(); ++i)
      directions_matrix.row(i) = dirs[i].cast<float>();
    if (save_as_nifti) {
      MR::save_matrix (directions_matrix, Path::join (fixel_directory_path, Fixel::basename_lookup + ".csv"));
    } else {
      Eigen::IOFormat format (Eigen::FullPrecision, Eigen::DontAlignCols, ",", "\n", "", "", "", "");
      std::stringstream s;
      s << std::fixed << directions_matrix.format (format);
      lookup_header.keyval()["directions"] = s.str();
    }
    lookup_image = LookupImage::create (Path::join (fixel_directory_path, Fixel::basename_lookup + base_extension), lookup_header);
  }

  if (afd_path.size()) {
    auto afd_header (fixel_data_header);
    afd_header.size(1) = 1;
    afd_image = DataImage::create (Path::join(fixel_directory_path, afd_path), afd_header);
    afd_image.index(1) = 0;
  }

  if (peak_amp_path.size()) {
    auto peak_amp_header (fixel_data_header);
    peak_amp_header.size(1) = 1;
    peak_amp_image = DataImage::create (Path::join(fixel_directory_path, peak_amp_path), peak_amp_header);
    peak_amp_image.index(1) = 0;
  }

  if (disp_path.size()) {
    auto disp_header (fixel_data_header);
    disp_header.size(1) = 1;
    disp_image = DataImage::create (Path::join(fixel_directory_path, disp_path), disp_header);
    disp_image.index(1) = 0;
  }

  size_t offset = 0;
  for (const auto& vox_fixels : lobes) {
    size_t n_vox_fixels = vox_fixels.size();

    assign_pos_of (vox_fixels.vox).to (index_image);
    index_image.index(3) = 0;
    index_image.value() = n_vox_fixels;
    index_image.index(3) = 1;
    index_image.value() = offset;

    for (size_t i = 0; i < n_vox_fixels; ++i) {
      dir_image.index(0) = offset + i;
      dir_image.row(1) = vox_fixels[i].dir;
    }

    if (lookup_image.valid()) {
      assign_pos_of (vox_fixels.vox).to (lookup_image);
      lookup_image.row(3) = vox_fixels.lut.matrix();
    }

    if (afd_image.valid()) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        afd_image.index(0) = offset + i;
        afd_image.value() = vox_fixels[i].integral;
      }
    }

    if (peak_amp_image.valid()) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        peak_amp_image.index(0) = offset + i;
        peak_amp_image.value() = vox_fixels[i].max_peak_amp;
      }
    }

    if (disp_image.valid()) {
      for (size_t i = 0; i < n_vox_fixels; ++i) {
        disp_image.index(0) = offset + i;
        disp_image.value() = vox_fixels[i].integral / vox_fixels[i].max_peak_amp;
      }
    }

    offset += n_vox_fixels;
  }

  assert (offset == fixel_count);
}




void run ()
{
  Header H = Header::open (argument[0]);
  Math::SH::check (H);
  auto fod_data = H.get_image<float>();

  const std::string fixel_directory_path = argument[1];
  Fixel::check_fixel_directory (fixel_directory_path, true, true);

  const DWI::Directions::FastLookupSet dirs (DEFAULT_DIRECTION_SET);

  auto opt = get_options ("fixel_dirs");
  fixel_dir_t fixel_directions = fixel_dir_t::MEAN;
  switch (int(opt[0][0])) {
    case 0: fixel_directions = fixel_dir_t::MEAN; break;
    case 1: fixel_directions = fixel_dir_t::PEAK; break;
    case 2: fixel_directions = fixel_dir_t::LSQ;  break;
    default: assert (false);
  }

  Segmented_FOD_receiver receiver (H,
                                   dirs,
                                   fixel_directory_path,
                                   fixel_directions,
                                   get_options("nii").size(),
                                   !get_options("nolookup").size());

  opt = get_options ("afd");      if (opt.size()) receiver.set_afd_output      (opt[0][0]);
  opt = get_options ("peak_amp"); if (opt.size()) receiver.set_peak_amp_output (opt[0][0]);
  opt = get_options ("disp");     if (opt.size()) receiver.set_disp_output     (opt[0][0]);

  opt = get_options ("mask");
  Image<float> mask;
  if (opt.size()) {
    mask = Image<float>::open (std::string (opt[0][0]));
    if (!dimensions_match (fod_data, mask, 0, 3))
      throw Exception ("Cannot use image \"" + str(opt[0][0]) + "\" as mask image; dimensions do not match FOD image");
  }
  FMLS::FODQueueWriter writer (fod_data, mask);

  Segmenter fmls (dirs, Math::SH::LforN (H.size(3)));
  load_fmls_thresholds (fmls);
  fmls.set_max_num_fixels (get_option_value ("maxnum", 0));
  fmls.set_calculate_lsq_dir (fixel_directions == fixel_dir_t::LSQ);

  Thread::run_queue (writer, Thread::batch (SH_coefs()), Thread::multi (fmls), Thread::batch (FOD_lobes()), receiver);
  receiver.commit ();
}
