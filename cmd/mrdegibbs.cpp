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


#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_loop.h"
#include <numeric>
#include <fftw3.h>

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


void unring_1d (fftw_complex* data, size_t n, size_t numlines, size_t nsh, size_t minW, size_t maxW) 
{
  fftw_complex* in  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*n);
  fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*n);
  fftw_plan p    = fftw_plan_dft_1d(n, in, out, FFTW_FORWARD , FFTW_ESTIMATE);
  fftw_plan pinv = fftw_plan_dft_1d(n, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_complex* sh  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*n*(2*nsh+1));
  fftw_complex* sh2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*n*(2*nsh+1));

  double nfac = 1/double(n);
  size_t* shifts = (size_t*) malloc(sizeof(size_t)*(2*nsh+1));
  shifts[0] = 0;
  for (size_t j = 0; j < nsh; j++) {
    shifts[j+1] = j+1;
    shifts[1+nsh+j] = -(j+1);
  }

  double TV1arr[2*nsh+1];
  double TV2arr[2*nsh+1];

  for (size_t k = 0; k < numlines; k++) {
    fftw_execute_dft(p, &(data[n*k]), sh);
    size_t maxn = (n%2==1) ? (n-1)/2 : n/2-1;

    for (size_t j = 1; j < 2*nsh+1; j++) {
      double phi = Math::pi/double(n) * double(shifts[j])/double(nsh);
      fftw_complex u = {cos(phi),sin(phi)};
      fftw_complex e = {1,0};
      sh[j*n ][0] = sh[0][0];
      sh[j*n ][1] = sh[0][1];

      if (n%2 == 0) {
        sh[j*n + n/2][0] = 0;
        sh[j*n + n/2][1] = 0;
      }

      for (size_t l = 0; l < maxn; l++) {
        double tmp = e[0];
        e[0] = u[0]*e[0] - u[1]*e[1];
        e[1] = tmp*u[1] + u[0]*e[1];

        size_t L = l+1;
        sh[j*n +L][0] = (e[0]*sh[L][0] - e[1]*sh[L][1]);
        sh[j*n +L][1] = (e[0]*sh[L][1] + e[1]*sh[L][0]);
        L = n-1-l;
        sh[j*n +L][0] = (e[0]*sh[L][0] + e[1]*sh[L][1]);
        sh[j*n +L][1] = (e[0]*sh[L][1] - e[1]*sh[L][0]);

      }
    }


    for (size_t j = 0; j < 2*nsh+1; j++)
      fftw_execute_dft(pinv, &(sh[j*n]), &sh2[j*n]);

    for (size_t j=0; j < 2*nsh+1; j++) {
      TV1arr[j] = 0;
      TV2arr[j] = 0;
      const size_t l = 0;
      for (size_t t = minW; t <= maxW; t++) {
        TV1arr[j] += fabs(sh2[j*n + (l-t+n)%n ][0] - sh2[j*n + (l-(t+1)+n)%n ][0]);
        TV1arr[j] += fabs(sh2[j*n + (l-t+n)%n ][1] - sh2[j*n + (l-(t+1)+n)%n ][1]);
        TV2arr[j] += fabs(sh2[j*n + (l+t+n)%n ][0] - sh2[j*n + (l+(t+1)+n)%n ][0]);
        TV2arr[j] += fabs(sh2[j*n + (l+t+n)%n ][1] - sh2[j*n + (l+(t+1)+n)%n ][1]);
      }
    }

    for(size_t l=0; l < n; l++) {
      double minTV = 999999999999;
      size_t minidx= 0;
      for (size_t j=0;j < 2*nsh+1; j++) {

        if (TV1arr[j] < minTV) {
          minTV = TV1arr[j];
          minidx = j;
        }
        if (TV2arr[j] < minTV) {
          minTV = TV2arr[j];
          minidx = j;
        }

        TV1arr[j] += fabs(sh2[j*n + (l-minW+1+n)%n ][0] - sh2[j*n + (l-(minW)+n)%n ][0]);
        TV1arr[j] -= fabs(sh2[j*n + (l-maxW+n)%n ][0] - sh2[j*n + (l-(maxW+1)+n)%n ][0]);
        TV2arr[j] += fabs(sh2[j*n + (l+maxW+1+n)%n ][0] - sh2[j*n + (l+(maxW+2)+n)%n ][0]);
        TV2arr[j] -= fabs(sh2[j*n + (l+minW+n)%n ][0] - sh2[j*n + (l+(minW+1)+n)%n ][0]);

        TV1arr[j] += fabs(sh2[j*n + (l-minW+1+n)%n ][1] - sh2[j*n + (l-(minW)+n)%n ][1]);
        TV1arr[j] -= fabs(sh2[j*n + (l-maxW+n)%n ][1] - sh2[j*n + (l-(maxW+1)+n)%n ][1]);
        TV2arr[j] += fabs(sh2[j*n + (l+maxW+1+n)%n ][1] - sh2[j*n + (l+(maxW+2)+n)%n ][1]);
        TV2arr[j] -= fabs(sh2[j*n + (l+minW+n)%n ][1] - sh2[j*n + (l+(minW+1)+n)%n ][1]);
      }

      double a0r = sh2[minidx*n + (l-1+n)%n ][0];
      double a1r = sh2[minidx*n + l][0];
      double a2r = sh2[minidx*n + (l+1+n)%n ][0];
      double a0i = sh2[minidx*n + (l-1+n)%n ][1];
      double a1i = sh2[minidx*n + l][1];
      double a2i = sh2[minidx*n + (l+1+n)%n ][1];
      double s = double(shifts[minidx])/nsh/2;

      if (s>0) {
        data[k*n + l][0] =  (a1r*(1-s) + a0r*s)*nfac;
        data[k*n + l][1] =  (a1i*(1-s) + a0i*s)*nfac;
      }
      else {
        s = -s;
        data[k*n + l][0] =  (a1r*(1-s) + a2r*s)*nfac;
        data[k*n + l][1] =  (a1i*(1-s) + a2i*s)*nfac;
      }
    }
  }
  free(shifts);
  fftw_destroy_plan(p);
  fftw_destroy_plan(pinv);
  fftw_free(in);
  fftw_free(out);
  fftw_free(sh);
  fftw_free(sh2);
}







