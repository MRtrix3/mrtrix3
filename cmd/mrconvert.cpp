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

#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_copy.h"
#include "adapter/extract.h"
#include "adapter/permute_axes.h"
#include "dwi/gradient.h"


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
  + Argument ("output", "the output image.").type_image_out ();

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

  + Option ("axes",
            "specify the axes from the input image that will be used to form the output "
            "image. This allows the permutation, ommission, or addition of axes into the "
            "output image. The axes should be supplied as a comma-separated list of axes. "
            "Any ommitted axes must have dimension 1. Axes can be inserted by supplying "
            "-1 at the corresponding position in the list.")
  + Argument ("axes").type_sequence_int()

  + Stride::Options

  + DataType::options()

  + DWI::GradImportOptions (false)
  + DWI::GradExportOptions();
}



typedef cfloat complex_type;




template <class HeaderType>
inline std::vector<int> set_header (Header& header, const HeaderType& input)
{
  header = input.header();

  header.intensity_offset() = 0.0;
  header.intensity_scale() = 1.0;

  if (get_options ("grad").size() || get_options ("fslgrad").size())
    header.set_DW_scheme (DWI::get_DW_scheme<default_type> (header));

  Options opt = get_options ("axes");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    header.set_ndim (axes.size());
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] >= static_cast<int> (input.ndim()))
        throw Exception ("axis supplied to option -axes is out of bounds");
      header.size(i) = axes[i] < 0 ? 1 : input.size (axes[i]);
    }
  }

  opt = get_options ("vox");
  if (opt.size()) {
    std::vector<default_type> vox = opt[0][0];
    if (vox.size() > header.ndim())
      throw Exception ("too many axes supplied to -vox option");
    for (size_t n = 0; n < vox.size(); ++n) {
      if (std::isfinite (vox[n]))
        header.voxsize(n) = vox[n];
    }
  }

  Stride::set_from_command_line (header);

  return axes;
}




  template <class InputImageType>
inline void copy_permute (InputImageType& in, Header& header, const std::string& output_filename)
{
  DataType datatype = header.datatype();
  std::vector<int> axes = set_header (header, in);
  header.datatype() = datatype;
  auto out = header.create (output_filename).get_image<complex_type>();
  DWI::export_grad_commandline (out.header());

  if (axes.size()) {
    Adapter::PermuteAxes<InputImageType> perm (in, axes);
    threaded_copy_with_progress (perm, out, 2);
  }
  else 
    threaded_copy_with_progress (in, out, 2);
}










void run ()
{
  auto in = Header::open (argument[0]).get_image<complex_type>();

  Header header (in.header());
  header.datatype() = DataType::from_command_line (header.datatype());

  if (in.datatype().is_complex() && !header.datatype().is_complex())
    WARN ("requested datatype is real but input datatype is complex - imaginary component will be ignored");


  Options opt = get_options ("coord");
  if (opt.size()) {
    std::vector<std::vector<int> > pos (in.ndim());
    for (size_t n = 0; n < opt.size(); n++) {
      int axis = opt[n][0];
      if (pos[axis].size())
        throw Exception ("\"coord\" option specified twice for axis " + str (axis));
      pos[axis] = parse_ints (opt[n][1], in.size(axis)-1);
      if (axis == 3 && in.header().keyval().find("dw_scheme") != in.header().keyval().end()) {
        auto grad = in.header().parse_DW_scheme<default_type>();
        if ((int)grad.rows() != in.size(3)) {
          WARN ("Diffusion encoding of input file does not match number of image volumes; omitting gradient information from output image");
          header.keyval().erase ("dw_scheme");
        }
        else {
          Math::Matrix<default_type> extract_grad (pos[3].size(), 4);
          for (size_t dir = 0; dir != pos[3].size(); ++dir)
            extract_grad.row(dir) = grad.row((pos[3])[dir]);
          header.set_DW_scheme (extract_grad);
        }
      }
    }

    for (size_t n = 0; n < in.ndim(); ++n) {
      if (pos[n].empty()) {
        pos[n].resize (in.size (n));
        for (size_t i = 0; i < pos[n].size(); i++)
          pos[n][i] = i;
      }
    }

    Adapter::Extract<decltype(in)> extract (in, pos);
    copy_permute (extract, header, argument[1]);
  }
  else
    copy_permute (in, header, argument[1]);

}

