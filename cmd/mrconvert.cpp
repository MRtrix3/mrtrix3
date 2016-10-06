/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */


#include "command.h"
#include "header.h"
#include "image.h"
#include "transform.h"
#include "algo/threaded_copy.h"
#include "adapter/extract.h"
#include "adapter/permute_axes.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

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
  + Argument ("axis").type_integer (0)
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

  + Option ("scaling",
            "specify the data scaling parameters used to rescale the intensity values. "
            "These take the form of a comma-separated 2-vector of floating-point values, "
            "corresponding to offset & scale, with final intensity values being given by "
            "offset + scale * stored_value. "
            "By default, the values in the input image header are passed through to the "
            "output image header when writing to an integer image, and reset to 0,1 (no "
            "scaling) for floating-point and binary images. Note that his option has no "
            "effect for floating-point and binary images.")
  + Argument ("values").type_sequence_float()

  + Stride::Options

  + DataType::options()

  + DWI::GradImportOptions (false)
  + DWI::GradExportOptions();
}




void permute_DW_scheme (Header& H, const std::vector<int>& axes)
{
  auto in = DWI::get_DW_scheme (H);
  if (!in.rows())
    return;

  Transform T (H);
  Eigen::Matrix3d permute = Eigen::Matrix3d::Zero();
  for (size_t axis = 0; axis != 3; ++axis)
    permute(axes[axis], axis) = 1.0;
  const Eigen::Matrix3d R = T.scanner2voxel.rotation() * permute * T.voxel2scanner.rotation();

  Eigen::MatrixXd out (in.rows(), 4);
  out.col(3) = in.col(3); // Copy b-values
  for (int row = 0; row != in.rows(); ++row)
    out.block<1,3>(row, 0) = in.block<1,3>(row, 0) * R;

  DWI::set_DW_scheme (H, out);
}




template <class ImageType>
inline std::vector<int> set_header (Header& header, const ImageType& input)
{
  header.ndim() = input.ndim();
  for (size_t n = 0; n < header.ndim(); ++n) {
    header.size(n) = input.size(n);
    header.spacing(n) = input.spacing(n);
    header.stride(n) = input.stride(n);
  }
  header.transform() = input.transform();

  auto opt = get_options ("axes");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    header.ndim() = axes.size();
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] >= static_cast<int> (input.ndim()))
        throw Exception ("axis supplied to option -axes is out of bounds");
      header.size(i) = axes[i] < 0 ? 1 : input.size (axes[i]);
    }
    permute_DW_scheme (header, axes);
  } else {
    header.ndim() = input.ndim();
    axes.assign (input.ndim(), 0);
    for (size_t i = 0; i < axes.size(); ++i) {
      axes[i] = i;
      header.size (i) = input.size (i);
    }
  }

  opt = get_options ("vox");
  if (opt.size()) {
    std::vector<default_type> vox = opt[0][0];
    if (vox.size() > header.ndim())
      throw Exception ("too many axes supplied to -vox option");
    for (size_t n = 0; n < vox.size(); ++n) {
      if (std::isfinite (vox[n]))
        header.spacing(n) = vox[n];
    }
  }

  Stride::set_from_command_line (header);

  return axes;
}




template <typename T>
inline void copy_permute (Header& header_in, Header& header_out, const std::vector<std::vector<int>>& pos, const std::string& output_filename)
{

  auto in = header_in.get_image<T>();

  if (pos.empty()) {

    const auto axes = set_header (header_out, in);

    auto out = Header::create (output_filename, header_out).get_image<T>();
    DWI::export_grad_commandline (out);

    auto perm = Adapter::make <Adapter::PermuteAxes> (in, axes);
    threaded_copy_with_progress (perm, out, 0, std::numeric_limits<size_t>::max(), 2);

  } else {

    auto extract = Adapter::make<Adapter::Extract> (in, pos);
    const auto axes = set_header (header_out, extract);
    auto out = Image<T>::create (output_filename, header_out);
    DWI::export_grad_commandline (out);

    auto perm = Adapter::make <Adapter::PermuteAxes> (extract, axes);
    threaded_copy_with_progress (perm, out, 0, std::numeric_limits<size_t>::max(), 2);

  }

}










