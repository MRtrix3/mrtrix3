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

//#include <atomic>

#include "command.h"
#include "debug.h"
#include "timer.h"
#include "math/rng.h"
#include "math/math.h"
#include "image/threaded_loop.h"
#include "image/threaded_copy.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/loop.h"


using namespace MR;
using namespace App;



void usage () {
  DESCRIPTION 
    + "test multi-threaded writing to binary image";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}



class FillIn {
  public:
    Math::RNG rng;
    template <class VoxType>
      inline void operator() (VoxType& vox) { vox.value() = rng.uniform() > 0.5; }
};


class Check {
  public:
    size_t& grand_total;
    size_t total;
    Check (size_t& grand_total) : grand_total (grand_total), total (0) { }
    ~Check () { grand_total += total; }

    template <class VoxType1, class VoxType2>
      inline void operator() (const VoxType1& in, const VoxType2& out) { if (in.value() != out.value()) ++total; }
};

template <typename... Types>
void test (std::tuple<Types&...>& stuff) {
  VAR (std::get<0> (stuff));
}

void run () 
{
  // Math constants:
  VAR (Math::pi);
  VAR (M_PI);

  VAR (Math::pi_2);
  VAR (M_PI_2);

  VAR (Math::pi_4);
  VAR (M_PI_4);

  VAR (Math::sqrt2);
  VAR (M_SQRT2);

  VAR (Math::sqrt1_2);
  VAR (M_SQRT1_2);


  // Check multi-threaded binary writes:

  Image::Info info;
  info.set_ndim (3);

  // use a prime number at least for the first dimension.
  // using a mulitple of 8 leads to no effect if strides are
  // identical...
  info.dim(0) = 217; 
  info.dim(1) = info.dim(2) = 513; 
  info.vox(0) = info.vox(1) = info.vox(2) = 1.0;
  info.stride(0) = 1;
  info.stride(1) = 2;
  info.stride(2) = 3;

  Image::BufferScratch<uint8_t> scratch_buffer_in (info);
  Image::BufferScratch<uint8_t>::voxel_type in (scratch_buffer_in);

  // uncomment following line to make problem far worse...
  info.stride(0) = 2; info.stride(1) = 1;
  Image::BufferScratch<bool> scratch_buffer_out (info);
  Image::BufferScratch<bool>::voxel_type out (scratch_buffer_out);

  CONSOLE ("test buffer is " + str(in.dim(0)) + " x " + str(in.dim(1)) + " x " + str(in.dim(2)) + " = " + str(Image::voxel_count (in)) + " voxels");

  Image::ThreadedLoop ("filling in test buffer...", in).run (FillIn(), in);
  Timer timer;
  Image::ThreadedLoop ("multi-threaded copy...", in)
    .run ([] (decltype(in)& in, decltype(out)& out) { out.value()=in.value(); }, in, out);
  CONSOLE ("time taken: " + str(timer.elapsed()) + "ms");
  size_t grand_total = 0;
  Image::ThreadedLoop (in).run (Check (grand_total), in, out);
  CONSOLE ("number of errors: " + str(grand_total) + " (" + str(100.0f*float(grand_total)/float(Image::voxel_count (in))) + "%)");




  // test new image looping constructs:

  {
    CONSOLE ("============= new Image::Loop ================");
    Image::ThreadedLoop (out).run (FillIn(), out);
    timer.start();
    Image::Loop loop;
    for (loop.start (std::forward_as_tuple (in, out)); loop.ok(); loop.next (std::forward_as_tuple (in, out)))
      out.value() = in.value();
    CONSOLE ("time taken: " + str(timer.elapsed()) + "ms");
    grand_total = 0;
    Image::ThreadedLoop (in).run (Check (grand_total), in, out);
    CONSOLE ("number of errors: " + str(grand_total) + " (" + str(100.0f*float(grand_total)/float(Image::voxel_count (in))) + "%)");
  }

  {
    CONSOLE ("============= new Image::Loop (using iterator) ================");
    Image::ThreadedLoop (out).run (FillIn(), out);
    timer.start();
    for (auto i = Image::Loop() (in, out); i; ++i)
      out.value() = in.value();
    CONSOLE ("time taken: " + str(timer.elapsed()) + "ms");
    grand_total = 0;
    Image::ThreadedLoop (in).run (Check (grand_total), in, out);
    CONSOLE ("number of errors: " + str(grand_total) + " (" + str(100.0f*float(grand_total)/float(Image::voxel_count (in))) + "%)");
  }


  {
    CONSOLE ("============= new Image::LoopInOrder ================");
    Image::ThreadedLoop (out).run (FillIn(), out);
    timer.start();
    Image::LoopInOrder loop (in);
    for (loop.start (std::forward_as_tuple (in, out)); loop.ok(); loop.next (std::forward_as_tuple (in, out)))
      out.value() = in.value();
    CONSOLE ("time taken: " + str(timer.elapsed()) + "ms");
    grand_total = 0;
    Image::ThreadedLoop (in).run (Check (grand_total), in, out);
    CONSOLE ("number of errors: " + str(grand_total) + " (" + str(100.0f*float(grand_total)/float(Image::voxel_count (in))) + "%)");
  }

  {
    CONSOLE ("============= new Image::LoopInOrder (using iterator) ================");
    Image::ThreadedLoop (out).run (FillIn(), out);
    timer.start();
    Image::LoopInOrder loop (in);
    for (auto i = Image::LoopInOrder(in)(in, out); i; ++i) 
      out.value() = in.value();
    CONSOLE ("time taken: " + str(timer.elapsed()) + "ms");
    grand_total = 0;
    Image::ThreadedLoop (in).run (Check (grand_total), in, out);
    CONSOLE ("number of errors: " + str(grand_total) + " (" + str(100.0f*float(grand_total)/float(Image::voxel_count (in))) + "%)");
  }

  /*const size_t num = 10000;
    uint8_t buf [num];
    for (size_t n = 0; n < num; ++n) {
    std::atomic<uint8_t>* at = reinterpret_cast<std::atomic<uint8_t>*> (buf+n);
    at->store (n);
//buf[n] = n;
}
size_t total = 0;
for (size_t n = 0; n < num; ++n) {
total += buf[n];
}
VAR (total);*/
}

