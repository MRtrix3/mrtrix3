/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 28/10/09.

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


#include "app.h"
#include "point.h"
#include "math/SH.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "math/vector.h"
#include "math/rng.h"
#include "image/loop.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "sample_header simulated FOD field.";

  ARGUMENTS
  + Argument ("FOD", "the input image containing the SH coefficients of the simulated FOD field.").type_image_in()
  + Argument ("sample_header", "the output image containing the directions sample_headerd from the FOD field.").type_image_out();

  OPTIONS
  + Option ("cutoff",
            "do not sample_header from regions of the FOD with amplitude "
            "lower than this threshold (default: 0.1).")
  + Argument ("value").type_float (0.0, 0.1, std::numeric_limits<float>::max())

  + Option ("ceiling",
            "use value supplied as ceiling for rejection sampling (default = 4.0)")
  + Argument ("value").type_float (0.0, 4.0, std::numeric_limits<float>::max());
}


typedef float value_type;


void run ()
{
  Image::Buffer<float> FOD_data (argument[0]);
  if (FOD_data.ndim() != 4)
    throw Exception ("input FOD image should have 4 dimensions");

  const int lmax = Math::SH::LforN (FOD_data.dim (3));
  inform ("assuming lmax = " + str (lmax));

  Image::Header sample_header (FOD_data);
  sample_header.dim(3) = 3;
  sample_header.datatype() = DataType::Float32;
  Image::Buffer<value_type> sample_data (argument[1], sample_header);

  Image::Buffer<value_type>::voxel_type in (FOD_data);
  Image::Buffer<value_type>::voxel_type out (sample_data);

  value_type threshold = 0.1;
  Options opt = get_options ("cutoff");
  if (opt.size())
    threshold = to<value_type> (opt[0][0]);

  value_type ceiling = 4.0;
  opt = get_options ("ceiling");
  if (opt.size())
    ceiling = to<value_type> (opt[0][0]);

  value_type maximum = 0.0;
  Math::RNG rng;

  {
    Image::Loop loop ("sampling FOD field...", 0, 3);
    Image::Loop inner (3);
    for (loop.start (in, out); loop.ok(); loop.next (in, out)) {
      value_type val [in.dim (3)];
      for (inner.start (in); inner.ok(); inner.next (in))
        val [in[3]] = in.value();

      Point<value_type> d;

      for (size_t n = 0; n < 1000; ++n) {
        Point<value_type> dir (rng.normal(), rng.normal(), rng.normal());
        dir.normalise();
        value_type amp = Math::SH::value (val, dir, lmax);
        if (amp > maximum)
          maximum = amp;

        if (amp > ceiling * rng.uniform()) {
          d = dir;
          break;
        }
      }

      out[3] = 0;
      out.value() = d[0];
      ++out[3];
      out.value() = d[1];
      ++out[3];
      out.value() = d[2];
    }
  }

  if (maximum > ceiling)
    print ("rejection sampling ceiling exceeded (max val = " + str (maximum) + ")\n");
}