void unring_2d (fftw_complex* data1,fftw_complex* tmp2, const size_t* dim_sz, size_t nsh, size_t minW, size_t maxW) 
{
  fftw_complex* tmp1  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * dim_sz[0]*dim_sz[1]);
  fftw_complex* data2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * dim_sz[0]*dim_sz[1]);

  fftw_plan p = fftw_plan_dft_2d(dim_sz[1],dim_sz[0], data1, tmp1, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_plan pinv = fftw_plan_dft_2d(dim_sz[1],dim_sz[0], data1, tmp1, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_plan p_tr = fftw_plan_dft_2d(dim_sz[0],dim_sz[1], data2, tmp2, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_plan pinv_tr = fftw_plan_dft_2d(dim_sz[0],dim_sz[1], data2, tmp2, FFTW_BACKWARD, FFTW_ESTIMATE);
  double nfac = 1/double(dim_sz[0]*dim_sz[1]);

  for (size_t k = 0; k < dim_sz[1]; k++) {
    for (size_t j = 0; j < dim_sz[0]; j++) {
      data2[j*dim_sz[1]+k][0] = data1[k*dim_sz[0]+j][0];
      data2[j*dim_sz[1]+k][1] = data1[k*dim_sz[0]+j][1];
    }
  }

  fftw_execute_dft(p,data1,tmp1);
  fftw_execute_dft(p_tr,data2,tmp2);

  for (size_t k = 0; k < dim_sz[1]; k++) {
    double ck = (1+cos(2*Math::pi*(double(k)/dim_sz[1])))*0.5;
    for (size_t j = 0 ; j < dim_sz[0]; j++) {
      double cj = (1+cos(2.0*Math::pi*(double(j)/dim_sz[0])))*0.5;
      tmp1[k*dim_sz[0]+j][0] = nfac*(tmp1[k*dim_sz[0]+j][0] * ck) / (ck+cj);
      tmp1[k*dim_sz[0]+j][1] = nfac*(tmp1[k*dim_sz[0]+j][1] * ck) / (ck+cj);
      tmp2[j*dim_sz[1]+k][0] = nfac*(tmp2[j*dim_sz[1]+k][0] * cj) / (ck+cj);
      tmp2[j*dim_sz[1]+k][1] = nfac*(tmp2[j*dim_sz[1]+k][1] * cj) / (ck+cj);
    }
  }

  fftw_execute_dft(pinv,tmp1,data1);
  fftw_execute_dft(pinv_tr,tmp2,data2);

  unring_1d(data1,dim_sz[0],dim_sz[1],nsh,minW,maxW);
  unring_1d(data2,dim_sz[1],dim_sz[0],nsh,minW,maxW);

  fftw_execute_dft(p,data1,tmp1);
  fftw_execute_dft(p_tr,data2,tmp2);

  for (size_t k = 0; k < dim_sz[1]; k++) {
    for (size_t j = 0; j < dim_sz[0]; j++) {
      tmp1[k*dim_sz[0]+j][0] = nfac*(tmp1[k*dim_sz[0]+j][0] + tmp2[j*dim_sz[1]+k][0]);
      tmp1[k*dim_sz[0]+j][1] = nfac*(tmp1[k*dim_sz[0]+j][1] + tmp2[j*dim_sz[1]+k][1]);
    }
  }

  fftw_execute_dft(pinv,tmp1,tmp2);

  fftw_free(data2);
  fftw_free(tmp1);
}





class ComputeSlice
{
  public:
    ComputeSlice (const vector<size_t>& outer_axes, const vector<size_t>& slice_axes, const size_t& nsh, const size_t& minW, const size_t& maxW, Image<value_type>& in, Image<value_type>& out) :
      outer_axes (outer_axes),
      slice_axes (slice_axes),
      nsh (nsh),
      minW (minW),
      maxW (maxW),
      in (in), 
      out (out) { }

    void operator() (const Iterator& pos)
    {
      assign_pos_of (pos, outer_axes).to (in, out);
      size_t dims[2];
      dims[0] = in.size(slice_axes[0]);
      dims[1] = in.size(slice_axes[1]);
      fftw_complex* in_complex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*dims[0]*dims[1]);
      size_t i = 0;
      for (auto l = Loop (slice_axes) (in); l; ++l) {
        in_complex[i][0] = in.value();
        in_complex[i++][1] = 0;
      }
      fftw_complex* out_complex  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*dims[0]*dims[1]);
      unring_2d(in_complex,out_complex,dims,nsh,minW,maxW);
      fftw_free(in_complex);
      i = 0;
      for (auto l = Loop (slice_axes) (out); l; ++l)
        out.value() = out_complex[i++][0];

      fftw_free(out_complex);
    }

  private:
    const vector<size_t>& outer_axes;
    const vector<size_t>& slice_axes;
    const size_t nsh;
    const size_t minW;
    const size_t maxW;
    Image<value_type> in, out; 
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

