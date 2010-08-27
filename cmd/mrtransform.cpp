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
#include "dataset/interp/nearest.h"
#include "dataset/interp/linear.h"
#include "dataset/interp/cubic.h"
#include "dataset/interp/reslice.h"
#include "dataset/misc.h"
#include "dataset/loop.h"
#include "dataset/copy.h"

using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
 "apply spatial transformations or reslice images.",
 "In most cases, this command will only modify the transform matrix, "
   "without reslicing the image. Only the \"reslice\" option will "
   "actually modify the image data.",
  NULL
};

ARGUMENTS = {
  Argument ("input", "input image to be transformed.").type_image_in (),
  Argument ("output", "the output image.").type_image_out (),
  Argument ()
};


const char* interp_choices[] = { "nearest", "linear", "cubic", NULL };

OPTIONS = {

  Option ("transform", "specify the 4x4 transform to apply, in the form of a 4x4 ascii file.")
    + Argument ("transform").type_file (),

  Option ("replace",
      "replace the transform of the original image by that specified, "
      "rather than applying it to the original image."),

  Option ("inverse", 
      "invert the specified transform before using it."),

  Option ("reslice",
      "reslice the input image to match the specified template image.")
    + Argument ("template").type_image_in (),

  Option ("interp", 
      "set the interpolation method to use when reslicing (default: linear).")
    + Argument ("method").type_choice (interp_choices),

  Option ("oversample",
      "set the oversampling factor to use when reslicing (i.e. the "
      "number of samples to take per voxel along each spatial dimension). "
      "This should be supplied as a vector of 3 integers. By default, the "
      "oversampling factor is determined based on the differences between "
      "input and output voxel sizes.")
    + Argument ("factors").type_sequence_int(),

  Option ("datatype", "specify output image data type (default: same as input image).")
    + Argument ("spec").type_choice (DataType::identifiers),

  Option ()
};






EXECUTE {
  Math::Matrix<float> T;

  Options opt = get_options ("transform");
  if (opt.size()) {
    T.load (opt[0][0]);
    if (T.rows() != 4 || T.columns() != 4) 
      throw Exception (std::string("transform matrix supplied in file \"") + opt[0][0] + "\" is not 4x4");
  }


  Image::Header header_in (argument[0]);
  Image::Header header_out (header_in);

  opt = get_options ("datatype");
  if (opt.size()) 
    header_out.set_datatype (opt[0][0]);

  bool inverse = get_options("inverse").size();
  bool replace = get_options("replace").size();

  if (inverse) {
    if (!T.is_set()) 
      throw Exception ("no transform provided for option '-inverse' (specify using '-transform' option)");
    Math::Matrix<float> I;
    Math::LU::inv (I, T);
    T.swap (I);
  }


  if (replace) 
    if (!T.is_set()) 
      throw Exception ("no transform provided for option '-replace' (specify using '-transform' option)");

  

  opt = get_options("reslice"); // need to reslice
  if (opt.size()) {
    Image::Header template_header (opt[0][0]);

    header_out.set_dim (0, template_header.dim(0));
    header_out.set_dim (1, template_header.dim(1));
    header_out.set_dim (2, template_header.dim(2));

    header_out.set_vox (0, template_header.vox(0));
    header_out.set_vox (1, template_header.vox(1));
    header_out.set_vox (2, template_header.vox(2));

    header_out.set_transform (template_header.transform());
    header_out.add_comment ("resliced to reference image \"" + template_header.name() + "\"");

    std::string interp ("linear");
    opt = get_options ("interp");
    if (opt.size()) 
      interp = lowercase (opt[0][0]);

    std::vector<int> oversample;
    opt = get_options ("oversample");
    if (opt.size()) {
      oversample = parse_ints (opt[0][0]);

      if (oversample.size() != 3) 
        throw Exception ("option \"oversample\" expects a vector of 3 values");

      if (oversample[0] < 1 || oversample[1] < 1 || oversample[2] < 1) 
        throw Exception ("oversample factors must be greater than zero");
    }

    if (replace) {
      header_in.get_transform().swap (T);
      T.clear();
    }

    header_out.create (argument[1]);

    Image::Voxel<float> in (header_in);
    Image::Voxel<float> out (header_out);

    if (interp == "nearest")     DataSet::Interp::reslice<DataSet::Interp::Nearest> (out, in, T, oversample); 
    else if (interp == "linear") DataSet::Interp::reslice<DataSet::Interp::Linear> (out, in, T, oversample); 
    else if (interp == "cubic")  DataSet::Interp::reslice<DataSet::Interp::Cubic> (out, in, T, oversample); 
    else assert (0);

  }
  else {
    // straight copy:
    if (T.is_set()) {
      header_out.add_comment ("transform modified");
      if (replace) 
        header_out.get_transform().swap (T);
      else {
        Math::Matrix<float> M (header_out.transform());
        Math::mult (header_out.get_transform(), T, M);
      }
    }

    header_out.create (argument[1]);
    Image::Voxel<float> in (header_in);
    Image::Voxel<float> out (header_out);
    DataSet::copy_with_progress (out, in);
  }
}

