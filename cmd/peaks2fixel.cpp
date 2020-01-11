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
#include "image.h"
#include "algo/loop.h"
#include "fixel/helpers.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert peak directions image to a fixel directory";

  ARGUMENTS
  + Argument ("directions", "the input directions image; each volume corresponds to the x, y & z "
                            "component of each direction vector in turn.").type_image_in()

  + Argument ("fixels", "the output fixel directory.").type_directory_out();
}



vector<Eigen::Vector3> get (Image<float>& data)
{
  data.index(3) = 0;
  vector<Eigen::Vector3> result;
  while (data.index(3) < data.size(3)) {
    Eigen::Vector3 direction;
    for (size_t axis = 0; axis != 3; ++axis) {
      direction[axis] = data.value();
      data.index(3)++;
    }
    if (direction.allFinite() && direction.squaredNorm())
      result.push_back (direction);
  }
  return result;
}



void run ()
{
  auto input_header = Header::open (argument[0]);
  Peaks::check (input_header);
  auto input_directions = input_header.get_image<float>();
  uint32_t nfixels = 0;
  bool all_unit_norm = true;
  for (auto l = Loop("counting fixels in input image", 0, 3) (input_directions); l; ++l) {
    auto dirs = get (input_directions);
    nfixels += dirs.size();
    for (const auto& d : dirs) {
      if (MR::abs (d.squaredNorm() - 1.0) > 1e-4)
        all_unit_norm = false;
    }
  }
  INFO ("Number of fixels in input peaks image: " + str(nfixels));
  if (all_unit_norm) {
    INFO ("All peaks have unit norm; no need to create amplitudes fixel data file");
  } else {
    INFO ("Peaks have variable amplitudes; will create additional fixel data file for amplitudes");
  }

  Fixel::check_fixel_directory (argument[1], true, true);

  // Easiest if we first make the index image
  const std::string index_path = Path::join (argument[1], "index.mif");
  Header index_header (input_header);
  index_header.name() = index_path;
  index_header.datatype() = DataType::UInt32;
  index_header.datatype().set_byte_order_native();
  index_header.size(3) = 2;
  index_header.keyval()[Fixel::n_fixels_key] = str(nfixels);
  auto index_image = Image<uint32_t>::create (index_path, index_header);

  Header directions_header = Fixel::directions_header_from_index (index_header);
  directions_header.datatype() = DataType::Float32;
  directions_header.datatype().set_byte_order_native();
  auto directions_image = Image<float>::create (Path::join (argument[1], "directions.mif"), directions_header);

  Image<float> amplitudes_image;
  if (!all_unit_norm) {
    Header amplitudes_header = Fixel::data_header_from_index (index_header);
    amplitudes_image = Image<float>::create (Path::join (argument[1], "amplitudes.mif"), amplitudes_header);
  }

  uint32_t output_index = 0;
  for (auto l = Loop("converting peaks to fixel format", 0, 3) (input_directions, index_image); l; ++l) {
    auto dirs = get (input_directions);
    index_image.index(3) = 0; index_image.value() = dirs.size();
    index_image.index(3) = 1; index_image.value() = dirs.size() ? output_index : 0;
    for (auto d : dirs) {
      directions_image.index(0) = output_index;
      if (amplitudes_image.valid()) {
        directions_image.row(1) = d.normalized();
        amplitudes_image.index(0) = output_index;
        amplitudes_image.value() = d.norm();
      } else {
        directions_image.row(1) = d;
      }
      ++output_index;
    }
  }

}
