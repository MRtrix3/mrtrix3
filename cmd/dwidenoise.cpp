/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "image.h"

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>


#define DEFAULT_SIZE 5

using namespace MR;
using namespace App;

const char* const dtypes[] = { "float32", "float64", NULL };


void usage ()
{
  SYNOPSIS = "Denoise DWI data and estimate the noise level based on the optimal threshold for PCA";

  DESCRIPTION
    + "DWI data denoising and noise map estimation by exploiting data redundancy in the PCA domain "
    "using the prior knowledge that the eigenspectrum of random covariance matrices is described by "
    "the universal Marchenko Pastur distribution."

    + "Important note: image denoising must be performed as the first step of the image processing pipeline. "
    "The routine will fail if interpolation or smoothing has been applied to the data prior to denoising."

    + "Note that this function does not correct for non-Gaussian noise biases.";

  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk) & Jelle Veraart (jelle.veraart@nyumc.org) & J-Donald Tournier (jdtournier@gmail.com)";

  REFERENCES
    + "Veraart, J.; Novikov, D.S.; Christiaens, D.; Ades-aron, B.; Sijbers, J. & Fieremans, E. " // Internal
    "Denoising of diffusion MRI using random matrix theory. "
    "NeuroImage, 2016, 142, 394-406, doi: 10.1016/j.neuroimage.2016.08.016"

    + "Veraart, J.; Fieremans, E. & Novikov, D.S. " // Internal
    "Diffusion MRI noise mapping using random matrix theory. "
    "Magn. Res. Med., 2016, 76(5), 1582-1593, doi: 10.1002/mrm.26059";

  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in ()

  + Argument ("out", "the output denoised DWI image.").type_image_out ();


  OPTIONS
    + Option ("mask", "only perform computation within the specified binary brain mask image.")
    +   Argument ("image").type_image_in()

    + Option ("extent", "set the window size of the denoising filter. (default = " + str(DEFAULT_SIZE) + "," + str(DEFAULT_SIZE) + "," + str(DEFAULT_SIZE) + ")")
    +   Argument ("window").type_sequence_int ()

    + Option ("noise", "the output noise map.")
    +   Argument ("level").type_image_out()

    + Option ("datatype", "datatype for SVD (float32 or float64).")
    +   Argument ("spec").type_choice(dtypes);


  COPYRIGHT = "Copyright (c) 2016 New York University, University of Antwerp, and the MRtrix3 contributors \n \n"
      "Permission is hereby granted, free of charge, to any non-commercial entity ('Recipient') obtaining a copy of this software and "
      "associated documentation files (the 'Software'), to the Software solely for non-commercial research, including the rights to "
      "use, copy and modify the Software, subject to the following conditions: \n \n"
      "\t 1. The above copyright notice and this permission notice shall be included by Recipient in all copies or substantial portions of "
      "the Software. \n \n"
      "\t 2. THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES"
      "OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE"
      "LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR"
      "IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. \n \n"
      "\t 3. In no event shall NYU be liable for direct, indirect, special, incidental or consequential damages in connection with the Software. "
      "Recipient will defend, indemnify and hold NYU harmless from any claims or liability resulting from the use of the Software by recipient. \n \n"
      "\t 4. Neither anything contained herein nor the delivery of the Software to recipient shall be deemed to grant the Recipient any right or "
      "licenses under any patents or patent application owned by NYU. \n \n"
      "\t 5. The Software may only be used for non-commercial research and may not be used for clinical care. \n \n"
      "\t 6. Any publication by Recipient of research involving the Software shall cite the references listed below.";

}


using value_type = float;


template <class ImageType, typename F = float>
class DenoisingFunctor {
  MEMALIGN(DenoisingFunctor)

public:

  typedef Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic> MatrixX;
  typedef Eigen::Matrix<F, Eigen::Dynamic, 1> VectorX;

  DenoisingFunctor (ImageType& dwi, vector<int> extent, Image<bool>& mask, ImageType& noise)
    : extent {{extent[0]/2, extent[1]/2, extent[2]/2}},
      m (dwi.size(3)),
      n (extent[0]*extent[1]*extent[2]),
      r ((m<n) ? m : n),
      X (m,n),
      Xm (r),
      pos {{0, 0, 0}},
      mask (mask),
      noise (noise)
  { }

