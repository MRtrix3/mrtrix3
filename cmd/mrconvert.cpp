/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "header.h"
#include "image.h"
#include "phase_encoding.h"
#include "transform.h"
#include "algo/threaded_copy.h"
#include "adapter/extract.h"
#include "adapter/permute_axes.h"
#include "file/json_utils.h"
#include "file/ofstream.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Perform conversion between different file types and optionally "
  "extract a subset of the input image";

DESCRIPTION
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


  + OptionGroup ("Options for handling JSON (JavaScript Object Notation) files")

  + Option ("json_import", "import data from a JSON file into header key-value pairs")
    + Argument ("file").type_file_in()

  + Option ("json_export", "export data from an image header key-value pairs into a JSON file")
    + Argument ("file").type_file_out()


  + OptionGroup ("Options to modify generic header entries")

  + Option ("clear_property", 
            "remove the specified key from the image header altogether.").allow_multiple()
  + Argument ("key").type_text()

  + Option ("set_property", 
            "set the value of the specified key in the image header.").allow_multiple()
  + Argument ("key").type_text()
  + Argument ("value").type_text()

  + Option ("append_property", 
            "append the given value to the specified key in the image header (this adds the value specified as a new line in the header value).").allow_multiple()
  + Argument ("key").type_text()
  + Argument ("value").type_text()


  + Stride::Options

  + DataType::options()

  + DWI::GradImportOptions (false)
  + DWI::GradExportOptions()

  + PhaseEncoding::ImportOptions
  + PhaseEncoding::ExportOptions;
}




void permute_DW_scheme (Header& H, const vector<int>& axes)
{
  auto in = DWI::get_DW_scheme (H);
  if (!in.rows())
    return;

  Transform T (H);
  Eigen::Matrix3d permute = Eigen::Matrix3d::Zero();
  for (size_t axis = 0; axis != 3; ++axis)
    permute(axes[axis], axis) = 1.0;
  const Eigen::Matrix3d R = T.scanner2voxel.rotation() * permute * T.voxel2scanner.rotation();

  Eigen::MatrixXd out (in.rows(), in.cols());
  out.block(0, 3, out.rows(), out.cols()-3) = in.block(0, 3, in.rows(), in.cols()-3); // Copy b-values (and anything else stored in dw_scheme)
  for (int row = 0; row != in.rows(); ++row)
    out.block<1,3>(row, 0) = in.block<1,3>(row, 0) * R;

  DWI::set_DW_scheme (H, out);
}



void permute_PE_scheme (Header& H, const vector<int>& axes)
{
  auto in = PhaseEncoding::parse_scheme (H);
  if (!in.rows())
    return;

  Eigen::Matrix3d permute = Eigen::Matrix3d::Zero();
  for (size_t axis = 0; axis != 3; ++axis)
    permute(axes[axis], axis) = 1.0;

  Eigen::MatrixXd out (in.rows(), in.cols());
  out.block(0, 3, out.rows(), out.cols()-3) = in.block(0, 3, in.rows(), in.cols()-3); // Copy total readout times (and anything else stored in pe_scheme)
  for (int row = 0; row != in.rows(); ++row)
    out.block<1,3>(row, 0) = in.block<1,3>(row, 0) * permute;

  PhaseEncoding::set_scheme (H, out);
}




