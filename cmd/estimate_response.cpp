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

#include "app.h"
#include "image/thread_voxelwise.h"
#include "progressbar.h"
#include "file/config.h"
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "math/gradient_descent.h"
#include "math/matrix.h"
#include "math/least_squares.h"
#include "math/eigen.h"
#include "math/SH.h"
#include "math/rician.h"

using namespace std; 
using namespace MR; 


class ResponseEstimator : public Image::ThreadVoxelWise {
  public:

    typedef float T;

    ResponseEstimator (Image::Object& dwi_object, RefPtr<Image::Voxel> mask_voxel, Math::Matrix<T>& grad, int up_to_lmax) :
      Image::ThreadVoxelWise (dwi_object, mask_voxel) {

        DWI::normalise_grad (grad);
        Math::Matrix<T> bmat;
        DWI::grad2bmatrix (bmat, grad);
        Math::pinv (binv, bmat);

        std::vector<int> bzeros;
        DWI::guess_DW_directions (dwis, bzeros, grad);
        info ("found " + str(dwis.size()) + " diffusion-weighted directions");

        Math::Matrix<T> dirs;
        DWI::gen_direction_matrix (dirs, grad, dwis);

        if (up_to_lmax < 2) up_to_lmax = Math::SH::LforN (dwis.size());
        info ("calculating even spherical harmonic components up to order " + str(up_to_lmax));
        Math::SH::init_transform (SHT, dirs, up_to_lmax);
        Math::pinv (iSHT, SHT);

        mean_response.allocate (up_to_lmax/2+1);
      }



    void run (const char* dump_file = NULL) {
      count = 0;
      mean_response.zero();
      if (dump_file) {
        dump.open (dump_file, std::ios::out | std::ios::binary);
        if (!dump) throw Exception (std::string ("error creating response dump file \"") + dump_file + "\":" + strerror(errno));
      }

      Image::ThreadVoxelWise::run ("estimating response function coefficients...");

      if (dump.is_open()) dump.close();

      for (size_t n = 0; n < mean_response.size(); n++) mean_response[n] /= count;
    }



    void write_to_file (const std::string& filename) const
    {
      std::ofstream out (filename.c_str());
      for (size_t i = 0; i < mean_response.size(); i++)
        out << mean_response[i] << " ";
      out << "\n";
    }



  protected:
    int count;
    Mutex store_mutex;
    Math::Matrix<T> binv, SHT, iSHT;
    std::vector<int>  dwis;
    bool              done;
    Math::Vector<T>   mean_response;
    std::ofstream     dump;

    bool get_data (Image::Voxel& dwi, Math::VectorView<T>& all_sigs, Math::VectorView<T>& DW_sigs)
    {
      for (dwi[3] = 0; dwi[3] < source.dim(3); dwi[3]++) {
        all_sigs[dwi[3]] = dwi.value();
        if (isnan (all_sigs[dwi[3]])) return (true); 
        if (all_sigs[dwi[3]] <= 0.0) all_sigs[dwi[3]] = 0.1;
      }

      for (size_t n = 0; n < dwis.size(); n++) 
        DW_sigs[n] = all_sigs[dwis[n]];

      return (false);
    }





    void store (const T* state) 
    {
      Mutex::Lock lock (store_mutex);
      count++;
      for (size_t i = 0; i < mean_response.size(); i++) {
        mean_response[i] += state[i+1];
        if (dump.is_open()) dump << state[i+1] << " ";
      }
      if (dump.is_open()) dump << "\n";
    }


    void execute (int threadID)
    {
      Cost cost (*this);
      Math::Optim::GradientDescent<Cost,T> optim (cost);
      Math::Vector<T> DW_sigs (dwis.size());
      Math::Vector<T> all_sigs (source.dim(3));
      Image::Voxel dwi (source);

      do { 
        if (get_next (dwi)) return;
        if (get_data (dwi, cost.signals, cost.DW_signals)) continue;

        try { optim.run(); }
        catch (Exception) { }
        store (optim.state());
      } while (true);
    }



    class Cost {
      public:
        Cost (const ResponseEstimator& parent) : 
          signals (parent.source.dim(3)),
          DW_signals (parent.dwis.size()),
          C (parent), 
          lmax (2*(C.mean_response.size()-1)), 
          nSH (Math::SH::NforL (lmax)), 
          delta (nSH),
          sh (C.iSHT.rows()),
          dt (7),
          S (C.dwis.size()),
          dS (C.dwis.size()),
          A (C.dwis.size(), C.mean_response.size()),  
          D (3,3),
          V (3,3),
          ev (3),
          eig (3),
          AL0 (new T [lmax+1]),
          AL1 (new T [lmax+1]) {
            Math::Legendre::Plm_sph<T> (AL0, lmax, 0, 0.0);
            Math::Legendre::Plm_sph<T> (AL1, lmax, 0, 1.0);
          } 

        ~Cost () { delete [] AL0; delete [] AL1; }

        int  nm () const { return (C.dwis.size()); }
        int  size () const { return (1+C.mean_response.size()); }

        void print (const T* x) const 
        { 
          std::cerr << "final state: noise = " << x[0] << " [ " << 1.0 / Math::sqrt(Math::exp (noise_multiplier * x[0])) << " ], SH coefs = [ ";
          for (size_t i = 0; i < C.mean_response.size(); i++) {
            std::cerr << x[i+1] << " ";
          }
          std::cerr << "]\n";
        }

