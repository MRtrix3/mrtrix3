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
#include "dataset/loop.h"
#include "image/voxel.h"
#include "dwi/sdeconv/constrained.h"

using namespace MR;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{
  DESCRIPTION 
    + "perform non-negativity constrained spherical deconvolution."

    + "Note that this program makes use of implied symmetries in the diffusion "
    "profile. First, the fact the signal attenuation profile is real implies "
    "that it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes "
    "the complex conjugate). Second, the diffusion profile should be "
    "antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l "
    "components should be zero. Therefore, this program only computes the even "
    "elements."

    + "Note that the spherical harmonics equations used here differ slightly "
    "from those conventionally used, in that the (-1)^m factor has been "
    "omitted. This should be taken into account in all subsequent calculations."

    + Math::SH::encoding_description;

  ARGUMENTS 
    + Argument ("dwi", 
        "the input diffusion-weighted image.").type_image_in()
    + Argument ("response", 
        "the diffusion-weighted signal response function for a single fibre population.").type_file()
    + Argument ("SH", 
        "the output spherical harmonics coefficients image.").type_image_out();

  OPTIONS 
    + DWI::GradOption
    + DWI::CSD_options;
}







typedef float value_type;

class Item
{
  public:
    Math::Vector<value_type> data;
    ssize_t pos[3];
};

class Allocator
{
  public:
    Allocator (size_t data_size) : N (data_size) { }
    Item* alloc () {
      Item* item = new Item;
      item->data.allocate (N);
      return (item);
    }
    void reset (Item* item) { }
    void dealloc (Item* item) {
      delete item;
    }
  private:
    size_t N;
};


typedef Thread::Queue<Item,Allocator> Queue;





class DataLoader
{
  public:
    DataLoader (Queue& queue,
                Image::Header& dwi_header,
                Image::Header* mask_header,
                const std::vector<int>& vec_bzeros,
                const std::vector<int>& vec_dwis,
                bool normalise_to_b0) :
      writer (queue),
      dwi (dwi_header),
      mask (mask_header),
      bzeros (vec_bzeros),
      dwis (vec_dwis),
      normalise (normalise_to_b0) { }

    void execute () {
      Queue::Writer::Item item (writer);
      DataSet::Loop loop ("performing constrained spherical deconvolution...", 0, 3);
      if (mask) {
        Image::Voxel<value_type> mask_vox (*mask);
        DataSet::check_dimensions (mask_vox, dwi, 0, 3);
        for (loop.start (mask_vox, dwi); loop.ok(); loop.next (mask_vox, dwi))
          if (mask_vox.value() > 0.5)
            load (item);
      }
      else {
        for (loop.start (dwi); loop.ok(); loop.next (dwi))
          load (item);
      }
    }

  private:
    Queue::Writer writer;
    Image::Voxel<value_type>  dwi;
    Image::Header* mask;
    const std::vector<int>&  bzeros;
    const std::vector<int>&  dwis;
    bool  normalise;

    void load (Queue::Writer::Item& item) {
      value_type norm = 0.0;
      if (normalise) {
        for (size_t n = 0; n < bzeros.size(); n++) {
          dwi[3] = bzeros[n];
          norm += dwi.value ();
        }
        norm /= bzeros.size();
      }

      for (size_t n = 0; n < dwis.size(); n++) {
        dwi[3] = dwis[n];
        item->data[n] = dwi.value();
        if (!finite (item->data[n])) return;
        if (item->data[n] < 0.0) item->data[n] = 0.0;
        if (normalise) item->data[n] /= norm;
      }

      item->pos[0] = dwi[0];
      item->pos[1] = dwi[1];
      item->pos[2] = dwi[2];

      if (!item.write()) throw Exception ("error writing to work queue");
    }
};





class Processor
{
  public:
    Processor (Queue& queue,
               Image::Header& header,
               const DWI::CSDeconv<value_type>::Shared& shared) :
      reader (queue), SH (header), sdeconv (shared) { }

    void execute () {
      Queue::Reader::Item item (reader);
      while (item.read()) {
        sdeconv.set (item->data);

        size_t n;
        for (n = 0; n < sdeconv.P.niter; n++)
          if (sdeconv.iterate())
            break;

        if (n == sdeconv.P.niter)
          info ("voxel [ " + str (item->pos[0]) + " " + str (item->pos[1]) + " " + str (item->pos[2]) +
                " ] did not reach full convergence");

        SH[0] = item->pos[0];
        SH[1] = item->pos[1];
        SH[2] = item->pos[2];

        for (SH[3] = 0; SH[3] < SH.dim (3); ++SH[3])
          SH.value() = sdeconv.FOD() [SH[3]];
      }
    }

  private:
    Queue::Reader reader;
    Image::Voxel<value_type> SH;
    DWI::CSDeconv<value_type> sdeconv;
};








void run ()
{
  Image::Header dwi_header (argument[0]);

  Image::Header* mask_header = NULL;
  Options opt = get_options ("mask");
  if (opt.size())
    mask_header = new Image::Header (opt[0][0]);

  DWI::CSDeconv<value_type>::Shared shared (dwi_header, argument[1]);

  bool normalise = get_options ("normalise").size();

  Image::Header SH_header (dwi_header);
  SH_header.set_dim (3, shared.nSH());
  SH_header.set_datatype (DataType::Float32);
  SH_header.set_stride (0, 2);
  SH_header.set_stride (1, 3);
  SH_header.set_stride (2, 4);
  SH_header.set_stride (3, 1);
  SH_header.create (argument[2]);

  Queue queue ("work queue", 100, Allocator (shared.dwis.size()));
  DataLoader loader (queue, dwi_header, mask_header, shared.bzeros, shared.dwis, normalise);

  Processor processor (queue, SH_header, shared);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}

