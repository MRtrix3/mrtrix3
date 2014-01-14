/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 29/08/2011

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

#include "command.h"
#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "dwi/gradient.h"
#include "math/SH.h"
#include "math/legendre.h"
#include "dwi/directions/predefined.h"

using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "compute diffusion ODFs using Q-ball imaging";

  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in()
  + Argument ("SH", "the output spherical harmonics coefficients image.").type_image_out();

  OPTIONS
  + DWI::GradOption

  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images.")
  + Argument ("order").type_integer (2, 8, 30)

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in()

  + Option ("filter",
            "the linear frequency filtering parameters (default = [ 1 1 1 1 1 ]). "
            "These should be supplied as a text file containing the filtering "
            "coefficients for each even harmonic order.")
  + Argument ("spec").type_file()

  + Option ("normalise", "min-max normalise the ODFs")

  + Option ("directions",
            "specify the directions to sample the ODF for min-max normalisation,"
            "(by default, the built-in 300 direction set is used). These should be "
            "supplied as a text file containing the [ el az ] pairs for the directions.")
  + Argument ("file").type_file();
}


typedef float value_type;

class Item
{
  public:
    Math::Vector<value_type> data;
    ssize_t pos[3];
};



typedef Thread::Queue<Item> Queue;


class DataLoader
{
  public:
    DataLoader (Queue& queue,
                Image::Buffer<value_type>& dwi_data,
                Image::Buffer<bool>* mask_data,
                const std::vector<int>& vec_bzeros,
                const std::vector<int>& vec_dwis,
                bool normalise_to_b0) :
      writer (queue),
      dwi (dwi_data),
      mask (mask_data),
      bzeros (vec_bzeros),
      dwis (vec_dwis),
      normalise (normalise_to_b0) { }

