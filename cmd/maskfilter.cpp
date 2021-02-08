/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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
#include "image.h"
#include "filter/base.h"
#include "filter/connected_components.h"
#include "filter/dilate.h"
#include "filter/erode.h"
#include "filter/fill.h"
#include "filter/mask_clean.h"
#include "filter/median.h"

using namespace MR;
using namespace App;

#define DEFAULT_CLEAN_SCALE 2


const char* filters[] = { "clean", "connect", "dilate", "erode", "fill", "median", nullptr };



const OptionGroup CleanOption = OptionGroup ("Options for mask cleaning filter")

  + Option ("scale", "the maximum scale used to cut bridges. A certain maximum scale cuts "
                     "bridges up to a width (in voxels) of 2x the provided scale. (Default: " + str(DEFAULT_CLEAN_SCALE, 2) + ")")
    + Argument ("value").type_integer (1, 1e6);



const OptionGroup ConnectOption = OptionGroup ("Options for connected-component filter")

+ Option ("axes", "specify which axes should be included in the connected components. By default only "
                  "the first 3 axes are included. The axes should be provided as a comma-separated list of values.")
  + Argument ("axes").type_sequence_int()

+ Option ("largest", "only retain the largest connected component")

+ Option ("connectivity", "use 26-voxel-neighbourhood connectivity (Default: 6)");



const OptionGroup DilateErodeOption = OptionGroup ("Options for dilate / erode filters")

  + Option ("npass", "the number of times to repeatedly apply the filter")
    + Argument ("value").type_integer (1, 1e6);



const OptionGroup FillOption = OptionGroup ("Options for interior-filling filter")

+ Option ("axes", "specify which axes should be included in the connected components. By default only "
                  "the first 3 axes are included. The axes should be provided as a comma-separated list of values.")
  + Argument ("axes").type_sequence_int()

+ Option ("connectivity", "use 26-voxel-neighbourhood connectivity (Default: 6)");


const OptionGroup MedianOption = OptionGroup ("Options for median filter")

  + Option ("extent", "specify the extent (width) of kernel size in voxels. "
            "This can be specified either as a single value to be used for all axes, "
            "or as a comma-separated list of the extent for each axis. The default is 3x3x3.")
  + Argument ("voxels").type_sequence_int();





void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au), Thijs Dhollander (thijs.dhollander@gmail.com) and J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Perform filtering operations on 3D / 4D mask images";

  DESCRIPTION
  + "Many filters have their own unique set of optional parameters; "
    "see the option groups dedicated to each filter type.";

  ARGUMENTS
  + Argument ("input",  "the input mask.").type_image_in ()
  + Argument ("filter", "the name of the filter to be applied; options are: " + join(filters, ", ")).type_choice (filters)
  + Argument ("output", "the output mask.").type_image_out ();


  OPTIONS
  + CleanOption
  + ConnectOption
  + DilateErodeOption
  + FillOption
  + MedianOption

  + Stride::Options;
}


using value_type = bool;

void run () {

  auto input_image = Image<value_type>::open (argument[0]);

  int filter_index = argument[1];

  if (filter_index == 0) { // Mask clean
    Filter::MaskClean filter (input_image, std::string("applying mask cleaning filter to image ") + Path::basename (argument[0]));
    filter.set_scale(get_option_value ("scale", DEFAULT_CLEAN_SCALE));

    Stride::set_from_command_line (filter);

    auto output_image = Image<value_type>::create (argument[2], filter);
    filter (input_image, output_image);
    return;
  }

  if (filter_index == 1) { // Connected components
    Filter::ConnectedComponents filter (input_image, std::string("applying connected-component filter to image ") + Path::basename (argument[0]));
    auto opt = get_options ("axes");
    if (opt.size()) {
      const vector<int> axes = opt[0][0];
      filter.set_axes (axes);
    }
    bool largest_only = false;
    opt = get_options ("largest");
    if (opt.size()) {
      largest_only = true;
      filter.set_largest_only (true);
    }
    opt = get_options ("connectivity");
    if (opt.size())
      filter.set_26_connectivity (true);

    Stride::set_from_command_line (filter);

    if (largest_only) {
      filter.datatype() = DataType::UInt8;
      auto output_image = Image<value_type>::create (argument[2], filter);
      filter (input_image, output_image);
    } else {
      filter.datatype() = DataType::UInt32;
      filter.datatype().set_byte_order_native();
      auto output_image = Image<uint32_t>::create (argument[2], filter);
      filter (input_image, output_image);
    }
    return;
  }

  if (filter_index == 2) { // Dilate
    Filter::Dilate filter (input_image, std::string("applying dilate filter to image ") + Path::basename (argument[0]));
    auto opt = get_options ("npass");
    if (opt.size())
      filter.set_npass (int(opt[0][0]));

    Stride::set_from_command_line (filter);
    filter.datatype() = DataType::Bit;

    auto output_image = Image<value_type>::create (argument[2], filter);
    filter (input_image, output_image);
    return;
  }

  if (filter_index == 3) { // Erode
    Filter::Erode filter (input_image, std::string("applying erode filter to image ") + Path::basename (argument[0]));
    auto opt = get_options ("npass");
    if (opt.size())
      filter.set_npass (int(opt[0][0]));

    Stride::set_from_command_line (filter);
    filter.datatype() = DataType::Bit;

    auto output_image = Image<value_type>::create (argument[2], filter);
    filter (input_image, output_image);
    return;
  }

  if (filter_index == 4) { // Fill
    Filter::Fill filter (input_image, std::string("filling interior of image ") + Path::basename (argument[0]));
    auto opt = get_options ("axes");
    if (opt.size()) {
      const vector<int> axes = opt[0][0];
      filter.set_axes (axes);
    }
    opt = get_options ("connectivity");
    if (opt.size())
      filter.set_26_connectivity (true);
    Stride::set_from_command_line (filter);
    auto output_image = Image<value_type>::create (argument[2], filter);
    filter (input_image, output_image);
    return;
  }

  if (filter_index == 5) { // Median
    Filter::Median filter (input_image, std::string("applying median filter to image ") + Path::basename (argument[0]));
    auto opt = get_options ("extent");
    if (opt.size())
      filter.set_extent (parse_ints<uint32_t> (opt[0][0]));

    Stride::set_from_command_line (filter);
    filter.datatype() = DataType::Bit;

    auto output_image = Image<value_type>::create (argument[2], filter);
    filter (input_image, output_image);
    return;
  }

}
