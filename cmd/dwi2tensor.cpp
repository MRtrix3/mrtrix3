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

#include "app.h"
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

MRTRIX_APPLICATION

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
    + Option ("grad",
      "specify the diffusion-weighted gradient scheme used in the acquisition. "
      "The program will normally attempt to use the encoding stored in image header. "
      "This should be supplied as a 4xN text file with each line is in the format "
      "[ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, "
      "and b gives the b-value in units (1000 s/mm^2).")
    + Argument ("encoding").type_file ()

    + Option ("mask",
      "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in ()

    + Option ("method",
      "select method used to perform the fitting (Valid choices are: loglinear, "
      "nonlinear, sech, rician. Default: non-linear)")
    + Argument ("name").type_choice (method_choices)

    + Option ("ignoreslices",
      "ignore the image slices specified when computing the tensor. "
      "A single slice (z) coordinate should be supplied, followed by a list "
      "of volume numbers to be ignored in the computation.")
    .allow_multiple()
    + Argument ("slice").type_integer ()
    + Argument ("volumes").type_sequence_int ()

    + Option ("ignorevolumes",
      "ignore the image volumes specified when computing the tensor.")
    .allow_multiple()
    + Argument ("volumes").type_sequence_int ()

    + Option ("regularisation",
      "specify the strength of the regularisation term on the magnitude of the "
      "tensor elements (default = 5000). This only applies to the non-linear methods.")
    + Argument ("term").type_float (0.0, 5000.0, 1e12);


}


typedef float value_type;


class Cost
{
  public:
    typedef float value_type;

    Cost (const Math::Matrix<value_type>& bmatrix, int method, const value_type regularisation_term) :
      fitting_method (method),
      diag_bij (0.0),
      offdiag_bij (0.0),
      regularisation (regularisation_term)
    {
      for (size_t i = 0; i < bmatrix.rows(); i++) {
        size_t j = 0;
        for (; j < 3; j++) diag_bij += Math::pow2 (bmatrix(i,j));
        for (; j < 6; j++) offdiag_bij += Math::pow2 (bmatrix(i,j));
      }
      diag_bij = Math::sqrt (3.0*bmatrix.rows() / diag_bij);
      offdiag_bij = Math::sqrt (3.0*bmatrix.rows() / offdiag_bij);
    }

    size_t nm () const { return (B->rows()); }
    size_t size () const { return (8); }

    void set_bmatrix (const Math::Matrix<value_type>* bmatrix, const Math::Matrix<value_type>* binverse)
    {
      B = bmatrix;
      binv = binverse;

      A.allocate (nm());
      dP.allocate (nm());
    }

    void set_voxel (const Math::Vector<value_type>* signals, const Math::Vector<value_type>* dt)
    {
      S = signals;
      dt_init = dt;
    }

    value_type init (Math::Vector<value_type>& x)
    {
      for (size_t i = 0; i < 7; ++i)
        x[i] = (*dt_init)[i];

      //std::cerr << "dt: "; for (int i = 0; i < 7; i++) std::cerr << x[i] << " "; std::cerr << "\n";
      value_type lnM0_cond = exp(x[6]-4.0);
      cond[0] = lnM0_cond * diag_bij / MAX (x[0], 1e-4);
      cond[1] = lnM0_cond * diag_bij / MAX (x[1], 1e-4);
      cond[2] = lnM0_cond * diag_bij / MAX (x[2], 1e-4);
      noise_multiplier = lnM0_cond * 1000.0 / Math::exp (x[6]);
      offdiag_multiplier = lnM0_cond * offdiag_bij;
      //std::cerr << "cond: "; for (int i = 0; i < 3; i++) std::cerr << cond[i] << " "; std::cerr << noise_multiplier << " " << lnM0_cond << "\n";
      x[0] = Math::log (MAX (x[0], 1e-7)) / cond[0];
      x[1] = Math::log (MAX (x[1], 1e-7)) / cond[1];
      x[2] = Math::log (MAX (x[2], 1e-7)) / cond[2];
      x[3] /= offdiag_multiplier;
      x[4] /= offdiag_multiplier;
      x[5] /= offdiag_multiplier;
      x[7] = 2.0 * (Math::log (noise_multiplier) - x[6])/noise_multiplier;
      //std::cerr << "init[x]: "; for (int i = 0; i < 8; i++) std::cerr << x[i] << " "; std::cerr << "\n";
      return (0.1);
    }





