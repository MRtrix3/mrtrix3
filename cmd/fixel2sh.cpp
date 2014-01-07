/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "image/sparse/fixel_metric.h"
#include "image/sparse/voxel.h"

#include "math/SH.h"




using namespace MR;
using namespace App;



using Image::Sparse::FixelMetric;



void usage ()
{

  DESCRIPTION
  + "convert a fixel-based sparse-data image into an SH image that can be visually evaluated using MRview";

  ARGUMENTS
  + Argument ("fixel_in", "the input sparse fixel image.").type_image_in ()
  + Argument ("sh_out",   "the output sh image.").type_image_out ();

}




void run ()
{

  Image::Header H_in (argument[0]);
  Image::BufferSparse<FixelMetric> fixel_data (H_in);
  Image::BufferSparse<FixelMetric>::voxel_type fixel (fixel_data);

  const size_t lmax = 8;
  const ssize_t n = Math::SH::NforL (lmax);
  Math::SH::aPSF<float> aPSF (lmax);

  Image::Header H_out (H_in);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  const size_t sh_dim = H_in.ndim();
  H_out.set_ndim (H_in.ndim() + 1);
  H_out.dim (sh_dim) = n;

  Image::Buffer<float> sh_data (argument[1], H_out);
  Image::Buffer<float>::voxel_type sh (sh_data);
  std::vector<float> values;
  Math::Vector<float> apsf_values;

  Image::LoopInOrder loop (fixel, "converting sparse fixel data to SH image... ");
  for (loop.start (fixel, sh); loop.ok(); loop.next (fixel, sh)) {
    values.assign (n, float(0.0));
    for (size_t index = 0; index != fixel.value().size(); ++index) {
      apsf_values = aPSF (apsf_values, fixel.value()[index].dir);
      const float scale_factor = fixel.value()[index].value;
      for (ssize_t i = 0; i != n; ++i)
        values[i] += apsf_values[i] * scale_factor;
    }
    for (sh[sh_dim] = 0; sh[sh_dim] != n; ++sh[sh_dim])
      sh.value() = values[sh[sh_dim]];
  }

}

