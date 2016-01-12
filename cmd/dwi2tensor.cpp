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
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_copy.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"

using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;
const int default_iter = 2;

void usage ()
{
  ARGUMENTS 
  + Argument ("dwi", "the input dwi image.").type_image_in ()
  + Argument ("dt", "the output dt image.").type_image_out ();

  OPTIONS 
  + Option ("mask", "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in()
  + Option ("b0", "the output b0 image.")
  + Argument ("image").type_image_out()
  + Option ("dkt", "the output dkt image.")
  + Argument ("image").type_image_out()
  + Option ("iter","number of iterative reweightings (default: 2); set to 0 for ordinary linear least squares.")
  + Argument ("integer").type_integer (0, default_iter, 10)
  + DWI::GradImportOptions();
  
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)";
  
  COPYRIGHT = "Copyright (C) 2015 Vision Lab, University of Antwerp, Belgium. "
    "This is free software; see the source for copying conditions. "
    "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
  
  DESCRIPTION
  + "Diffusion (kurtosis) tensor estimation using iteratively reweighted linear least squares estimator.";
  
  REFERENCES 
  + "Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. "
    "Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. "
    "NeuroImage, 2013, 81, 335-346";
}

template <class MASKType, class B0Type, class DKTType>
class Processor
{
  public:
    Processor (const Eigen::MatrixXd& b, const int iter, MASKType* mask_image, B0Type* b0_image, DKTType* dkt_image) :
      mask_image (mask_image),
      b0_image (b0_image),
      dkt_image (dkt_image),
      dwi(b.rows()),
      p(b.cols()),
      w(b.rows()),
      work(b.cols(),b.cols()),
      llt(work.rows()),
      b(b),
      maxit(iter) { }

    template <class DWIType, class DTType>
      void operator() (DWIType& dwi_image, DTType& dt_image)
      {
        if (mask_image) {
          assign_pos_of (dwi_image, 0, 3).to (*mask_image);
          if (!mask_image->value())
            return;
        }
        
        for (auto l = Loop (3) (dwi_image); l; ++l)
          dwi[dwi_image.index(3)] = dwi_image.value();
        
        double small_intensity = 1.0e-6 * dwi.maxCoeff(); 
        for (int i = 0; i < dwi.rows(); i++) {
          if (dwi[i] < small_intensity)
            dwi[i] = small_intensity;
          dwi[i] = std::log (dwi[i]);
        }
        
        work.setZero();
        work.selfadjointView<Eigen::Lower>().rankUpdate (b.transpose());
        p = llt.compute (work.selfadjointView<Eigen::Lower>()).solve(b.transpose()*dwi);
        for (int it = 0; it < maxit; it++) {
          w = (b*p).array().exp();
          work.setZero();
          work.selfadjointView<Eigen::Lower>().rankUpdate (b.transpose()*w.asDiagonal());
          p = llt.compute (work.selfadjointView<Eigen::Lower>()).solve(b.transpose()*w.asDiagonal()*dwi);
        }
             
        if (b0_image) {
          assign_pos_of (dwi_image, 0, 3).to (*b0_image);
          b0_image->value() = exp(p[6]);
        }
        
        for (auto l = Loop(3)(dt_image); l; ++l) {
          dt_image.value() = p[dt_image.index(3)];
        }
        
        if (dkt_image) {
          assign_pos_of (dwi_image, 0, 3).to (*dkt_image);
          double adc_sq = (p[0]+p[1]+p[2])*(p[0]+p[1]+p[2])/9.0;
          for (auto l = Loop(3)(*dkt_image); l; ++l) {
            dkt_image->value() = p[dkt_image->index(3)+7]/adc_sq;
          }
        }
      }

  private:
    copy_ptr<MASKType> mask_image;
    copy_ptr<B0Type> b0_image;
    copy_ptr<DKTType> dkt_image;
    Eigen::VectorXd dwi;
    Eigen::VectorXd p;
    Eigen::VectorXd w;
    Eigen::MatrixXd work;
    Eigen::LLT<Eigen::MatrixXd> llt;
    const Eigen::MatrixXd& b;
    const int maxit;
};

template <class MASKType, class B0Type, class DKTType> 
inline Processor<MASKType, B0Type, DKTType> processor (const Eigen::MatrixXd& b, const int& iter, MASKType* mask_image, B0Type* b0_image, DKTType* dkt_image) {
  return { b, iter, mask_image, b0_image, dkt_image };
}

void run ()
{
  auto dwi = Header::open (argument[0]).get_image<value_type>();
  auto grad = DWI::get_valid_DW_scheme (dwi.original_header());
  
  Image<bool>* mask = nullptr;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = new Image<bool> (Image<bool>::open (opt[0][0]));
    check_dimensions (dwi, *mask, 0, 3);
  }
  
  auto iter = get_option_value ("iter", default_iter);

  auto header = dwi.original_header();
  header.datatype() = DataType::Float32;
  header.set_ndim (4);
  header.size(3) = 6;
  auto dt = Header::create (argument[1], header).get_image<value_type>();

  Image<value_type>* b0 = nullptr;
  opt = get_options ("b0");
  if (opt.size()) {
    header.set_ndim (3);
    b0 = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }

  Image<value_type>* dkt = nullptr;
  opt = get_options ("dkt");
  if (opt.size()) {
    header.set_ndim (4);
    header.size(3) = 15;
    dkt = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }
  
  Eigen::MatrixXd b = -DWI::grad2bmatrix<double> (grad, opt.size()>0);

  ThreadedLoop("computing tensors", dwi, 0, 3).run (processor (b, iter, mask, b0, dkt), dwi, dt);
}