void run ()
{
  Header header_in = Header::open (argument[0]);

  Header header_out (header_in);
  header_out.datatype() = DataType::from_command_line (header_out.datatype());

  if (header_in.datatype().is_complex() && !header_out.datatype().is_complex())
    WARN ("requested datatype is real but input datatype is complex - imaginary component will be ignored");

  if (get_options ("grad").size() || get_options ("fslgrad").size())
    DWI::set_DW_scheme (header_out, DWI::get_DW_scheme (header_in));

  auto opt = get_options ("coord");
  std::vector<std::vector<int>> pos;
  if (opt.size()) {
    pos.assign (header_in.ndim(), std::vector<int>());
    for (size_t n = 0; n < opt.size(); n++) {
      int axis = opt[n][0];
      if (axis >= (int)header_in.ndim())
        throw Exception ("axis " + str(axis) + " provided with -coord option is out of range of input image");
      if (pos[axis].size())
        throw Exception ("\"coord\" option specified twice for axis " + str (axis));
      pos[axis] = parse_ints (opt[n][1], header_in.size(axis)-1);
      auto maxval = std::max_element(std::begin(pos[axis]), std::end(pos[axis]));
      if (*maxval >= header_in.size(axis))
        throw Exception ("coordinate position " + str(*maxval) + " for axis " + str(axis) + " provided with -coord option is out of range of input image");
      auto grad = DWI::get_DW_scheme (header_out);
      if (axis == 3 && grad.rows()) {
        if ((ssize_t)grad.rows() != header_in.size(3)) {
          WARN ("Diffusion encoding of input file does not match number of image volumes; omitting gradient information from output image");
          header_out.keyval().erase ("dw_scheme");
        }
        else {
          Eigen::MatrixXd extract_grad (pos[3].size(), grad.cols());
          for (size_t dir = 0; dir != pos[3].size(); ++dir)
            extract_grad.row (dir) = grad.row (pos[3][dir]);
          DWI::set_DW_scheme (header_out, extract_grad);
        }
      }
    }

    for (size_t n = 0; n < header_in.ndim(); ++n) {
      if (pos[n].empty()) {
        pos[n].resize (header_in.size (n));
        for (size_t i = 0; i < pos[n].size(); i++)
          pos[n][i] = i;
      }
    }
  }


  opt = get_options ("scaling");
  if (opt.size()) {
    if (header_out.datatype().is_integer()) {
      std::vector<default_type> scaling = opt[0][0];
      if (scaling.size() != 2)
        throw Exception ("-scaling option expects comma-separated 2-vector of floating-point values");
      header_out.intensity_offset() = scaling[0];
      header_out.intensity_scale()  = scaling[1];
    }
    else
      WARN ("-scaling option has no effect for floating-point or binary images");
  }


  if (header_out.intensity_offset() == 0.0 && header_out.intensity_scale() == 1.0 && !header_out.datatype().is_floating_point()) {
    switch (header_out.datatype()() & DataType::Type) {
      case DataType::Bit:
      case DataType::UInt8:
      case DataType::UInt16:
      case DataType::UInt32:
        if (header_out.datatype().is_signed())
          copy_permute<int32_t> (header_in, header_out, pos, argument[1]);
        else
          copy_permute<uint32_t> (header_in, header_out, pos, argument[1]);
        break;
      case DataType::UInt64:
        if (header_out.datatype().is_signed())
          copy_permute<int64_t> (header_in, header_out, pos, argument[1]);
        else
          copy_permute<uint64_t> (header_in, header_out, pos, argument[1]);
        break;
      case DataType::Undefined: throw Exception ("invalid output image data type"); break;

    }
  }
  else {
    if (header_out.datatype().is_complex())
      copy_permute<cdouble> (header_in, header_out, pos, argument[1]);
    else
      copy_permute<double> (header_in, header_out, pos, argument[1]);
  }

}

