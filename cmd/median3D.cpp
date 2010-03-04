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
    DataLoader (Queue& queue, const Image::Header& src_header) : writer (queue), src (src_header) { } 

    void execute () {
      Exec exec (*this);
      DataSet::loop1 ("median filtering...", exec, src, 1, SIZE_MAX);
    }

  private:
    Queue::Writer writer;
    Image::Voxel<value_type>  src;

    class Exec {
      public:
        Exec (DataLoader& parent) : P (parent), item (P.writer) { }
        void operator() (Image::Voxel<value_type>& D) 
        { 
          // get data:
          if (D[1] == 0) get_plane (D,0);
          if (D[1] < D.dim(1)-1) get_plane (D,1);

          // set item data:
          for (size_t j = 0; j < 3; ++j) 
            for (size_t i = 0; i < 3; ++i) 
              item->data[j][i] = data[j][i];

          // set item position:
          for (size_t n = 1; n < D.ndim(); ++n) 
            item->pos[n] = D[n];

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

      private:
        DataLoader& P;
        Queue::Writer::Item item;
        Array<value_type>::RefPtr data[3][3];

        void get_plane (Image::Voxel<value_type>& D, ssize_t plane) 
        {
          D[1] += plane;
          assert (D[1] < D.dim(1));
          value_type* row;

          if (D[2] > 0) {
            --D[2];
            row = new value_type [D.dim(0)];
            for (D[0] = 0; D[0] < D.dim(0); ++D[0]) row[D[0]] = D.value();
            data[plane+1][0] = row;
            D[2]++;
          }
          else data[plane+1][0] = NULL;

          row = new value_type [D.dim(0)];
          for (D[0] = 0; D[0] < D.dim(0); ++D[0]) row[D[0]] = D.value();
          data[plane+1][1] = row;

          if (D[2] < D.dim(2)-1) {
            ++D[2];
            row = new value_type [D.dim(0)];
            for (D[0] = 0; D[0] < D.dim(0); ++D[0]) row[D[0]] = D.value();
            data[plane+1][2] = row;
            D[2]--;
          }
          else data[plane+1][2] = NULL;

          D[1] -= plane;
        }
    };
    friend class Exec;
};





class Processor {
  public:
    Processor (Queue& queue, const Image::Header& dest_header) : reader (queue), dest (dest_header) { }

    void execute () {
      Queue::Reader::Item item (reader);
      value_type v[14];

      while (item.read()) {
        size_t nrows = 0;
        for (size_t j = 0; j < 3; ++j)
          for (size_t i = 0; i < 3; ++i)
            if (item->data[j][i]) ++nrows;

        for (size_t i = 1; i < dest.ndim(); ++i)
          dest[i] = item->pos[i];

        for (dest[0] = 0; dest[0] < dest.dim(0); ++dest[0]) {
          size_t from = dest[0] > 0 ? dest[0]-1 : 0;
          size_t to = dest[0] < dest.dim(0)-1 ? dest[0]+2 : dest.dim(0);
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
};



EXECUTE {
  const Image::Header source = argument[0].get_image();
  const Image::Header destination (argument[1].get_image (source));

  Queue queue ("work queue", 100, Allocator (source.ndim()));
  DataLoader loader (queue, source);
  Processor processor (queue, destination);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}

