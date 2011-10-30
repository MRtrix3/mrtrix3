/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 06/07/11.

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
#include "math/SH.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "dataset/loop.h"
#include "image/voxel.h"
#include "dwi/gradient.h"

using namespace MR;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Evaluate the amplitude of an image of spherical harmonic functions "
    "along the specified directions";

  ARGUMENTS
    + Argument ("input",
                "the input image consisting of spherical harmonic (SH) "
                "coefficients.").type_image_in ()
    + Argument ("directions",
                "the list of directions along which the SH functions will "
                "be sampled, generated using the gendir command").type_file ()
    + Argument ("output",
                "the output image consisting of the amplitude of the SH "
                "functions along the specified directions.").type_image_out ();

  OPTIONS
    + Option ("gradient",
              "assume input directions are supplied as a gradient encoding file")
    + Option ("nonnegative",
              "cap all negative amplitudes to zero");
}

typedef float value_type;


class Item {
  public:
    Math::Vector<value_type> data;
    ssize_t pos[3];
};


class ItemAllocator {
  public:
    ItemAllocator (size_t data_size) : N (data_size) { }
    Item* alloc () { Item* item = new Item; item->data.allocate (N); return (item); }
    void reset (Item* item) { }
    void dealloc (Item* item) { delete item; }
  private:
    size_t N;
};


typedef Thread::Queue<Item,ItemAllocator> Queue;


class DataLoader {
  public:
  DataLoader (Queue& queue,
              Image::Header& header) :
              writer (queue), sh_voxel (header) { }

    void execute () {
      Queue::Writer::Item item (writer);

      DataSet::Loop outer ("computing amplitudes...", 0, 3 );
      for (outer.start (sh_voxel); outer.ok(); outer.next (sh_voxel)) {
        item->pos[0] = sh_voxel[0];
        item->pos[1] = sh_voxel[1];
        item->pos[2] = sh_voxel[2];
        DataSet::Loop inner (3);
        unsigned int c = 0;
        for (inner.start (sh_voxel); inner.ok(); inner.next (sh_voxel))
          item->data[c++] = sh_voxel.value();
        if (!item.write()) throw Exception ("error writing to work queue");
      }
    }

  private:
    Queue::Writer writer;
    Image::Voxel<value_type>  sh_voxel;
};


class Processor {
  public:
    Processor (Queue& queue,
               Image::Header& header,
               Math::Matrix<value_type>& directions,
               int lmax,
               bool nonnegative) :
               reader (queue),
               amp_voxel (header),
               transformer (directions, lmax),
               nonnegative (nonnegative) { }

    void execute () {
      Queue::Reader::Item item (reader);

      while ((item.read())) {
        Math::Vector<value_type> amplitudes;
        transformer.SH2A(amplitudes, item->data);

        amp_voxel[0] = item->pos[0];
        amp_voxel[1] = item->pos[1];
        amp_voxel[2] = item->pos[2];
        for (amp_voxel[3] = 0; amp_voxel[3] < amp_voxel.dim(3); ++amp_voxel[3]) {
          if (nonnegative && amplitudes[amp_voxel[3]] < 0)
            amplitudes[amp_voxel[3]] = 0.0;
          amp_voxel.value() = amplitudes[amp_voxel[3]];
        }
      }
    }

 private:
   Queue::Reader reader;
   Image::Voxel<value_type> amp_voxel;
   Math::SH::Transform<value_type> transformer;
   bool nonnegative;
};


void run ()
{
  Image::Header sh_header(argument[0]);
  assert (!sh_header.is_complex());

  if (sh_header.ndim() != 4)
    throw Exception ("The input spherical harmonic image should contain 4 dimensions");

  Image::Header amp_header (sh_header);

  Math::Matrix<value_type> dirs;
  bool gradients = get_options("gradient").size();

  if (gradients) {
    Math::Matrix<value_type> grad;
    grad.load (argument[1]);
    std::vector<int> bzeros, dwis;
    DWI::guess_DW_directions (dwis, bzeros, grad);
    DWI::gen_direction_matrix(dirs, grad, dwis);

    Math::Matrix<value_type> grad_dwis(dwis.size(), 4);
    for (unsigned int i = 0; i < dwis.size(); i++){
      grad_dwis(i,0) = grad(dwis[i],0);
      grad_dwis(i,1) = grad(dwis[i],1);
      grad_dwis(i,2) = grad(dwis[i],2);
      grad_dwis(i,3) = grad(dwis[i],3);
    }
    amp_header.set_DW_scheme(grad_dwis);
  } else {
    dirs.load(argument[1]);
  }
  amp_header.set_dim(3, dirs.rows());
  amp_header.set_stride (0, 2);
  amp_header.set_stride (1, 3);
  amp_header.set_stride (2, 4);
  amp_header.set_stride (3, 1);
  amp_header.create(argument[2]);

  Queue queue ("sh2amp queue", 100, ItemAllocator (sh_header.dim(3)));
  DataLoader loader (queue, sh_header);
  Processor processor (queue, amp_header, dirs, Math::SH::LforN(sh_header.dim(3)), get_options("nonnegative").size());

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}
