/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "image.h"
#include <Eigen/Dense>
#include <Eigen/SVD>

#define DEFAULT_SIZE 5

using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "denoise DWI data and estimate the noise level based on the optimal threshold for PCA.";

  
  AUTHOR = "Daan Christiaens (daan.christiaens@kuleuven.be) & Jelle Veraart (jelle.veraart@nyumc.org) & J-Donald Tournier (jdtournier.gmail.com)";
  
  
  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in ()

  + Argument ("out", "the output denoised DWI image.").type_image_out ();


  OPTIONS
  + Option ("size", "set the window size of the denoising filter. (default = " + str(DEFAULT_SIZE) + ")")
    + Argument ("window").type_integer (0, 50)
  
  + Option ("noise", "the output noise map.")
    + Argument ("level").type_image_out();

}


typedef float value_type;


template <class ImageType>
class DenoisingFunctor
{
public:
  DenoisingFunctor (ImageType& dwi, int size)
    : extent(size/2),
      m(dwi.size(3)),
      n(size*size*size),
      r((m<n) ? m : n),
      X(m,n), 
      pos{{0, 0, 0}}
  { }
  
  void operator () (ImageType& dwi, ImageType& out)
  {
    // Load data in local window
    load_data(dwi);
    // Compute SVD
    Eigen::JacobiSVD<Eigen::MatrixXf> svd (X, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::VectorXf s = svd.singularValues();
    // Marchenko-Pastur optimal threshold
    const double lam_r = s[r-1]*s[r-1] / n;
    double clam = 0.0;
    sigma = NaN;
    for (ssize_t p = r-1; p >= 0; --p)
    {
      double lam = s[p]*s[p] / n;
      clam += lam;
      double gam = double(m-p) / double(n);
      double sigsq1 = clam / (r-p) / ((gam<1.0) ? 1.0 : gam);
      double sigsq2 = (lam - lam_r) / 4 / std::sqrt(gam);
      // sigsq2 > sigsq1 if signal else noise
      if (sigsq2 < sigsq1) {
        s[p] = 0.0;
      } 
      else {
        sigma = std::sqrt(sigsq1);
        break;
      }
    }
    // Restore DWI data
    X = svd.matrixU() * s.asDiagonal() * svd.matrixV().adjoint();
    // Store output
    assign_pos_of(dwi).to(out);
    for (auto l = Loop (3) (out); l; ++l)
      out.value() = X(out.index(3), n/2);
  }
  
  void operator () (ImageType& dwi, ImageType& out, ImageType& noise)
  {
    operator ()(dwi, out);
    assign_pos_of(dwi).to(noise);
    noise.value() = sigma;
  }
  
  void load_data (ImageType& dwi)
  {
    pos[0] = dwi.index(0); pos[1] = dwi.index(1); pos[2] = dwi.index(2);
    X.setZero();
    ssize_t k = 0;
    for (dwi.index(2) = pos[2]-extent; dwi.index(2) <= pos[2]+extent; ++dwi.index(2))
      for (dwi.index(1) = pos[1]-extent; dwi.index(1) <= pos[1]+extent; ++dwi.index(1))
        for (dwi.index(0) = pos[0]-extent; dwi.index(0) <= pos[0]+extent; ++dwi.index(0), ++k)
          if (! is_out_of_bounds(dwi))
            X.col(k) = dwi.row(3).template cast<float>();
    // reset image position
    dwi.index(0) = pos[0];
    dwi.index(1) = pos[1];
    dwi.index(2) = pos[2];
  }
  
private:
  const int extent;
  const size_t m, n, r;
  Eigen::MatrixXf X;
  std::array<ssize_t, 3> pos;
  float sigma;
  
};



void run ()
{
  auto dwi_in = Image<value_type>::open (argument[0]).with_direct_io(3);

  auto header = Header (dwi_in);
  header.datatype() = DataType::Float32;
  auto dwi_out = Image<value_type>::create (argument[1], header);
  
  int extent = get_option_value("size", DEFAULT_SIZE);
  
  DenoisingFunctor< Image<value_type> > func (dwi_in, extent);
  
  auto opt = get_options("noise");
  if (opt.size())
  {
    header.set_ndim(3);
    auto noise = Image<value_type>::create (opt[0][0], header);
    ThreadedLoop ("running MP-PCA denoising", dwi_in, 0, 3)
      .run (func, dwi_in, dwi_out, noise);
  } 
  else
  {
    ThreadedLoop ("running MP-PCA denoising", dwi_in, 0, 3)
      .run (func, dwi_in, dwi_out);
  }


}


