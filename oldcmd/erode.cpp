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
 "erode (or dilate) mask (i.e. binary) image",
  NULL
};

ARGUMENTS = {
  Argument ("input", "input image", "input mask image to be eroded.").type_image_in (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End
};


OPTIONS = { 

  Option ("dilate", "perform dilation", "perform dilation rather than erosion."),

  Option ("npass", "number of passes", "the number of passes (default: 1).")
    .append (Argument ("numebr", "number of passes", "the number of passes.").type_integer (1, 1000, 1)),

  Option::End 

};


float erode (Image::Voxel& in) 
{
  if (in.value() < 0.5) return (0.0);
  float val;
  if (in[0] == 0) return (0.0);
  if (in[1] == 0) return (0.0);
  if (in[2] == 0) return (0.0);
  if (in[0] == in.dim(0)-1) return (0.0);
  if (in[1] == in.dim(1)-1) return (0.0);
  if (in[2] == in.dim(2)-1) return (0.0);
  if (in[0] > 0) { in[0]--; val = in.value(); in[0]++; if (val < 0.5) return (0.0); }
  if (in[1] > 0) { in[1]--; val = in.value(); in[1]++; if (val < 0.5) return (0.0); }
  if (in[2] > 0) { in[2]--; val = in.value(); in[2]++; if (val < 0.5) return (0.0); }
  if (in[0] < in.dim(0)-1) { in[0]++; val = in.value(); in[0]--; if (val < 0.5) return (0.0); }
  if (in[1] < in.dim(1)-1) { in[1]++; val = in.value(); in[1]--; if (val < 0.5) return (0.0); }
  if (in[2] < in.dim(2)-1) { in[2]++; val = in.value(); in[2]--; if (val < 0.5) return (0.0); }
  return (1.0);
}



float dilate (Image::Voxel& in) 
{
  if (in.value() >= 0.5) return (1.0);
  float val;
  if (in[0] > 0) { in[0]--; val = in.value(); in[0]++; if (val >= 0.5) return (1.0); }
  if (in[1] > 0) { in[1]--; val = in.value(); in[1]++; if (val >= 0.5) return (1.0); }
  if (in[2] > 0) { in[2]--; val = in.value(); in[2]++; if (val >= 0.5) return (1.0); }
  if (in[0] < in.dim(0)-1) { in[0]++; val = in.value(); in[0]--; if (val >= 0.5) return (1.0); }
  if (in[1] < in.dim(1)-1) { in[1]++; val = in.value(); in[1]--; if (val >= 0.5) return (1.0); }
  if (in[2] < in.dim(2)-1) { in[2]++; val = in.value(); in[2]--; if (val >= 0.5) return (1.0); }
  return (0.0);
}


EXECUTE {

  RefPtr<Image::Object> obj_in (argument[0].get_image());
  Image::Header header (obj_in->header());
  header.data_type = DataType::Bit;

  std::vector<OptBase> opt = get_options (0); // dilate
  bool dilation = opt.size();

  opt = get_options (1); // npass
  int npasses = opt.size() ? opt[0][0].get_int() : 1;

  obj_in->map();

  for (int npass = 0; npass < npasses; npass++) {
    RefPtr<Image::Object> obj_out;
    if (npass < npasses-1) {
       obj_out = new Image::Object;
       obj_out->create ("", header);
    }
    else {
      obj_out = argument[1].get_image (header);
      obj_out->map();
    }

    Image::Voxel in (*obj_in);
    Image::Voxel out (*obj_out);
    ProgressBar::init (voxel_count(out), (dilation ? "dilat" : "erod" ) + std::string ("ing (pass ") + str(npass+1) +") ...");
    do {
      float val = dilation ? dilate (in) : erode (in);
      out.value() = val;
      in++;
      ProgressBar::inc();
    } while (out++);
    ProgressBar::done();

    if (npass < npasses-1) obj_in = obj_out;
  }
}