        T init (T* x) 
        {
          for (int i = 0; i < C.source.dim(3); i++) 
            signals[i] = -log (signals[i]);

          Math::mult (dt, C.binv, signals);

          D(0,0) = dt[0];
          D(1,1) = dt[1];
          D(2,2) = dt[2];
          D(0,1) = D(1,0) = dt[3];
          D(0,2) = D(2,0) = dt[4];
          D(1,2) = D(2,1) = dt[5];

          eig (ev, D, V);
          Math::Eigen::sort (ev, V);
          Math::SH::delta (delta, Point (V(0,2), V(1,2), V(2,2)), lmax);

          Math::mult (sh, C.iSHT, DW_signals);

          T noise = 0.0;
          T mean_DW = 0.0;
          for (size_t i = 0; i < C.SHT.rows(); i++) {
            T v = -DW_signals[i];
            mean_DW += DW_signals[i];
            for (size_t j = 0; j < C.SHT.columns(); j++)
              v += C.SHT(i,j) * sh[j];
            noise += Math::pow2 (v);
          }
          T std = sqrt (noise/T(nm()));

          mean_DW *= AL1[0] / (T(nm()) * delta[0] * C.SHT(0,0));
          for (size_t l = 0; l < C.mean_response.size(); l++) 
            x[l+1] = mean_DW * AL0[2*l];

          for (size_t i = 0; i < C.SHT.rows(); i++) {
            size_t l = 0, n_m = 1;
            A(i,0) = 0.0;
            for (size_t j = 0; j < C.SHT.columns(); j++) {
              if (j == n_m) { l++; n_m += 4*l+1; A(i,l) = 0.0; }
              A(i,l) += C.SHT(i,j) * delta[j];
            }
            for (size_t l = 0; l < A.columns(); l++) A(i,l) /= AL1[2*l];
          }

          noise_multiplier = A(0,0) / std;
          negative_signal_multiplier = 5.0 / Math::pow2 (std);
          x[0] = - 2.0 * Math::log (std) / noise_multiplier; 

          T step_size = 1.0 * sqrt (Math::pow2(std) * nm() * Math::pow2 (A(0,0)));
          return (step_size);
        }




        T operator() (const T* x, T* dE)
        {
          T noise = Math::exp (noise_multiplier * x[0]);
          T& dnoise (dE[0]);

          const Math::VectorView<T> R (const_cast<T*> (x+1), C.mean_response.size());
          Math::VectorView<T> dR (dE+1, C.mean_response.size());

          Math::mult (S, A, R);
          T lnP = Math::Rician::lnP (DW_signals, S, noise, dS, dnoise);
          for (size_t i = 0; i < S.size(); i++) {
            if (S[i] < 0.0) { 
              lnP += negative_signal_multiplier * Math::pow2(S[i]); 
              dS[i] += 2.0 * negative_signal_multiplier * S[i]; 
            }
          }
          Math::mult (dR, T(0.0), T(1.0), CblasTrans, A, dS);
          dnoise *= noise_multiplier * noise;
          return (lnP);
        }

        Math::Vector<T> signals, DW_signals;

      protected:
        const ResponseEstimator& C;
        int   lmax, nSH;
        T noise_multiplier, negative_signal_multiplier;
        Math::Vector<T> delta, sh, dt, S, dS;
        Math::Matrix<T> A;
        Math::Matrix<double> D, V;
        Math::Vector<double> ev;
        Math::Eigen::SymmV<double> eig;
        T* AL0;
        T* AL1;
    };
};






SET_VERSION_DEFAULT;

DESCRIPTION = {
  "estimate response function coefficients using blind spherical deconvolution with a Rician noise model and a Bayesian algorithm.",
  NULL
};

ARGUMENTS = {
  Argument ("dwi", "input DW image", "the input diffusion-weighted image.").type_image_in (),
  Argument ("mask", "single-fibre mask image", "the mask image of the voxels assumed to contain a single fibre population.").type_image_in (),
  Argument ("response", "response file", "the output text file where the even l, m=0 SH coefficients of the response function will be stored.").type_file (),
  Argument::End
};


OPTIONS = { 
  Option ("grad", "supply gradient encoding", "specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in image header.")
    .append (Argument ("encoding", "gradient encoding", "the gradient encoding, supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units (1000 s/mm^2).").type_file ()),

  Option ("lmax", "maximum harmonic order", "set the maximum harmonic order for the output series. By default, the program will use the highest possible lmax given the number of diffusion-weighted images.")
    .append (Argument ("order", "order", "the maximum harmonic order to use.").type_integer (0, 30, 8)),

  Option ("dump", "dump all responses", "dump all response function coefficients to file")
    .append (Argument ("file", "response file", "the text file where the SH coefficients will be dumped").type_file ()),

  Option::End 
};





EXECUTE {
  Image::Object &dwi_obj (*argument[0].get_image());
  Image::Object &mask_obj (*argument[1].get_image());
  Image::Header header (dwi_obj);

  if (header.axes.size() != 4) 
    throw Exception ("dwi image should contain 4 dimensions");

  Math::Matrix<ResponseEstimator::T> grad;

  std::vector<OptBase> opt = get_options (0);
  if (opt.size()) grad.load (opt[0][0].get_string());
  else {
    if (!header.DW_scheme.is_set()) 
      throw Exception ("no diffusion encoding found in image \"" + header.name + "\"");
    grad = header.DW_scheme;
  }

  if (grad.rows() < 7 || grad.columns() != 4) 
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  if (header.axes[3].dim != (int) grad.rows()) 
    throw Exception ("number of studies in base image does not match that in encoding file");

  opt = get_options (1);
  int lmax = opt.size() ? opt[0][0].get_int() : -1;

  opt = get_options (2);
  const char* dump_file = opt.size() ? opt[0][0].get_string() : NULL;

  RefPtr<Image::Voxel> mask (new Image::Voxel (mask_obj));

  ResponseEstimator estimator (dwi_obj, mask, grad, lmax);
  estimator.run (dump_file);

  estimator.write_to_file (argument[2].get_string());
}

