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
#include "adapter/jacobian.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "compute the Jacobian matrix from a 4D deformation field";

  ARGUMENTS
  + Argument ("in", "the input deformation field").type_image_in ()
  + Argument ("out", "the output Jacobian matrix image stored in column-major order along the 4th dimension.").type_image_out ();

  OPTIONS
  + Option ("determinant", "output the Jacobian determinant instead of the full matrix");
}


typedef float value_type;


void run ()
{
  auto input = Image<value_type>::open (argument[0]).with_direct_io (3);
  if (input.ndim() != 4)
    throw Exception ("input deformation field is not a 4D image");

  if (input.size(3) != 3)
    throw Exception ("input deformation field should have 3 volumes in the 4th dimension");

  Header output_header (input);
  bool determinant = get_options("determinant").size();
  std::string progress_string;
  if (determinant) {
    progress_string = "computing Jacobian determinant";
    output_header.set_ndim(3);
  } else {
    progress_string = "computing Jacobian matrix";
    output_header.size(3) = 9;
  }

  auto output = Image<value_type>::create (argument[1], output_header);

  Adapter::Jacobian<Image<value_type> > jacobian (input);

  auto func = [determinant](Adapter::Jacobian<Image<value_type> >& in, Image<value_type>& out) {
    auto jacobian_matrix = in.value();
    if (determinant) {
      out.value() = jacobian_matrix.determinant();
    } else {
      for (size_t i = 0; i < 9; ++i) {
        out.index(3) = i;
        out.value() = jacobian_matrix.data()[i];
      }
    }
  };

  ThreadedLoop (progress_string, output, 0, 3)
    .run (func, jacobian, output);
}
