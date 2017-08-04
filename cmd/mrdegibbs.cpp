/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <unsupported/Eigen/FFT>

#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_loop.h"
#include <numeric>

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be) & J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Remove Gibbs Ringing Artifacts";

  DESCRIPTION 
    + "This application attempts to remove Gibbs ringing artefacts from MRI images using the method "
    "of local subvoxel-shifts proposed by Kellner et al. (see reference below for details)."
    
    + "This command is designed to run on data directly after it has been reconstructed by the scanner, "
    "before any interpolation of any kind has taken place. You should not run this command after any "
    "form of motion correction (e.g. not after dwipreproc). Similarly, if you intend running dwidenoise, "
    "you should run this command afterwards, since it has the potential to alter the noise structure, "
    "which would impact on dwidenoise's performance."
    
    + "Note that this method is designed to work on images acquired with full k-space coverage. "
    "Running this method on partial Fourier ('half-scan') data may lead to suboptimal and/or biased "
    "results, as noted in the original reference below. There is currently no means of dealing with this; "
    "users should exercise caution when using this method on partial Fourier data, and inspect its output "
    "for any obvious artefacts. ";


  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();


  OPTIONS
  + Option ("axes",
            "select the slice axes (default: 0,1 - i.e. x-y).")
  +   Argument ("list").type_sequence_int ()

  + Option ("nshifts", "discretization of subpixel spacing (default: 20).")
  +   Argument ("value").type_integer (8, 128)

  + Option ("minW", "left border of window used for TV computation (default: 1).")
  +   Argument ("value").type_integer (0, 10)

  + Option ("maxW", "right border of window used for TV computation (default: 3).")
  +   Argument ("value").type_integer (0, 128)

  + DataType::options();


  REFERENCES
    + "Kellner, E; Dhital, B; Kiselev, V.G & Reisert, M. "
    "Gibbs-ringing artifact removal based on local subvoxel-shifts. "
    "Magnetic Resonance in Medicine, 2016, 76, 1574–1581.";

}


typedef double value_type;




class ComputeSlice
{ MEMALIGN (ComputeSlice)
  public:
    ComputeSlice (const vector<size_t>& outer_axes, const vector<size_t>& slice_axes, const int& nsh, const int& minW, const int& maxW, Image<value_type>& in, Image<value_type>& out) :
      outer_axes (outer_axes),
      slice_axes (slice_axes),
      nsh (nsh),
      minW (minW),
      maxW (maxW),
      in (in), 
      out (out),
      im1 (in.size(slice_axes[0]), in.size(slice_axes[1])),
      im2 (im1.rows(), im1.cols()) { 
        prealloc_FFT();
      }
    
    ComputeSlice (const ComputeSlice& other) :
      outer_axes (other.outer_axes),
      slice_axes (other.slice_axes),
      nsh (other.nsh),
      minW (other.minW),
      maxW (other.maxW),
      in (other.in), 
      out (other.out),
      fft (),
      im1 (in.size(slice_axes[0]), in.size(slice_axes[1])),
      im2 (im1.rows(), im1.cols()) { 
        prealloc_FFT();
      }


    void operator() (const Iterator& pos)
    {
      const int X = slice_axes[0];
      const int Y = slice_axes[1];
      assign_pos_of (pos, outer_axes).to (in, out);

      for (auto l = Loop (slice_axes) (in); l; ++l) 
        im1 (in.index(X), in.index(Y)) = cdouble (in.value(), 0.0);
      
      unring_2d ();

      for (auto l = Loop (slice_axes) (out); l; ++l)
        out.value() = im1 (out.index(X), out.index(Y)).real(); 
    }

  private:
    const vector<size_t>& outer_axes;
    const vector<size_t>& slice_axes;
    const int nsh, minW, maxW;
    Image<value_type> in, out; 
    Eigen::FFT<double> fft;
    Eigen::MatrixXcd im1, im2, shifted;
    Eigen::VectorXcd v;

    void prealloc_FFT () {
      // needed to avoid within-thread allocations, 
      // which aren't thread-safe in FFTW:
#ifdef EIGEN_FFTW_DEFAULT
      Eigen::VectorXcd tmp (im1.rows());
      FFT (tmp);
      iFFT (tmp);
      tmp.resize (im1.cols());
      FFT (tmp);
      iFFT (tmp);
#endif
    }

