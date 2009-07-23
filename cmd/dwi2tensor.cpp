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


    28-07-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix option parsing to allow multiple ignoreslices and ignorestudies instances
    
*/

#include "app.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "math/matrix.h"
#include "math/least_squares.h"
#include "dwi/gradient.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
 "convert diffusion-weighted images to tensor images.",
  NULL
};

ARGUMENTS = {
  Argument ("dwi", "input DW image", "the input diffusion-weighted image.").type_image_in (),
  Argument ("tensor", "output tensor image", "the output diffusion tensor image.").type_image_out (),
  Argument::End
};


OPTIONS = {
  Option ("grad", "supply gradient encoding", "specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in image header.")
    .append (Argument ("encoding", "gradient encoding", "the gradient encoding, supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units (1000 s/mm^2).").type_file ()),

  Option ("ignoreslices", "ignore image slices", "ignore the image slices specified when computing the tensor.", Optional | AllowMultiple)
    .append (Argument ("slice", "slice z coordinate", "the z coordinate of the slice to be ignored").type_integer (0, INT_MAX, 0))
    .append (Argument ("volumes", "study numbers", "the volume numbers of the slice to be ignored").type_sequence_int ()),

  Option ("ignorevolumes", "ignore image volumes", "ignore the image volumes specified when computing the tensor.", Optional | AllowMultiple)
    .append (Argument ("volumes", "volume numbers", "the volumes to be ignored").type_sequence_int ()),

  Option::End
};


EXECUTE {
  Image::Object &dwi_obj (*argument[0].get_image());
  Image::Header header (dwi_obj);

  if (header.axes.size() != 4) 
    throw Exception ("dwi image should contain 4 dimensions");

  Math::Matrix<float> grad, bmat, binv;

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
  DWI::grad2bmatrix (bmat, grad);

  grad.allocate (bmat.rows(), bmat.columns());
  binv.allocate (bmat.columns(), bmat.rows());



  std::vector<std::vector<int> > islc (header.axes[2].dim);
  std::vector<int> ivol;

  opt = get_options (1);
  for (uint n = 0; n < opt.size(); n++) {
    int z = opt[n][0].get_int();
    if (z >= (int) islc.size()) throw Exception ("slice number out of bounds");
    islc[z] = parse_ints (opt[n][1].get_string());
  }

  opt = get_options (2);
  for (uint n = 0; n < opt.size(); n++) {
    std::vector<int> v = parse_ints (opt[n][0].get_string());
    ivol.insert (ivol.end(), v.begin(), v.end());
  }


  header.axes[3].dim = 6;
  header.data_type = DataType::Float32;
  header.DW_scheme.clear();

  Image::Voxel dwi (dwi_obj);
  Image::Voxel dt (*argument[1].get_image (header));

  info ("converting base image \"" + dwi.name() + " to tensor image \"" + dt.name() + "\"");

  dwi.image.map();
  dt.image.map();

  ProgressBar::init (dwi.dim(0)*dwi.dim(1)*dwi.dim(2), "converting DW images to tensor image..."); 

  float d[dwi.dim(3)];
  for (dwi[2] = dt[2] = 0; dwi[2] < dwi.dim(2); dwi[2]++, dt[2]++) {

    grad.view() = bmat.view();
    for (uint i = 0; i < ivol.size(); i++)
      for (int j = 0; j < 7; j++)
        grad (ivol[i],j) = 0.0;

    for (uint i = 0; i < islc[dt[2]].size(); i++)
      for (int j = 0; j < 7; j++)
        grad (islc[dt[2]][i],j) = 0.0;

    Math::pinv (binv, grad);

    for (dwi[1] = dt[1] = 0; dwi[1] < dwi.dim(1); dwi[1]++, dt[1]++) {
      for (dwi[0] = dt[0] = 0; dwi[0] < dwi.dim(0); dwi[0]++, dt[0]++) {
        for (dwi[3] = 0; dwi[3] < dwi.dim(3); dwi[3]++) {
          d[dwi[3]] = dwi.value();
          d[dwi[3]] = d[dwi[3]] > 0.0 ? -log (d[dwi[3]]) : 1e-12;
        }
        for (dt[3] = 0; dt[3] < dt.dim(3); dt[3]++) {
          float val = 0.0;
          for (int i = 0; i < dwi.dim(3); i++)
            val += (float) (binv(dt[3], i)*d[i]);
          dt.value() = val;
        }
        ProgressBar::inc();
      }
    }
  }

  ProgressBar::done();
}

