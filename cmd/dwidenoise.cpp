/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "image.h"

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>


using namespace MR;
using namespace App;

const char* const dtypes[] = { "float32", "float64", NULL };

const char* const estimators[] = { "exp1", "exp2", NULL };


void usage ()
{
  SYNOPSIS = "dMRI noise level estimation and denoising using Marchenko-Pastur PCA";

  DESCRIPTION
    + "DWI data denoising and noise map estimation by exploiting data redundancy in the PCA domain "
      "using the prior knowledge that the eigenspectrum of random covariance matrices is described by "
      "the universal Marchenko-Pastur (MP) distribution. Fitting the MP distribution to the spectrum "
      "of patch-wise signal matrices hence provides an estimator of the noise level 'sigma', as was "
      "first shown in Veraart et al. (2016) and later improved in Cordero-Grande et al. (2019). This "
      "noise level estimate then determines the optimal cut-off for PCA denoising."

    + "Important note: image denoising must be performed as the first step of the image processing pipeline. "
      "The routine will fail if interpolation or smoothing has been applied to the data prior to denoising."

    + "Note that this function does not correct for non-Gaussian noise biases present in "
      "magnitude-reconstructed MRI images. If available, including the MRI phase data can "
      "reduce such non-Gaussian biases, and the command now supports complex input data.";

  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk) & "
           "Jelle Veraart (jelle.veraart@nyumc.org) & "
           "J-Donald Tournier (jdtournier@gmail.com)";

  REFERENCES
    + "Veraart, J.; Novikov, D.S.; Christiaens, D.; Ades-aron, B.; Sijbers, J. & Fieremans, E. " // Internal
    "Denoising of diffusion MRI using random matrix theory. "
    "NeuroImage, 2016, 142, 394-406, doi: 10.1016/j.neuroimage.2016.08.016"

    + "Veraart, J.; Fieremans, E. & Novikov, D.S. " // Internal
    "Diffusion MRI noise mapping using random matrix theory. "
    "Magn. Res. Med., 2016, 76(5), 1582-1593, doi: 10.1002/mrm.26059"

    + "Cordero-Grande, L.; Christiaens, D.; Hutter, J.; Price, A.N.; Hajnal, J.V. " // Internal
    "Complex diffusion-weighted image estimation via matrix recovery under general noise models. "
    "NeuroImage, 2019, 200, 391-404, doi: 10.1016/j.neuroimage.2019.06.039";

  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in ()

  + Argument ("out", "the output denoised DWI image.").type_image_out ();


  OPTIONS
    + Option ("mask", "Only process voxels within the specified binary brain mask image.")
    +   Argument ("image").type_image_in()

    + Option ("extent", "Set the patch size of the denoising filter. "
                        "By default, the command will select the smallest isotropic patch size "
                        "that exceeds the number of DW images in the input data, e.g., 5x5x5 for "
                        "data with <= 125 DWI volumes, 7x7x7 for data with <= 343 DWI volumes, etc.")
    +   Argument ("window").type_sequence_int ()

    + Option ("noise", "The output noise map, i.e., the estimated noise level 'sigma' in the data. "
                       "Note that on complex input data, this will be the total noise level across "
                       "real and imaginary channels, so a scale factor sqrt(2) applies.")
    +   Argument ("level").type_image_out()

    + Option ("datatype", "Datatype for the eigenvalue decomposition (single or double precision). "
                          "For complex input data, this will select complex float32 or complex float64 datatypes.")
    +   Argument ("float32/float64").type_choice(dtypes)

    + Option ("estimator", "Select the noise level estimator (default = Exp2), either: \n"
                           "* Exp1: the original estimator used in Veraart et al. (2016), or \n"
                           "* Exp2: the improved estimator introduced in Cordero-Grande et al. (2019).")
    +   Argument ("Exp1/Exp2").type_choice(estimators);


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


using real_type = float;


template <typename F = float>
class DenoisingFunctor {
  MEMALIGN(DenoisingFunctor)

public:

  using MatrixType = Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic>;
  using SValsType = Eigen::VectorXd;

  DenoisingFunctor (int ndwi, const vector<uint32_t>& extent,
                    Image<bool>& mask, Image<real_type>& noise, bool exp1)
    : extent {{extent[0]/2, extent[1]/2, extent[2]/2}},
      m (ndwi), n (extent[0]*extent[1]*extent[2]),
      r (std::min(m,n)), q (std::max(m,n)), exp1(exp1),
      X (m,n), pos {{0, 0, 0}},
      mask (mask), noise (noise)
  { }

