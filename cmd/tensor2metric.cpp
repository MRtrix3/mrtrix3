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

#include "command.h"
#include "progressbar.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "math/matrix.h"
#include "math/eigen.h"
#include "dwi/tensor.h"
#include "image/value.h"
#include "image/position.h"


using namespace MR;
using namespace App;

const char* modulate_choices[] = { "none", "fa", "eval", NULL };

void usage ()
{
  DESCRIPTION
  + "generate maps of tensor-derived parameters.";

  REFERENCES 
    + "Basser, P. J.; Mattiello, J. & Lebihan, D. "
    "MR diffusion tensor spectroscopy and imaging. "
    "Biophysical Journal, 1994, 66, 259-267";

  ARGUMENTS
  + Argument ("tensor", "the input diffusion tensor image.").type_image_in ();

  OPTIONS
  + Option ("adc",
            "compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor.")
  + Argument ("image").type_image_out()

  + Option ("fa",
            "compute the fractional anisotropy of the diffusion tensor.")
  + Argument ("image").type_image_out()

  + Option ("num",
            "specify the desired eigenvalue/eigenvector(s). Note that several eigenvalues "
            "can be specified as a number sequence. For example, '1,3' specifies the "
            "major (1) and minor (3) eigenvalues/eigenvectors (default = 1).")
  + Argument ("image").type_sequence_int()

  + Option ("vector",
            "compute the selected eigenvector(s) of the diffusion tensor.")
  + Argument ("image").type_image_out()

  + Option ("value",
            "compute the selected eigenvalue(s) of the diffusion tensor.")
  + Argument ("image").type_image_out()

  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in()

  + Option ("modulate",
            "specify how to modulate the magnitude of the eigenvectors. Valid choices "
            "are: none, FA, eval (default = FA).")
  + Argument ("spec").type_choice (modulate_choices);
}





typedef float value_type;

class ImagePair
{
  public:
    typedef ::value_type value_type;

    ImagePair (const Image::Header& header, const std::string& name, size_t nvols) :
      data (name, param (header, nvols)), 
      vox (data) { }

    ImagePair (const std::string& name) : 
      data (name), 
      vox (data) { }

    Image::Buffer<value_type> data;
    Image::Buffer<value_type>::voxel_type vox;

    static Image::Header param (const Image::Header& header, size_t nvols) {
      Image::Header ret (header);
      ret.datatype() = DataType::Float32;
      if (nvols) ret.dim(3) = nvols;
      else ret.set_ndim (3);
      return ret;
    }
};


inline void set_zero (size_t axis, 
    std::unique_ptr<ImagePair>& i0, 
    std::unique_ptr<ImagePair>& i1, 
    std::unique_ptr<ImagePair>& i2, 
    std::unique_ptr<ImagePair>& i3, 
    std::unique_ptr<ImagePair>& i4)
{
  if (i0) i0->vox[axis] = 0;
  if (i1) i1->vox[axis] = 0;
  if (i2) i2->vox[axis] = 0;
  if (i3) i3->vox[axis] = 0;
  if (i4) i4->vox[axis] = 0;
}

inline void increment (size_t axis, 
    std::unique_ptr<ImagePair>& i0, 
    std::unique_ptr<ImagePair>& i1, 
    std::unique_ptr<ImagePair>& i2, 
    std::unique_ptr<ImagePair>& i3, 
    std::unique_ptr<ImagePair>& i4)
{
  if (i0) ++i0->vox[axis];
  if (i1) ++i1->vox[axis];
  if (i2) ++i2->vox[axis];
  if (i3) ++i3->vox[axis];
  if (i4) ++i4->vox[axis];
}

