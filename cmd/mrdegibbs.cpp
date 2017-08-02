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
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)";

  SYNOPSIS = "Remove Gibbs Ringing Artifact";

  DESCRIPTION 
    + "to be filled in";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();


  OPTIONS
  + Option ("axes",
            "select the slice axes (default: 0,1 - i.e. x-y)")
  +   Argument ("list").type_sequence_int ();


  REFERENCES
    + "Kellner ref";
}


typedef double value_type;




class ComputeSlice
{
  public:
    ComputeSlice (const vector<size_t>& outer_axes, const vector<size_t>& slice_axes, const int& nsh, const int& minW, const int& maxW, Image<value_type>& in, Image<value_type>& out) :
      outer_axes (outer_axes),
      slice_axes (slice_axes),
      nsh (nsh),
      minW (minW),
      maxW (maxW),
      X (slice_axes[0]),
      Y (slice_axes[1]),
      in (in), 
      out (out),
      im1 (in.size(X), in.size(Y)),
      im2 (im1.rows(), im1.cols()),
      ft1 (im1.rows(), im1.cols()),
      ft2 (im1.rows(), im1.cols()) { }

    void operator() (const Iterator& pos)
    {
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
    const int nsh, minW, maxW, X, Y;
    Image<value_type> in, out; 
    Eigen::FFT<double> fftx, ffty;
    Eigen::MatrixXcd im1, im2, ft1, ft2, shifted;
    Eigen::VectorXcd vx, vy;

    void FFT_2D (const Eigen::MatrixXcd& in, Eigen::MatrixXcd& out) 
    {
      for (auto n = 0; n < in.rows(); ++n) {
        fftx.fwd (vx, in.row(n));
        out.row(n) = vx;
      }
      for (auto n = 0; n < out.cols(); ++n) {
        ffty.fwd (vy, out.col(n));
        out.col(n) = vy;
      }
    }

    void FFT_2D_inv (const Eigen::MatrixXcd& in, Eigen::MatrixXcd& out) 
    {
      for (auto n = 0; n < in.rows(); ++n) {
        fftx.inv (vx, in.row(n));
        out.row(n) = vx;
      }
      for (auto n = 0; n < out.cols(); ++n) {
        ffty.inv (vy, out.col(n));
        out.col(n) = vy;
      }
    }






    void unring_2d ()
    {
      FFT_2D (im1, ft1);

      for (int k = 0; k < im1.cols(); k++) {
        double ck = (1.0+cos(2.0*Math::pi*(double(k)/im1.cols())))*0.5;
        for (int j = 0 ; j < im1.rows(); j++) {
          double cj = (1.0+cos(2.0*Math::pi*(double(j)/im1.rows())))*0.5;

          if (ck+cj != 0.0) {
            ft2(j,k) = ft1(j,k) * cj / (ck+cj);
            ft1(j,k) *= ck / (ck+cj);
          }
          else 
            ft1(j,k) = ft2(j,k) = cdouble(0.0, 0.0);
        }
      }

      FFT_2D_inv (ft1, im1);
      FFT_2D_inv (ft2, im2);

      unring_1d (im1);
      auto im2_t = im2.transpose();
      unring_1d (im2_t);

      FFT_2D (im1, ft1);
      FFT_2D (im2, ft2);

      ft1 += ft2;

      FFT_2D_inv (ft1, im1);
    }


    template <typename Derived> 
      void unring_1d (Eigen::MatrixBase<Derived>& eig)
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
          fftx.fwd (vx, eig.col(k));
          shifted.col(0) = vx;

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


          for (int j = 0; j < 2*nsh+1; ++j) {
            fftx.inv (vx, shifted.col(j));
            shifted.col(j) = vx;
          }

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


};







void run ()
{
  auto in = Image<value_type>::open (argument[0]);
  Header header (in);

  header.datatype() = DataType::Float64;
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

  ThreadedLoop ("computing stuff...", in, outer_axes, slice_axes)
    .run_outer (ComputeSlice (outer_axes, slice_axes, 30, 1, 3, in, out));
}

