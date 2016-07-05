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
#include <Eigen/Eigenvalues>

using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;
const char* modulate_choices[] = { "none", "fa", "eigval", NULL };

void usage ()
{
  ARGUMENTS 
  + Argument ("tensor", "the input tensor image.").type_image_in ();

  OPTIONS 
  + Option ("adc",
            "compute the mean apparent diffusion coefficient (ADC) of the diffusion tensor. "
            "(sometimes also referred to as the mean diffusivity (MD))")
  + Argument ("image").type_image_out()
 
  + Option ("fa",
            "compute the fractional anisotropy (FA) of the diffusion tensor.")
  + Argument ("image").type_image_out()
 
  + Option ("ad",
            "compute the axial diffusivity (AD) of the diffusion tensor. "
            "(equivalent to the principal eigenvalue)")
  + Argument ("image").type_image_out()
 
  + Option ("rd",
            "compute the radial diffusivity (RD) of the diffusion tensor. "
            "(equivalent to the mean of the two non-principal eigenvalues)")
  + Argument ("image").type_image_out()
  
  + Option ("cl",
            "compute the linearity metric of the diffusion tensor. "
            "(one of the three Westin shape metrics)")
  + Argument ("image").type_image_out()
 
  + Option ("cp",
            "compute the planarity metric of the diffusion tensor. "
            "(one of the three Westin shape metrics)")
  + Argument ("image").type_image_out()
  
  + Option ("cs",
            "compute the sphericity metric of the diffusion tensor. "
            "(one of the three Westin shape metrics)")
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
            "principal (1) and minor (3) eigenvalues/eigenvectors (default = 1).")
  + Argument ("sequence").type_sequence_int()

  + Option ("modulate",
            "specify how to modulate the magnitude of the eigenvectors. Valid choices "
            "are: none, FA, eigval (default = FA).")
  + Argument ("choice").type_choice (modulate_choices)
 
  + Option ("mask",
            "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in();
  
  AUTHOR = "Thijs Dhollander (thijs.dhollander@gmail.com) & Ben Jeurissen (ben.jeurissen@uantwerpen.be) & J-Donald Tournier (jdtournier@gmail.com)";

  
  DESCRIPTION
  + "Generate maps of tensor-derived parameters.";
  
  REFERENCES 
  + "Basser, P. J.; Mattiello, J. & Lebihan, D. "
    "MR diffusion tensor spectroscopy and imaging. "
    "Biophysical Journal, 1994, 66, 259-267"
  + "Westin, C. F.; Peled, S.; Gudbjartsson, H.; Kikinis, R. & Jolesz, F. A. "
    "Geometrical diffusion measures for MRI from tensor basis analysis. "
    "Proc Intl Soc Mag Reson Med, 1997, 5, 1742";
}

class Processor
{
  public:
    Processor (Image<bool>& mask_img, Image<value_type>& adc_img, Image<value_type>& fa_img, Image<value_type>& ad_img, Image<value_type>& rd_img, Image<value_type>& cl_img, Image<value_type>& cp_img, Image<value_type>& cs_img, Image<value_type>& value_img, Image<value_type>& vector_img, std::vector<int> vals, int modulate) :
      mask_img (mask_img),
      adc_img (adc_img),
      fa_img (fa_img),
      ad_img (ad_img),
      rd_img (rd_img),
      cl_img (cl_img),
      cp_img (cp_img),
      cs_img (cs_img),
      value_img (value_img),
      vector_img (vector_img),
      vals (vals),
      modulate (modulate) { }

