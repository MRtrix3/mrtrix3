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
#include "progressbar.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/loop.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* operations[] = {
  "mean",
  "sum",
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
  + "compute summary statistic (e.g. mean, min, max, ...) on image intensities along specified axis.";

  ARGUMENTS
  + Argument ("input", "the input image.").type_image_in ()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("axis", "the axis along which to compute summary statistic.").type_integer (0)
  + Argument ("output", "the output image.").type_image_out ();
}


typedef float value_type;

typedef Image::BufferPreload<value_type> InputBufferType;
typedef InputBufferType::voxel_type InputVoxelType;

typedef Image::Buffer<value_type> OutputBufferType;
typedef OutputBufferType::voxel_type OutputVoxelType;

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
      return sum_sqr / (count-1) - Math::pow2 (sum / (count-1)); 
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
class Kernel {
  public:
    Kernel (InputVoxelType& in, OutputVoxelType& out, size_t axis) :
      in (in), out (out), axis (axis) { }
    void operator() (const Image::Iterator& pos) {
      Image::voxel_assign (in, pos);
      Image::voxel_assign (out, pos);
      Operation op;
      for (in[axis] = 0; in[axis] < in.dim(axis); ++in[axis]) {
        op (in.value());
      }
      out.value() = op.result();
    }
  protected:
    InputVoxelType in;
    OutputVoxelType out;
    const size_t axis;
};





void run ()
{
  int op = argument[1];
  size_t axis = argument[2];

  InputBufferType buffer_in (argument[0], Image::Stride::contiguous_along_axis (axis));

  Image::Header header_out (buffer_in);
  header_out.datatype() = DataType::Float32;
  header_out.dim(axis) = 1;
  Image::squeeze_dim (header_out);

  OutputBufferType buffer_out (argument[3], header_out);

  InputVoxelType vox_in (buffer_in);
  OutputVoxelType vox_out (buffer_out);


  Image::ThreadedLoop loop (std::string("computing ") + operations[op] + " along axis " + str(axis) + "...", vox_in);

  switch (op) {
    case 0: loop.run_outer (Kernel<Mean> (vox_in, vox_out, axis)); return;
    case 1: loop.run_outer (Kernel<Sum> (vox_in, vox_out, axis)); return;
    case 2: loop.run_outer (Kernel<Var> (vox_in, vox_out, axis)); return;
    case 3: loop.run_outer (Kernel<Std> (vox_in, vox_out, axis)); return;
    case 4: loop.run_outer (Kernel<Min> (vox_in, vox_out, axis)); return;
    case 5: loop.run_outer (Kernel<Max> (vox_in, vox_out, axis)); return;
    case 6: loop.run_outer (Kernel<AbsMax> (vox_in, vox_out, axis)); return;
    case 7: loop.run_outer (Kernel<MagMax> (vox_in, vox_out, axis)); return;
    default: assert (0);
  }
}

