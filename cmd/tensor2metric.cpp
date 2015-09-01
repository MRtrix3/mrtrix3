/*
    Copyright 2015 Vision Lab, Univeristy of Antwerp, Belgium

    Written by Ben Jeurissen, 28/08/15.

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

#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "algo/threaded_copy.h"
#include "dwi/gradient.h"
#include "dwi/tensor.h"
#include <Eigen/Eigenvalues>

using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;
const char* modulate_choices[] = { "none", "fa", "eigval", NULL };

void usage ()
{
  ARGUMENTS 
  + Argument ("dwi", "the input dwi image.").type_image_in ();

  OPTIONS 
  + Option ("adc",
            "compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor.")
  + Argument ("image").type_image_out()
 
  + Option ("fa",
            "compute the fractional anisotropy of the diffusion tensor.")
  + Argument ("image").type_image_out()
 
  + Option ("value",
            "compute the selected eigenvalue(s) of the diffusion tensor.")
  + Argument ("image").type_image_out()
 
  + Option ("vector",
            "compute the selected eigenvector(s) of the diffusion tensor.")
  + Argument ("image").type_image_out()
 
  + Option ("num",
            "specify the desired eigenvalue/eigenvector(s). Note that several eigenvalues "
            "can be specified as a number sequence. For example, '1,3' specifies the "
            "major (1) and minor (3) eigenvalues/eigenvectors (default = 1).")
  + Argument ("image")

  + Option ("modulate",
            "specify how to modulate the magnitude of the eigenvectors. Valid choices "
            "are: none, FA, eigval (default = FA).")
  + Argument ("spec").type_choice (modulate_choices)
 
  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in();
  
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be) & J-Donald Tournier (jdtournier@gmail.com)";

  
  DESCRIPTION
  + "Generate maps of tensor-derived parameters.";
  
  REFERENCES 
  + "Basser, P. J.; Mattiello, J. & Lebihan, D. "
    "MR diffusion tensor spectroscopy and imaging. "
    "Biophysical Journal, 1994, 66, 259-267";
}

template <class MASKType, class ADCType, class FAType, class VALUEType, class VECTORType>
class Processor
{
  public:
    Processor (MASKType* mask_image, ADCType* adc_image, FAType* fa_image, VALUEType* value_image, VECTORType* vector_image, std::vector<int> vals, int modulate) :
      mask_image (mask_image),
      adc_image (adc_image),
      fa_image (fa_image),
      value_image (value_image),
      vector_image (vector_image),
      vals (vals),
      modulate (modulate) { }

    template <class DTType>
      void operator() (DTType& dt_image)
      {
        /* check mask */ 
        if (mask_image) {
          assign_pos_of (dt_image, 0, 3).to (*mask_image);
          if (!mask_image->value())
            return;
        }
        
        /* input dt */
        Eigen::Matrix<double, 6, 1> dt;
        for (auto l = Loop (3) (dt_image); l; ++l)
          dt[dt_image.index(3)] = dt_image.value();
        
        /* output adc */
        if (adc_image) {
          assign_pos_of (dt_image, 0, 3).to (*adc_image);
          adc_image->value() = DWI::tensor2ADC(dt);
        }

        double fa;
        if (fa_image || (vector_image && (modulate == 1)))
          fa = DWI::tensor2FA(dt);
 
        /* output fa */
        if (fa_image) {
          assign_pos_of (dt_image, 0, 3).to (*fa_image);
          fa_image->value() = fa;
        }
    
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es;
	if (value_image || vector_image) {
          Eigen::Matrix3d M;
          M (0,0) = dt[0];
          M (1,1) = dt[1];
          M (2,2) = dt[2];
          M (0,1) = M (1,0) = dt[3];
          M (0,2) = M (2,0) = dt[4];
          M (1,2) = M (2,1) = dt[5];
          es = Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d>(M, vector_image ? Eigen::ComputeEigenvectors : Eigen::EigenvaluesOnly);
        }

        Eigen::Vector3d eigval;
        if (value_image || (vector_image && (modulate == 2)))
          eigval = es.eigenvalues();
          
        /* output value */
        if (value_image) {
          assign_pos_of (dt_image, 0, 3).to (*value_image);
          auto l = Loop(3)(*value_image);
          for (int i = 0; i < vals.size(); i++) {
            value_image->value() = eigval(3-vals[i]); l++;
          }
        }
     
        /* output vector */
        if (vector_image) {
          Eigen::Matrix3d eigvec = es.eigenvectors();
          assign_pos_of (dt_image, 0, 3).to (*vector_image);
          auto l = Loop(3)(*vector_image);
          for (int i = 0; i < vals.size(); i++) {
            double fact = 1.0;
            if (modulate == 1)
              fact = fa;
            else if (modulate == 2)
              fact = eigval(3-vals[i]);
            vector_image->value() = eigvec(0,3-vals[i])*fact; l++;
            vector_image->value() = eigvec(1,3-vals[i])*fact; l++;
            vector_image->value() = eigvec(2,3-vals[i])*fact; l++;
          }
        }                   
      }

  private:
    copy_ptr<MASKType> mask_image;
    copy_ptr<ADCType> adc_image;
    copy_ptr<FAType> fa_image;
    copy_ptr<VALUEType> value_image;
    copy_ptr<VECTORType> vector_image;
    std::vector<int> vals;
    int modulate;
};


template <class MASKType, class ADCType, class FAType, class VALUEType, class VECTORType> 
inline Processor<MASKType, ADCType, FAType, VALUEType, VECTORType> processor (MASKType* mask_image, ADCType* adc_image, FAType* fa_image, VALUEType* value_image, VECTORType* vector_image, std::vector<int> vals, int modulate) {
  return { mask_image, adc_image, fa_image, value_image, vector_image, vals, modulate };
}

void run ()
{
  auto dt = Header::open (argument[0]).get_image<value_type>();
  auto header = dt.header();
  header.datatype() = DataType::Float32;
  
  Image<bool>* mask = nullptr;
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = new Image<bool> (Image<bool>::open (opt[0][0]));
    check_dimensions (dt, *mask, 0, 3);
  }
  
  Image<value_type>* adc = nullptr;
  opt = get_options ("adc");
  if (opt.size()) {
    header.set_ndim (3);
    adc = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }
  
  Image<value_type>* fa = nullptr;
  opt = get_options ("fa");
  if (opt.size()) {
    header.set_ndim (3);
    fa = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }
  
  std::vector<int> vals = {1};
  opt = get_options ("num");
  if (opt.size()) {
    vals = opt[0][0];
  if (vals.empty())
    throw Exception ("invalid eigenvalue/eigenvector number specifier");
  for (size_t i = 0; i < vals.size(); ++i)
    if (vals[i] < 1 || vals[i] > 3)
      throw Exception ("eigenvalue/eigenvector number is out of bounds");
  }

  int modulate = 1;
  opt = get_options ("modulate");
  if (opt.size())
    modulate = opt[0][0];
  
  Image<value_type>* value = nullptr;
  opt = get_options ("value");
  if (opt.size()) {
    header.set_ndim (4);
    header.size (3) = vals.size();
    value = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }
  
  Image<value_type>* vector = nullptr;
  opt = get_options ("vector");
  if (opt.size()) {
    header.set_ndim (4);
    header.size (3) = vals.size()*3;
    vector = new Image<value_type> (Image<value_type>::create (opt[0][0], header));
  }
  
  ThreadedLoop("computing metrics...", dt, 0, 3).run (processor (mask, adc, fa, value, vector, vals, modulate), dt);
}
