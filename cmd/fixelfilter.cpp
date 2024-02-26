/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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
#include "file/path.h"
#include "file/utils.h"
#include "fixel/fixel.h"
#include "fixel/helpers.h"
#include "header.h"
#include "image.h"
#include "progressbar.h"

#include "fixel/filter/base.h"
#include "fixel/filter/connect.h"
#include "fixel/filter/smooth.h"
#include "fixel/matrix.h"
#include "stats/cfe.h"

using namespace MR;
using namespace App;
using namespace MR::Fixel;

const char *const filters[] = {"connect", "smooth", nullptr};

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Perform filtering operations on fixel-based data";

  DESCRIPTION
  + "If the first input to the command is a specific fixel data file,"
    " then a filtered version of only that file will be generated by the command."
    " Alternatively, if the input is the location of a fixel directory,"
    " then the command will create a duplicate of the fixel directory,"
    " and apply the specified filter operation to all fixel data files within the directory."

  + Fixel::format_description;

  ARGUMENTS
  + Argument ("input", "the input: either a fixel data file, or a fixel directory (see Description)").type_various()
  + Argument ("filter", "the filtering operation to perform;"
                        " options are: " + join (filters, ", ")).type_choice (filters)
  + Argument ("output", "the output: either a fixel data file, or a fixel directory (see Description)").type_various();

  OPTIONS
  + Option ("matrix", "provide a fixel-fixel connectivity matrix"
                      " for filtering operations that require it").required()
    + Argument ("file").type_directory_in()

  + OptionGroup ("Options specific to the \"connect\" filter")
  + Option ("threshold_value", "specify a threshold for the input fixel data file values"
                               " (default = " + str(DEFAULT_FIXEL_CONNECT_VALUE_THRESHOLD) + ")")
    + Argument ("value").type_float ()
  + Option ("threshold_connectivity", "specify a fixel-fixel connectivity threshold for connected-component analysis"
                                      " (default = " + str(DEFAULT_FIXEL_CONNECT_CONNECTIVITY_THRESHOLD) + ")")
    + Argument ("value").type_float (0.0)

  + OptionGroup ("Options specific to the \"smooth\" filter")
  + Option ("fwhm", "the full-width half-maximum (FWHM) of the spatial component of the smoothing filter"
                    " (default = " + str(DEFAULT_FIXEL_SMOOTHING_FWHM) + "mm)")
    + Argument ("value").type_float (0.0)
  + Option ("minweight", "apply a minimum threshold to smoothing weights"
                         " (default = " + str(DEFAULT_FIXEL_SMOOTHING_MINWEIGHT) + ")")
    + Argument ("value").type_float (0.0)
  + Option ("mask", "only perform smoothing within a specified binary fixel mask")
    + Argument ("image").type_image_in();

}
// clang-format on

using value_type = float;

void run() {

  std::set<std::string> option_list{"threhsold_value", "threshold_connectivity", "fwhm", "minweight", "mask"};

  Image<float> single_file;
  std::vector<Header> multiple_files;
  std::unique_ptr<Fixel::Filter::Base> filter;

  {
    Header index_header;
    Header output_header;
    try {
      index_header = Fixel::find_index_header(argument[0]);
      multiple_files = Fixel::find_data_headers(argument[0], index_header);
      if (multiple_files.empty())
        throw Exception("No fixel data files found in directory \"" + argument[0] + "\"");
      output_header = Header(multiple_files[0]);
    } catch (...) {
      try {
        index_header = Fixel::find_index_header(Fixel::get_fixel_directory(argument[0]));
        single_file = Image<float>::open(argument[0]);
        Fixel::check_data_file(single_file);
        output_header = Header(single_file);
      } catch (...) {
        throw Exception("Could not interpret first argument \"" + argument[0] +
                        "\" as either a fixel data file, or a fixel directory");
      }
    }

    if (single_file.valid() && !Fixel::fixels_match(index_header, single_file))
      throw Exception("File \"" + argument[0] +
                      "\" is not a valid fixel data file (does not match corresponding index image)");

    auto opt = get_options("matrix");
    Fixel::Matrix::Reader matrix(opt[0][0]);

    Image<index_type> index_image = index_header.get_image<index_type>();
    const size_t nfixels = Fixel::get_number_of_fixels(index_image);
    if (nfixels != matrix.size())
      throw Exception("Number of fixels in input (" + str(nfixels) +
                      ") does not match number of fixels in connectivity matrix (" + str(matrix.size()) + ")");

    switch (int(argument[1])) {
    case 0: {
      const float value = get_option_value("threshold_value", float(DEFAULT_FIXEL_CONNECT_VALUE_THRESHOLD));
      const float connect =
          get_option_value("threshold_connectivity", float(DEFAULT_FIXEL_CONNECT_CONNECTIVITY_THRESHOLD));
      filter.reset(new Fixel::Filter::Connect(matrix, value, connect));
      output_header.datatype() = DataType::UInt32;
      output_header.datatype().set_byte_order_native();
      option_list.erase("threshold_value");
      option_list.erase("threshold_connectivity");
    } break;
    case 1: {
      const float fwhm = get_option_value("fwhm", float(DEFAULT_FIXEL_SMOOTHING_FWHM));
      const float threshold = get_option_value("minweight", float(DEFAULT_FIXEL_SMOOTHING_MINWEIGHT));
      opt = get_options("mask");
      if (opt.size()) {
        Image<bool> mask_image = Image<bool>::open(opt[0][0]);
        filter.reset(new Fixel::Filter::Smooth(index_image, matrix, mask_image, fwhm, threshold));
      } else {
        filter.reset(new Fixel::Filter::Smooth(index_image, matrix, fwhm, threshold));
      }
      option_list.erase("fwhm");
      option_list.erase("minweight");
      option_list.erase("mask");
    } break;
    default:
      assert(0);
    }
  }

  for (const auto &i : option_list) {
    if (get_options(i).size())
      WARN("Option -" + i + " ignored; not relevant to " + filters[int(argument[1])] + " filter");
  }

  if (single_file.valid()) {
    auto output_image = Image<float>::create(argument[2], single_file);
    CONSOLE(std::string("Applying \"") + filters[argument[1]] + "\" operation to fixel data file \"" +
            single_file.name() + "\"");
    (*filter)(single_file, output_image);
  } else {
    Fixel::copy_index_and_directions_file(argument[0], argument[2]);
    ProgressBar progress(std::string("Applying \"") + filters[argument[1]] + "\" operation to " +
                             str(multiple_files.size()) + " fixel data files",
                         multiple_files.size());
    for (auto &H : multiple_files) {
      auto input_image = H.get_image<float>();
      auto output_image = Image<float>::create(Path::join(argument[2], Path::basename(H.name())), H);
      (*filter)(input_image, output_image);
      ++progress;
    }
  }
}