  template <typename ImageType>
  void operator () (ImageType& dwi, ImageType& out)
  {
    // Process voxels in mask only
    if (mask.valid()) {
      assign_pos_of (dwi, 0, 3).to (mask);
      if (!mask.value())
        return;
    }

    // Load data in local window
    load_data (dwi);

    // Compute Eigendecomposition:
    MatrixType XtX (r,r);
    if (m <= n)
      XtX.template triangularView<Eigen::Lower>() = X * X.adjoint();
    else
      XtX.template triangularView<Eigen::Lower>() = X.adjoint() * X;
    Eigen::SelfAdjointEigenSolver<MatrixType> eig (XtX);
    // eigenvalues sorted in increasing order:
    SValsType s = eig.eigenvalues().template cast<double>();

    // Marchenko-Pastur optimal threshold
    const double lam_r = std::max(s[0], 0.0) / q;
    double clam = 0.0;
    sigma2 = 0.0;
    ssize_t cutoff_p = 0;
    for (ssize_t p = 0; p < r; ++p)     // p+1 is the number of noise components
    {                                   // (as opposed to the paper where p is defined as the number of signal components)
      double lam = std::max(s[p], 0.0) / q;
      clam += lam;
      double gam = double(p+1) / (exp1 ? q : q-(r-p-1));
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
        X.col (n/2) = eig.eigenvectors() * ( s.cast<F>().asDiagonal() * ( eig.eigenvectors().adjoint() * X.col(n/2) ));
      else
        X.col (n/2) = X * ( eig.eigenvectors() * ( s.cast<F>().asDiagonal() * eig.eigenvectors().adjoint().col(n/2) ));
    }

    // Store output
    assign_pos_of(dwi).to(out);
    out.row(3) = X.col(n/2);

    // store noise map if requested:
    if (noise.valid()) {
      assign_pos_of(dwi, 0, 3).to(noise);
      noise.value() = real_type (std::sqrt(sigma2));
    }
  }

private:
  const std::array<ssize_t, 3> extent;
  const ssize_t m, n, r, q;
  const bool exp1;
  MatrixType X;
  std::array<ssize_t, 3> pos;
  double sigma2;
  Image<bool> mask;
  Image<real_type> noise;

  template <typename ImageType>
  void load_data (ImageType& dwi) {
    pos[0] = dwi.index(0); pos[1] = dwi.index(1); pos[2] = dwi.index(2);
    // fill patch
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

  inline size_t wrapindex(int r, int axis, int max) const {
    // patch handling at image edges
    int rr = pos[axis] + r;
    if (rr < 0)    rr = extent[axis] - r;
    if (rr >= max) rr = (max-1) - extent[axis] - r;
    return rr;
  }

};


template <typename T>
void process_image (Header& data, Image<bool>& mask, Image<real_type> noise,
                    const std::string& output_name, const vector<uint32_t>& extent, bool exp1)
  {
    auto input = data.get_image<T>().with_direct_io(3);
    // create output
    Header header (data);
    header.datatype() = DataType::from<T>();
    auto output = Image<T>::create (output_name, header);
    // run
    DenoisingFunctor<T> func (data.size(3), extent, mask, noise, exp1);
    ThreadedLoop ("running MP-PCA denoising", data, 0, 3).run (func, input, output);
  }



void run ()
{
  auto dwi = Header::open (argument[0]);

  if (dwi.ndim() != 4 || dwi.size(3) <= 1)
    throw Exception ("input image must be 4-dimensional");

  Image<bool> mask;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    check_dimensions (mask, dwi, 0, 3);
  }

  opt = get_options("extent");
  vector<uint32_t> extent;
  if (opt.size()) {
    extent = parse_ints<uint32_t> (opt[0][0]);
    if (extent.size() == 1)
      extent = {extent[0], extent[0], extent[0]};
    if (extent.size() != 3)
      throw Exception ("-extent must be either a scalar or a list of length 3");
    for (int i = 0; i < 3; i++) {
      if (!(extent[i] & 1))
        throw Exception ("-extent must be a (list of) odd numbers");
      if (extent[i] > dwi.size(i))
        throw Exception ("-extent must nott exceed the image dimensions");
    }
  } else {
    uint32_t e = 1;
    while (e*e*e < dwi.size(3))
      e += 2;
    extent = { std::min(e, uint32_t(dwi.size(0))),
               std::min(e, uint32_t(dwi.size(1))),
               std::min(e, uint32_t(dwi.size(2))) };
  }
  INFO("selected patch size: " + str(extent[0]) + " x " + str(extent[1]) + " x " + str(extent[2]) + ".");

  bool exp1 = get_option_value("estimator", 1) == 0;    // default: Exp2 (unbiased estimator)

  Image<real_type> noise;
  opt = get_options("noise");
  if (opt.size()) {
    Header header (dwi);
    header.ndim() = 3;
    header.datatype() = DataType::Float32;
    noise = Image<real_type>::create (opt[0][0], header);
  }

  int prec = get_option_value("datatype", 0);     // default: single precision
  if (dwi.datatype().is_complex()) prec += 2;     // support complex input data
  switch (prec) {
    case 0:
      INFO("select real float32 for processing");
      process_image<float>(dwi, mask, noise, argument[1], extent, exp1);
      break;
    case 1:
      INFO("select real float64 for processing");
      process_image<double>(dwi, mask, noise, argument[1], extent, exp1);
      break;
    case 2:
      INFO("select complex float32 for processing");
      process_image<cfloat>(dwi, mask, noise, argument[1], extent, exp1);
      break;
    case 3:
      INFO("select complex float64 for processing");
      process_image<cdouble>(dwi, mask, noise, argument[1], extent, exp1);
      break;
  }



}


