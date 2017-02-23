/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */
#include "__mrtrix_plugin.h"

#include "command.h"
#include "exception.h"
#include "mrtrix.h"
#include "progressbar.h"

#include "image.h"
#include "algo/loop.h"

#include "math/math.h"
#include "math/SH.h"
#include "math/ZSH.h"

#include "dwi/gradient.h"
#include "dwi/shells.h"





using namespace MR;
using namespace App;






void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Generate an appropriate response function from the image data for spherical deconvolution";

  ARGUMENTS
    + Argument ("SH", "the spherical harmonic decomposition of the diffusion-weighted images").type_image_in()
    + Argument ("mask", "the mask containing the voxels from which to estimate the response function").type_image_in()
    + Argument ("directions", "a 4D image containing the direction vectors along which to estimate the response function").type_image_in()
    + Argument ("response", "the output axially-symmetric spherical harmonic coefficients").type_file_out();

  OPTIONS

    + Option ("lmax", "specify the maximum harmonic degree of the response function to estimate")
      + Argument ("value").type_integer (0, 20)
    + Option ("dump", "dump the m=0 SH coefficients from all voxels in the mask to the output file, rather than their mean")
      + Argument ("file").type_file_out();
}



typedef double value_type;



void run () 
{
  auto SH = Image<value_type>::open(argument[0]);
  Math::SH::check (SH);
  auto mask = Image<bool>::open(argument[1]);
  auto dir = Image<value_type>::open(argument[2]).with_direct_io();

  int lmax = get_option_value ("lmax", Math::SH::LforN (SH.size(3)));

  check_dimensions (SH, mask, 0, 3);
  check_dimensions (SH, dir, 0, 3);
  if (dir.ndim() != 4)
    throw Exception ("input direction image \"" + std::string (argument[2]) + "\" must be a 4D image");
  if (dir.size(3) != 3)
    throw Exception ("input direction image \"" + std::string (argument[2]) + "\" must contain precisely 3 volumes");

  Eigen::VectorXd delta;
  Eigen::VectorXd response = Eigen::VectorXd::Zero (Math::ZSH::NforL (lmax));
  size_t count = 0;

  File::OFStream dump_stream;
  auto opt = get_options ("dump");
  if (opt.size())
    dump_stream.open (opt[0][0]);

  Eigen::Matrix<value_type,Eigen::Dynamic,1,0,64> AL (lmax+1);
  Math::Legendre::Plm_sph (AL, lmax, 0, value_type (1.0));

  auto loop = Loop ("estimating response function", SH, 0, 3);
  for (auto l = loop(mask, SH, dir); l; ++l) {

    if (!mask.value()) 
      continue;

    Eigen::Vector3d d = dir.row(3);
    if (!d.allFinite()) {
      WARN ("voxel with invalid direction [ " + str(dir.index(0)) + " " + str(dir.index(1)) + " " + str(dir.index(2)) + " ]; skipping");
      continue;
    }
    d.normalize();
    // Uncertainty regarding Eigen's behaviour when normalizing a zero vector; may change behaviour between versions
    if (!d.allFinite() || !d.squaredNorm()) {
      WARN ("voxel with zero direction [ " + str(dir.index(0)) + " " + str(dir.index(1)) + " " + str(dir.index(2)) + " ]; skipping");
      continue;
    }

    Math::SH::delta (delta, d, lmax);

    for (int l = 0; l <= lmax; l += 2) {
      value_type d_dot_s = 0.0;
      value_type d_dot_d = 0.0;
      for (int m = -l; m <= l; ++m) {
        size_t i = Math::SH::index (l,m);
        SH.index(3) = i;
        value_type s = SH.value();
        // TODO: currently this does NOT handle the non-orthonormal basis
        d_dot_s += s*delta[i];
        d_dot_d += Math::pow2 (delta[i]);
      }
      value_type val = AL[l] * d_dot_s / d_dot_d;
      response[Math::ZSH::index(l)] += val;

      if (dump_stream.is_open()) 
        dump_stream << val << " ";
    }
    if (dump_stream.is_open()) 
      dump_stream << "\n";

    ++count;
  }

  response /= count;

  if (std::string(argument[3]) == "-") {
    for (ssize_t n = 0; n < response.size(); ++n)
      std::cout << response[n] << " ";
    std::cout << "\n";
  }
  else {
    save_vector(response, argument[3]);
  }
}
