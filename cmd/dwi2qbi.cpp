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
#include "image/threaded_loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
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

  REFERENCES = "Hess, C. P.; Mukherjee, P.; Han, E. T.; Xu, D. & Vigneron, D. B. "
               "Q-ball reconstruction of multimodal fiber orientations using the spherical harmonic basis. "
               "Magnetic Resonance in Medicine, 2006, 56, 104-117";

  OPTIONS
  + DWI::GradOption
  + DWI::ShellOption

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
  + Argument ("spec").type_file_in()

  + Option ("normalise", "min-max normalise the ODFs")

  + Option ("directions",
            "specify the directions to sample the ODF for min-max normalisation,"
            "(by default, the built-in 300 direction set is used). These should be "
            "supplied as a text file containing the [ el az ] pairs for the directions.")
  + Argument ("file").type_file_in();
}


typedef float value_type;

bool normalise = false;
int lmax = 8;

class DWI2QBI {
  public:
    DWI2QBI (const Math::Matrix<value_type>& FRT_SHT, Math::Matrix<value_type>& normalise_SHT, const DWI::Shells& shells) :
      FRT_SHT (FRT_SHT), 
      normalise_SHT (normalise_SHT), 
      shells (shells),
      dwi (FRT_SHT.columns()),
      qbi (FRT_SHT.rows()),
      amps (normalise ? normalise_SHT.rows() : 0) { }

    template <class VoxTypeIN, class VoxTypeOUT>
      void operator() (VoxTypeIN& in, VoxTypeOUT& out) 
      {
        value_type norm = 1.0;
        if (normalise) {
          for (size_t n = 0; n < shells.smallest().count(); n++) {
            in[3] = shells.smallest().get_volumes()[n];
            norm += in.value();
          }
          norm = shells.smallest().count() / norm;
        }

        for (size_t n = 0; n < shells.largest().count(); n++) {
          in[3] = shells.largest().get_volumes()[n];
          dwi[n] = in.value();
          if (!std::isfinite (dwi[n])) return;
          if (dwi[n] < 0.0) dwi[n] = 0.0;
          dwi[n] *= norm;
        }

        Math::mult (qbi, FRT_SHT, dwi);

        if (normalise_SHT.rows()) {
          Math::mult (amps, normalise_SHT, qbi);
          auto min = std::numeric_limits<value_type>::infinity();
          auto max = -std::numeric_limits<value_type>::infinity();
          for (size_t d = 0; d < amps.size(); d++) {
            if (min > amps[d]) min = amps[d];
            if (max < amps[d]) max = amps[d];
          }
          max = 1.0/(max-min);
          qbi[0] -= min/Math::Legendre::Plm_sph(0, 0, 0.0);
          for (size_t i = 0; i < qbi.size(); i++) 
            qbi[i] *= max;
        }

        for (auto l = Image::Loop(3) (out); l; ++l) 
          out.value() = qbi[out[3]];
      }

    template <class VoxTypeMASK, class VoxTypeIN, class VoxTypeOUT>
      void operator() (VoxTypeMASK& mask, VoxTypeIN& in, VoxTypeOUT& out) {
        if (mask.value())
          operator() (in, out);
      }

  protected:
    const Math::Matrix<value_type>& FRT_SHT;
    const Math::Matrix<value_type>& normalise_SHT;
    const DWI::Shells& shells;
    Math::Vector<value_type> dwi, qbi, amps;
};





void run ()
{
  Image::Buffer<value_type> dwi_data (argument[0]);

  if (dwi_data.ndim() != 4)
    throw Exception ("dwi image should contain 4 dimensions");

  Math::Matrix<value_type> grad = DWI::get_valid_DW_scheme<value_type> (dwi_data);
  DWI::Shells shells (grad);
  // Keep the b=0 shell (may be used for normalisation), but force single non-zero shell
  shells.select_shells (true, true);

  Math::Matrix<value_type> DW_dirs;
  DWI::gen_direction_matrix (DW_dirs, grad, shells.largest().get_volumes());

  Options opt = get_options ("lmax");
  lmax = opt.size() ? opt[0][0] : Math::SH::LforN (shells.largest().count());
  INFO ("calculating even spherical harmonic components up to order " + str (lmax));

  Math::Matrix<value_type> HR_dirs;
  Math::Matrix<value_type> HR_SHT;
  opt = get_options ("normalise");
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
  for (i = 0; Math::SH::NforL(i) < shells.largest().count(); i += 2);
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

  Image::Header qbi_header (dwi_data);
  qbi_header.dim(3) = Math::SH::NforL (lmax);
  qbi_header.datatype() = DataType::Float32;
  Image::Stride::set (qbi_header, Image::Stride::contiguous_along_axis (3, dwi_data));
  Image::Buffer<value_type> qbi_data (argument[1], qbi_header);


  opt = get_options ("mask");
  if (opt.size()) {
    Image::Buffer<bool> mask_data (opt[0][0]);
    Image::ThreadedLoop ("estimating dODFs using Q-ball imaging...", dwi_data, 0, 3)
      .run (DWI2QBI (FRT_SHT.mat_A2SH(), HR_SHT, shells), mask_data.voxel(), dwi_data.voxel(), qbi_data.voxel());
  }
  else 
    Image::ThreadedLoop ("estimating dODFs using Q-ball imaging...", dwi_data, 0, 3)
      .run (DWI2QBI (FRT_SHT.mat_A2SH(), HR_SHT, shells), dwi_data.voxel(), qbi_data.voxel());
}
