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
#include "thread/exec.h"
#include "thread/queue.h"
#include "dataset/misc.h"
#include "dataset/loop.h"
#include "image/voxel.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

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


typedef float value_type;

class Item {
  public:
    Item (size_t ndim) : pos (ndim) { }
    Array<value_type>::RefPtr data[3][3];
    std::vector<ssize_t> pos;
};

class Allocator {
  public:
    Allocator (size_t ndim) : N (ndim) { }
    Item* alloc () { Item* item = new Item (N); return (item); }
    void reset (Item* item) { }
    void dealloc (Item* item) { delete item; }
  private:
    size_t N;
};


typedef Thread::Queue<Item,Allocator> Queue;





class DataLoader {
  public:
    DataLoader (Queue& queue, const Image::Header& src_header, const std::vector<size_t>& axes) : 
      writer (queue), src (src_header), x (axes[0]), y (axes[1]), z(axes[2]) { } 

    void execute () {
      Queue::Writer::Item item (writer);
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
            item->data[j][i] = data[j][i];

        // set item position:
        for (size_t n = 0; n < src.ndim(); ++n) 
          item->pos[n] = src[n];

        // dispatch:
        if (!item.write()) throw Exception ("error writing to work queue");

        // shuffle along:
        data[0][0] = data[1][0];
        data[0][1] = data[1][1];
        data[0][2] = data[1][2];
        data[1][0] = data[2][0];
        data[1][1] = data[2][1];
        data[1][2] = data[2][2];
        data[2][0] = data[2][1] = data[2][2] = NULL; 
      }
    }

  private:
    Queue::Writer writer;
    Image::Voxel<value_type>  src;
    Array<value_type>::RefPtr data[3][3];
    const size_t x, y, z;

    void get_plane (ssize_t plane) 
    {
      src[y] += plane;
      assert (src[y] < src.dim(y));
      value_type* row;

      if (src[z] > 0) {
        --src[z];
        row = new value_type [src.dim(x)];
        for (src[x] = 0; src[x] < src.dim(x); ++src[x]) 
          row[src[x]] = src.value();
        data[plane+1][0] = row;
        ++src[z];
      }
      else data[plane+1][0] = NULL;

      row = new value_type [src.dim(x)];
      for (src[x] = 0; src[x] < src.dim(x); ++src[x]) 
        row[src[x]] = src.value();
      data[plane+1][1] = row;

      if (src[z] < src.dim(z)-1) {
        ++src[z];
        row = new value_type [src.dim(x)];
        for (src[x] = 0; src[x] < src.dim(x); ++src[x]) 
          row[src[x]] = src.value();
        data[plane+1][z] = row;
        --src[z];
      }
      else data[plane+1][2] = NULL;

      src[y] -= plane;
    }
};





class Processor {
  public:
    Processor (Queue& queue, const Image::Header& dest_header, const std::vector<size_t>& axes) : 
      reader (queue), dest (dest_header), x (axes[0]), y (axes[1]), z(axes[2]) { }

    void execute () {
      Queue::Reader::Item item (reader);
      value_type v[14];

      while (item.read()) {
        size_t nrows = 0;
        for (size_t j = 0; j < 3; ++j)
          for (size_t i = 0; i < 3; ++i)
            if (item->data[j][i]) ++nrows;

        for (size_t i = 0; i < dest.ndim(); ++i)
          dest[i] = item->pos[i];

        for (dest[x] = 0; dest[x] < dest.dim(x); ++dest[x]) {
          size_t from = dest[x] > 0 ? dest[x]-1 : 0;
          size_t to = dest[x] < dest.dim(x)-1 ? dest[x]+2 : dest.dim(x);
          size_t nc = 0;
          value_type cm = -INFINITY;
          size_t n = (to-from) * nrows;
          size_t m = n/2 + 1;

          for (size_t x = from; x < to; ++x) {
            for (size_t z = 0; z < 3; ++z) {
              for (size_t y = 0; y < 3; ++y) {
                if (!item->data[z][y]) continue;
                value_type val = item->data[z][y].get()[x];
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
            value_type t = cm = -INFINITY;
            for (size_t i = 0; i < m; ++i) {
              if (v[i] > cm) {
                t = cm;
                cm = v[i];
              }
              else if (v[i] > t) t = v[i];
            }
            cm = (cm+t)/2.0;
          }

          dest.value() = cm;
        }
      }
    }

  private:
    Queue::Reader reader;
    Image::Voxel<value_type> dest;
    const size_t x, y, z;
};



EXECUTE {
  const Image::Header source = argument[0].get_image();
  const Image::Header destination (argument[1].get_image (source));
  std::vector<size_t> axes = DataSet::Stride::order (source, 0, 3);
  /*std::vector<size_t> axes (3);
  axes[0] = 0;
  axes[1] = 1;
  axes[2] = 2;*/

  Queue queue ("work queue", 100, Allocator (source.ndim()));
  DataLoader loader (queue, source, axes);
  Processor processor (queue, destination, axes);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}

