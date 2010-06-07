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
#include "image/voxel.h"
#include "math/math.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "dwi/gradient.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "generate a white matter probability mask from the DW images.",
  NULL
};


ARGUMENTS = {
  Argument ("dwi", "input DW image", "the input diffusion-weighted image.").type_image_in (),
  Argument ("mask", "brain mask", "a binary mask of the brain.").type_image_in(),
  Argument ("prob", "white matter probability image", "the output white matter 'probability' image.").type_image_out (),
  Argument::End
};


OPTIONS = { 
  Option ("grad", "supply gradient encoding", "specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in image header.", Optional | AllowMultiple)
    .append (Argument ("encoding", "gradient encoding", "the gradient encoding, supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units (1000 s/mm^2).").type_file ()),

  Option ("margin", "image margin", "specify the width of the margin on either side of the image to be used to estimate the noise level (default = 10).")
    .append (Argument ("width", "width", "the width to use.").type_integer (1, 10, 100)),

  Option::End 
};



inline float sigmoid (float val, float range) { return (1.0 / (1.0 + exp (-val/range))); }

EXECUTE {
  Image::Object &dwi_obj (*argument[0].get_image());
  Image::Header header (dwi_obj);

  if (header.axes.size() != 4) 
    throw Exception ("dwi image should contain 4 dimensions");

  Math::Matrix<float> grad;

  std::vector<OptBase> opt = get_options (0);
  if (opt.size()) grad.load (opt[0][0].get_string());
  else {
    if (!header.DW_scheme.is_set()) 
      throw Exception ("no diffusion encoding found in image \"" + header.name + "\"");
    grad = header.DW_scheme;
  }

  if (grad.rows() < 7 || grad.columns() != 4) 
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  info ("found " + str(grad.rows()) + "x" + str(grad.columns()) + " diffusion-weighted encoding");

  if (header.axes[3].dim != (int) grad.rows()) 
    throw Exception ("number of studies in base image does not match that in encoding file");

  DWI::normalise_grad (grad);
  if (grad.rows() < 7 || grad.columns() != 4) 
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  std::vector<int> bzeros, dwis;
  DWI::guess_DW_directions (dwis, bzeros, grad);
  info ("found " + str(dwis.size()) + " diffusion-weighted directions");

  Math::Matrix<float> DW_dirs;
  DWI::gen_direction_matrix (DW_dirs, grad, dwis);



  int margin = 10;
  opt = get_options (1);
  if (opt.size()) margin = opt[0][0].get_int();


  header.axes.resize (3);
  header.data_type = DataType::Float32;
  header.DW_scheme.clear();


  Image::Voxel dwi (dwi_obj);
  Image::Voxel mask (*argument[1].get_image());
  Image::Voxel prob (*argument[2].get_image (header));

  if (mask.dim(0) != dwi.dim(0) || mask.dim(1) != dwi.dim(1) || mask.dim(2) != dwi.dim(2)) 
    throw Exception ("dimensions of brain mask and dwi images do not match");

  info ("generating WM mask from DW image \"" + dwi.name() + "\n");

  float m_s_b0 = 0.0;
  float m_std_b0 = 0.0;
  float m_s_dw = 0.0;
  float m_std_dw = 0.0;
  float m_s_n = 0.0;
  float m_std_n = 0.0;

  uint num_vox = mask.dim(0)*mask.dim(1)*mask.dim(2);
  uint count_dw = 0, count_n = 0;

  dwi.image.map();
  mask.image.map();
  prob.image.map();

  ProgressBar::init (num_vox, "calibrating..."); 
  for (dwi[2] = mask[2] = 0; dwi[2] < dwi.dim(2); dwi[2]++, mask[2]++) {
    for (dwi[1] = mask[1] = 0; dwi[1] < dwi.dim(1); dwi[1]++, mask[1]++) {
      for (dwi[0] = mask[0] = 0; dwi[0] < dwi.dim(0); dwi[0]++, mask[0]++) {

        if (mask.value() > 0.5) {
          float val;
          for (uint n = 0; n < dwis.size(); n++) {
            dwi[3] = dwis[n];
            val = dwi.value();
            m_s_dw += val;
            m_std_dw += Math::pow2 (val);
          }
          for (uint n = 0; n < bzeros.size(); n++) {
            dwi[3] = bzeros[n];
            val = dwi.value();
            m_s_b0 += val;
            m_std_b0 += Math::pow2 (val);
          }
          count_dw++;
        }
        else if (dwi[0] < margin || dwi.dim(0)-dwi[0]-1 < margin) {
          float val;
          for (dwi[3] = 0; dwi[3] < dwi.dim(3); dwi[3]++) {
            val = dwi.value();
            m_s_n += val;
            m_std_n += Math::pow2 (val);
          }
          count_n++;
        }
        ProgressBar::inc();
      }
    }
  }
  ProgressBar::done();






  uint count_b0 = count_dw * bzeros.size();
  m_s_b0 /= count_b0;
  m_std_b0 = sqrt (m_std_b0/count_b0 - Math::pow2 (m_s_b0));

  count_dw *= dwis.size();
  m_s_dw /= count_dw;
  m_std_dw = sqrt (m_std_dw/count_dw - Math::pow2 (m_s_dw));

  
  count_n *= dwis.size();
  m_s_n /= count_n;
  m_std_n = sqrt (m_std_n/count_n - Math::pow2 (m_s_n));



  ProgressBar::init (num_vox, "generating WM mask from DW images..."); 

  for (dwi[2] = mask[2] = prob[2] = 0; dwi[2] < dwi.dim(2); dwi[2]++, mask[2]++, prob[2]++) {
    for (dwi[1] = mask[1] = prob[1] = 0; dwi[1] < dwi.dim(1); dwi[1]++, mask[1]++, prob[1]++) {
      for (dwi[0] = mask[0] = prob[0] = 0; dwi[0] < dwi.dim(0); dwi[0]++, mask[0]++, prob[0]++) {

        if (mask.value() > 0.5) {

          float val;
          float s_b0 = 0.0;
          float std_b0 = 0.0;
          for (uint n = 0; n < bzeros.size(); n++) {
            dwi[3] = bzeros[n];
            val = dwi.value();
            s_b0 += val;
            std_b0 += Math::pow2 (val);
          }
          s_b0 /= bzeros.size();
          std_b0 = sqrt (std_b0/bzeros.size() - Math::pow2 (s_b0));

          float s_dw = 0.0;
          float std_dw = 0.0;
          for (uint n = 0; n < dwis.size(); n++) {
            dwi[3] = dwis[n];
            val = dwi.value();
            s_dw += val;
            std_dw += Math::pow2 (val);
          }
          s_dw /= dwis.size();
          std_dw = sqrt (std_dw/dwis.size() - Math::pow2 (s_dw));

          val = sigmoid (s_dw / s_b0 - m_s_dw / m_s_b0, 0.03);
          val *= sigmoid ((s_b0 / std_b0) - 2.0, 1.0);

          prob.value() = val;
        }

        ProgressBar::inc();
      }
    }
  }

  ProgressBar::done();
}


