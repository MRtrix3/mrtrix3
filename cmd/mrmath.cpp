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

#include "app.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/threaded_loop.h"
#include "image/utils.h"
#include "image/voxel.h"

#include <vector>

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* operations[] = {
  "mean",
  "sum",
  "rms",
  "var",
  "std",
  "min",
  "max",
  "absmax",
  "magmax",
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
      if (finite (val)) {
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
      if (finite (val)) 
        sum += val;
    }
    value_type result () const { 
      return sum; 
    }
    double sum;
};

class RMS {
  public:
    RMS() : sum (0.0), count (0) { }
    void operator() (value_type val) {
      if (finite (val)) {
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
      if (finite (val)) {
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
    value_type result () const { return Math::sqrt (Var::result()); }
};

class Min {
  public:
    Min () : min (std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (finite (val) && val < min) 
        min = val;
    }
    value_type result () const { return finite (min) ? min : NAN; }
    value_type min;
};

class Max {
  public:
    Max () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (finite (val) && val > max) 
        max = val;
    }
    value_type result () const { return finite (max) ? max : NAN; }
    value_type max;
};

class AbsMax {
  public:
    AbsMax () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (finite (val) && Math::abs(val) > max) 
        max = Math::abs(val);
    }
    value_type result () const { return finite (max) ? max : NAN; }
    value_type max;
};

class MagMax {
  public:
    MagMax () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) { 
      if (finite (val) && Math::abs(val) > max) 
        max = val;
    }
    value_type result () const { return finite (max) ? max : NAN; }
    value_type max;
};





template <class Operation>
class AxisKernel {
  public:
    AxisKernel (PreloadBufferType& buffer_in, BufferType& buffer_out, size_t axis) :
      in (buffer_in), out (buffer_out), axis (axis) { }
    void operator() (const Image::Iterator& pos) {
      Image::voxel_assign (in,  pos);
      Image::voxel_assign (out, pos);
      Operation op;
      for (in[axis] = 0; in[axis] < in.dim(axis); ++in[axis])
        op (in.value());
      out.value() = op.result();
    }
  protected:
    PreloadVoxelType in;
    VoxelType out;
    const size_t axis;
};





template <class Operation>
class ImageKernel {
  public:
    ImageKernel (std::vector<BufferType*>& input_buffers, BufferType& output_buffer) :
        out (output_buffer)
    {
      for (std::vector<BufferType*>::const_iterator i = input_buffers.begin(); i != input_buffers.end(); ++i)
        inputs.push_back (new VoxelType (**i));
    }
    ImageKernel (const ImageKernel& that) :
        out (that.out)
    {
      inputs.reserve (that.inputs.size());
      for (std::vector<VoxelType*>::const_iterator i = that.inputs.begin(); i != that.inputs.end(); ++i)
        inputs.push_back (new VoxelType (**i));
    }
    ~ImageKernel()
    {
      for (std::vector<VoxelType*>::iterator i = inputs.begin(); i != inputs.end(); ++i) {
        delete *i; *i = NULL;
      }
    }
    void operator() (const Image::Iterator& pos) {
      Operation op;
      for (std::vector<VoxelType*>::iterator i = inputs.begin(); i != inputs.end(); ++i) {
        Image::voxel_assign (**i, pos);
        op ((*i)->value());
      }
      Image::voxel_assign (out, pos);
      out.value() = op.result();
    }
  protected:
    std::vector<VoxelType*> inputs;
    VoxelType out;
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

    Image::Header header_out (buffer_in);
    header_out.datatype() = DataType::Float32;
    header_out.dim(axis) = 1;
    Image::squeeze_dim (header_out);

    BufferType buffer_out (output_path, header_out);

    Image::ThreadedLoop loop (std::string("computing ") + operations[op] + " along axis " + str(axis) + "...", buffer_in);

    switch (op) {
      case 0: loop.run_outer (AxisKernel<Mean>   (buffer_in, buffer_out, axis)); return;
      case 1: loop.run_outer (AxisKernel<Sum>    (buffer_in, buffer_out, axis)); return;
      case 2: loop.run_outer (AxisKernel<RMS>    (buffer_in, buffer_out, axis)); return;
      case 3: loop.run_outer (AxisKernel<Var>    (buffer_in, buffer_out, axis)); return;
      case 4: loop.run_outer (AxisKernel<Std>    (buffer_in, buffer_out, axis)); return;
      case 5: loop.run_outer (AxisKernel<Min>    (buffer_in, buffer_out, axis)); return;
      case 6: loop.run_outer (AxisKernel<Max>    (buffer_in, buffer_out, axis)); return;
      case 7: loop.run_outer (AxisKernel<AbsMax> (buffer_in, buffer_out, axis)); return;
      case 8: loop.run_outer (AxisKernel<MagMax> (buffer_in, buffer_out, axis)); return;
      default: assert (0);
    }

  } else {

    if (num_inputs < 2)
      throw Exception ("mrmath requires either multiple input images, or the -axis option to be provided");

    std::vector<BufferType*> buffers;
    for (size_t index = 0; index != num_inputs; ++index)
      buffers.push_back (new BufferType (argument[index]));

    Image::Header header_out (*buffers.front());
    header_out.datatype() = DataType::Float32;
    for (size_t index = 1; index != buffers.size(); ++index) {
      if (!Image::dimensions_match (header_out, *buffers[index]))
        throw Exception ("Input images must have the same dimensions!");
    }

    BufferType buffer_out (output_path, header_out);

    Image::ThreadedLoop loop (std::string("computing ") + operations[op] + " across " + str(num_inputs) + " images...", buffer_out);

    switch (op) {
      case 0: loop.run (ImageKernel<Mean>   (buffers, buffer_out)); return;
      case 1: loop.run (ImageKernel<Sum>    (buffers, buffer_out)); return;
      case 2: loop.run (ImageKernel<RMS>    (buffers, buffer_out)); return;
      case 3: loop.run (ImageKernel<Var>    (buffers, buffer_out)); return;
      case 4: loop.run (ImageKernel<Std>    (buffers, buffer_out)); return;
      case 5: loop.run (ImageKernel<Min>    (buffers, buffer_out)); return;
      case 6: loop.run (ImageKernel<Max>    (buffers, buffer_out)); return;
      case 7: loop.run (ImageKernel<AbsMax> (buffers, buffer_out)); return;
      case 8: loop.run (ImageKernel<MagMax> (buffers, buffer_out)); return;
      default: assert (0);
    }

    while (buffers.size()) {
      delete buffers.back();
      buffers.pop_back();
    }

  }

}