    value_type operator() (const Math::Vector<value_type>& x, Math::Vector<value_type>& dE)
    {
      for (size_t i = 0; i < nm(); i++) {
        value_type v =
          - (*B)(i,0) * Math::exp (cond[0] * x[0])
          - (*B)(i,1) * Math::exp (cond[1] * x[1])
          - (*B)(i,2) * Math::exp (cond[2] * x[2])
          - offdiag_multiplier * (
              (*B)(i,3) * x[3] +
              (*B)(i,4) * x[4] +
              (*B)(i,5) * x[5] )
          + x[6];

        A[i] = Math::exp (v);
        assert (finite (A[i]));
      }

      value_type noise = Math::exp (noise_multiplier * x[7]);
      value_type E = NAN;

      if (fitting_method == 1) // nonlinear
        E = Math::Gaussian::lnP (*S, A, noise, dP, dE[7]);
      else if (fitting_method == 2) // sech
        E = Math::Sech::lnP (*S, A, noise, dP, dE[7]);
      else if (fitting_method == 3) // rician
        E = Math::Rician::lnP (*S, A, noise, dP, dE[7]);

      assert (finite (E));

      value_type reg = 0.0;
      for (size_t i = 0; i < 3; i++)
        reg += Math::pow2 (Math::exp (cond[i] * x[i]));

      for (size_t i = 3; i < 6; i++)
        reg += Math::pow2 (offdiag_multiplier * x[i]);

      E += regularisation * reg;

      value_type sum_Si = 0.0;
      for (size_t i = 0; i < nm(); i++) {
        A[i] *= dP[i];
        sum_Si += A[i];
      }
      dE[6] = sum_Si;
      dE[7] *= noise_multiplier * noise;

      for (size_t j = 0; j < 3; j++) {
        value_type v = 0.0;
        for (size_t i = 0; i < nm(); i++) v -= (*B)(i,j) * A[i];
        value_type dj = Math::exp (cond[j] * x[j]);
        dE[j] = (2.0 * regularisation + cond[j] * v) * dj;
      }

      for (size_t j = 3; j < 6; j++) {
        value_type v = 0.0;
        for (size_t i = 0; i < nm(); i++)
          v -= (*B)(i,j) * A[i];
        dE[j] = offdiag_multiplier * (2.0 * regularisation * x[j] + v);
      }
      return (E);
    }



    void get_values (Math::Vector<value_type>& x, const Math::Vector<value_type>& state) const
    {
      for (int i = 0; i < 3; i++)
        x[i] = Math::exp (cond[i] * state[i]);
      for (int i = 3; i < 6; i++)
        x[i] = offdiag_multiplier * state[i];
      x[6] = Math::exp (state[6]);
    }



    void print (const Math::Vector<value_type>& x) const
    {
      for (int i = 0; i < 3; i++)
        std::cout << Math::exp (cond[i] * x[i]) << " ";
      for (int i = 3; i < 6; i++)
        std::cout << offdiag_multiplier * x[i] << " ";
      std::cout << Math::exp (x[6]) << " " << 1.0/Math::sqrt (Math::exp (noise_multiplier * x[7])) << "\n";
    }

    void test (const Math::Vector<value_type>& x)
    {
      Math::Vector<value_type> p (8), dE (8);
      for (value_type dx = -0.1; dx < 0.1; dx += 0.001) {
        for (size_t i = 0; i < 8; i++) {
          p = x;
          p[i] = x[i] + dx;
          std::cout << operator() (p, dE) << " ";
        }
        std::cout << "\n";
      }
    }

  protected:
    const Math::Matrix<value_type> *B, *binv;
    const Math::Vector<value_type> *S, *dt_init;
    Math::Vector<value_type> A, dP;
    int fitting_method;
    value_type diag_bij, offdiag_bij, offdiag_multiplier, noise_multiplier, regularisation;
    value_type cond[3];
};



class Item
{
  public:
    Math::Matrix<value_type> dwi;
    RefPtr<Math::Matrix<value_type> > B, binv;
    std::vector<bool> in_mask;
    ssize_t y, z;
};



typedef Thread::Queue<Item> Queue;


class DataLoader
{
  public:
    DataLoader (Queue& queue,
        Image::BufferPreload<value_type>& dwi_data,
        Ptr<Image::Buffer<value_type>::voxel_type>& mask,
        const Math::Matrix<value_type>& bmatrix,
        const std::vector<int>& volumes_to_be_ignored,
        const std::vector<std::vector<int> >& slices_to_be_ignored) :
      writer (queue),
      dwi (dwi_data),
      mask (mask),
      bmat (bmatrix),
      islices (slices_to_be_ignored) {
        for (int i = 0; i < dwi.dim(3); i++)
          volumes.push_back (i);
        for (size_t i = 0; i < volumes_to_be_ignored.size(); i++)
          std::remove (volumes.begin(), volumes.end(), volumes_to_be_ignored[i]);
      }

