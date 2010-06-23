/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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
#include "progressbar.h"
#include "image/voxel.h"
#include "image/misc.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "add or subtract images",
  NULL
};

ARGUMENTS = {
  Argument ("image1", "first input image", "the first input image.").type_image_in (),
  Argument ("image2", "second input image", "the second input image.", AllowMultiple).type_image_in (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End
};

OPTIONS = { Option::End };




EXECUTE {
  uint num_images = argument.size()-1;

  RefPtr<Image::Object> in[num_images];
  in[0] = argument[0].get_image();
  Image::Header header (in[0]->header());

  header.data_type = DataType::Float32;

  for (size_t i = 1; i < num_images; i++) {
    in[i] = argument[i].get_image();

    if (in[i]->is_complex()) header.data_type = DataType::CFloat32;

    if (in[i]->ndim() > header.axes.ndim()) 
      header.axes.resize (in[i]->ndim());

    for (size_t n = 0; n < header.axes.size(); n++) { 
      if (header.axes[n].dim != in[i]->dim(n)) {
        if (header.axes[n].dim < 2) header.axes[n] = in[i]->header().axes[n];
        else if (in[i]->dim(n) > 1) throw Exception ("dimension mismatch between input files");
      }
    }
  }



  Image::Voxel out (*argument[num_images].get_image (header));
  out.image.map();


  ProgressBar::init (voxel_count(out), "adding...");

  for (size_t i = 0; i < num_images; i++) {
    Image::Voxel y (*in[i]);
    y.image.map();

    do {

      for (size_t n = 0; n < y.ndim(); n++)
        y[n] = y.dim(n) > 1 ? out[n] : 0;

      float val = i ? out.real() : 0.0;
      out.real() = val + y.real();
      
      if (out.is_complex()) {
        val = i ? out.imag() : 0.0;
        if (y.is_complex()) val += y.imag();
        out.imag() = val;
      }

      ProgressBar::inc();

    } while (out++);

  }

  ProgressBar::done();
}

