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
#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "dataset/misc.h"
#include "dataset/loop.h"
#include "image/voxel.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
 "smooth images using a 3x3x3 median filter.",
  NULL
};

ARGUMENTS = {
  Argument ("input", "input image", "input image to be median-filtered.").type_image_in (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End
};

OPTIONS = { Option::End };


namespace KernelOp 
{

  template <typename T> 
    class Item 
    {
      public:
        Item (size_t ndim) : pos (ndim) { }
        RefPtr<T,true> data[3][3];
        std::vector<ssize_t> pos;
    };


  template <typename T> 
    class Kernel
    {
      public:
        Kernel (const Item<T>& item, size_t x_axis, size_t y_axis, size_t z_axis) : I (item), x (x_axis), y (y_axis), z(z_axis) { }
        ssize_t from[3], to[3], offset;

        T operator() (ssize_t p[3]) const { return (I.data[p[y]+1][p[z]+1].get()[p[x]+offset]); }
        size_t count () const { return ((to[0]-from[0]) * (to[1]-from[1]) * (to[2]-from[2])); }
      private:
        const Item<T>& I;
        const size_t x, y, z;
    };



  template <typename T>
    class Allocator 
    {
      public:
        Allocator (size_t ndim) : N (ndim) { }
        Item<T>* alloc () { Item<T>* item = new Item<T> (N); return (item); }
        void reset (Item<T>* item) { }
        void dealloc (Item<T>* item) { delete item; }
      private:
        size_t N;
    };


  template <class Input> 
    class Loader 
    {
      public:
        typedef typename Input::value_type value_type;
        typedef Thread::Queue<Item<value_type>,Allocator<value_type> > Queue;

        Loader (Queue& queue, Input& input, const std::vector<size_t>& axes) : 
          writer (queue), src (input), x (axes[0]), y (axes[1]), z(axes[2]) { } 

        void execute () 
        {
          typename Queue::Writer::Item item (writer);
          std::vector<size_t> axes (src.ndim()-1);
          axes[0] = y;
          axes[1] = z;
          for (size_t n = 3; n < src.ndim(); ++n)
            axes[n-1] = n;

          DataSet::LoopInOrder loop (axes, "median filtering...");

          for (loop.start (src); loop.ok(); loop.next (src)) {
            // get data:
            if (src[y] == 0) get_plane (0);
            if (src[y] < src.dim(y)-1) get_plane (1);

            // set item data:
            for (size_t j = 0; j < 3; ++j) 
              for (size_t i = 0; i < 3; ++i) 
                set_row (item->data[j][i], data[j][i]);

            // set item position:
            for (size_t n = 0; n < src.ndim(); ++n) 
              item->pos[n] = src[n];

            // dispatch:
            if (!item.write()) throw Exception ("error writing to work queue");

            // shuffle along:
            set_row (data[0][0], data[1][0]);
            set_row (data[0][1], data[1][1]);
            set_row (data[0][2], data[1][2]);
            set_row (data[1][0], data[2][0]);
            set_row (data[1][1], data[2][1]);
            set_row (data[1][2], data[2][2]);
            release_row (data[2][0]);
            release_row (data[2][1]);
            release_row (data[2][2]); 
          }
        }

      private:
        typename Queue::Writer writer;
        Input&  src;
        RefPtr<value_type,true> data[3][3];
        const size_t x, y, z;

        std::vector<RefPtr<value_type,true> > row_buffer;

        void new_row (RefPtr<value_type,true>& row) 
        {
          if (row_buffer.size()) {
            row = row_buffer.back();
            row_buffer.pop_back();
          }
          else row = new value_type [src.dim(x)];
        }

        void set_row (RefPtr<value_type,true>& row, RefPtr<value_type,true>& original) 
        {
          if (row && row.unique()) 
            row_buffer.push_back (row);
          row = original;
        }

        void release_row (RefPtr<value_type,true>& row) 
        {
          if (row && row.unique()) 
            row_buffer.push_back (row);
          row = NULL;
        }

        void get_plane (ssize_t plane) 
        {
          src[y] += plane;
          assert (src[y] < src.dim(y));
          value_type* row;

          if (src[z] > 0) {
            --src[z];
            new_row (data[plane+1][0]);
            row = data[plane+1][0].get();
            for (src[x] = 0; src[x] < src.dim(x); ++src[x]) 
              row[src[x]] = src.value();
            ++src[z];
          }
          else release_row (data[plane+1][0]);

          new_row (data[plane+1][1]);
          row = data[plane+1][1].get();
          for (src[x] = 0; src[x] < src.dim(x); ++src[x]) 
            row[src[x]] = src.value();

          if (src[z] < src.dim(z)-1) {
            ++src[z];
            new_row (data[plane+1][2]);
            row = data[plane+1][2].get();
            for (src[x] = 0; src[x] < src.dim(x); ++src[x]) 
              row[src[x]] = src.value();
            --src[z];
          }
          else release_row (data[plane+1][2]);

          src[y] -= plane;
        }
    };





  template <class Input, class Output, class Functor> 
    class Processor 
    {
      public:
        typedef typename Input::value_type value_type;
        typedef typename Loader<Input>::Queue Queue;

        Processor (Queue& queue, Output& output, Functor functor, const std::vector<size_t>& axes) : 
          reader (queue), dest (output), x (axes[0]), y (axes[1]), z(axes[2]), func (functor) { }

        void execute () 
        {
          typename Queue::Reader::Item item (reader);

          while (item.read()) {
            for (size_t i = 0; i < dest.ndim(); ++i)
              dest[i] = item->pos[i];

            Kernel<value_type> kernel (*item, x, y, z);
            kernel.from[y] = dest[y] > 0 ? -1 : 0;
            kernel.to[y] = dest[y] < dest.dim(y)-1 ? 2 : 1;
            kernel.from[z] = dest[z] > 0 ? -1 : 0;
            kernel.to[z] = dest[z] < dest.dim(z)-1 ? 2 : 1;

            for (dest[x] = 0; dest[x] < dest.dim(x); ++dest[x]) {
              kernel.offset = dest[x];
              kernel.from[x] = dest[x] > 0 ? -1 : 0;
              kernel.to[x] = dest[x] < dest.dim(x)-1 ? 2 : 1;

              dest.value() = func (kernel);
            }
          }
        }

      private:
        typename Queue::Reader reader;
        Output dest;
        const size_t x, y, z;
        Functor func;
    };






  template <class Input, class Output, class Functor> 
    inline void run (Output& output, Input& input, Functor functor)
    {
      typename Loader<Input>::Queue queue ("work queue", 100, Allocator<typename Input::value_type> (input.ndim()));
      std::vector<size_t> axes = DataSet::Stride::order (input, 0, 3);
      Loader<Input> loader (queue, input, axes);
      Processor<Input,Output,Functor> processor (queue, output, functor, axes);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array<Processor<Input,Output,Functor> > processor_list (processor);
      Thread::Exec processor_threads (processor_list, "processor");
    }

}










template <typename T> inline T MedianFunc (const KernelOp::Kernel<T>& kernel)
{
  T v[14];
  size_t nc = 0;
  T cm = -INFINITY;
  const size_t n = kernel.count();
  const size_t m = n/2 + 1;

  ssize_t x[3];
  for (x[2] = kernel.from[2]; x[2] < kernel.to[2]; ++x[2]) {
    for (x[1] = kernel.from[1]; x[1] < kernel.to[1]; ++x[1]) {
      for (x[0] = kernel.from[0]; x[0] < kernel.to[0]; ++x[0]) {
        const T val = kernel (x);
        if (nc < m) {
          v[nc] = val;
          if (v[nc] > cm) cm = val;
          ++nc;
        }
        else if (val < cm) {
          size_t i;
          for (i = 0; v[i] != cm; ++i);
          v[i] = val;
          cm = -INFINITY;
          for (i = 0; i < m; i++)
            if (v[i] > cm) cm = v[i];
        }
      }
    }
  }

  if ((n+1) & 1) {
    T t = cm = -INFINITY;
    for (size_t i = 0; i < m; ++i) {
      if (v[i] > cm) {
        t = cm;
        cm = v[i];
      }
      else if (v[i] > t) t = v[i];
    }
    cm = (cm+t)/2.0;
  }

  return (cm);
}






EXECUTE {
  const Image::Header source = argument[0].get_image();
  const Image::Header destination (argument[1].get_image (source));

  Image::Voxel<float> src (source);
  Image::Voxel<float> dest (destination);

  KernelOp::run (dest, src, MedianFunc<float>);
}

