/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 01/07/13.
    
    This file is part of the Global Tractography module for MRtrix.
    
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

#include "externalenergy.h"

#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "math/SH.h"
#include "algo/loop.h"

//#include "math/lsnonneg.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
//        ExternalEnergyComputer::Shared::Shared(const Image<float>& dwimage, const Properties& props)
//          : lmax(props.Lmax), ncols(Math::SH::NforL(lmax)), nf(props.resp_ISO.size()), 
//            beta(props.beta), mu(props.ppot)
//        {
////          // Set buffers
////          Header info (dwimage);
////          info.datatype() = DataType::Float32;
////          info.size(3) = ncols;
////          tod = new Image::BufferScratch<float>(info);
////          tod->zero();
          
////          info.dim(3) = nf;
////          fiso = new Image::BufferScratch<float>(info);
////          fiso->zero();
          
////          info.set_ndim(3);
////          eext = new Image::BufferScratch<float>(info);
////          eext->zero();
          
//          // Set kernel matrices
//          auto grad = DWI::get_DW_scheme(dwimage);
//          nrows = grad.rows();
//          DWI::Shells shells (grad);
          
//          if (props.resp_WM.rows() != shells.count())
//            FAIL("WM kernel size does not match the no. b-values in the image.");
//          for (size_t j = 0; j < props.resp_ISO.size(); j++) {
//            if (props.resp_ISO[j].size() != shells.count())
//              FAIL("Isotropic kernel size does not match the no. b-values in the image.");
//          }
                    
//          K.resize(nrows, ncols);
//          K.setZero();
//          Ak.resize(nrows, nf+1);
//          Ak.setZero();
          
//          Eigen::VectorXd delta_vec (ncols);
//          Eigen::VectorXd wmr_sh (lmax/2+1), wmr_rh (lmax/2+1);
//          wmr_sh.setZero();
//          Eigen::Vector3d unit_dir;
//          double wmr0;
          
//          for (size_t s = 0; s < shells.count(); s++)
//          {
//            for (int l = 0; l <= lmax/2; l++)
//              wmr_sh[l] = (l < props.resp_WM.cols()) ? props.resp_WM(s, l) : 0.0;
//            wmr_rh = Math::SH::SH2RH(wmr_sh);
//            wmr0 = props.resp_WM(s,0) / std::sqrt(4*M_PI);
            
//            for (std::vector<size_t>::const_iterator it = shells[s].get_volumes().begin(); it != shells[s].get_volumes().end(); ++it)
//            {
//              size_t r = *it;
//              // K
//              unit_dir << grad(r,0), grad(r,1), grad(r,2);
//              double n = unit_dir.norm();
//              if (n > 0.0)
//                unit_dir /= n;
//              Math::SH::delta(delta_vec, unit_dir, lmax);
//              Math::SH::sconv(delta_vec, wmr_rh, delta_vec);
//              K.row(r) = delta_vec;
//              // Ak
//              Ak(r,0) = wmr0;
//              for (size_t j = 0; j < props.resp_ISO.size(); j++)
//                Ak(r,j+1) = props.resp_ISO[j][s];
//            }
//          }
//          K *= props.weight;
//          //VAR(K);
//          //VAR(Ak);
          