    template <typename Derived> FORCE_INLINE void FFT      (Eigen::MatrixBase<Derived>&& vec) { fft.fwd (v, vec); vec = v; }
    template <typename Derived> FORCE_INLINE void FFT      (Eigen::MatrixBase<Derived>& vec) { FFT (std::move (vec)); }
    template <typename Derived> FORCE_INLINE void iFFT     (Eigen::MatrixBase<Derived>&& vec) { fft.inv (v, vec); vec = v; } 
    template <typename Derived> FORCE_INLINE void iFFT     (Eigen::MatrixBase<Derived>& vec) { iFFT (std::move (vec)); }
    template <typename Derived> FORCE_INLINE void row_FFT  (Eigen::MatrixBase<Derived>& mat) { for (auto n = 0; n < mat.rows(); ++n)  FFT (mat.row(n)); } 
    template <typename Derived> FORCE_INLINE void row_iFFT (Eigen::MatrixBase<Derived>& mat) { for (auto n = 0; n < mat.rows(); ++n) iFFT (mat.row(n)); }
    template <typename Derived> FORCE_INLINE void col_FFT  (Eigen::MatrixBase<Derived>& mat) { for (auto n = 0; n < mat.cols(); ++n)  FFT (mat.col(n)); } 
    template <typename Derived> FORCE_INLINE void col_iFFT (Eigen::MatrixBase<Derived>& mat) { for (auto n = 0; n < mat.cols(); ++n) iFFT (mat.col(n)); }



    FORCE_INLINE void unring_2d ()
    {
      row_FFT (im1);
      col_FFT (im1);

      for (int k = 0; k < im1.cols(); k++) {
        double ck = (1.0+cos(2.0*Math::pi*(double(k)/im1.cols())))*0.5;
        for (int j = 0 ; j < im1.rows(); j++) {
          double cj = (1.0+cos(2.0*Math::pi*(double(j)/im1.rows())))*0.5;

          if (ck+cj != 0.0) {
            im2(j,k) = im1(j,k) * cj / (ck+cj);
            im1(j,k) *= ck / (ck+cj);
          }
          else 
            im1(j,k) = im2(j,k) = cdouble(0.0, 0.0);
        }
      }

      row_iFFT (im1);
      col_iFFT (im2);

      unring_1d (im1);
      unring_1d (im2.transpose());

      im1 += im2;
    }





