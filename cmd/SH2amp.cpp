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

#include <sstream>

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
              "cap all negative amplitudes to zero")

    + Image::Stride::StrideOption;
}

typedef float value_type;


class Item {
  public:
    Math::Matrix<value_type> data;
    ssize_t pos[2];
};



typedef Thread::Queue<Item> Queue;


class DataLoader {
  public:
  DataLoader (Queue& queue,
              Image::Buffer<value_type>& SH_data) :
              writer (queue), sh_voxel (SH_data) { }

    void execute () {
      Queue::Writer::Item item (writer);
      std::vector<size_t> out_axes(2, 1);
      out_axes[1] = 2;
      Image::LoopInOrder outer_loop (out_axes, "computing amplitudes...");
      Image::Loop row_loop (0,1);
      Image::Loop sh_loop (3,4);

      for (outer_loop.start (sh_voxel); outer_loop.ok(); outer_loop.next (sh_voxel)) {
        item->data.allocate (sh_voxel.dim(3), sh_voxel.dim(0));
        item->pos[0] = sh_voxel[1];
        item->pos[1] = sh_voxel[2];
        for (row_loop.start (sh_voxel); row_loop.ok(); row_loop.next (sh_voxel)) {
          for (sh_loop.start (sh_voxel); sh_loop.ok(); sh_loop.next (sh_voxel)) {
            item->data(sh_voxel[3], sh_voxel[0]) = sh_voxel.value();
          }
        }
        if (!item.write())
          throw Exception ("error writing to work queue");
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
        Math::Matrix<value_type> amplitudes;
        Math::mult(amplitudes, transformer.mat_SH2A(), item->data);
        amp_voxel[1] = item->pos[0];
        amp_voxel[2] = item->pos[1];
        Image::Loop row_loop (0,1);
        Image::Loop amp_loop (3,4);
        for (row_loop.start (amp_voxel); row_loop.ok(); row_loop.next (amp_voxel)) {
          for (amp_loop.start (amp_voxel); amp_loop.ok(); amp_loop.next (amp_voxel)) {
            if (nonnegative && amplitudes(amp_voxel[3], amp_voxel[2]) < 0)
              amplitudes(amp_voxel[3], amp_voxel[2]) = 0.0;
            amp_voxel.value() = amplitudes(amp_voxel[3], amp_voxel[0]);
          }
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
  } else {
    dirs.load(argument[1]);
    Math::Matrix<value_type> grad (dirs.rows(), 4);
  }
  std::stringstream dir_stream;
  for (size_t d = 0; d < dirs.rows() - 1; ++d)
    dir_stream << dirs(d,0) << "," << dirs(d,1) << "\n";
  dir_stream << dirs(dirs.rows() - 1,0) << "," << dirs(dirs.rows() - 1,1);
  amp_header.insert(std::pair<std::string, std::string> ("directions", dir_stream.str()));

  amp_header.dim(3) = dirs.rows();
  Options opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > amp_header.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n)
      amp_header.stride(n) = strides[n];
  } else {
    amp_header.stride(0) = 2;
    amp_header.stride(1) = 3;
    amp_header.stride(2) = 4;
    amp_header.stride(3) = 1;
  }

  Image::Buffer<value_type> amp_data (argument[2], amp_header);

  Queue queue ("sh2amp queue");
  DataLoader loader (queue, sh_data);
  Processor processor (queue, amp_data, dirs, Math::SH::LforN(sh_data.dim(3)), get_options("nonnegative").size());

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}
