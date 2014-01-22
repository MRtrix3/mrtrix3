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

#include <algorithm>

#include "command.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "math/rician.h"
#include "math/gaussian.h"
#include "math/sech.h"
#include "math/least_squares.h"
#include "math/gradient_descent.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"

#include "math/check_gradient.h"


using namespace std;
using namespace MR;
using namespace App;


const char* method_choices[] = { "loglinear", "nonlinear", "sech", "rician", NULL };

void usage ()
{

  DESCRIPTION
    + "convert diffusion-weighted images to tensor images." +
      "The following five algorithms are available to perform the fit:"
      "loglinear: standard log-linear least-squares fit."
      "wloglinear: weighted log-linear least-squares fit."
      "nonlinear: non-linear least-squares fit, with positivity constraint on diagonal elements of tensor."
      "sech: non-linear fit assuming a sech() noise model, with positivity constraint on diagonal elements of tensor. This method has improved robustness to outliers."
      "rician: non-linear fit assuming a Rician noise model, with positivity constraint on diagonal elements of tensor.";

  ARGUMENTS
  + Argument ("dwi", "the input diffusion-weighted image.").type_image_in ()
  + Argument ("tensor", "the output diffusion tensor image.").type_image_out ();


  OPTIONS
    + Option ("mask",
      "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in ()

    + Option ("method",
      "select method used to perform the fitting (Valid choices are: loglinear, "
      "nonlinear, sech, rician. Default: non-linear)")
    + Argument ("name").type_choice (method_choices)

    + Option ("regularisation",
      "specify the strength of the regularisation term on the magnitude of the "
      "tensor elements (default = 5000). This only applies to the non-linear methods.")
    + Argument ("term").type_float (0.0, 5000.0, 1e12)

    + DWI::GradOption;


}


typedef float value_type;
typedef double cost_value_type;
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<value_type> OutputBufferType;
typedef Image::Buffer<bool> MaskBufferType;


class Cost
{
  public:
    typedef cost_value_type value_type;

    Cost (const Math::Matrix<cost_value_type>& b_matrix, int method, const cost_value_type regularisation_term) :
      bmatrix (b_matrix),
      A (nm()),
      dP (nm()),
      fitting_method (method),
      diag_bij (0.0),
      offdiag_bij (0.0),
      regularisation (regularisation_term) {
        for (size_t i = 0; i < bmatrix.rows(); i++) {
          size_t j = 0;
          for (; j < 3; j++) 
            diag_bij += Math::pow2 (bmatrix(i,j));
          for (; j < 6; j++) 
            offdiag_bij += Math::pow2 (bmatrix(i,j));
        }
        diag_bij = Math::sqrt (3.0*bmatrix.rows() / diag_bij);
        offdiag_bij = Math::sqrt (3.0*bmatrix.rows() / offdiag_bij);
      }

    size_t nm () const { return bmatrix.rows(); }
    size_t size () const { return 8; }

    void set_voxel (const Math::Vector<cost_value_type>* signals, const Math::Vector<cost_value_type>* dt)
    {
      S = signals;
      dt_init = dt;
    }

    cost_value_type init (Math::Vector<cost_value_type>& x)
    {
      for (size_t i = 0; i < 7; ++i)
        x[i] = (*dt_init)[i];
      noise_multiplier = 1.5e3;
      b0_multiplier = 1.0e2;
      x[6] /= b0_multiplier;
      x[7] = 2.0 * (Math::log (noise_multiplier) - x[6])/noise_multiplier;
      return 0.01 * (x[0]+x[1]+x[2]);
    }





    cost_value_type operator() (const Math::Vector<cost_value_type>& x, Math::Vector<cost_value_type>& dE)
    {
      for (size_t i = 0; i < nm(); i++) {
        cost_value_type v =
          - bmatrix(i,0) * x[0]
          - bmatrix(i,1) * x[1]
          - bmatrix(i,2) * x[2]
          - bmatrix(i,3) * x[3] 
          - bmatrix(i,4) * x[4] 
          - bmatrix(i,5) * x[5] 
          + b0_multiplier * x[6];

        A[i] = Math::exp (v);
        assert (std::isfinite (A[i]));
      }

      cost_value_type noise = Math::exp (noise_multiplier * x[7]);
      cost_value_type E = NAN;

      if (fitting_method == 1) // nonlinear
        E = Math::Gaussian::lnP (*S, A, noise, dP, dE[7]);
      else if (fitting_method == 2) // sech
        E = Math::Sech::lnP (*S, A, noise, dP, dE[7]);
      else if (fitting_method == 3) // rician
        E = Math::Rician::lnP (*S, A, noise, dP, dE[7]);

      assert (std::isfinite (E));

      cost_value_type reg = 0.0;
      for (size_t i = 0; i < 6; i++)
        reg += Math::pow2 (x[i]);

      E += regularisation * reg;

      cost_value_type sum_Si = 0.0;
      for (size_t i = 0; i < nm(); i++) {
        A[i] *= dP[i];
        sum_Si += A[i];
      }
      dE[6] = sum_Si * b0_multiplier;
      dE[7] *= noise_multiplier * noise;

      for (size_t j = 0; j < 6; j++) {
        cost_value_type v = 0.0;
        for (size_t i = 0; i < nm(); i++) 
          v -= bmatrix(i,j) * A[i];
        dE[j] = 2.0 * regularisation * x[j] + v;
      }
      return E;
    }



    void get_values (Math::Vector<cost_value_type>& x, const Math::Vector<cost_value_type>& state) const
    {
      for (int i = 0; i < 6; i++)
        x[i] = state[i];
      x[6] = Math::exp (b0_multiplier * state[6]);
    }



    void print (const Math::Vector<cost_value_type>& x) const
    {
      for (int i = 0; i < 6; i++)
        std::cout << x[i] << " ";
      std::cout << Math::exp (b0_multiplier * x[6]) << " " << 1.0/Math::sqrt (Math::exp (noise_multiplier * x[7])) << "\n";
    }

    void test (const Math::Vector<cost_value_type>& x)
    {
      Math::Vector<cost_value_type> p (8), dE (8);
      for (cost_value_type dx = -0.1; dx < 0.1; dx += 0.001) {
        for (size_t i = 0; i < 8; i++) {
          p = x;
          p[i] = x[i] + dx;
          std::cout << operator() (p, dE) << " ";
        }
        std::cout << "\n";
      }
    }

  protected:
    const Math::Matrix<cost_value_type>& bmatrix;
    const Math::Vector<cost_value_type> *S, *dt_init;
    Math::Vector<cost_value_type> A, dP;
    int fitting_method;
    cost_value_type diag_bij, offdiag_bij, offdiag_multiplier, noise_multiplier, regularisation;
    cost_value_type diag_cond, b0_multiplier;
};









class Processor 
{
  public:
    Processor (
        InputBufferType::voxel_type& dwi_vox, 
        OutputBufferType::voxel_type& dt_vox,
        Ptr<MaskBufferType::voxel_type>& mask_vox,
        const Math::Matrix<cost_value_type>& bmatrix,
        const Math::Matrix<cost_value_type>& inverse_bmatrix,
        int fitting_method, 
        const cost_value_type regularisation_term,
        ssize_t inner_axis,
        ssize_t dwi_axis = 3) :
      dwi (dwi_vox),
      dt (dt_vox),
      mask (mask_vox),
      cost (bmatrix, fitting_method, regularisation_term),
      binv (inverse_bmatrix),
      method (fitting_method),
      reg_norm (regularisation_term),
      row_axis (inner_axis),
      sig_axis (dwi_axis) { 
      }



    void operator () (const Image::Iterator& pos) {
      if (!load_data (pos)) 
        return;

      // compute tensors via log-linear least-squares:
      Math::mult (tensors, cost_value_type(0.0), cost_value_type(1.0), CblasNoTrans, logsignals, CblasTrans, binv);

      if (method > 0) 
        solve_nonlinear();

      write_back ();
    }


  protected:
    InputBufferType::voxel_type dwi;
    OutputBufferType::voxel_type dt;
    Ptr<MaskBufferType::voxel_type> mask;
    Math::Matrix<cost_value_type> signals, logsignals, tensors;
    Cost cost;

    const Math::Matrix<cost_value_type>& binv;

    const int method;
    const cost_value_type reg_norm;
    const size_t row_axis, sig_axis;


    bool load_data (const Image::Iterator& pos) {
      Image::voxel_assign (dwi, pos);
      Image::voxel_assign (dt, pos);

      size_t nvox = dwi.dim (row_axis);
      if (mask) {
        size_t N = 0;
        Image::voxel_assign (*mask, pos);
        for ((*mask)[row_axis] = 0; (*mask)[row_axis] < mask->dim(row_axis); ++(*mask)[row_axis]) 
          if (mask->value())
            ++N;
        nvox = N;
      }
      if (!nvox) 
        return false;

      signals.allocate (nvox, dwi.dim (sig_axis));
      logsignals.allocate (nvox, dwi.dim (sig_axis));
      tensors.allocate (nvox, 7);

      size_t N = 0;
      for (dwi[row_axis] = 0; dwi[row_axis] < dwi.dim(row_axis); ++dwi[row_axis]) {
        if (mask) {
          (*mask)[row_axis] = dwi[row_axis];
          if (!mask->value()) continue;
        }
        for (dwi[sig_axis] = 0; dwi[sig_axis] < dwi.dim(sig_axis); ++dwi[sig_axis]) {
          cost_value_type val = std::max (cost_value_type (dwi.value()), cost_value_type (1.0));
          signals(N, dwi[sig_axis]) = val;
          logsignals(N, dwi[sig_axis]) = -Math::log (val);
        }
        ++N;
      }
      return true;
    }



    void write_back () {
      size_t N = 0;
      for (dt[row_axis] = 0; dt[row_axis] < dt.dim(row_axis); ++dt[row_axis]) {
        if (mask) {
          (*mask)[row_axis] = dt[row_axis];
          if (!mask->value()) continue;
        }
        for (dt[3] = 0; dt[3] < dt.dim(3); ++dt[3]) 
          dt.value() = tensors (N, dt[3]);
        ++N;
      }

    }


    void solve_nonlinear () {
      for (size_t i = 0; i < signals.rows(); ++i) {
        const Math::Vector<cost_value_type> signal (signals.row(i));
        Math::Vector<cost_value_type> values (tensors.row(i));

        cost.set_voxel (&signal, &values);

        Math::Vector<cost_value_type> x (cost.size());
        cost.init (x);
        //Math::check_function_gradient (cost, x, 1e-10, true);

        Math::GradientDescent<Cost> optim (cost);
        try { optim.run (10000, 1e-8); }
        catch (Exception& E) {
          E.display();
        }

        //x = optim.state();
        //Math::check_function_gradient (cost, x, 1e-10, true);

        cost.get_values (values, optim.state());
      }
    }

};











void run()
{
  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  InputBufferType dwi_buffer (argument[0], strides);
  Math::Matrix<cost_value_type> grad = DWI::get_valid_DW_scheme<cost_value_type> (dwi_buffer);

  size_t dwi_axis = 3;
  while (dwi_buffer.dim (dwi_axis) < 2) ++dwi_axis;
  INFO ("assuming DW images are stored along axis " + str (dwi_axis));

  DWI::normalise_grad (grad);
  Math::Matrix<cost_value_type> bmatrix;
  DWI::grad2bmatrix (bmatrix, grad);

  Math::Matrix<cost_value_type> binv (bmatrix.columns(), bmatrix.rows());
  Math::pinv (binv, bmatrix);

  int method = 1;
  Options opt = get_options ("method");
  if (opt.size()) method = opt[0][0];

  opt = get_options ("regularisation");
  cost_value_type regularisation = 5000.0;
  if (opt.size()) regularisation = opt[0][0];

  opt = get_options ("mask");
  Ptr<MaskBufferType> mask_buffer;
  Ptr<MaskBufferType::voxel_type> mask_vox;
  if (opt.size()){
    mask_buffer = new MaskBufferType (opt[0][0]);
    Image::check_dimensions (*mask_buffer, dwi_buffer, 0, 3);
    mask_vox = new MaskBufferType::voxel_type (*mask_buffer);
  }


  Image::Header dt_header (dwi_buffer);
  dt_header.set_ndim (4);
  dt_header.dim (3) = 6;
  dt_header.datatype() = DataType::Float32;
  dt_header.stride(0) = dt_header.stride(1) = dt_header.stride(2) = 0;
  dt_header.stride(3) = 1;
  dt_header.DW_scheme() = grad;

  OutputBufferType dt_buffer (argument[1], dt_header);

  InputBufferType::voxel_type dwi_vox (dwi_buffer);
  OutputBufferType::voxel_type dt_vox (dt_buffer);

  Image::ThreadedLoop loop ("estimating tensor components...", dwi_vox, 1, 0, 3);
  Processor processor (dwi_vox, dt_vox, mask_vox, bmatrix, binv, method, regularisation, loop.inner_axes()[0], dwi_axis);

  loop.run_outer (processor);
}

