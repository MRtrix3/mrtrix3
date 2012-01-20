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
#include "image/data.h"
#include "image/voxel.h"
#include "image/axis.h"
#include "image/copy.h"
#include "image/extract.h"
#include "image/permute_axes.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "perform conversion between different file types and optionally "
  "extract a subset of the input image."

  + "If used correctly, this program can be a very useful workhorse. "
  "In addition to converting images between different formats, it can "
  "be used to extract specific studies from a data set, extract a "
  "specific region of interest, or flip the images.";

  ARGUMENTS
  + Argument ("input", "the input image.").type_image_in ()
  + Argument ("ouput", "the output image.").type_image_out ();

  OPTIONS
  + Option ("coord",
            "extract data from the input image only at the coordinates specified.")
  .allow_multiple()
  + Argument ("axis").type_integer (0, 0, std::numeric_limits<int>::max())
  + Argument ("coord").type_sequence_int()

  + Option ("vox",
            "change the voxel dimensions of the output image. The new sizes should "
            "be provided as a comma-separated list of values. Only those values "
            "specified will be changed. For example: 1,,3.5 will change the voxel "
            "size along the x & z axes, and leave the y-axis voxel size unchanged.")
  + Argument ("sizes").type_sequence_float()

  + Option ("stride",
            "specify the strides of the output data in memory, as a comma-separated list. "
            "The actual strides produced will depend on whether the output image "
            "format can support it.")
  + Argument ("spec").type_sequence_int()

  + Option ("axes",
            "specify the axes from the input image that will be used to form the output "
            "image. This allows the permutation, ommission, or addition of axes into the "
            "output image. The axes should be supplied as a comma-separated list of axes. "
            "Any ommitted axes must have dimension 1. Axes can be inserted by supplying "
            "-1 at the corresponding position in the list.")
  + Argument ("axes").type_sequence_int()

  + Option ("prs",
            "assume that the DW gradients are specified in the PRS frame (Siemens DICOM only).")

  + DataType::options();
}



template <class Set>
inline void set_header_out (
  Image::Header& header_out,
  const Set& S,
  const std::vector<int>& axes,
  const std::vector<float>& vox,
  const std::vector<int>& strides)
{
  header_out.set_transform (S.transform());

  if (axes.size()) {
    header_out.set_ndim (axes.size());
    for (size_t n = 0; n < header_out.ndim(); ++n)
      header_out.set_dim (n, axes[n]<0 ? 1 : S.dim (axes[n]));
  }
  else
    for (size_t n = 0; n < header_out.ndim(); ++n)
      header_out.set_dim (n, S.dim (n));

  for (size_t n = 0; n < header_out.ndim(); ++n) {
    if (n < vox.size())
      if (std::isfinite (vox[n]))
        header_out.set_vox (n, vox[n]);
    if (strides.size())
      header_out.set_stride (n, (n < strides.size() ? strides[n] : 0));
  }
}



void run ()
{

  Image::Header header_in (argument[0]);
  Image::Header header_out (header_in);
  header_out.reset_scaling();

  header_out.set_datatype_from_command_line ();

  Options opt = get_options ("vox");
  std::vector<float> vox;
  if (opt.size())
    vox = opt[0][0];


  opt = get_options ("stride");
  std::vector<int> strides;
  if (opt.size())
    strides = opt[0][0];

  opt = get_options ("axes");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    for (size_t i = 0; i < axes.size(); ++i)
      if (axes[i] >= static_cast<int> (header_in.ndim()))
        throw Exception ("axis supplied to option -axes is out of bounds");
  }



  opt = get_options ("prs");
  if (opt.size() &&
      header_out.DW_scheme().rows() &&
      header_out.DW_scheme().columns()) {
    Math::Matrix<float>& M (header_out.get_DW_scheme());
    for (size_t row = 0; row < M.rows(); ++row) {
      float tmp = M (row, 0);
      M (row, 0) = M (row, 1);
      M (row, 1) = tmp;
      M (row, 2) = -M (row, 2);
    }
  }

  std::vector<std::vector<int> > pos;

  opt = get_options ("coord");
  for (size_t n = 0; n < opt.size(); n++) {
    pos.resize (header_in.ndim());
    int axis = opt[n][0];
    if (pos[axis].size())
      throw Exception ("\"coord\" option specified twice for axis " + str (axis));
    pos[axis] = opt[n][1];
  }

  assert (!header_in.is_complex());
  Image::Data<float> in_data (header_in);
  Image::Data<float>::voxel_type in (in_data);

  if (pos.size()) {

    // extract specific coordinates:
    for (size_t n = 0; n < header_in.ndim(); n++) {
      if (pos[n].empty()) {
        pos[n].resize (header_in.dim (n));
        for (size_t i = 0; i < pos[n].size(); i++)
          pos[n][i] = i;
      }
    }
    Image::Extract<Image::Data<float>::voxel_type > extract (in, pos);

    set_header_out (header_out, extract, axes, vox, strides);
    header_out.create (argument[1]);
    Image::Data<float> data_out (header_out);
    Image::Data<float>::voxel_type  out (data_out);

    if (axes.size()) {
      Image::PermuteAxes<Image::Extract<Image::Data<float>::voxel_type > > perm (extract, axes);
      Image::copy_with_progress (out, perm);
    }
    else
      Image::copy_with_progress (out, extract);
  }
  else {
    // straight copy:
    set_header_out (header_out, in, axes, vox, strides);
    header_out.create (argument[1]);
    Image::Data<float> data_out (header_out);
    Image::Data<float>::voxel_type out (data_out);
    if (axes.size()) {
      Image::PermuteAxes<Image::Data<float>::voxel_type > perm (in, axes);
      Image::copy_with_progress (out, perm);
    }
    else
      Image::copy_with_progress (out, in);
  }
}


