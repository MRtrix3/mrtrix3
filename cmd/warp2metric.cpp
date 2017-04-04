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
#include "image.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "sparse/fixel_metric.h"
#include "sparse/image.h"
#include "adapter/jacobian.h"

using namespace MR;
using namespace App;

using Sparse::FixelMetric;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "compute fixel or voxel-wise metrics from a 4D deformation field";

  ARGUMENTS
  + Argument ("in", "the input deformation field").type_image_in ();

  OPTIONS
  + Option ("fc", "use an input template fixel image to define fibre orientations and output "
                  "a fixel image describing the change in fibre cross-section (FC) in the perpendicular plane to the fixel orientation")
    + Argument ("template_input").type_image_in ()
    + Argument ("output").type_image_out ()

  + Option ("jmat", "output a Jacobian matrix image stored in column-major order along the 4th dimension."
                       "Note the output jacobian describes the warp gradient w.r.t the scanner space coordinate system")
    + Argument ("output").type_image_out ()

  + Option ("jdet", "output the Jacobian determinant instead of the full matrix")
    + Argument ("output").type_image_out ();

  //TODO add FC paper reference
}


using value_type = float;


void run ()
{
  auto input = Image<value_type>::open (argument[0]).with_direct_io (3);
  if (input.ndim() != 4)
    throw Exception ("input deformation field is not a 4D image");
  if (input.size(3) != 3)
    throw Exception ("input deformation field should have 3 volumes in the 4th dimension");

  std::unique_ptr<Image<value_type> > jmatrix_output;
  std::unique_ptr<Image<value_type> > jdeterminant_output;
  std::unique_ptr<Sparse::Image<FixelMetric> > fixel_template;
  std::unique_ptr<Sparse::Image<FixelMetric> > fc_output;


  auto opt = get_options ("fc");
  if (opt.size()) {
    Header output_header (input);
    output_header.ndim() = 3;
    output_header.datatype() = DataType::UInt64;
    output_header.datatype().set_byte_order_native();
    output_header.keyval()[Sparse::name_key] = str(typeid(FixelMetric).name());
    output_header.keyval()[Sparse::size_key] = str(sizeof(FixelMetric));
    fixel_template.reset (new Sparse::Image<FixelMetric> (opt[0][0]));
    fc_output.reset (new Sparse::Image<FixelMetric> (opt[0][1], output_header));
  }


  opt = get_options ("jmat");
  if (opt.size()) {
    Header output_header (input);
    output_header.size(3) = 9;
    jmatrix_output.reset (new Image<value_type> (Image<value_type>::create (opt[0][0], output_header)));
  }

  opt = get_options ("jdet");
  if (opt.size()) {
    Header output_header (input);
    output_header.ndim() = 3;
    jdeterminant_output.reset (new Image<value_type> (Image<value_type>::create (opt[0][0], output_header)));
  }

  if (!(jmatrix_output || jdeterminant_output || fc_output))
    throw Exception ("Nothing to do; please specify at least one output image type");

  Adapter::Jacobian<Image<value_type> > jacobian (input);

  for (auto i = Loop ("outputting warp metric(s)", jacobian, 0, 3) (jacobian); i; ++i) {
    auto jacobian_matrix = jacobian.value();

    if (fc_output) {
      assign_pos_of (jacobian, 0, 3).to (*fc_output, *fixel_template);
      fc_output->value().set_size (fixel_template->value().size());
      for (size_t f = 0; f != fixel_template->value().size(); ++f) {
        fc_output->value()[f] = fixel_template->value()[f];
        Eigen::Vector3f fixel_direction =  fixel_template->value()[f].dir;
        fixel_direction.normalize();
        Eigen::Vector3f fixel_direction_transformed = jacobian_matrix * fixel_direction;
        fc_output->value()[f].value = jacobian_matrix.determinant() / fixel_direction_transformed.norm();
      }
    }
    if (jmatrix_output) {
      assign_pos_of (jacobian, 0, 3).to (*jmatrix_output);
      for (size_t j = 0; j < 9; ++j) {
        jmatrix_output->index(3) = j;
        jmatrix_output->value() = jacobian_matrix.data()[j];
      }
    }
    if (jdeterminant_output) {
      assign_pos_of (jacobian, 0, 3).to (*jdeterminant_output);
      jdeterminant_output->value() = jacobian_matrix.determinant();
    }
  }
}
