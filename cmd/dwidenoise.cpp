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
#include <Eigen/Eigenvalues>

#define DEFAULT_SIZE 5

using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "denoise DWI data and estimate the noise level based on the optimal threshold for PCA.";

  
  AUTHOR = "Daan Christiaens (daan.christiaens@kuleuven.be) & Jelle Veraart (jelle.veraart@nyumc.org) & J-Donald Tournier (jdtournier@gmail.com)";
  
  
  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in ()

  + Argument ("out", "the output denoised DWI image.").type_image_out ();


  OPTIONS
    + Option ("mask", "only perform computation within the specified binary brain mask image.")
    +   Argument ("image").type_image_in()

    + Option ("extent", "set the window size of the denoising filter. (default = " + str(DEFAULT_SIZE) + ")")
    +   Argument ("window").type_integer (0, 50)

    + Option ("noise", "the output noise map.")
    +   Argument ("level").type_image_out();

}


typedef float value_type;


template <class ImageType>
class DenoisingFunctor
{
  public:
  DenoisingFunctor (ImageType& dwi, int extent, Image<bool>& mask, ImageType& noise)
    : extent (extent/2),
      m (dwi.size(3)),
      n (extent*extent*extent),
      r ((m<n) ? m : n),
      X (m,n), 
      pos {{0, 0, 0}}, 
      mask (mask),
      noise (noise)
  { }
  
  void operator () (ImageType& dwi, ImageType& out)
  {
    if (mask.valid()) {
      assign_pos_of (dwi).to (mask);
      if (!mask.value())
        return;
    }

    // Load data in local window
    load_data (dwi);

    // Compute Eigendecomposition:
    Eigen::MatrixXf XtX (r,r);
    if (m <= n)
      XtX.template triangularView<Eigen::Lower>() = X * X.transpose();
    else 
      XtX.template triangularView<Eigen::Lower>() = X.transpose() * X;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf> eig (XtX);
    // eigenvalues provide squared singular values:
    Eigen::VectorXf s = eig.eigenvalues();
   
    // Marchenko-Pastur optimal threshold
    const double lam_r = s[0] / n;
    double clam = 0.0;
    sigma2 = NaN;
    ssize_t cutoff_p = 0;
    for (ssize_t p = 0; p < r; ++p)
    {
      double lam = s[p] / n;
      clam += lam;
      double gam = double(m-r+p+1) / double(n);
      double sigsq1 = clam / (m-r+p+1) / std::max (gam, 1.0);
      double sigsq2 = (lam - lam_r) / 4 / std::sqrt(gam);
      // sigsq2 > sigsq1 if signal else noise
      if (sigsq2 < sigsq1) {
        sigma2 = sigsq1;
        cutoff_p = p+1;
      } 
    }

    if (cutoff_p > 0) {
      // recombine data using only eigenvectors above threshold:
      s.head (cutoff_p).setZero();
      s.tail (r-cutoff_p).setOnes();
      if (m <= n) 
        X.col (n/2) = eig.eigenvectors() * s.asDiagonal() * eig.eigenvectors().adjoint() * X.col(n/2);
      else 
        X.col (n/2) = X * eig.eigenvectors() * s.asDiagonal() * eig.eigenvectors().adjoint().col(n/2);
    }

    // Store output
    assign_pos_of(dwi).to(out);
    for (auto l = Loop (3) (out); l; ++l)
      out.value() = X(out.index(3), n/2);

    // store noise map if requested:
    if (noise.valid()) {
      assign_pos_of(dwi).to(noise);
      noise.value() = value_type (std::sqrt(sigma2));
    }
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
  const ssize_t m, n, r;
  Eigen::MatrixXf X;
  std::array<ssize_t, 3> pos;
  double sigma2;
  Image<bool> mask;
  ImageType noise;
  
};



void run ()
{
  auto dwi_in = Image<value_type>::open (argument[0]).with_direct_io(3);

  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (mask, dwi_in, 0, 3);
  }

  auto header = Header (dwi_in);
  header.datatype() = DataType::Float32;
  auto dwi_out = Image<value_type>::create (argument[1], header);
  
  int extent = get_option_value("extent", DEFAULT_SIZE);
  if (!(extent & 1))
    throw Exception ("-extent must be an odd number");
  
  Image<value_type> noise;
  opt = get_options("noise");
  if (opt.size()) {
    header.set_ndim(3);
    noise = Image<value_type>::create (opt[0][0], header);
  }

  DenoisingFunctor< Image<value_type> > func (dwi_in, extent, mask, noise);
  ThreadedLoop ("running MP-PCA denoising", dwi_in, 0, 3)
    .run (func, dwi_in, dwi_out);
}