    void execute ()
    {
      Queue::Writer::Item item (writer);
      ProgressBar progress ("converting DW images to tensor image...", dwi.dim(1)*dwi.dim(2));
      if (mask) {
        for (dwi[2] = (*mask)[2] = 0; dwi[2] < dwi.dim(2); ++dwi[2], ++(*mask)[2]) {
          prepare_slice();
          for (dwi[1] = (*mask)[1] = 0; dwi[1] < dwi.dim(1); ++dwi[1], ++(*mask)[1]) {
            load_row (*mask, item);
            ++progress;
          }
        }
      }
      else {
        for (dwi[2] = 0; dwi[2] < dwi.dim(2); ++dwi[2]) {
          prepare_slice();
          for (dwi[1] = 0; dwi[1] < dwi.dim(1); ++dwi[1]) {
            load_row (item);
            ++progress;
          }
        }
      }
    }


  private:
    Queue::Writer writer;
    Image::BufferPreload<value_type>::voxel_type  dwi;
    Ptr<Image::Buffer<value_type>::voxel_type > mask;
    const Math::Matrix<value_type>& bmat;
    RefPtr<Math::Matrix<value_type> > B, binv;
    std::vector<size_t> volumes;
    const std::vector<std::vector<int> > islices;

    void prepare_slice ()
    {
      std::vector<size_t> svols (volumes);
      const std::vector<int>& islc (islices[dwi[2]]);
      for (size_t i = 0; i < islc.size(); i++)
        std::remove (svols.begin(), svols.end(), islc[i]);

      B = new Math::Matrix<value_type> (svols.size(), 7);
      for (size_t i = 0; i < svols.size(); i++)
        for (size_t j = 0; j < 7; j++)
          (*B)(i,j) = bmat(svols[i],j);

      binv = new Math::Matrix<value_type> (B->columns(), B->rows());
      Math::pinv (*binv, *B);
    }

    void load_row (Image::Buffer<value_type>::voxel_type& mask, Queue::Writer::Item& item)
    {
      item->in_mask.resize (dwi.dim(0));
      size_t nvox = 0;
      for (mask[0] = 0; mask[0] < mask.dim(0); ++mask[0]) {
        bool in_mask (mask.value() > 0.5);
        item->in_mask[mask[0]] = in_mask;
        if (in_mask) ++nvox;
      }

      if (nvox == 0) return;

      item->dwi.allocate (nvox, B->rows());

      size_t index = 0;
      for (dwi[0] = 0; dwi[0] < dwi.dim(0); ++dwi[0])
        if (item->in_mask[dwi[0]])
          load (item, index++);

      dispatch (item);
    }

    void load_row (Queue::Writer::Item& item)
    {
      item->dwi.allocate (dwi.dim(0), dwi.dim(3));
      item->in_mask.clear();
      for (dwi[0] = 0; dwi[0] < dwi.dim(0); ++dwi[0])
        load (item, dwi[0]);
      dispatch (item);
    }

    void load (Queue::Writer::Item& item, size_t index)
    {
      for (dwi[3] = 0; dwi[3] < dwi.dim(3); ++dwi[3]) {
        float val = dwi.value();
        if (val <= 0.0) val = 1.0;
        item->dwi(index, dwi[3]) = val;
      }
    }

    void dispatch (Queue::Writer::Item& item)
    {
      item->y = dwi[1];
      item->z = dwi[2];
      item->B = B;
      item->binv = binv;

      if (!item.write())
        throw Exception ("error writing to work queue");
    }
};









class Processor
{
  public:
    Processor (Queue& queue, Image::Buffer<value_type> & data,
        const Math::Matrix<value_type>& bmatrix,
        int fitting_method, int max_num_iterations,
        const value_type regularisation_term = 5000.0) :
      reader (queue),
      DT (data),
      cost (bmatrix, fitting_method, regularisation_term),
      method (fitting_method),
      niter (max_num_iterations) { }

