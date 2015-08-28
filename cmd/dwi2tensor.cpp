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
#include "adapter/allow_empty.h"

using namespace MR;
using namespace App;
using namespace std;

typedef double value_type;
constexpr int default_iter = 2;

void usage ()
{
  ARGUMENTS 
  + Argument ("dwi", "the input dwi image.").type_image_in ()
  + Argument ("dt", "the output dt image.").type_image_out ();

  OPTIONS 
  + Option ("iter","number of iterative reweightings (default: 2); set to 0 for ordinary linear least squares.")
  + Argument ("iter").type_integer (0, default_iter, 10)
  + Option ("b0", "the output b0 image.")
  + Argument ("image").type_image_out()
  + Option ("mask", "only perform computation within the specified binary brain mask image.")
  + Argument ("image").type_image_in()
  + DWI::GradImportOptions();
  
  AUTHOR = "Ben Jeurissen (ben.jeurissen@uantwerpen.be)";
  
  COPYRIGHT = "Copyright (C) 2015 Vision Lab, University of Antwerp, Belgium. "
    "This is free software; see the source for copying conditions. "
    "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
  
  DESCRIPTION
  + "Diffusion tensor estimation using iteratively reweighted linear least squares estimator.";
  
  REFERENCES 
  + "Veraart, J.; Sijbers, J.; Sunaert, S.; Leemans, A. & Jeurissen, B. "
    "Weighted linear least squares estimation of diffusion MRI parameters: strengths, limitations, and pitfalls. "
    "NeuroImage, 2013, 81, 335-346";
}

class Processor
{
  public:
    Processor (const Eigen::MatrixXd& b, const int& iter) :
      dwi(b.rows()),
      logdwi(b.rows()),
      p(7),
      w(b.rows(),b.rows()),
      b(b),
      maxit(iter) { }

    template <class DWIType, class MASKType, class B0Type, class DTType>
      void operator() (DWIType& dwi_image, MASKType& mask_image, B0Type& b0_image, DTType& dt_image)
      {
        /* check mask */ 
        if (mask_image.valid())
        {
          assign_pos_of(dwi_image, 0, 3).to(mask_image);
          if (!mask_image.value())
            return;
        }
        
        /* input dwi */
        for (auto l = Loop (3) (dwi_image); l; ++l)
          dwi[dwi_image.index(3)] = dwi_image.value();
        
        /* avoid issues with dwi < 1 */
        value_type scale = 1000000.0/dwi.maxCoeff(); // avoid issues with DWI images with ranges < 1
        dwi = dwi*scale;
        for (int i = 0; i < dwi.rows(); i++)
          if (dwi[i] < 1)
            dwi[i] = 1;
        
        /* log dwi fit */
        logdwi = dwi.array().log().matrix();
        p = b.colPivHouseholderQr().solve(logdwi);
        
        /* weighted linear least squares fit with iterative weights */
        for (int it = 0; it < maxit; it++)
        {
          w = (b*p).array().exp().matrix().asDiagonal();
          p = (w*b).colPivHouseholderQr().solve(w*logdwi);
        }
        
        /* output b0 */
        if (b0_image.valid())
          b0_image.value() = exp(p[6])/scale;
        
        /* output dt */
        dt_image.index(3) = 0; dt_image.value() = p[0];
        dt_image.index(3) = 1; dt_image.value() = p[1];
        dt_image.index(3) = 2; dt_image.value() = p[2];
        dt_image.index(3) = 3; dt_image.value() = p[3];
        dt_image.index(3) = 4; dt_image.value() = p[4];
        dt_image.index(3) = 5; dt_image.value() = p[5];
      }

  private:
    Eigen::VectorXd dwi;
    Eigen::VectorXd logdwi;
    Eigen::VectorXd p;
    Eigen::MatrixXd w;
    const Eigen::MatrixXd b;
    const int maxit;
};


void run ()
{
  auto dwi = Header::open (argument[0]).get_image<value_type>();
  auto grad = DWI::get_valid_DW_scheme (dwi.header());
  Eigen::MatrixXd b = -DWI::grad2bmatrix<value_type> (grad);

  auto mask = Image<bool>();
  auto opt = get_options ("mask");
  if (opt.size())
  {
    mask = Header::open (opt[0][0]).get_image<bool>();
    check_dimensions (dwi, mask, 0, 3);
  }
  
  auto iter = default_iter;
  opt = get_options ("iter");
  if (opt.size())
  {
    iter = opt[0][0];
  }

  auto header = dwi.header();
  header.datatype() = DataType::Float32;
  header.set_ndim (4);
  header.size(3) = 6;
  auto dt = Header::create (argument[1], header).get_image<value_type>();

  
  auto b0 = Image<value_type>();
  opt = get_options ("b0");
  if (opt.size())
  {
    header.set_ndim (3);
    b0 = Header::create (opt[0][0], header).get_image<value_type>();
  }
  
  // TODO: fix crash if mask or b0 are not valid() (i.e. when those options are not supplied)
  ThreadedLoop("computing tensors...", dwi, 0, 3).run (Processor(b,iter), dwi, Adapter::allow_empty (mask), Adapter::allow_empty (b0), dt);
}