//          H = Ak.transpose() * Ak;
//          Hinv = H.inverse();  // invert in place
//        }
        
        
//        ExternalEnergyComputer::Shared::~Shared()
//        {
//          delete tod;
//          delete fiso;
//          delete eext;
//        }
        
        
        
        ExternalEnergyComputer::ExternalEnergyComputer(Stats& stat, const Image<float>& dwimage, const Properties& props)
          : EnergyComputer(stat),
            dwi(dwimage),
            T(Transform(dwimage).scanner2voxel),
            lmax(props.Lmax), ncols(Math::SH::NforL(lmax)), nf(props.resp_ISO.size()),
            beta(props.beta), mu(props.ppot), dE(0.0)
        {
          DEBUG("Initialise computation of external energy.");
          // Create images --------------------------------------------------------------
          Header header (dwimage);
          header.datatype() = DataType::Float32;
          header.size(3) = ncols;
          tod = Image<float>::scratch(header, "TOD image");
          
          header.size(3) = nf;
          fiso = Image<float>::scratch(header, "isotropic fractions");
          
          header.set_ndim(3);
          eext = Image<float>::scratch(header, "external energy");
          
          // Set kernel matrices --------------------------------------------------------
          auto grad = DWI::get_DW_scheme(dwimage);
          nrows = grad.rows();
          DWI::Shells shells (grad);
          
          if (props.resp_WM.rows() != shells.count())
            FAIL("WM kernel size does not match the no. b-values in the image.");
          for (size_t j = 0; j < props.resp_ISO.size(); j++) {
            if (props.resp_ISO[j].size() != shells.count())
              FAIL("Isotropic kernel size does not match the no. b-values in the image.");
          }
                    
          K.resize(nrows, ncols);
          K.setZero();
          Ak.resize(nrows, nf+1);
          Ak.setZero();
          
          Eigen::VectorXd delta_vec (ncols);
          Eigen::VectorXd wmr_sh (lmax/2+1), wmr_rh (lmax/2+1);
          wmr_sh.setZero();
          Eigen::Vector3d unit_dir;
          double wmr0;
          
          for (size_t s = 0; s < shells.count(); s++)
          {
            for (int l = 0; l <= lmax/2; l++)
              wmr_sh[l] = (l < props.resp_WM.cols()) ? props.resp_WM(s, l) : 0.0;
            wmr_rh = Math::SH::SH2RH(wmr_sh);
            wmr0 = props.resp_WM(s,0) / std::sqrt(4*M_PI);
            
            for (size_t r : shells[s].get_volumes())
            {
              // K
              unit_dir << grad(r,0), grad(r,1), grad(r,2);
              double n = unit_dir.norm();
              if (n > 0.0)
                unit_dir /= n;
              Math::SH::delta(delta_vec, unit_dir, lmax);
              Math::SH::sconv(delta_vec, wmr_rh, delta_vec);
              K.row(r) = delta_vec;
              // Ak
              Ak(r,0) = wmr0;
              for (size_t j = 0; j < props.resp_ISO.size(); j++)
                Ak(r,j+1) = props.resp_ISO[j][s];
            }
          }
          K *= props.weight;
          
          // Allocate temporary memory --------------------------------------------------
          y.resize(nrows);
          t.resize(ncols);
          d.resize(ncols);
          fk.resize(nf+1);
          
          // Set NNLS solver ------------------------------------------------------------
          nnls = Math::ICLS::Problem<double>(Ak, Eigen::MatrixXd::Identity(nf+1, nf+1));
          
          // Reset energy ---------------------------------------------------------------
          resetEnergy();
        }
        
        
        
        void ExternalEnergyComputer::resetEnergy()
        {
          DEBUG("Reset external energy.");
//          Image::Loop loop (0, 3);  // Loop over spatial dimensions
          double e;
          dE = 0.0;
//          for (loop.start(dwi_vox); loop.ok(); loop.next(dwi_vox))
          for (auto l = Loop(dwi, 0, 3) (dwi, tod, fiso, eext); l; ++l)
          {
//            tod_vox[0] = fiso_vox[0] = eext_vox[0] = dwi_vox[0];
//            tod_vox[1] = fiso_vox[1] = eext_vox[1] = dwi_vox[1];
//            tod_vox[2] = fiso_vox[2] = eext_vox[2] = dwi_vox[2];
            y = dwi.row(3).cast<double>();
            t = tod.row(3).cast<double>();
            e = calcEnergy();
            eext.value() = e;
            dE += e;
            //memcpy(fiso_vox.address(), fiso.ptr(), s.nf*sizeof(float));
            fiso.row(3) = fk.tail(nf).cast<float>();
          }
          stats.incEextTotal(dE - stats.getEextTotal());  // Reset total external energy
          dE = 0.0;
        }
        
        
        void ExternalEnergyComputer::acceptChanges()
        {
          for (int k = 0; k != changes_vox.size(); ++k) 
          {
//            tod_vox[0] = fiso_vox[0] = eext_vox[0] = changes_vox[k][0];
//            tod_vox[1] = fiso_vox[1] = eext_vox[1] = changes_vox[k][1];
//            tod_vox[2] = fiso_vox[2] = eext_vox[2] = changes_vox[k][2];
            assign_pos_of(changes_vox[k], 0, 3).to(tod, fiso, eext);
//            memcpy(tod_vox.address(), changes_tod[k].ptr(), s.ncols*sizeof(float));
//            memcpy(fiso_vox.address(), changes_fiso[k].ptr(), s.nf*sizeof(float));
            tod.row(3) = changes_tod[k].cast<float>();
            fiso.row(3) = changes_fiso[k].cast<float>();
            eext.value() = changes_eext[k];
          }
          stats.incEextTotal(dE);
          clearChanges();
        }
        
        
        void ExternalEnergyComputer::clearChanges()
        {
          changes_vox.clear();
          changes_tod.clear();
          changes_fiso.clear();
          changes_eext.clear();
          dE = 0.0;
        }
        
        
        void ExternalEnergyComputer::add(const Point_t &pos, const Point_t &dir, const double factor)
        {
          Point_t p = T.cast<float>() * pos;
          Point_t v = Point_t(Math::floor<float>(p[0]), Math::floor<float>(p[1]), Math::floor<float>(p[2]));
          Point_t w = Point_t(hanning(p[0]-v[0]), hanning(p[1]-v[1]), hanning(p[2]-v[2]));
          
          Math::SH::delta(d, dir, lmax);
          
          Eigen::Vector3i x = v.cast<int>();
          add2vox(x, factor*(1.-w[0])*(1.-w[1])*(1.-w[2]));
          x[2]++;
          add2vox(x, factor*(1.-w[0])*(1.-w[1])*w[2]);
          x[1]++;
          add2vox(x, factor*(1.-w[0])*w[1]*w[2]);
          x[2]--;
          add2vox(x, factor*(1.-w[0])*w[1]*(1.-w[2]));
          x[0]++;
          add2vox(x, factor*w[0]*w[1]*(1.-w[2]));
          x[2]++;
          add2vox(x, factor*w[0]*w[1]*w[2]);
          x[1]--;
          add2vox(x, factor*w[0]*(1.-w[1])*w[2]);
          x[2]--;
          add2vox(x, factor*w[0]*(1.-w[1])*(1.-w[2]));
        }
        
        
        void ExternalEnergyComputer::add2vox(const Eigen::Vector3i& vox, const double w)
        {
          if (w == 0.0)
            return;
//          tod_vox[0] = vox[0];
//          tod_vox[1] = vox[1];
//          tod_vox[2] = vox[2];
          assign_pos_of(vox, 0, 3).to(tod);
          if (!tod.valid())
            return;
          t = d;
          t *= w;
          for (int k = 0; k != changes_vox.size(); ++k) {
            if (changes_vox[k] == vox) {
              changes_tod[k] += t;
              return;
            }
          }
          changes_vox.push_back(vox);
//          t += Math::Vector<float>(tod_vox.address(), s.ncols);
          t += tod.row(3).cast<double>();
          changes_tod.push_back(t);
        }
        
        
        double ExternalEnergyComputer::eval()
        {
          dE = 0.0;
          double e;
          for (int k = 0; k != changes_vox.size(); ++k) 
          {
//            dwi_vox[0] = eext_vox[0] = changes_vox[k][0];
//            dwi_vox[1] = eext_vox[1] = changes_vox[k][1];
//            dwi_vox[2] = eext_vox[2] = changes_vox[k][2];
            assign_pos_of(changes_vox[k], 0, 3).to(dwi, eext);
//            y = Math::Vector<float>(dwi_vox.address(), s.nrows);
            y = dwi.row(3).cast<double>();
            t = changes_tod[k];
            e = calcEnergy();
            changes_fiso.push_back(fk.tail(nf));
            dE += e;
            dE -= eext.value();
            changes_eext.push_back(e);
          }
          return dE / stats.getText();
        }
        
        
        double ExternalEnergyComputer::calcEnergy()
        {
          //Math::mult(y, 1.0, -1.0, CblasNoTrans, s.K, t);   // y = d - K t
          //Math::mult(c, 1.0, CblasTrans, s.Ak, y);          // c = Ak^T y
          //Math::solve_LS_nonneg_Hf(fk, s.H, s.Hinv, c);     // H fk = c
          
          y.noalias() -= K * t;
          Math::ICLS::Solver<double> nnls_solver (nnls);
          nnls_solver(fk, y);
          
//          Math::mult(y, 1.0, -1.0, CblasNoTrans, A, f);     // res = y - A f
//          return Math::norm2(y) / s.nrows + s.mu * t[0]*M_sqrt4PI;  // MSE + L1 regularizer
          y.noalias() -= Ak.rightCols(nf) * fk.tail(nf);
          return y.squaredNorm() / nrows + mu * t[0]*M_sqrt4PI;  // MSE + L1 regularizer
        }
        
        
        
      }
    }
  }
}


