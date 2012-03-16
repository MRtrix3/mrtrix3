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
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/axis.h"
#include "image/threaded_copy.h"
#include "image/adapter/extract.h"
#include "image/adapter/permute_axes.h"

#include "dwi/gradient.h"

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

  + DataType::options() 
  
  + DWI::GradOption

  + OptionGroup ("for complex input images")
  + Option ("real", 
      "return the real part of each voxel value")
  + Option ("imag", 
      "return the imaginary part of each voxel value")
  + Option ("phase", 
      "return the phase of each voxel value")
  + Option ("magnitude", 
      "return the magnitude of each voxel value");
}






inline std::vector<int> set_header (
  Image::Header& H_out,
  const Image::Header& H_in)
{
  H_out.info() = H_in.info();

  H_out.intensity_offset() = 0.0;
  H_out.intensity_scale() = 1.0;


  Options opt = get_options ("axes");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    H_out.set_ndim (axes.size());
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] >= static_cast<int> (H_in.ndim()))
        throw Exception ("axis supplied to option -axes is out of bounds");
      H_out.dim(i) = axes[i] < 0 ? 1 : H_in.dim (axes[i]);
    }
  }

  opt = get_options ("vox");
  if (opt.size()) {
    std::vector<float> vox = opt[0][0];
    if (vox.size() > H_out.ndim())
      throw Exception ("too many axes supplied to -vox option");
    for (size_t n = 0; n < vox.size(); ++n) {
      if (std::isfinite (vox[n]))
        H_out.vox(n) = vox[n];
    }
  }

  opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > H_out.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n) 
      H_out.stride(n) = strides[n];
  }

  opt = get_options ("prs");
  if (opt.size() &&
      H_out.DW_scheme().rows() &&
      H_out.DW_scheme().columns()) {
    Math::Matrix<float>& M (H_out.DW_scheme());
    for (size_t row = 0; row < M.rows(); ++row) {
      float tmp = M (row, 0);
      M (row, 0) = M (row, 1);
      M (row, 1) = tmp;
      M (row, 2) = -M (row, 2);
    }
  }

  try {
    H_out.DW_scheme() = DWI::get_DW_scheme<float> (H_in);
  }
  catch (...) {
    inform ("No valid diffusion encoding found; ignoring");
  }

  return axes;
}



  template <typename OutputValueType, class InputVoxelType>
inline void copy_permute (const Image::Header& header_in, InputVoxelType& in, Image::Header& header_out, const std::string& output_filename)
{
  std::vector<int> axes = set_header (header_out, header_in);
  Image::Buffer<OutputValueType> buffer_out (output_filename, header_out);
  typename Image::Buffer<OutputValueType>::voxel_type out (buffer_out);

  if (axes.size()) {
    Image::Adapter::PermuteAxes<InputVoxelType> perm (in, axes);
    Image::threaded_copy_with_progress (perm, out, 2);
  }
  else
    Image::threaded_copy_with_progress (in, out, 2);
}






template <typename ValueType, class FunctionType, class VoxelType>
class VoxelValueModified : public Image::Adapter::Voxel<VoxelType> {
  public:
    VoxelValueModified (const VoxelType& parent, FunctionType function) : 
      Image::Adapter::Voxel<VoxelType> (parent),
      func (function) { }
    typedef ValueType value_type;
    value_type value () const { return func (this->parent_vox.value()); }
    FunctionType func;
};





template <typename InputValueType, typename OutputValueType, class FunctionType>
void copy_extract (const Image::Header& header_in, const std::string& output_filename, FunctionType function)
{
  Image::Buffer<InputValueType> buffer_in (header_in);
  typedef VoxelValueModified<OutputValueType, FunctionType, typename Image::Buffer<InputValueType>::voxel_type> InputVoxelType;
  InputVoxelType in (buffer_in, function);

  Image::Header header_out (header_in);
  DataType requested_datatype = DataType::from_command_line ();

  if (requested_datatype.undefined()) 
    header_out.datatype() = DataType::from<OutputValueType>();
  else {
    bool is_complex = requested_datatype.is_complex();
    bool should_be_complex = DataType::from<OutputValueType>().is_complex();
    if (is_complex && !should_be_complex)
      throw Exception ("datatype must be real");
    if (!is_complex && should_be_complex )
      throw Exception ("datatype must be complex");
    header_out.datatype() = requested_datatype;
  }


  Options opt = get_options ("coord");
  if (opt.size()) {
    std::vector<std::vector<int> > pos (buffer_in.ndim());
    for (size_t n = 0; n < opt.size(); n++) {
      int axis = opt[n][0];
      if (pos[axis].size())
        throw Exception ("\"coord\" option specified twice for axis " + str (axis));
      pos[axis] = parse_ints (opt[n][1], buffer_in.dim(axis)-1);
    }

    for (size_t n = 0; n < buffer_in.ndim(); ++n) {
      if (pos[n].empty()) {
        pos[n].resize (buffer_in.dim (n));
        for (size_t i = 0; i < pos[n].size(); i++)
          pos[n][i] = i;
      }
    }

    Image::Adapter::Extract<InputVoxelType> extract (in, pos);
    copy_permute<OutputValueType> (header_in, extract, header_out, output_filename);
  }
  else 
    copy_permute<OutputValueType> (header_in, in, header_out, output_filename);
  
}

typedef float RealValueType;
typedef cfloat ComplexValueType;


inline RealValueType func_no_op_real (RealValueType x) { return x; }
inline ComplexValueType func_no_op_complex (ComplexValueType x) { return x; }
inline RealValueType func_real (ComplexValueType x) { return x.real(); }
inline RealValueType func_imag (ComplexValueType x) { return x.imag(); }
inline RealValueType func_phase (ComplexValueType x) { return std::arg (x); }
inline RealValueType func_abs (ComplexValueType x) { return std::abs (x); }



void run ()
{
  Image::Header header_in (argument[0]);

  if (header_in.datatype().is_complex()) {
    if (get_options ("real").size()) 
      copy_extract<ComplexValueType,RealValueType> (header_in, argument[1], func_real);
    else if (get_options ("imag").size()) 
      copy_extract<ComplexValueType,RealValueType> (header_in, argument[1], func_imag);
    else if (get_options ("phase").size()) 
      copy_extract<ComplexValueType,RealValueType> (header_in, argument[1], func_phase);
    else if (get_options ("magnitude").size()) 
      copy_extract<ComplexValueType,RealValueType> (header_in, argument[1], func_abs);
    else 
      copy_extract<ComplexValueType,ComplexValueType> (header_in, argument[1], func_no_op_complex);
  }
  else 
    copy_extract<RealValueType,RealValueType> (header_in, argument[1], func_no_op_real);
}

