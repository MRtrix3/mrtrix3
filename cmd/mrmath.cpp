/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/11/09.

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
#include "ptr.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/iterator.h"
#include "image/threaded_loop.h"
#include "image/utils.h"
#include "image/voxel.h"
#include "math/math.h"

#include <limits>
#include <vector>


using namespace MR;
using namespace App;

const char* operations[] = {
  "mean",
  "sum",
  "product",
  "rms",
  "var",
  "std",
  "min",
  "max",
  "absmax", // Maximum of absolute values
  "magmax", // Value for which the magnitude is the maximum (i.e. preserves signed-ness)
  NULL
};

void usage ()
{
  DESCRIPTION
  + "compute summary statistic (e.g. mean, min, max, ...) on image intensities either across images, or along a specified axis for a single image. "
  + "See also 'mrcalc' to compute per-voxel operations.";

  ARGUMENTS
  + Argument ("input", "the input image.").type_image_in ().allow_multiple()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("axis", "perform operation along a specified axis of a single input image")
    + Argument ("index").type_integer();

}


typedef float value_type;

typedef Image::BufferPreload<value_type> PreloadBufferType;
typedef PreloadBufferType::voxel_type PreloadVoxelType;

typedef Image::Buffer<value_type> BufferType;
typedef BufferType::voxel_type VoxelType;



class Mean {
  public:
    Mean () : sum (0.0), count (0) { }
    void operator() (value_type val) { 
      if (std::isfinite (val)) {
        sum += val;
        ++count;
      }
    }
    value_type result () const { 
      if (!count)
        return NAN;
      return sum / count; 
    }
    double sum;
    size_t count;
};


class Sum {
  public:
    Sum () : sum (0.0) { }
    void operator() (value_type val) { 
      if (std::isfinite (val)) 
        sum += val;
    }
    value_type result () const { 
      return sum; 
    }
    double sum;
};


class Product {
  public:
    Product () : product (0.0) { }
    void operator() (value_type val) {
      if (std::isfinite (val))
        product += val;
    }
    value_type result () const {
      return product;
    }
    double product;
};


class RMS {
  public:
    RMS() : sum (0.0), count (0) { }
    void operator() (value_type val) {
      if (std::isfinite (val)) {
        sum += Math::pow2 (val);
        ++count;
      }
    }
    value_type result() const {
      if (!count)
        return NAN;
      return Math::sqrt(sum / count);
    }
    double sum;
    size_t count;
};


class Var {
  public:
    Var () : sum (0.0), sum_sqr (0.0), count (0) { }
    void operator() (value_type val) { 
      if (std::isfinite (val)) {
        sum += val;
        sum_sqr += Math::pow2 (val);
        ++count;
      }
    }
    value_type result () const { 
      if (count < 2) 
        return NAN;
      return  (sum_sqr - Math::pow2 (sum) / static_cast<double> (count)) / (static_cast<double> (count) - 1.0);
    }
    double sum, sum_sqr;
    size_t count;
};


class Std : public Var {
  public:
    Std() : Var() { }
    value_type result () const { return Math::sqrt (Var::result()); }
};


class Min {
  public:
    Min () : min (std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (std::isfinite (val) && val < min) 
        min = val;
    }
    value_type result () const { return std::isfinite (min) ? min : NAN; }
    value_type min;
};


class Max {
  public:
    Max () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (std::isfinite (val) && val > max) 
        max = val;
    }
    value_type result () const { return std::isfinite (max) ? max : NAN; }
    value_type max;
};


class AbsMax {
  public:
    AbsMax () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (std::isfinite (val) && Math::abs(val) > max) 
        max = Math::abs(val);
    }
    value_type result () const { return std::isfinite (max) ? max : NAN; }
    value_type max;
};

