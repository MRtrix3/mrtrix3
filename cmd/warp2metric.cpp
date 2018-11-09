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
#include "algo/threaded_loop.h"
#include "image.h"
#include "adapter/jacobian.h"
#include "registration/warp/helpers.h"
#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Compute fixel-wise or voxel-wise metrics from a 4D deformation field";

  REFERENCES
  + "Raffelt, D.; Tournier, JD/; Smith, RE.; Vaughan, DN.; Jackson, G.; Ridgway, GR. Connelly, A." // Internal
    "Investigating White Matter Fibre Density and Morphology using Fixel-Based Analysis. \n"
    "Neuroimage, 2016, 10.1016/j.neuroimage.2016.09.029\n";

  ARGUMENTS
  + Argument ("in", "the input deformation field").type_image_in();

  OPTIONS
  + Option ("fc", "use an input template fixel image to define fibre orientations and output "
                  "a fixel image describing the change in fibre cross-section (FC) in the perpendicular "
                  "plane to the fixel orientation. e.g. warp2metric warp.mif -fc fixel_template_directory output_fixel_directory fc.mif")
    + Argument ("template_fixel_directory").type_image_in()
    + Argument ("output_fixel_directory").type_text()
    + Argument ("output_fixel_data").type_text()

  + Option ("jmat", "output a Jacobian matrix image stored in column-major order along the 4th dimension."
                    "Note the output jacobian describes the warp gradient w.r.t the scanner space coordinate system")
    + Argument ("output").type_image_out()

  + Option ("jdet", "output the Jacobian determinant instead of the full matrix")
    + Argument ("output").type_image_out();

}


using value_type = float;


void run ()
{
  auto input = Image<value_type>::open (argument[0]).with_direct_io (3);
  Registration::Warp::check_warp (input);

  Image<value_type> jmatrix_output;
  Image<value_type> jdeterminant_output;

  Image<uint32_t> fixel_template_index;
  Image<value_type> fixel_template_directions;
  Image<value_type> fc_output_data;

  auto opt = get_options ("fc");
  if (opt.size()) {
    std::string template_fixel_directory (opt[0][0]);
    fixel_template_index = Fixel::find_index_header (template_fixel_directory).get_image<uint32_t>();
    fixel_template_directions = Fixel::find_directions_header (template_fixel_directory).get_image<value_type>().with_direct_io();

    std::string output_fixel_directory (opt[0][1]);
    if (template_fixel_directory != output_fixel_directory) {
      Fixel::copy_index_file (template_fixel_directory, output_fixel_directory);
      Fixel::copy_directions_file (template_fixel_directory, output_fixel_directory);
    }

    uint32_t num_fixels = 0;
    fixel_template_index.index(0) = 0;
    for (auto l = Loop (fixel_template_index, 0, 3) (fixel_template_index); l; ++l)
      num_fixels += fixel_template_index.value();

    fc_output_data = Image<value_type>::create (Path::join (output_fixel_directory, opt[0][2]), Fixel::data_header_from_index (fixel_template_index));
  }


  opt = get_options ("jmat");
  if (opt.size()) {
    Header output_header (input);
    output_header.size(3) = 9;
    jmatrix_output = Image<value_type>::create (opt[0][0], output_header);
  }

  opt = get_options ("jdet");
  if (opt.size()) {
    Header output_header (input);
    output_header.ndim() = 3;
    jdeterminant_output = Image<value_type>::create (opt[0][0], output_header);
  }

  if (!(jmatrix_output.valid() || jdeterminant_output.valid() || fc_output_data.valid()))
    throw Exception ("Nothing to do; please specify at least one output image type");

  Adapter::Jacobian<Image<value_type> > jacobian (input);

  for (auto i = Loop ("outputting warp metric(s)", jacobian, 0, 3) (jacobian); i; ++i) {
    auto jacobian_matrix = jacobian.value();

    if (fc_output_data.valid()) {
      assign_pos_of (jacobian, 0, 3).to (fixel_template_index);
      for (auto f = Fixel::Loop (fixel_template_index) (fixel_template_directions, fc_output_data); f; ++f) {
        Eigen::Vector3f fixel_direction = fixel_template_directions.row(1);
        fixel_direction.normalize();
        Eigen::Vector3f fixel_direction_transformed = jacobian_matrix * fixel_direction;
        fc_output_data.value() = jacobian_matrix.determinant() / fixel_direction_transformed.norm();
      }
    }
    if (jmatrix_output.valid()) {
      assign_pos_of (jacobian, 0, 3).to (jmatrix_output);
      for (size_t j = 0; j < 9; ++j) {
        jmatrix_output.index(3) = j;
        jmatrix_output.value() = jacobian_matrix.data()[j];
      }
    }
    if (jdeterminant_output.valid()) {
      assign_pos_of (jacobian, 0, 3).to (jdeterminant_output);
      jdeterminant_output.value() = jacobian_matrix.determinant();
    }
  }
}
