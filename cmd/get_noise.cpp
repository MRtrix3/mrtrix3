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
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/threaded_loop.h"
#include "dwi/gradient.h"
#include "math/matrix.h"
#include "math/least_squares.h"
#include "math/SH.h"

using namespace MR;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{
  DESCRIPTION
  + "estimate noise level voxel-wise using residuals from a truncated SH fit";

  ARGUMENTS
  + Argument ("dwi",
              "the input diffusion-weighted image.")
  .type_image_in ()

  + Argument ("noise",
              "the output noise map")
  .type_image_out ();



  OPTIONS
  + Option ("lmax",
            "set the maximum harmonic order for the output series. By default, the "
            "program will use the highest possible lmax given the number of "
            "diffusion-weighted images up to lmax = 8.")
  +   Argument ("order").type_integer (0, 8, 8)

  + DWI::GradOption;

}





typedef float value_type;
typedef Image::Buffer<value_type> InputBufferType; 
typedef Image::Buffer<value_type> OutputBufferType; 

class NoiseEstimator {
  public:
    NoiseEstimator (InputBufferType::voxel_type& dwi, OutputBufferType::voxel_type& noise, int axis) :
      dwi (dwi),
      noise (noise),
      axis (axis) {
        Math::Matrix<value_type> grad = DWI::get_valid_DW_scheme<value_type> (dwi.buffer());
        DWI::normalise_grad (grad);
        DWI::guess_DW_directions (dwis, bzeros, grad);

        App::Options opt = App::get_options ("lmax");
        int lmax = opt.size() ? to<int> (opt[0][0]) : Math::SH::LforN (dwis.size());
        if (lmax > 8)
          lmax = 8;

        Math::Matrix<value_type> dirs;
        DWI::gen_direction_matrix (dirs, grad, dwis);

        Math::Matrix<value_type> SH;
        Math::SH::init_transform (SH, dirs, lmax);

        Math::Matrix<value_type> iSH (SH.columns(), SH.rows());
        Math::pinv (iSH, SH);

        H.allocate (dwis.size(), dwis.size());
        Math::mult (H, SH, iSH);

        S.allocate (H.columns(), dwi.dim(axis));
        R.allocate (S);

        leverage.allocate (H.rows());
        for (size_t n = 0; n < leverage.size(); ++n) 
          leverage[n] = H(n,n) < 1.0 ? 1.0 / Math::sqrt (1.0 - H(n,n)) : 1.0;
      }

    void operator () (const Image::Iterator& pos) {
      Image::voxel_assign (dwi, pos);
      for (dwi[axis] = 0; dwi[axis] < dwi.dim(axis); ++dwi[axis]) {
        for (size_t n = 0; n < dwis.size(); ++n) {
          dwi[3] = dwis[n];
          S(n,dwi[axis]) = dwi.value();
        }
      }

      // S.save ("S.txt");
      // H.save ("H.txt");
      Math::mult (R, CblasLeft, value_type(0.0), value_type(1.0), CblasUpper, H, S);
      // R.save ("R.txt");

      // PAUSE;

      Image::voxel_assign (noise, pos);
      for (noise[axis] = 0; noise[axis] < noise.dim(axis); ++noise[axis]) {
        value_type MSE = 0.0;
        for (size_t n = 0; n < R.rows(); ++n) 
          MSE += Math::pow2 ((S(n,noise[axis]) - R(n,noise[axis])) * leverage[n]);
        MSE /= R.rows();
        noise.value() = Math::sqrt (MSE);
      }
    }

  protected:
    InputBufferType::voxel_type dwi;
    OutputBufferType::voxel_type noise;
    Math::Matrix<value_type> H, S, R;
    Math::Vector<value_type> leverage;
    int axis;
    std::vector<int> dwis, bzeros;
};




void run ()
{
  InputBufferType dwi_buffer (argument[0]);

  Image::Header header (dwi_buffer);
  header.set_ndim (3);
  header.datatype() = DataType::Float32;
  OutputBufferType noise_buffer (argument[1], header);

  InputBufferType::voxel_type dwi (dwi_buffer);
  OutputBufferType::voxel_type noise (noise_buffer);

  Image::ThreadedLoop loop ("estimating noise level...", dwi, 1, 0, 3);
  NoiseEstimator estimator (dwi, noise, loop.inner_axes()[0]);

  loop.run_outer (estimator);
}


