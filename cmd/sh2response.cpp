/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier 2014

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
#include "args.h"
#include "exception.h"
#include "mrtrix.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/loop.h"
#include "image/position.h"
#include "image/threaded_loop.h"
#include "image/utils.h"
#include "image/value.h"

#include "math/math.h"
#include "math/SH.h"
#include "math/vector.h"

#include "dwi/gradient.h"
#include "dwi/shells.h"





using namespace MR;
using namespace App;






void usage () {

  DESCRIPTION 
    + "generate an appropriate response function from the image data for spherical deconvolution";

  ARGUMENTS
    + Argument ("SH", "the spherical harmonic decomposition of the diffusion-weighted images").type_image_in()
    + Argument ("mask", "the mask containing the voxels from which to estimate the response function").type_image_in()
    + Argument ("directions", "a 4D image containing the direction vectors along which to estimate the response function").type_image_in()
    + Argument ("response", "the output axially-symmetric spherical harmonic coefficients").type_file_out();

  OPTIONS

    + DWI::GradImportOptions()
    + DWI::ShellOption

    + Option ("lmax", "specify the maximum harmonic degree of the response function to estimate")
      + Argument ("value").type_integer (0, 8, 20);
}



typedef double value_type;



void run () 
{
  Image::Buffer<value_type> SH_buffer (argument[0]);
  Math::SH::check (SH_buffer);
  Image::Buffer<bool> mask_buffer (argument[1]);
  Image::Buffer<value_type> dir_buffer (argument[2]);

  int lmax = Math::SH::LforN (SH_buffer.dim(3));
  Options opt = get_options ("lmax");
  if (opt.size())
    lmax = opt[0][0];

  Image::check_dimensions (SH_buffer, mask_buffer, 0, 3);
  Image::check_dimensions (SH_buffer, dir_buffer, 0, 3);
  if (dir_buffer.ndim() < 4 || dir_buffer.dim(3) < 3) 
    throw Exception ("input direction image \"" + std::string (argument[2]) + "\" does not have expected dimensions");

  auto SH = SH_buffer.voxel();
  auto mask = mask_buffer.voxel();
  auto dir = dir_buffer.voxel();


  Math::Vector<value_type> delta;
  std::vector<value_type> response (lmax/2 + 1, 0.0);
  size_t count = 0;

  Image::LoopInOrder loop (SH, "estimating response function...", 0, 3);
  for (auto l = loop(mask, SH, dir); l; ++l) {

    if (!mask.value()) 
      continue;

    Point<value_type> d;
    for (dir[3] = 0; dir[3] < 3; ++dir[3])
      d[size_t(dir[3])] = dir.value();
    d.normalise();
    Math::SH::delta (delta, d, lmax);

    for (int l = 0; l <= lmax; l += 2) {
      value_type d_dot_s = 0.0;
      value_type d_dot_d = 0.0;
      for (int m = -l; m <= l; ++m) {
        size_t i = Math::SH::index (l,m);
        SH[3] = i;
        value_type s = SH.value();
        // TODO: currently this does NOT handle the non-orthonormal basis
        d_dot_s += s*delta[i];
        d_dot_d += Math::pow2 (delta[i]);
      }
      response[l/2] += d_dot_s / d_dot_d;
    }
    ++count;
  }

  VLA_MAX (AL, value_type, lmax+1, 64);
  Math::Legendre::Plm_sph (AL, lmax, 0, value_type (1.0));
  for (size_t l = 0; l < response.size(); l++)
    response[l] *= AL[2*l];

  File::OFStream out (argument[3]);
  for (auto r : response)
    out << r/count << " ";
  out << "\n";
}