template <class ImageType>
inline vector<int> set_header (Header& header, const ImageType& input)
{
  header.ndim() = input.ndim();
  for (size_t n = 0; n < header.ndim(); ++n) {
    header.size(n) = input.size(n);
    header.spacing(n) = input.spacing(n);
    header.stride(n) = input.stride(n);
  }
  header.transform() = input.transform();

  auto opt = get_options ("axes");
  vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    header.ndim() = axes.size();
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] >= static_cast<int> (input.ndim()))
        throw Exception ("axis supplied to option -axes is out of bounds");
      header.size(i) = axes[i] < 0 ? 1 : input.size (axes[i]);
    }
    permute_DW_scheme (header, axes);
    permute_PE_scheme (header, axes);
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
    vector<default_type> vox = opt[0][0];
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
inline void copy_permute (Header& header_in, Header& header_out, const vector<vector<int>>& pos, const std::string& output_filename)
{

  auto in = header_in.get_image<T>();

  if (pos.empty()) {

    const auto axes = set_header (header_out, in);

    auto out = Header::create (output_filename, header_out).get_image<T>();
    DWI::export_grad_commandline (out);
    PhaseEncoding::export_commandline (out);

    auto perm = Adapter::make <Adapter::PermuteAxes> (in, axes);
    threaded_copy_with_progress (perm, out, 0, std::numeric_limits<size_t>::max(), 2);

  } else {

    auto extract = Adapter::make<Adapter::Extract> (in, pos);
    const auto axes = set_header (header_out, extract);
    auto out = Image<T>::create (output_filename, header_out);
    DWI::export_grad_commandline (out);
    PhaseEncoding::export_commandline (out);

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

  if (get_options ("import_pe_table").size() || get_options ("import_pe_eddy").size())
    PhaseEncoding::set_scheme (header_out, PhaseEncoding::get_scheme (header_in));

  auto opt = get_options ("json_import");
  if (opt.size())
    File::JSON::load (header_out, opt[0][0]);



  opt = get_options ("clear_property");
  for (size_t n = 0; n < opt.size(); ++n) {
    auto entry = header_out.keyval().find (opt[n][0]);
    if (entry == header_out.keyval().end()) 
      WARN ("No header key/value entry \"" + opt[n][0] + "\" found; ignored");
    else 
      header_out.keyval().erase (entry);
  }

  opt = get_options ("set_property");
  for (size_t n = 0; n < opt.size(); ++n) 
    header_out.keyval()[opt[n][0].as_text()] = opt[n][1].as_text();

  opt = get_options ("append_property");
  for (size_t n = 0; n < opt.size(); ++n) 
    add_line (header_out.keyval()[opt[n][0].as_text()], opt[n][1].as_text());




  opt = get_options ("coord");
  vector<vector<int>> pos;
  if (opt.size()) {
    pos.assign (header_in.ndim(), vector<int>());
    for (size_t n = 0; n < opt.size(); n++) {
      int axis = opt[n][0];
      if (axis >= (int)header_in.ndim())
        throw Exception ("axis " + str(axis) + " provided with -coord option is out of range of input image");
      if (pos[axis].size())
        throw Exception ("\"coord\" option specified twice for axis " + str (axis));
      pos[axis] = parse_ints (opt[n][1], header_in.size(axis)-1);

      auto minval = std::min_element(std::begin(pos[axis]), std::end(pos[axis]));
      if (*minval < 0)
        throw Exception ("coordinate position " + str(*minval) + " for axis " + str(axis) + " provided with -coord option is negative");
      auto maxval = std::max_element(std::begin(pos[axis]), std::end(pos[axis]));
      if (*maxval >= header_in.size(axis))
        throw Exception ("coordinate position " + str(*maxval) + " for axis " + str(axis) + " provided with -coord option is out of range of input image");

      if (axis == 3) {
        const auto grad = DWI::get_DW_scheme (header_out);
        if (grad.rows()) {
          if ((ssize_t)grad.rows() != header_in.size(3)) {
            WARN ("Diffusion encoding of input file does not match number of image volumes; omitting gradient information from output image");
            header_out.keyval().erase ("dw_scheme");
          } else {
            Eigen::MatrixXd extract_grad (pos[3].size(), grad.cols());
            for (size_t dir = 0; dir != pos[3].size(); ++dir)
              extract_grad.row (dir) = grad.row (pos[3][dir]);
            DWI::set_DW_scheme (header_out, extract_grad);
          }
        }
        Eigen::MatrixXd pe_scheme;
        try {
          pe_scheme = PhaseEncoding::parse_scheme (header_out);
          if (pe_scheme.rows()) {
            Eigen::MatrixXd extract_scheme (pos[3].size(), pe_scheme.cols());
            for (size_t vol = 0; vol != pos[3].size(); ++vol)
              extract_scheme.row (vol) = pe_scheme.row (pos[3][vol]);
            PhaseEncoding::set_scheme (header_out, extract_scheme);
          }
        } catch (...) {
          WARN ("Phase encoding scheme of input file does not match number of image volumes; omitting information from output image");
          PhaseEncoding::set_scheme (header_out, pe_scheme);
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
      vector<default_type> scaling = opt[0][0];
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


  opt = get_options ("json_export");
  if (opt.size())
    File::JSON::save (header_out, opt[0][0]);
}