    template <typename Derived> 
      FORCE_INLINE void unring_1d (Eigen::MatrixBase<Derived>&& eig)
      {
        const int n = eig.rows();
        const int numlines = eig.cols();
        shifted.resize (n, 2*nsh+1);

        int shifts [2*nsh+1];
        shifts[0] = 0;
        for (int j = 0; j < nsh; j++) {
          shifts[j+1] = j+1;
          shifts[1+nsh+j] = -(j+1);
        }

        double TV1arr[2*nsh+1];
        double TV2arr[2*nsh+1];

        for (int k = 0; k < numlines; k++) {
          shifted.col(0) = eig.col(k);

          const int maxn = (n&1) ? (n-1)/2 : n/2-1;

          for (int j = 1; j < 2*nsh+1; j++) {
            double phi = Math::pi*double(shifts[j])/double(n*nsh);
            cdouble u (std::cos(phi), std::sin(phi));
            cdouble e (1.0, 0.0);
            shifted(0,j) = shifted(0,0);

            if (!(n&1)) 
              shifted(n/2,j) = cdouble(0.0, 0.0);

            for (int l = 0; l < maxn; l++) {
              e = u*e;
              int L = l+1; shifted(L,j) = e * shifted(L,0);
              L = n-1-l;   shifted(L,j) = std::conj(e) * shifted(L,0);
            }
          }


          col_iFFT (shifted);

          for (int j = 0; j < 2*nsh+1; ++j) {
            TV1arr[j] = 0.0;
            TV2arr[j] = 0.0;
            for (int t = minW; t <= maxW; t++) {
              TV1arr[j] += std::abs (shifted((n-t)%n,j).real() - shifted((n-t-1)%n,j).real()); 
              TV1arr[j] += std::abs (shifted((n-t)%n,j).imag() - shifted((n-t-1)%n,j).imag());
              TV2arr[j] += std::abs (shifted((n+t)%n,j).real() - shifted((n+t+1)%n,j).real());
              TV2arr[j] += std::abs (shifted((n+t)%n,j).imag() - shifted((n+t+1)%n,j).imag());
            }
          }

          for (int l = 0; l < n; ++l) {
            double minTV = std::numeric_limits<double>::max();
            int minidx = 0;
            for (int j = 0; j < 2*nsh+1; ++j) {

              if (TV1arr[j] < minTV) {
                minTV = TV1arr[j];
                minidx = j;
              }
              if (TV2arr[j] < minTV) {
                minTV = TV2arr[j];
                minidx = j;
              }

              TV1arr[j] += std::abs (shifted((l-minW+1+n)%n,j).real() - shifted((l-(minW  )+n)%n,j).real());
              TV1arr[j] -= std::abs (shifted((l-maxW  +n)%n,j).real() - shifted((l-(maxW+1)+n)%n,j).real());
              TV2arr[j] += std::abs (shifted((l+maxW+1+n)%n,j).real() - shifted((l+(maxW+2)+n)%n,j).real());
              TV2arr[j] -= std::abs (shifted((l+minW  +n)%n,j).real() - shifted((l+(minW+1)+n)%n,j).real());

              TV1arr[j] += std::abs (shifted((l-minW+1+n)%n,j).imag() - shifted((l-(minW  )+n)%n,j).imag());
              TV1arr[j] -= std::abs (shifted((l-maxW  +n)%n,j).imag() - shifted((l-(maxW+1)+n)%n,j).imag());
              TV2arr[j] += std::abs (shifted((l+maxW+1+n)%n,j).imag() - shifted((l+(maxW+2)+n)%n,j).imag());
              TV2arr[j] -= std::abs (shifted((l+minW  +n)%n,j).imag() - shifted((l+(minW+1)+n)%n,j).imag());
            }

            double a0r = shifted((l-1+n)%n,minidx).real();
            double a1r = shifted(l,minidx).real();
            double a2r = shifted((l+1+n)%n,minidx).real();
            double a0i = shifted((l-1+n)%n,minidx).imag();
            double a1i = shifted(l,minidx).imag();
            double a2i = shifted((l+1+n)%n,minidx).imag();
            double s = double(shifts[minidx])/(2.0*nsh);

            if (s > 0.0) 
              eig(l,k) = cdouble (a1r*(1.0-s) + a0r*s, a1i*(1.0-s) + a0i*s);
            else 
              eig(l,k) = cdouble (a1r*(1.0+s) - a2r*s, a1i*(1.0+s) - a2i*s);
          }
        }
      }

    template <typename Derived> 
      FORCE_INLINE void unring_1d (Eigen::MatrixBase<Derived>& eig) { unring_1d (std::move (eig)); }

};







void run ()
{
  const int nshifts = App::get_option_value ("nshifts", 20);
  const int minW = App::get_option_value ("minW", 1);
  const int maxW = App::get_option_value ("maxW", 3);

  if (minW >= maxW) 
    throw Exception ("minW must be smaller than maxW");

  auto in = Image<value_type>::open (argument[0]);
  Header header (in);

  header.datatype() = DataType::from_command_line (DataType::Float32);
  auto out = Image<value_type>::create (argument[1], header);

  vector<size_t> slice_axes = { 0, 1 };
  auto opt = get_options ("axes");
  if (opt.size()) {
    vector<int> axes = opt[0][0];
    if (slice_axes.size() != 2) 
      throw Exception ("slice axes must be specified as a comma-separated 2-vector");
    slice_axes = { size_t(axes[0]), size_t(axes[1]) };
  }

  // build vector of outer axes:
  vector<size_t> outer_axes (header.ndim());
  std::iota (outer_axes.begin(), outer_axes.end(), 0);
  for (const auto axis : slice_axes) {
    auto it = std::find (outer_axes.begin(), outer_axes.end(), axis);
    if (it == outer_axes.end())
      throw Exception ("slice axis out of range!");
    outer_axes.erase (it);
  }

  ThreadedLoop ("performing Gibbs ringing removal", in, outer_axes, slice_axes)
    .run_outer (ComputeSlice (outer_axes, slice_axes, nshifts, minW, maxW, in, out));
}

