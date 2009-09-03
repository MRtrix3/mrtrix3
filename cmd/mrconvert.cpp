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
#include "image/axis.h"
#include "image/misc.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "perform conversion between different file types and optionally extract a subset of the input image.",
  "If used correctly, this program can be a very useful workhorse. In addition to converting images between different formats, it can be used to extract specific studies from a data set, extract a specific region of interest, flip the images, or to scale the intensity of the images.",
  NULL
};

ARGUMENTS = {
  Argument ("input", "input image", "the input image.").type_image_in (),
  Argument ("ouput", "output image", "the output image.").type_image_out (),
  Argument::End
};


const char* type_choices[] = { "REAL", "IMAG", "MAG", "PHASE", "COMPLEX", NULL };

OPTIONS = {
  Option ("coord", "select coordinates", "extract data only at the coordinates specified.", Optional | AllowMultiple)
    .append (Argument ("axis", "axis", "the axis of interest").type_integer (0, INT_MAX, 0))
    .append (Argument ("coord", "coordinates", "the coordinates of interest").type_sequence_int()),

  Option ("vox", "voxel size", "change the voxel dimensions.")
    .append (Argument ("sizes", "new dimensions", "A comma-separated list of values. Only those values specified will be changed. For example: 1,,3.5 will change the voxel size along the x & z axes, and leave the y-axis voxel size unchanged.")
        .type_sequence_float ()),

  Option ("datatype", "data type", "specify output image data type.")
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option ("scale", "scaling factor", "apply scaling to the intensity values.")
    .append (Argument ("factor", "factor", "the factor by which to multiply the intensities.").type_float (NAN, NAN, 1.0)),

  Option ("offset", "offset", "apply offset to the intensity values.")
    .append (Argument ("bias", "bias", "the value of the offset.").type_float (NAN, NAN, 0.0)),

  Option ("zero", "replace NaN by zero", "replace all NaN values with zero."),

  Option ("output", "output type", "specify type of output")
    .append (Argument ("type", "type", "type of output.")
        .type_choice (type_choices)),

  Option ("layout", "data layout", "specify the layout of the data in memory. The actual layout produced will depend on whether the output image format can support it.")
    .append (Argument ("spec", "specifier", "the data layout specifier.").type_string ()),

  Option ("prs", "DW gradient specified as PRS", "assume that the DW gradients are specified in the PRS frame (Siemens DICOM only)."),

  Option::End
};



inline bool next (Image::Voxel& ref, Image::Voxel& other, const std::vector<int>* pos)
{
  size_t axis = 0;
  do {
    ref.inc(axis);
    other.pos (axis, pos[axis][ref.pos(axis)]);
    if (ref.pos(axis) < ref.dim(axis)) return (true);
    ref.pos(axis, 0);
    other.pos (axis, pos[axis][0]);
    axis++;
  } while (axis < ref.ndim());
  return (false);
}





EXECUTE {
  std::vector<OptBase> opt = get_options (1); // vox
  std::vector<float> vox;
  if (opt.size()) 
    vox = parse_floats (opt[0][0].get_string());

  opt = get_options (3); // scale
  float scale = 1.0;
  if (opt.size()) scale = opt[0][0].get_float();

  opt = get_options (4); // offset
  float offset = 0.0;
  if (opt.size()) offset = opt[0][0].get_float();

  opt = get_options (5); // zero
  bool replace_NaN = opt.size();

  opt = get_options (6); // output
  Image::OutputType output_type = Image::Default;
  if (opt.size()) {
    switch (opt[0][0].get_int()) {
      case 0: output_type = Image::Real; break;
      case 1: output_type = Image::Imaginary; break;
      case 2: output_type = Image::Magnitude; break;
      case 3: output_type = Image::Phase; break;
      case 4: output_type = Image::RealImag; break;
    }
  }




  const Image::Header header_in = argument[0].get_image ();
  Image::Header header (header_in);

  if (output_type == 0) {
    if (header_in.is_complex()) output_type = Image::RealImag;
    else output_type = Image::Default;
  }

  if (output_type == Image::RealImag) header.datatype() = DataType::CFloat32;
  else if (output_type == Image::Phase) header.datatype() = DataType::Float32;
  else header.datatype().unset_flag (DataType::Complex);

  
  opt = get_options (2); // datatype
  if (opt.size()) header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);

  for (size_t n = 0; n < vox.size(); n++) 
    if (isfinite (vox[n])) header.axes.vox(n) = vox[n];

  opt = get_options (7); // layout
  if (opt.size()) {
    std::vector<Image::Axes::Order> ax = parse_axes_specifier (header.axes, opt[0][0].get_string());
    if (ax.size() != header.axes.ndim()) 
      throw Exception (std::string("specified layout \"") + opt[0][0].get_string() + "\" does not match image dimensions");

    for (size_t i = 0; i < ax.size(); i++) {
      header.axes.order(i) = ax[i].order;
      header.axes.forward(i) = ax[i].forward;
    }
  }


  opt = get_options (8); // prs
  if (opt.size() && header.DW_scheme.rows() && header.DW_scheme.columns()) {
    for (size_t row = 0; row < header.DW_scheme.rows(); row++) {
      double tmp = header.DW_scheme(row, 0);
      header.DW_scheme(row, 0) = header.DW_scheme(row, 1);
      header.DW_scheme(row, 1) = tmp;
      header.DW_scheme(row, 2) = -header.DW_scheme(row, 2);
    }
  }

  std::vector<int> pos[header_in.ndim()];

  opt = get_options (0); // coord
  for (size_t n = 0; n < opt.size(); n++) {
    int axis = opt[n][0].get_int();
    if (pos[axis].size()) throw Exception ("\"coord\" option specified twice for axis " + str (axis));
    pos[axis] = parse_ints (opt[n][1].get_string());
    header.axes.dim(axis) = pos[axis].size();
  }

  for (size_t n = 0; n < header_in.ndim(); n++) {
    if (pos[n].empty()) { 
      pos[n].resize (header_in.dim(n));
      for (size_t i = 0; i < pos[n].size(); i++) pos[n][i] = i;
    }
  }


  header.apply_scaling (scale, offset);






  Image::Voxel in (header_in);
  assert (!header_in.datatype().is_complex());

  const Image::Header header_out = argument[1].get_image (header);
  Image::Voxel out (header_out);

  for (size_t n = 0; n < in.ndim(); n++) in.pos (n, pos[n][0]);

  ProgressBar::init (voxel_count (out), "copying data...");

  do { 
    float val = in.get();
    if (replace_NaN) if (isnan (val)) val = 0.0;
    out.set (val);
    ProgressBar::inc();
  } while (next (out, in, pos));

  ProgressBar::done();
}