  void operator () (ImageType& dwi, ImageType& out)
  {
    // Process voxels in mask only
    if (mask.valid()) {
      assign_pos_of (dwi).to (mask);
      if (!mask.value())
        return;
    }

    // Load data in local window
    load_data (dwi);

    // Compute Eigendecomposition:
    MatrixX XtX (r,r);
    if (m <= n)
      XtX.template triangularView<Eigen::Lower>() = X * X.adjoint();
    else
      XtX.template triangularView<Eigen::Lower>() = X.adjoint() * X;
    Eigen::SelfAdjointEigenSolver<MatrixX> eig (XtX);
    // eigenvalues provide squared singular values, sorted in increasing order:
    VectorX s = eig.eigenvalues();

    // Marchenko-Pastur optimal threshold
    const double lam_r = std::max(double(s[0]), 0.0) / std::max(m,n);
    double clam = 0.0;
    sigma2 = 0.0;
    ssize_t cutoff_p = 0;
    for (ssize_t p = 0; p < r; ++p)     // p+1 is the number of noise components
    {                                   // (as opposed to the paper where p is defined as the number of signal components)
      double lam = std::max(double(s[p]), 0.0) / std::max(m,n);
      clam += lam;
      double gam = double(p+1) / (std::max(m,n) - (r-p-1));
      double sigsq1 = clam / double(p+1);
      double sigsq2 = (lam - lam_r) / (4.0 * std::sqrt(gam));
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
        X.col (n/2) = eig.eigenvectors() * ( s.asDiagonal() * ( eig.eigenvectors().adjoint() * X.col(n/2) ));
      else
        X.col (n/2) = X * ( eig.eigenvectors() * ( s.asDiagonal() * eig.eigenvectors().adjoint().col(n/2) ));
    }

    // Store output
    assign_pos_of(dwi).to(out);
    if (m <= n) {
      for (auto l = Loop (3) (out); l; ++l)
        out.value() = value_type (X(out.index(3), n/2));
    }
    else {
      for (auto l = Loop (3) (out); l; ++l)
        out.value() = value_type (X(out.index(3), n/2));
    }

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
    size_t k = 0;
    for (int z = -extent[2]; z <= extent[2]; z++) {
      dwi.index(2) = wrapindex(z, 2, dwi.size(2));
      for (int y = -extent[1]; y <= extent[1]; y++) {
        dwi.index(1) = wrapindex(y, 1, dwi.size(1));
        for (int x = -extent[0]; x <= extent[0]; x++, k++) {
          dwi.index(0) = wrapindex(x, 0, dwi.size(0));
          X.col(k) = dwi.row(3);
        }
      }
    }

    // reset image position
    dwi.index(0) = pos[0];
    dwi.index(1) = pos[1];
    dwi.index(2) = pos[2];
  }  

private:
  const std::array<ssize_t, 3> extent;
  const ssize_t m, n, r;
  MatrixX X;
  VectorX Xm;
  std::array<ssize_t, 3> pos;
  double sigma2;
  Image<bool> mask;
  ImageType noise;

  inline size_t wrapindex(int r, int axis, int max) const {
    int rr = pos[axis] + r;
    if (rr < 0)
      rr = extent[axis] - r;
    if (rr >= max)
      rr = (max-1) - extent[axis] - r;
    return rr;
  }

};



void run ()
{
  auto dwi_in = Image<value_type>::open (argument[0]).with_direct_io(3);

  if (dwi_in.ndim() != 4 || dwi_in.size(3) <= 1) 
    throw Exception ("input image must be 4-dimensional");

  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (mask, dwi_in, 0, 3);
  }

  auto header = Header (dwi_in);
  header.datatype() = DataType::Float32;
  auto dwi_out = Image<value_type>::create (argument[1], header);

  opt = get_options("extent");
  vector<int> extent = { DEFAULT_SIZE, DEFAULT_SIZE, DEFAULT_SIZE };
  if (opt.size()) {
    extent = parse_ints(opt[0][0]);
    if (extent.size() == 1)
      extent = {extent[0], extent[0], extent[0]};
    if (extent.size() != 3)
      throw Exception ("-extent must be either a scalar or a list of length 3");
    for (auto &e : extent)
      if (!(e & 1))
        throw Exception ("-extent must be a (list of) odd numbers");
  }

  Image<value_type> noise;
  opt = get_options("noise");
  if (opt.size()) {
    header.ndim() = 3;
    noise = Image<value_type>::create (opt[0][0], header);
  }

  opt = get_options("datatype");
  if (!opt.size() || (int(opt[0][0]) == 0)) {
    DEBUG("Computing SVD with single precision.");
    DenoisingFunctor< Image<value_type> , float > func (dwi_in, extent, mask, noise);
    ThreadedLoop ("running MP-PCA denoising", dwi_in, 0, 3)
      .run (func, dwi_in, dwi_out);
  }
  else if (int(opt[0][0]) == 1) {
    DEBUG("Computing SVD with double precision.");
    DenoisingFunctor< Image<value_type> , double > func (dwi_in, extent, mask, noise);
    ThreadedLoop ("running MP-PCA denoising", dwi_in, 0, 3)
      .run (func, dwi_in, dwi_out);
  }
  else {
    assert(0);
  }

}


