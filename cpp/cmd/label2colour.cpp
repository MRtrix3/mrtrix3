/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include <map>

#include "command.h"
#include "image.h"
#include "mrtrix.h"

#include "algo/loop.h"
#include "math/rng.h"

#include "connectome/connectome.h"
#include "connectome/lut.h"

#include <filesystem>

using namespace MR;
using namespace App;
using namespace MR::Connectome;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert a parcellated image (where values are node indices) into a colour image";

  DESCRIPTION
  + "Many software packages handle this colouring internally within their viewer program;"
    " this binary explicitly converts a parcellation image into a colour image"
    " that should be viewable in any software.";

  ARGUMENTS
  + Argument ("nodes_in",   "the input node parcellation image").type_image_in()
  + Argument ("colour_out", "the output colour image").type_image_out();

  OPTIONS
  + Option ("lut", "Provide the relevant colour lookup table"
                   " (if not provided, nodes will be coloured randomly)")
    + Argument ("file").type_file_in();

}
// clang-format on

void run() {
  const std::filesystem::path input_node_path{argument[0]};
  const std::filesystem::path output_colour_path{argument[1]};

  auto H = Header::open(input_node_path);
  Connectome::check(H);
  auto nodes = H.get_image<node_t>();

  const std::filesystem::path lut_path = get_option_value<std::string>("lut", "");
  LUT lut;
  if (!lut_path.empty()) {
    lut.load(lut_path);
  } else {
    INFO("No lookup table provided; colouring nodes randomly");

    node_t max_index = 0;
    for (auto l = Loop(nodes)(nodes); l; ++l) {
      const node_t index = nodes.value();
      if (index > max_index)
        max_index = index;
    }

    lut.insert(std::make_pair(0, LUT_node("None", 0, 0, 0, 0)));
    Math::RNG rng;
    std::uniform_int_distribution<uint8_t> dist;

    for (node_t i = 1; i <= max_index; ++i) {
      LUT_node::RGB colour;
      do {
        colour[0] = dist(rng);
        colour[1] = dist(rng);
        colour[2] = dist(rng);
      } while (int(colour[0]) + int(colour[1]) + int(colour[2]) < 100);
      lut.insert(std::make_pair(i, LUT_node(str(i), colour)));
    }
  }

  H.ndim() = 4;
  H.size(3) = 3;
  H.datatype() = DataType::UInt8;
  add_line(H.keyval()["comments"], "Coloured parcellation image generated by label2colour");
  if (!lut_path.empty())
    H.keyval()["LUT"] = lut_path.filename();
  auto out = Image<uint8_t>::create(output_colour_path, H);

  for (auto l = Loop("Colourizing parcellated node image", nodes)(nodes, out); l; ++l) {
    const node_t index = nodes.value();
    const LUT::const_iterator i = lut.find(index);
    if (i == lut.end()) {
      out.index(3) = 0;
      out.value() = 0;
      out.index(3) = 1;
      out.value() = 0;
      out.index(3) = 2;
      out.value() = 0;
    } else {
      const LUT_node::RGB &colour(i->second.get_colour());
      out.index(3) = 0;
      out.value() = colour[0];
      out.index(3) = 1;
      out.value() = colour[1];
      out.index(3) = 2;
      out.value() = colour[2];
    }
  }
}
