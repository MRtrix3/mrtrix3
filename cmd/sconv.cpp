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

#include "app.h"
#include "ptr.h"
#include "progressbar.h"
#include "image/threaded_loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "math/SH.h"

using namespace MR;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
    + "perform a spherical convolution";

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image.").type_image_in ()
    + Argument ("response", "the convolution kernel (response function)").type_file ()
    + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out ();

  OPTIONS
    + Option ("mask", "only perform computation within the specified binary brain mask image.")
    + Argument ("image", "the mask image to use.").type_image_in ()

    + Image::Stride::StrideOption;
}



typedef float value_type;


class SDeconvFunctor {
  public:
  SDeconvFunctor (Image::BufferPreload<value_type>& in,
                  Image::Buffer<value_type>& out,
                  Ptr<Image::Buffer<bool> >& mask,
                  const Math::Vector<value_type>& response) :
                    input_vox (in),
                    output_vox (out),
                    response (response) {
                      if (mask)
                        mask_vox_ptr = new Image::Buffer<bool>::voxel_type (*mask);
    }

    void operator() (const Image::Iterator& pos) {
      if (mask_vox_ptr) {
        Image::voxel_assign (*mask_vox_ptr, pos);
        if (!mask_vox_ptr->value()) {
          for (output_vox[3] = 0; output_vox[3] < output_vox.dim(3); ++output_vox[3])
            output_vox.value() = 0.0;
          return;
        }
      }
      Image::voxel_assign (input_vox, pos);
      input_vox[3] = 0;
      Math::Vector<value_type> input (input_vox.address(), input_vox.dim(3));
      Math::Vector<value_type> output (output_vox.dim(3));
      Math::SH::sconv (output, response, input);
      Image::voxel_assign (output_vox, pos);
      for (output_vox[3] = 0; output_vox[3] < output_vox.dim(3); ++output_vox[3])
        output_vox.value() = output[output_vox[3]];

    }

  protected:
    Image::BufferPreload<value_type>::voxel_type input_vox;
    Image::Buffer<value_type>::voxel_type output_vox;
    Ptr<Image::Buffer<bool>::voxel_type> mask_vox_ptr;
    Math::Vector<value_type> response;
};


void run() {

  Image::Header input_SH_header (argument[0]);
  if (input_SH_header.ndim() != 4)
    throw Exception ("input SH image should contain 4 dimensions");
  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  Image::BufferPreload<value_type> input_buf (input_SH_header, strides);

  Math::Vector<value_type> responseSH;
  responseSH.load (argument[1]);
  Math::Vector<value_type> responseRH;
  Math::SH::SH2RH (responseRH, responseSH);

  Ptr<Image::Buffer<bool> > mask_buf;
  Options opt = get_options ("mask");
  if (opt.size()) {
    mask_buf = new Image::Buffer<bool> (opt[0][0]);
    Image::check_dimensions (*mask_buf, input_buf, 0, 3);
  }

  Image::Header output_SH_header (input_SH_header);
  Image::Stride::set_from_command_line (output_SH_header, Image::Stride::contiguous_along_axis (3));
  Image::Buffer<value_type> output_SH_buf (argument[2], output_SH_header);

  SDeconvFunctor sconv (input_buf, output_SH_buf, mask_buf, responseRH);
  Image::ThreadedLoop loop ("performing convolution...", input_buf, 2, 0, 3);
  loop.run (sconv);
}