    void execute ()
    {
      Queue::Reader::Item item (reader);
      Math::Matrix<value_type> dt (DT.dim(0), 7);

      while (item.read()) {
        dt.resize (item->dwi.columns(), 7);
        loglinear (dt, *item->binv, item->dwi);

        if (method > 0) {
          cost.set_bmatrix (item->B, item->binv);
          for (size_t i = 0; i < item->dwi.rows(); ++i) {
            const Math::Vector<value_type> signal (item->dwi.row(i));
            Math::Vector<value_type> values (dt.row(i));

            cost.set_voxel (&signal, &values);
            Math::GradientDescent<Cost> optim (cost);
            try { optim.run (10000, 1e-8); }
            catch (Exception) { }
            cost.get_values (values, optim.state());
          }
        }

        DT[1] = item->y;
        DT[2] = item->z;
        size_t index = 0;
        for (DT[0] = 0; DT[0] < DT.dim(0); ++DT[0]) {
          if (item->in_mask.size()) {
            if (!item->in_mask[DT[0]]) {
              for (DT[3] = 0; DT[3] < 6; ++DT[3])
                DT.value() = 0.0;
              continue;
            }
          }

          for (DT[3] = 0; DT[3] < 6; ++DT[3])
            DT.value() = dt(index, DT[3]);

          ++index;
        }

      }
    }

  private:
    Queue::Reader reader;
    Image::Buffer<value_type>::voxel_type DT;
    Cost cost;
    int method, niter;

    void loglinear (Math::Matrix<value_type>& x, const Math::Matrix<value_type>& binv, const Math::Matrix<value_type>& dwi) const
    {
      x.allocate (dwi.rows(), binv.rows());
      Math::Matrix<value_type> log_dwi (dwi.rows(), dwi.columns());
      for (size_t i = 0; i < dwi.rows(); i++)
        for (size_t j = 0; j < dwi.columns(); j++)
          log_dwi(i,j) = -Math::log (dwi(i,j));
      Math::mult (x, value_type(0.0), value_type(1.0), CblasNoTrans, log_dwi, CblasTrans, binv);
    }
};




void run()
{

  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  Image::BufferPreload<value_type> dwi_data (argument[0], strides);

  Image::Header dt_header (dwi_data);

  if (dt_header.ndim() < 4)
    throw Exception ("dwi image should contain at least 4 dimensions");

  size_t dwi_dim = 3;
  while (dt_header.dim (dwi_dim) < 2) ++dwi_dim;
  inform ("assuming DW images are stored along axis " + str (dwi_dim));

  Math::Matrix<value_type> grad, bmat;

  Options opt = get_options ("grad");
  if (opt.size()) grad.load (opt[0][0]);
  else {
    if (!dt_header.DW_scheme().is_set())
      throw Exception ("no diffusion encoding found in image \"" + dt_header.name() + "\"");
    grad = dt_header.DW_scheme();
  }

  if (grad.rows() < 7 || grad.columns() != 4)
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  inform ("found " + str(grad.rows()) + "x" + str(grad.columns()) + " diffusion-weighted encoding");

  if (dt_header.dim(dwi_dim) != (int) grad.rows())
    throw Exception ("number of studies in base image does not match that in encoding file");

  DWI::normalise_grad (grad);
  DWI::grad2bmatrix (bmat, grad);

  std::vector<std::vector<int> > islc (dt_header.dim(2));
  std::vector<int> ivol;

  opt = get_options ("ignoreslices");
  for (size_t n = 0; n < opt.size(); n++) {
    int z = opt[n][0];
    if (z >= (int) islc.size()) throw Exception ("slice number out of bounds");
    islc[z] = opt[n][1];
  }

  opt = get_options ("ignorevolumes");
  for (size_t n = 0; n < opt.size(); n++) {
    std::vector<int> v = opt[n][0];
    ivol.insert (ivol.end(), v.begin(), v.end());
  }

  int method = 1;
  opt = get_options ("method");
  if (opt.size()) method = opt[0][0];

  opt = get_options ("regularisation");
  value_type regularisation = 5000.0;
  if (opt.size()) regularisation = opt[0][0];

  opt = get_options ("mask");
  Ptr<Image::Buffer<value_type> > mask_data;
  Ptr<Image::Buffer<value_type>::voxel_type> mask_voxel;
  if (opt.size()){
    mask_data = new Image::Buffer<value_type> (opt[0][0]);
    mask_voxel = new Image::Buffer<value_type>::voxel_type (*mask_data);
    Image::check_dimensions (*mask_voxel, dt_header, 0, 3);
  }

  dt_header.set_ndim (4);
  dt_header.dim (3) = 6;
  dt_header.datatype() = DataType::Float32;
  dt_header.stride(0) = 2;
  dt_header.stride(1) = 3;
  dt_header.stride(2) = 4;
  dt_header.stride(3) = 1;

  dt_header.DW_scheme().clear();

  Image::Buffer<value_type> dt_data (argument[1], dt_header);

  Queue queue ("item queue");

  DataLoader loader (queue, dwi_data, mask_voxel, bmat, ivol, islc);
  Processor processor (queue, dt_data, bmat, method, 10000, regularisation);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}