void run ()
{
  Image::Buffer<value_type> dt_data (argument[0]);

  if (dt_data.ndim() != 4)
    throw Exception ("base image should contain 4 dimensions");

  if (dt_data.dim (3) != 6)
    throw Exception ("expecting dimension 3 of image \"" + dt_data.name() + "\" to be 6");


  std::vector<int> vals (1);
  vals[0] = 1;
  Options opt = get_options ("num");
  if (opt.size()) {
    vals = opt[0][0];

    if (vals.empty())
      throw Exception ("invalid eigenvalue/eigenvector number specifier");

    for (size_t i = 0; i < vals.size(); ++i)
      if (vals[i] < 1 || vals[i] > 3)
        throw Exception ("eigenvalue/eigenvector number is out of bounds");
  }

  std::unique_ptr<ImagePair> adc, fa, eval, evec, mask;

  opt = get_options ("vector");
  if (opt.size())
    evec.reset (new ImagePair (dt_data, opt[0][0], 3*vals.size()));

  opt = get_options ("value");
  if (opt.size())
    eval.reset (new ImagePair (dt_data, opt[0][0], ( vals.size() > 1 ? vals.size() : 0 )));

  opt = get_options ("adc");
  if (opt.size())
    adc.reset (new ImagePair (dt_data, opt[0][0], 0));

  opt = get_options ("fa");
  if (opt.size()) 
    fa.reset (new ImagePair (dt_data, opt[0][0], 0));

  opt = get_options ("mask");
  if (opt.size()) {
    mask.reset (new ImagePair (opt[0][0]));
    Image::check_dimensions (mask->data, dt_data, 0, 3);
  }

  int modulate = 1;
  opt = get_options ("modulate");
  if (opt.size())
    modulate = opt[0][0];

  if (! (adc || fa || eval || evec))
    throw Exception ("no output metric specified - aborting");


  for (size_t i = 0; i < vals.size(); i++)
    vals[i] = 3-vals[i];


  Math::Matrix<double> V (3,3), M (3,3);
  Math::Vector<double> ev (3);
  float el[6], faval = NAN;

  std::unique_ptr<Math::Eigen::Symm<double> > eig;
  std::unique_ptr<Math::Eigen::SymmV<double> > eigv;
  if (evec)
    eigv.reset (new Math::Eigen::SymmV<double> (3));
  else
    eig.reset (new Math::Eigen::Symm<double> (3));

  auto dt = dt_data.voxel();

  ProgressBar progress ("computing tensor metrics...", Image::voxel_count (dt, 0, 3));

  for (dt[2] = 0; dt[2] < dt.dim (2); dt[2]++) {
    set_zero (1, mask, fa, adc, eval, evec);

    for (dt[1] = 0; dt[1] < dt.dim (1); dt[1]++) {
      set_zero (0, mask, fa, adc, eval, evec);

      for (dt[0] = 0; dt[0] < dt.dim (0); dt[0]++) {

        bool skip = false;
        if (mask) if (mask->vox.value() < 0.5) skip = true;

        if (!skip) {

          for (dt[3] = 0; dt[3] < dt.dim (3); dt[3]++)
            el[dt[3]] = dt.value();

          if (adc) adc->vox.value() = DWI::tensor2ADC (el);
          if (fa || modulate == 1) faval = DWI::tensor2FA (el);
          if (fa) fa->vox.value() = faval;

          if (eval || evec) {
            M (0,0) = el[0];
            M (1,1) = el[1];
            M (2,2) = el[2];
            M (0,1) = M (1,0) = el[3];
            M (0,2) = M (2,0) = el[4];
            M (1,2) = M (2,1) = el[5];

            if (evec) {
              (*eigv) (ev, M, V);
              Math::Eigen::sort (ev, V);
              if (modulate == 0) faval = 1.0;
              evec->vox[3] = 0;
              for (size_t i = 0; i < vals.size(); i++) {
                if (modulate == 2) faval = ev[vals[i]];
                evec->vox.value() = faval*V (0,vals[i]);
                ++evec->vox[3];
                evec->vox.value() = faval*V (1,vals[i]);
                ++evec->vox[3];
                evec->vox.value() = faval*V (2,vals[i]);
                ++evec->vox[3];
              }
            }
            else {
              (*eig) (ev, M);
              Math::Eigen::sort (ev);
            }

            if (eval) {
              if (eval->vox.ndim() > 3) {
                for (eval->vox[3] = 0; eval->vox[3] < (int) vals.size(); ++eval->vox[3])
                  eval->vox.value() = ev[vals[eval->vox[3]]];
              }
              else 
                eval->vox.value() = ev[vals[0]];
            }
          }
        }

        ++progress;
        increment (0, mask, fa, adc, eval, evec);
      }
      increment (1, mask, fa, adc, eval, evec);
    }
    increment (2, mask, fa, adc, eval, evec);
  }
}