class MagMax {
  public:
    MagMax () : max (-std::numeric_limits<value_type>::infinity()) { }
    MagMax (const int i) : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (std::isfinite (val) && (!std::isfinite (max) || Math::abs(val) > Math::abs (max)))
        max = val;
    }
    value_type result () const { return std::isfinite (max) ? max : NAN; }
    value_type max;
};





template <class Operation>
class AxisKernel {
  public:
    AxisKernel (size_t axis) : axis (axis) { }

    template <class InputVoxelType, class OutputVoxelType>
      void operator() (InputVoxelType& in, OutputVoxelType& out) {
        Operation op;
        for (in[axis] = 0; in[axis] < in.dim(axis); ++in[axis])
          op (in.value());
        out.value() = op.result();
      }
  protected:
    const size_t axis;
};





class ImageKernelBase {
  public:
    ImageKernelBase (const std::string& path) :
      output_path (path) { }
    virtual ~ImageKernelBase() { }
    virtual void process (const Image::Header& image_in) = 0;
  protected:
    const std::string output_path;
};



template <class Operation>
class ImageKernel : public ImageKernelBase {
  protected:
    class InitFunctor { 
      public: 
        template <class VoxelType> 
          void operator() (VoxelType& out) const { out.value() = Operation(); } 
    };
    class ProcessFunctor { 
      public: 
        template <class VoxelType1, class VoxelType2>
          void operator() (VoxelType1& out, VoxelType2& in) const { 
            Operation op = out.value(); 
            op (in.value()); 
            out.value() = op;
          } 
    };
    class ResultFunctor {
      public: 
        template <class VoxelType1, class VoxelType2>
          void operator() (VoxelType1& out, VoxelType2& in) const {
            Operation op = in.value(); 
            out.value() = op.result(); 
          } 
    };

  public:
    ImageKernel (const Image::Header& header, const std::string& path) :
        ImageKernelBase (path),
        header (header),
        buffer (header)
    {
      typename Image::BufferScratch<Operation>::voxel_type v_buffer (buffer);
      Image::ThreadedLoop (v_buffer).run (InitFunctor(), v_buffer);
    }

    ~ImageKernel()
    {
      Image::Buffer<value_type> out (output_path, header);
      Image::Buffer<value_type>::voxel_type v_out (out);
      typename Image::BufferScratch<Operation>::voxel_type v_buffer (buffer);
      Image::ThreadedLoop (v_buffer).run (ResultFunctor(), v_out, v_buffer);
    }

    void process (const Image::Header& image_in)
    {
      Image::Buffer<value_type> in (image_in);
      Image::Buffer<value_type>::voxel_type v_in (in);
      for (size_t axis = buffer.ndim(); axis < v_in.ndim(); ++axis)
        v_in[axis] = 0;
      typename Image::BufferScratch<Operation>::voxel_type v_buffer (buffer);
      Image::ThreadedLoop (v_buffer).run (ProcessFunctor(), v_buffer, v_in);
    }

  protected:
    const Image::Header& header;
    Image::BufferScratch<Operation> buffer;

};




