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
#include "progressbar.h"

#include "algo/loop.h"

#include "math/SH.h"

#include "sparse/fixel_metric.h"
#include "sparse/image.h"


using namespace MR;
using namespace App;



using Sparse::FixelMetric;



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "convert a fixel-based sparse-data image into an SH image that can be visually evaluated using MRview";

  ARGUMENTS
  + Argument ("fixel_in", "the input sparse fixel image.").type_image_in ()
  + Argument ("sh_out",   "the output sh image.").type_image_out ();

}




void run ()
{

  Header H_in = Header::open (argument[0]);
  Sparse::Image<FixelMetric> fixel (H_in);

  const size_t lmax = 8;
  const size_t N = Math::SH::NforL (lmax);
  Math::SH::aPSF<default_type> aPSF (lmax);

  Header H_out (H_in);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  const size_t sh_dim = H_in.ndim();
  H_out.ndim() = H_in.ndim() + 1;
  H_out.size (sh_dim) = N;

  auto sh = Image<float>::create (argument[1], H_out);
  std::vector<default_type> values;
  Eigen::Matrix<default_type, Eigen::Dynamic, 1> apsf_values;

  for (auto l1 = Loop("converting sparse fixel data to SH image", fixel) (fixel, sh); l1; ++l1) {
    values.assign (N, 0.0);
    for (size_t index = 0; index != fixel.value().size(); ++index) {
      apsf_values = aPSF (apsf_values, fixel.value()[index].dir);
      const default_type scale_factor = fixel.value()[index].value;
      for (size_t i = 0; i != N; ++i)
        values[i] += apsf_values[i] * scale_factor;
    }
    for (auto l2 = Loop(sh_dim) (sh); l2; ++l2)
      sh.value() = values[sh.index(sh_dim)];
  }

}

