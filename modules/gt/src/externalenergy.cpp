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
#include "image/loop.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        ExternalEnergyComputer::Shared::Shared(Image::BufferPreload<float>& dwimage, const Properties& props)
          : lmax(props.Lmax), ncols(Math::SH::NforL(lmax)), dwi(dwimage)
        {
          // Set buffers
          Image::Info info (dwimage);
          info.datatype() = DataType::Float32;
          info.dim(3) = ncols;
          tod = new Image::BufferScratch<float>(info);
          tod->zero();
          
          nf = (props.resp_GM) ? 2 : (props.resp_CSF) ? 1 : props.resp_WM.columns();
          info.dim(3) = nf;
          fiso = new Image::BufferScratch<float>(info);
          fiso->zero();
          
          info.set_ndim(3);
          eext = new Image::BufferScratch<float>(info);
          eext->zero();
          
          // Set kernel matrices
          Math::Matrix<float> grad = DWI::get_valid_DW_scheme<float> (dwimage);
          nrows = grad.rows();
          DWI::Shells shells (grad);
          
          if (props.resp_WM.rows() != shells.count())
            FAIL("WM kernel size does not match the no. b-values in the image.");
          if (props.resp_CSF && (props.resp_CSF->size() != shells.count()))
            FAIL("CSF kernel size does not match the no. b-values in the image.");
          if (props.resp_GM && (props.resp_GM->size() != shells.count()))
            FAIL("GM kernel size does not match the no. b-values in the image.");
          
          K.allocate(nrows, ncols);
          K.zero();
          A.allocate(nrows, nf);
          A.zero();
          
          Math::Vector<double> delta_vec (ncols);
          Math::Vector<double> wmr_sh (lmax/2+1), wmr_rh (lmax/2+1);
          wmr_sh.zero();
          Point<double> unit_dir;
          
          for (size_t s = 0; s < shells.count(); s++)
          {
            for (int l = 0; l <= lmax/2; l++)
              wmr_sh[l] = (l < props.resp_WM.columns()) ? props.resp_WM(s, l) : 0.0;
            wmr_rh = Math::SH::SH2RH(wmr_sh);
            
            for (std::vector<size_t>::const_iterator it = shells[s].get_volumes().begin(); it != shells[s].get_volumes().end(); ++it)
            {
              size_t r = *it;
              // K
              unit_dir = Point<double>(grad(r,0), grad(r,1), grad(r,2));
              unit_dir.normalise();
              Math::SH::delta(delta_vec, unit_dir, lmax);
              Math::SH::sconv(delta_vec, wmr_rh, delta_vec);
              K.row(r) = delta_vec;
              // A
              if (props.resp_CSF) {
                A(r,0) = (*props.resp_CSF)[s];
                if (props.resp_GM)
                  A(r,1) = (*props.resp_GM)[s];
              }
              else {
                A(r,s) = 1.0;
              }
            }
          }
          //VAR(K);
          //VAR(A);
        }
        
        
        ExternalEnergyComputer::Shared::~Shared()
        {
          delete tod;
          delete fiso;
          delete eext;
        }
        
        
        
        void ExternalEnergyComputer::resetEnergy()
        {
          Image::Loop loop (0, 3);  // Loop over spatial dimensions
  //        for (loop.start(dwi_vox, tod_vox, eext_vox); loop.ok(); loop.next(dwi_vox, tod_vox, eext_vox))
  //        {
  //          y = Math::Vector<float>(dwi_vox.address(), s.nrows);
  //          t = Math::Vector<float>(tod_vox.address(), s.ncols);
  //          eext_vox.value() = calcEnergy();
  //        }
          Math::Vector<float> fiso;
          for (loop.start(dwi_vox); loop.ok(); loop.next(dwi_vox))
          {
            tod_vox[0] = fiso_vox[0] = eext_vox[0] = dwi_vox[0];
            tod_vox[1] = fiso_vox[1] = eext_vox[1] = dwi_vox[1];
            tod_vox[2] = fiso_vox[2] = eext_vox[2] = dwi_vox[2];
            y = Math::Vector<float>(dwi_vox.address(), s.nrows);
            t = Math::Vector<float>(tod_vox.address(), s.ncols);
            eext_vox.value() = calcEnergy();
            fiso = f;                         // Cast double to float...
            memcpy(fiso_vox.address(), fiso.ptr(), s.nf*sizeof(float));
          }
        }
        
        void ExternalEnergyComputer::acceptChanges()
        {
          for (int k = 0; k != changes_vox.size(); ++k) 
          {
            tod_vox[0] = fiso_vox[0] = eext_vox[0] = changes_vox[k][0];
            tod_vox[1] = fiso_vox[1] = eext_vox[1] = changes_vox[k][1];
            tod_vox[2] = fiso_vox[2] = eext_vox[2] = changes_vox[k][2];
            memcpy(tod_vox.address(), changes_tod[k].ptr(), s.ncols*sizeof(float));
            memcpy(fiso_vox.address(), changes_fiso[k].ptr(), s.nf*sizeof(float));
            eext_vox.value() = changes_eext[k];
          }
          clearChanges();
        }
        
        
        void ExternalEnergyComputer::clearChanges()
        {
          changes_vox.clear();
          changes_tod.clear();
          changes_fiso.clear();
          changes_eext.clear();
        }
        
        
        void ExternalEnergyComputer::add(const Point_t &pos, const Point_t &dir, const double factor)
        {
          Point_t p = T.scanner2voxel(pos);
          Point_t v = Point_t(Math::floor(p[0]), Math::floor(p[1]), Math::floor(p[2]));
          Point_t w = Point_t(hanning(p[0]-v[0]), hanning(p[1]-v[1]), hanning(p[2]-v[2]));
          
          Math::SH::delta(d, Point<double>(dir), s.lmax);
          
          Point<int> x = Point<int>(v);
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
        
        
        void ExternalEnergyComputer::add2vox(const Point<int>& vox, const double w)
        {
          if (w == 0.0)
            return;
          tod_vox[0] = vox[0];
          tod_vox[1] = vox[1];
          tod_vox[2] = vox[2];
          if (!tod_vox.valid())
            return;
          t = d;
          t *= w;
          int k;
          for (k = 0; k != changes_vox.size(); ++k) {
            if (changes_vox[k] == vox) {
              changes_tod[k] += t;
              return;
            }
          }
          changes_vox.push_back(vox);
          t += Math::Vector<float>(tod_vox.address(), s.ncols);
          changes_tod.push_back(t);
        }
        
        
        double ExternalEnergyComputer::eval()
        {
          double dE = 0.0;
          for (int k = 0; k != changes_vox.size(); ++k) 
          {
            dwi_vox[0] = eext_vox[0] = changes_vox[k][0];
            dwi_vox[1] = eext_vox[1] = changes_vox[k][1];
            dwi_vox[2] = eext_vox[2] = changes_vox[k][2];
            y = Math::Vector<float>(dwi_vox.address(), s.nrows);
            t = changes_tod[k];
            double e = calcEnergy();
            changes_fiso.push_back(f);
            dE += e - eext_vox.value();
            changes_eext.push_back(e);
          }
          return dE;
        }
        
        
        double ExternalEnergyComputer::calcEnergy()
        {
          Math::mult(y, 1.0, -1.0, CblasNoTrans, s.K, t);
          Math::Matrix<double> work;
          
          // Original:
          //Math::solve_LS(f, s.A, y, work);
          
          // Fixed Tikhonov:
          //double reg = 1.0;
          //Math::solve_LS_reg(f, s.A, y, reg, work);
          
          // Adaptive Tikhonov (works well, but very slow):
          Math::Vector<double> w (s.nf), f0 (s.nf);
          f.zero();
          w = 1.0;
          do {
            f0 = f;
            Math::solve_LS_reg(f, s.A, y, w, work);
            for (int k = 0; k < s.nf; k++)
              w[k] = (f[k] < 0.0) ? 100.0 : 1.0;
            f0 -= f;
          } while (Math::norm2(f0) > 0.001);
          
          // Clean cutoff (still rather slow):
  //        Math::solve_LS(f, s.A, y, work);
  //        for (int k = 0; k < s.nf; k++)
  //          f[k] = (f[k] < 0.0) ? 0.0 : (f[k] > 1.0) ? 1.0 : f[k];
          
          Math::mult(y, 1.0, -1.0, CblasNoTrans, s.A, f);
          return Math::norm2(y) / s.nrows;
        }
        
        
        
      }
    }
  }
}


