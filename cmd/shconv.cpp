/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 06/07/11.

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
#include "memory.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "math/SH.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
    + "perform a spherical convolution";

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image.").type_image_in ()
    + Argument ("response", "the convolution kernel (response function)").type_file_in ()
    + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out ();

  OPTIONS
    + Option ("mask", "only perform computation within the specified binary brain mask image.")
    + Argument ("image", "the mask image to use.").type_image_in ()

    + Stride::Options;
}



typedef float value_type;


class SConvFunctor {
  public:
  SConvFunctor (const size_t n, Image<bool>& mask, 
                const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& response) :
                    image_mask (mask),
                    response (response),
                    SH_out (n){ }

    void operator() (Image<value_type>& in, Image<value_type>& out) {
      if (image_mask.valid()) {
        assign_pos_of(in).to(image_mask);
        if (!image_mask.value()) {
          out.row(3).setZero();
          return;
        }
      }
      out.row(3) = Math::SH::sconv (SH_out, response, in.row(3));
    }

  protected:
    Image<bool> image_mask;
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> response;
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> SH_out;

};


void run()
{

  auto image_in = Image<value_type>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis(3));
  Math::SH::check (image_in.header());

  auto responseSH = load_vector<value_type>(argument[1]);
  Eigen::Matrix<value_type, Eigen::Dynamic, 1> responseRH;
  Math::SH::SH2RH (responseRH, responseSH);

  auto mask = Image<bool>();
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Header::open (opt[0][0]).get_image<bool>();
    check_dimensions (image_in, mask, 0, 3);
  }

  auto header = image_in.header();
  Stride::set_from_command_line (header);
  auto image_out = Image<value_type>::create (argument[2], header);
  
  SConvFunctor sconv (image_in.size(3), mask, responseRH);
  ThreadedLoop ("performing convolution...", image_in, 0, 3, 2).run (sconv, image_in, image_out);
}