    void execute () {
      Queue::Writer::Item item (writer);
      Image::Loop loop ("estimating dODFs using Q-ball imaging...", 0, 3);
      if (mask) {
        Image::Buffer<bool>::voxel_type mask_vox (*mask);
        Image::check_dimensions (mask_vox, dwi, 0, 3);
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
    Image::Buffer<value_type>::voxel_type dwi;
    Image::Buffer<bool>* mask;
    const std::vector<int>&  bzeros;
    const std::vector<int>&  dwis;
    bool normalise;

    void load (Queue::Writer::Item& item) {

      value_type norm = 0.0;
      if (normalise) {
        for (size_t n = 0; n < bzeros.size(); n++) {
          dwi[3] = bzeros[n];
          norm += dwi.value ();
        }
        norm /= bzeros.size();
      }
      item->data.allocate (dwis.size());
      for (size_t n = 0; n < dwis.size(); n++) {
        dwi[3] = dwis[n];
        item->data[n] = dwi.value();
        if (!std::isfinite (item->data[n])) return;
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
               Math::Matrix<value_type> FRT_transform,
               Math::Matrix<value_type> normalise_transform,
               Image::Buffer<value_type>& SH_data) :
      reader (queue),
      FRT_SHT (FRT_transform),
      normalise_SHT (normalise_transform),
      SH (SH_data) { }

    void execute () {
      Queue::Reader::Item item (reader);
      while (item.read()) {

        Math::Vector<value_type> qball_SH(FRT_SHT.rows());
        Math::mult(qball_SH, FRT_SHT, item->data);

        if (normalise_SHT.rows()) {
          Math::Vector<value_type> HR_amps;
          Math::mult(HR_amps, normalise_SHT, qball_SH);
          value_type min = INFINITY, max = -INFINITY;
          for (size_t d = 0; d < HR_amps.size(); d++) {
            if (min > HR_amps[d]) min = HR_amps[d];
            if (max < HR_amps[d]) max = HR_amps[d];
          }
          max = 1.0/(max-min);
          qball_SH[0] -= min/Math::Legendre::Plm_sph(0, 0, 0.0);
          for (size_t i = 0; i < qball_SH.size(); i++) 
            qball_SH[i] *= max;
        }

        SH[0] = item->pos[0];
        SH[1] = item->pos[1];
        SH[2] = item->pos[2];

        for (SH[3] = 0; SH[3] < SH.dim (3); ++SH[3])
          SH.value() = qball_SH[SH[3]];
      }
    }

  private:
    Queue::Reader reader;
    Math::Matrix<value_type> FRT_SHT;
    Math::Matrix<value_type> normalise_SHT;
    Image::Buffer<value_type>::voxel_type SH;
    int niter;
    int lmax;
};


void run ()
{
  Image::Buffer<value_type> dwi_data (argument[0]);

  if (dwi_data.ndim() != 4)
    throw Exception ("dwi image should contain 4 dimensions");

  Math::Matrix<value_type> grad;
  Options opt = get_options ("grad");
  if (opt.size())
    grad.load (opt[0][0]);
  else {
    if (!dwi_data.DW_scheme().is_set())
      throw Exception ("no diffusion encoding found in image \"" + dwi_data.name() + "\"");
    grad = dwi_data.DW_scheme();
  }

  if (grad.rows() < 7 || grad.columns() != 4)
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  INFO ("found " + str (grad.rows()) + "x" + str (grad.columns()) + " diffusion-weighted encoding");

  if (dwi_data.dim (3) != (int) grad.rows())
    throw Exception ("number of studies in base image does not match that in encoding file");

  DWI::normalise_grad (grad);

  std::vector<int> bzeros, dwis;
  DWI::guess_DW_directions (dwis, bzeros, grad);
  INFO ("found " + str (dwis.size()) + " diffusion-weighted directions");

  Math::Matrix<value_type> DW_dirs;
  DWI::gen_direction_matrix (DW_dirs, grad, dwis);

  opt = get_options ("lmax");
  int lmax = opt.size() ? opt[0][0] : Math::SH::LforN (dwis.size());
  INFO ("calculating even spherical harmonic components up to order " + str (lmax));

  Math::Matrix<value_type> HR_dirs;
  Math::Matrix<value_type> HR_SHT;
  opt = get_options ("normalise");
  bool normalise = false;
  if (opt.size()) {
    normalise = true;
    opt = get_options ("directions");
    if (opt.size())
      HR_dirs.load (opt[0][0]);
    else
      DWI::Directions::electrostatic_repulsion_300 (HR_dirs);
    Math::SH::init_transform (HR_SHT, HR_dirs, lmax);
  }

  // set Lmax
  int i;
  for (i = 0; Math::SH::NforL(i) < dwis.size(); i += 2);
  i -= 2;
  if (lmax > i) {
    WARN ("not enough data for SH order " + str(lmax) + ", falling back to " + str(i));
    lmax = i;
  }
  INFO("setting maximum even spherical harmonic order to " + str(lmax));

  // Setup response function
  int num_RH = (lmax + 2)/2;
  Math::Vector<value_type> sigs(num_RH);
  std::vector<value_type> AL (lmax+1);
  Math::Legendre::Plm_sph<value_type>(&AL[0], lmax, 0, 0);
  for (int l = 0; l <= lmax; l += 2) sigs[l/2] = AL[l];
  Math::Vector<value_type> response(num_RH);
  Math::SH::SH2RH(response, sigs);

  opt = get_options ("filter");
  Math::Vector<value_type> filter;
  if (opt.size()) {
    filter.load (opt[0][0]);
    if (filter.size() <= response.size())
      throw Exception ("not enough filter coefficients supplied for lmax" + str(lmax));
    for (int i = 0; i <= lmax/2; i++) response[i] *= filter[i];
    INFO ("using initial filter coefficients: " + str (filter));
  }

  Math::SH::Transform<value_type> FRT_SHT(DW_dirs, lmax);
  FRT_SHT.set_filter(response);

  Ptr<Image::Buffer<bool> > mask_data;
  opt = get_options ("mask");
  if (opt.size()) 
    mask_data = new Image::Buffer<bool> (opt[0][0]);

  Image::Header SH_header (dwi_data);
  SH_header.dim(3) = Math::SH::NforL (lmax);
  SH_header.datatype() = DataType::Float32;
  SH_header.stride(0) = 2;
  SH_header.stride(1) = 3;
  SH_header.stride(2) = 4;
  SH_header.stride(3) = 1;
  Image::Buffer<value_type> SH_data (argument[1], SH_header);


  Queue queue ("work queue");
  DataLoader loader (queue, dwi_data, mask_data, bzeros, dwis, normalise);

  Processor processor (queue, FRT_SHT.mat_A2SH(), HR_SHT, SH_data);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}
