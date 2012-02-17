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
#include "image/loop.h"
#include "image/buffer.h"
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



typedef Thread::Queue<Item> Queue;


class DataLoader {
  public:
  DataLoader (Queue& queue,
              Image::Buffer<value_type>& SH_data) :
              writer (queue), sh_voxel (SH_data) { }

    void execute () {
      Queue::Writer::Item item (writer);

      Image::Loop outer ("computing amplitudes...", 0, 3 );
      Image::Loop inner (3);

      for (outer.start (sh_voxel); outer.ok(); outer.next (sh_voxel)) {
        item->data.allocate (sh_voxel.dim(3));
        item->pos[0] = sh_voxel[0];
        item->pos[1] = sh_voxel[1];
        item->pos[2] = sh_voxel[2];
        for (inner.start (sh_voxel); inner.ok(); inner.next (sh_voxel))
          item->data[sh_voxel[3]] = sh_voxel.value();
        if (!item.write()) throw Exception ("error writing to work queue");
      }
    }

  private:
    Queue::Writer writer;
    Image::Buffer<value_type>::voxel_type  sh_voxel;
};


class Processor {
  public:
    Processor (Queue& queue,
               Image::Buffer<value_type>& amp_data,
               Math::Matrix<value_type>& directions,
               int lmax,
               bool nonnegative) :
               reader (queue),
               amp_voxel (amp_data),
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
   Image::Buffer<value_type>::voxel_type amp_voxel;
   Math::SH::Transform<value_type> transformer;
   bool nonnegative;
};


void run ()
{
  Image::Buffer<value_type> sh_data (argument[0]);
  assert (!sh_data.datatype().is_complex());

  if (sh_data.ndim() != 4)
    throw Exception ("The input spherical harmonic image should contain 4 dimensions");

  Image::Header amp_header (sh_data);

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
    amp_header.DW_scheme() = grad_dwis;
  } else {
    dirs.load(argument[1]);
  }
  amp_header.dim(3) = dirs.rows();
  amp_header.stride(0) = 2;
  amp_header.stride(1) = 3;
  amp_header.stride(2) = 4;
  amp_header.stride(3) = 1;
  Image::Buffer<value_type> amp_data (argument[2], amp_header);


  Queue queue ("sh2amp queue");
  DataLoader loader (queue, sh_data);
  Processor processor (queue, amp_data, dirs, Math::SH::LforN(sh_data.dim(3)), get_options("nonnegative").size());

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}