void run ()
{
  const size_t num_inputs = argument.size() - 2;
  const int op = argument[num_inputs];
  const std::string& output_path = argument.back();

  Options opt = get_options ("axis");
  if (opt.size()) {

    if (num_inputs != 1)
      throw Exception ("Option -axis only applies if a single input image is used");

    const size_t axis = opt[0][0];

    PreloadBufferType buffer_in (argument[0], Image::Stride::contiguous_along_axis (axis));

    if (axis >= buffer_in.ndim())
      throw Exception ("Cannot perform operation along axis " + str (axis) + "; image only has " + str(buffer_in.ndim()) + " axes");

    Image::Header header_out (buffer_in);
    header_out.datatype() = DataType::Float32;
    header_out.dim(axis) = 1;
    Image::squeeze_dim (header_out);

    BufferType buffer_out (output_path, header_out);

    PreloadBufferType::voxel_type vox_in (buffer_in);
    BufferType::voxel_type vox_out (buffer_out);


    Image::ThreadedLoop loop (std::string("computing ") + operations[op] + " along axis " + str(axis) + "...", buffer_out);

    switch (op) {
      case 0: loop.run (AxisKernel<Mean>   (axis), vox_in, vox_out); return;
      case 1: loop.run (AxisKernel<Sum>    (axis), vox_in, vox_out); return;
      case 2: loop.run (AxisKernel<Product>(axis), vox_in, vox_out); return;
      case 3: loop.run (AxisKernel<RMS>    (axis), vox_in, vox_out); return;
      case 4: loop.run (AxisKernel<Var>    (axis), vox_in, vox_out); return;
      case 5: loop.run (AxisKernel<Std>    (axis), vox_in, vox_out); return;
      case 6: loop.run (AxisKernel<Min>    (axis), vox_in, vox_out); return;
      case 7: loop.run (AxisKernel<Max>    (axis), vox_in, vox_out); return;
      case 8: loop.run (AxisKernel<AbsMax> (axis), vox_in, vox_out); return;
      case 9: loop.run (AxisKernel<MagMax> (axis), vox_in, vox_out); return;
      default: assert (0);
    }

  } else {

    if (num_inputs < 2)
      throw Exception ("mrmath requires either multiple input images, or the -axis option to be provided");

    // Pre-load all image headers
    VecPtr<Image::Header> headers_in;

    // Header of first input image is the template to which all other input images are compared
    headers_in.push_back (new Image::Header (argument[0]));
    Image::Header header (*headers_in[0]);

    // Wipe any excess unary-dimensional axes
    while (header.dim (header.ndim() - 1) == 1)
      header.set_ndim (header.ndim() - 1);

    // Verify that dimensions of all input images adequately match
    for (size_t i = 1; i != num_inputs; ++i) {
      const std::string path = argument[i];
      headers_in.push_back (new Image::Header (path));
      const Image::Header& temp (*headers_in[i]);
      if (temp.ndim() < header.ndim())
        throw Exception ("Image " + path + " has fewer axes than first imput image " + header.name());
      for (size_t axis = 0; axis != header.ndim(); ++axis) {
        if (temp.dim(axis) != header.dim(axis))
          throw Exception ("Dimensions of image " + path + " do not match those of first input image " + header.name());
      }
      for (size_t axis = header.ndim(); axis != temp.ndim(); ++axis) {
        if (temp.dim(axis) != 1)
          throw Exception ("Image " + path + " has axis with non-unary dimension beyond first input image " + header.name());
      }
    }

    // Instantiate a kernel depending on the operation requested
    Ptr<ImageKernelBase> kernel;
    switch (op) {
      case 0: kernel = new ImageKernel<Mean>    (header, output_path); break;
      case 1: kernel = new ImageKernel<Sum>     (header, output_path); break;
      case 2: kernel = new ImageKernel<Product> (header, output_path); break;
      case 3: kernel = new ImageKernel<RMS>     (header, output_path); break;
      case 4: kernel = new ImageKernel<Var>     (header, output_path); break;
      case 5: kernel = new ImageKernel<Std>     (header, output_path); break;
      case 6: kernel = new ImageKernel<Min>     (header, output_path); break;
      case 7: kernel = new ImageKernel<Max>     (header, output_path); break;
      case 8: kernel = new ImageKernel<AbsMax>  (header, output_path); break;
      case 9: kernel = new ImageKernel<MagMax>  (header, output_path); break;
      default: assert (0);
    }

    // Feed the input images to the kernel one at a time
    {
      ProgressBar progress (std::string("computing ") + operations[op] + " across " 
          + str(headers_in.size()) + " images...", num_inputs);
      for (size_t i = 0; i != headers_in.size(); ++i) {
        kernel->process (*headers_in[i]);
        ++progress;
      }
    }

    // Image output is handled by the kernel destructor

  }

}

