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
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

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
                    image_mask (mask),// ? new Image::Buffer<bool>::voxel_type (*mask) : nullptr),
                    response (response), SH_in(n), SH_out(n) { } 

    void operator() (Image<value_type>& in, Image<value_type>& out) {
      if (image_mask.valid()) {
        assign_pos_of(in).to(image_mask);
//        Image::voxel_assign (*mask_vox_ptr, pos);
        if (!image_mask.value()) {
//          for (output_vox[3] = 0; output_vox[3] < output_vox.dim(3); ++output_vox[3])
//            output_vox.value() = 0.0;
          return;
        }
      }
      
      // copy input to vector.   TODO: view instead of copy?
      for (size_t i = 0; i < in.size(3); i++) {
        in.index(3) = i;
        SH_in[i] = in.value();
      }
      Math::SH::sconv(SH_out, response, SH_in);
      // copy result to output
      assign_pos_of(in).to(out);
      for (size_t i = 0; i < in.size(3); i++) {
        out.index(3) = i;
        out.value() = SH_out[i];
      }
//      Image::voxel_assign (input_vox, pos);
//      input_vox[3] = 0;
//      Math::Vector<value_type> input (input_vox.address(), input_vox.dim(3));
//      Math::Vector<value_type> output (output_vox.dim(3));
//      Math::SH::sconv (output, response, input);
//      Image::voxel_assign (output_vox, pos);
//      for (output_vox[3] = 0; output_vox[3] < output_vox.dim(3); ++output_vox[3])
//        output_vox.value() = output[output_vox[3]];

    }

  protected:
//    Image<value_type> image_in;   // not needed
//    Image<value_type> image_out;  // not needed
    Image<bool> image_mask;
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> response;
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> SH_in; // initialize size in constructor?
    Eigen::Matrix<value_type, Eigen::Dynamic, 1> SH_out;
};


void run() {

  auto image_in = Image<value_type>::open(argument[0]).with_direct_io(Stride::contiguous_along_axis(3));
  Math::SH::check(image_in);
//  Image::Header input_SH_header (argument[0]);
//  Math::SH::check (input_SH_header);
//  Image::BufferPreload<value_type> input_buf (input_SH_header, Image::Stride::contiguous_along_axis (3));

  auto responseSH = load_vector<value_type>(argument[1]);
  Eigen::Matrix<value_type, Eigen::Dynamic, 1> responseRH;
//  Math::Vector<value_type> responseSH;
//  responseSH.load (argument[1]);
//  Math::Vector<value_type> responseRH;
  Math::SH::SH2RH (responseRH, responseSH);

//  std::unique_ptr<Image::Buffer<bool> > mask_buf;
//  Options opt = get_options ("mask");
//  if (opt.size()) {
//    mask_buf.reset (new Image::Buffer<bool> (opt[0][0]));
//    Image::check_dimensions (*mask_buf, input_buf, 0, 3);
//  }
  auto mask = Image<bool>();
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Header::open (opt[0][0]).get_image<bool>();
    check_dimensions (image_in, mask, 0, 3);
  }

  auto header = Header(image_in);
//  header.set_ndim (4);
//  header.size(3) = shared.nSH();
//  header.datatype() = DataType::Float32;
  Stride::set_from_command_line (header);
  auto image_out = Image<value_type>::create (argument[2], header);
  
//  Image::Header output_SH_header (input_SH_header);
//  Image::Stride::set_from_command_line (output_SH_header, Image::Stride::contiguous_along_axis (3));
//  Image::Buffer<value_type> output_SH_buf (argument[2], output_SH_header);

  SConvFunctor sconv (image_in.size(3), mask, responseRH);
  ThreadedLoop ("performing convolution...", image_in, 0, 3, 2).run(sconv, image_in, image_out);
}