    void operator() (Image<value_type>& dt_img)
    {
      /* check mask */ 
      if (mask_img.valid()) {
        assign_pos_of (dt_img, 0, 3).to (mask_img);
        if (!mask_img.value())
          return;
      }
     
      /* input dt */
      Eigen::Matrix<double, 6, 1> dt;
      for (auto l = Loop (3) (dt_img); l; ++l)
        dt[dt_img.index(3)] = dt_img.value();
      
      /* output adc */
      if (adc_img.valid()) {
        assign_pos_of (dt_img, 0, 3).to (adc_img);
        adc_img.value() = DWI::tensor2ADC(dt);
      }
      
      double fa = 0.0;
      if (fa_img.valid() || (vector_img.valid() && (modulate == 1)))
        fa = DWI::tensor2FA(dt);
      
      /* output fa */
      if (fa_img.valid()) {
        assign_pos_of (dt_img, 0, 3).to (fa_img);
        fa_img.value() = fa;
      }
      
      bool need_eigenvalues = value_img.valid() || (vector_img.valid() && (modulate == 2)) || ad_img.valid() || rd_img.valid() || cl_img.valid() || cp_img.valid() || cs_img.valid();
      
      Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es;
      if (need_eigenvalues || vector_img.valid()) {
        Eigen::Matrix3d M;
        M (0,0) = dt[0];
        M (1,1) = dt[1];
        M (2,2) = dt[2];
        M (0,1) = M (1,0) = dt[3];
        M (0,2) = M (2,0) = dt[4];
        M (1,2) = M (2,1) = dt[5];
        es = Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d>(M, vector_img.valid() ? Eigen::ComputeEigenvectors : Eigen::EigenvaluesOnly);
      }
      
      Eigen::Vector3d eigval;
      if (need_eigenvalues)
        eigval = es.eigenvalues();
        
      /* output value */
      if (value_img.valid()) {
        assign_pos_of (dt_img, 0, 3).to (value_img);
        if (vals.size() > 1) {
          auto l = Loop(3)(value_img);
          for (size_t i = 0; i < vals.size(); i++) {
            value_img.value() = eigval(3-vals[i]); l++;
          }
        } else {
          value_img.value() = eigval(3-vals[0]);
        }
      }
      
      /* output ad */
      if (ad_img.valid()) {
        assign_pos_of (dt_img, 0, 3).to (ad_img);
        ad_img.value() = eigval(2);
      }
      
      /* output rd */
      if (rd_img.valid()) {
        assign_pos_of (dt_img, 0, 3).to (rd_img);
        rd_img.value() = (eigval(1) + eigval(0)) / 2;
      }
      
      /* output shape measures */
      if (cl_img.valid() || cp_img.valid() || cs_img.valid()) {
        double eigsum = eigval.sum();
        if (eigsum != 0.0) {
          if (cl_img.valid()) {
            assign_pos_of (dt_img, 0, 3).to (cl_img);
            cl_img.value() = (eigval(2) - eigval(1)) / eigsum;
          }
          if (cp_img.valid()) {
            assign_pos_of (dt_img, 0, 3).to (cp_img);
            cp_img.value() = 2.0 * (eigval(1) - eigval(0)) / eigsum;
          }
          if (cs_img.valid()) {
            assign_pos_of (dt_img, 0, 3).to (cs_img);
            cs_img.value() = 3.0 * eigval(0) / eigsum;
          }
        }
      }
      
      /* output vector */
      if (vector_img.valid()) {
        Eigen::Matrix3d eigvec = es.eigenvectors();
        assign_pos_of (dt_img, 0, 3).to (vector_img);
        auto l = Loop(3)(vector_img);
        for (size_t i = 0; i < vals.size(); i++) {
          double fact = 1.0;
          if (modulate == 1)
            fact = fa;
          else if (modulate == 2)
            fact = eigval(3-vals[i]);
          vector_img.value() = eigvec(0,3-vals[i])*fact; l++;
          vector_img.value() = eigvec(1,3-vals[i])*fact; l++;
          vector_img.value() = eigvec(2,3-vals[i])*fact; l++;
        }
      }                   
    }
    
  private:
    Image<bool> mask_img;
    Image<value_type> adc_img;
    Image<value_type> fa_img;
    Image<value_type> ad_img;
    Image<value_type> rd_img;
    Image<value_type> cl_img;
    Image<value_type> cp_img;
    Image<value_type> cs_img;
    Image<value_type> value_img;
    Image<value_type> vector_img;
    std::vector<int> vals;
    int modulate;
};

void run ()
{
  auto dt_img = Image<value_type>::open (argument[0]);
  Header header (dt_img);

  auto mask_img = Image<bool>();
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask_img = Image<bool>::open (opt[0][0]);
    check_dimensions (dt_img, mask_img, 0, 3);
  }
  
  auto adc_img = Image<value_type>();
  opt = get_options ("adc");
  if (opt.size()) {
    header.ndim() = 3;
    adc_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto fa_img = Image<value_type>();
  opt = get_options ("fa");
  if (opt.size()) {
    header.ndim() = 3;
    fa_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto ad_img = Image<value_type>();
  opt = get_options ("ad");
  if (opt.size()) {
    header.ndim() = 3;
    ad_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto rd_img = Image<value_type>();
  opt = get_options ("rd");
  if (opt.size()) {
    header.ndim() = 3;
    rd_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto cl_img = Image<value_type>();
  opt = get_options ("cl");
  if (opt.size()) {
    header.ndim() = 3;
    cl_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto cp_img = Image<value_type>();
  opt = get_options ("cp");
  if (opt.size()) {
    header.ndim() = 3;
    cp_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto cs_img = Image<value_type>();
  opt = get_options ("cs");
  if (opt.size()) {
    header.ndim() = 3;
    cs_img = Image<value_type>::create (opt[0][0], header);
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

  float modulate = get_option_value ("modulate", 1);

  auto value_img = Image<value_type>();
  opt = get_options ("value");
  if (opt.size()) {
    header.ndim() = 3;
    if (vals.size()>1) {
      header.ndim() = 4;
      header.size (3) = vals.size();
    }
    value_img = Image<value_type>::create (opt[0][0], header);
  }
  
  auto vector_img = Image<value_type>();
  opt = get_options ("vector");
  if (opt.size()) {
    header.ndim() = 4;
    header.size (3) = vals.size()*3;
    vector_img = Image<value_type>::create (opt[0][0], header);
  }
  
  ThreadedLoop ("computing metrics", dt_img, 0, 3)
    .run (Processor (mask_img, adc_img, fa_img, ad_img, rd_img, cl_img, cp_img, cs_img, value_img, vector_img, vals, modulate), dt_img);
}
