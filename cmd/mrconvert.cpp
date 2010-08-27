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
#include "dataset/copy.h"
#include "dataset/extract.h"

using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "perform conversion between different file types and optionally extract a subset of the input image.",
  "If used correctly, this program can be a very useful workhorse. In addition to converting images between different formats, it can be used to extract specific studies from a data set, extract a specific region of interest, flip the images, or to scale the intensity of the images.",
  NULL
};

ARGUMENTS = {
  Argument ("input", "the input image.").type_image_in (),
  Argument ("ouput", "the output image.").type_image_out (),
  Argument()
};


OPTIONS = {
  Option ("coord", "extract data only at the coordinates specified.")
    .allow_multiple()
    + Argument ("axis").type_integer (0, 0, std::numeric_limits<int>::max())
    + Argument ("coord").type_sequence_int(),

  Option ("vox",
      "change the voxel dimensions. The new sizes should be provided as a "
      "comma-separated list of values. Only those values specified will be "
      "changed. For example: 1,,3.5 will change the voxel size along the x & "
      "z axes, and leave the y-axis voxel size unchanged.") 
    + Argument ("sizes").type_sequence_float(),

  Option ("datatype", "specify output image data type.")
    + Argument ("spec").type_choice (DataType::identifiers),

  Option ("stride",
      "specify the strides of the data in memory, as a commo-separated list. "
      "The actual strides produced will depend on whether the output image "
      "format can support it.")
    + Argument ("spec"),

  Option ("prs",
      "assume that the DW gradients are specified in the PRS frame (Siemens DICOM only)."),

  Option()
};





EXECUTE {

  Image::Header header_in (argument[0]);
  Image::Header header_out (header_in);
  header_out.reset_scaling();

  Options opt = get_options ("datatype");
  if (opt.size()) 
    header_out.set_datatype (opt[0][0]);

  opt = get_options ("vox");
  if (opt.size()) {
    std::vector<float> vox = parse_floats (opt[0][0]);
    for (size_t n = 0; n < vox.size(); n++) 
      if (std::isfinite (vox[n]))
        header_out.set_vox (n, vox[n]);
  }

  opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> ax = parse_ints (opt[0][0]);
    size_t i;

    for (i = 0; i < std::min (ax.size(), header_out.ndim()); i++)
      header_out.set_stride (i, ax[i]);

    for (; i < header_out.ndim(); i++)
      header_out.set_stride (i, 0);
  }


  opt = get_options ("prs");
  if (opt.size() && 
      header_out.DW_scheme().rows() &&
      header_out.DW_scheme().columns()) {
    Math::Matrix<float>& M (header_out.get_DW_scheme());
    for (size_t row = 0; row < M.rows(); ++row) {
      float tmp = M(row, 0);
      M(row, 0) = M(row, 1);
      M(row, 1) = tmp;
      M(row, 2) = -M(row, 2);
    }
  }

  std::vector<std::vector<int> > pos;

  opt = get_options ("coord");
  for (size_t n = 0; n < opt.size(); n++) {
    pos.resize (header_out.ndim());
    int axis = to<int> (opt[n][0]);
    if (pos[axis].size()) 
      throw Exception ("\"coord\" option specified twice for axis " + str (axis));
    pos[axis] = parse_ints (opt[n][1]);
  }

  assert (!header_in.is_complex());
  Image::Voxel<float> in (header_in);

  if (pos.size()) { 
    // extract specific coordinates:
    for (size_t n = 0; n < header_in.ndim(); n++) {
      if (pos[n].empty()) { 
        pos[n].resize (header_in.dim(n));
        for (size_t i = 0; i < pos[n].size(); i++) 
          pos[n][i] = i;
      }
    }

    DataSet::Extract<Image::Voxel<float> > extract (in, pos);

    for (size_t n = 0; n < extract.ndim(); ++n)
      header_out.set_dim (n, extract.dim(n));

    header_out.create (argument[1]);
    Image::Voxel<float> out (header_out);
    DataSet::copy_with_progress (out, extract);
  }
  else { 
    // straight copy:
    header_out.create (argument[1]);
    Image::Voxel<float> out (header_out);
    DataSet::copy_with_progress (out, in);
  }
}


