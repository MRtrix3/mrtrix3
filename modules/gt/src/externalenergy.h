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

#ifndef __gt_externalenergy_h__
#define __gt_externalenergy_h__

#include "point.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/transform.h"

#include "particle.h"
#include "gt.h"
#include "energy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        class ExternalEnergyComputer : public EnergyComputer
        {
        public:
          
          class Shared
          {
          public:
            Shared(Image::BufferPreload<float>& dwimage, const Properties& props);
            
            ~Shared();
            
            Image::BufferScratch<float>& getTOD() 
            { 
              if (tod)
                return *tod;
              else
                throw Exception("Uninitialised TOD grid.");
            }
            
            Image::BufferScratch<float>& getFiso() 
            { 
              if (fiso)
                return *fiso;
              else
                throw Exception("Uninitialised TOD grid.");
            }
            
            Image::BufferScratch<float>& getEext() 
            { 
              if (eext)
                return *eext;
              else
                throw Exception("Uninitialised TOD grid.");
            }           
            
            
          protected:
            int lmax, nrows, ncols, nf;
            double beta, mu;
            
            Image::BufferPreload<float>& dwi;
            Image::BufferScratch<float>* tod; 
            Image::BufferScratch<float>* fiso;
            Image::BufferScratch<float>* eext;
            
            Math::Matrix<double> K, Ak, H, Hinv;
            
            friend class ExternalEnergyComputer;
          };
          
          
          ExternalEnergyComputer(Stats& stat, const Shared& shared)
            : EnergyComputer(stat), s(shared), dwi_vox(s.dwi), tod_vox(*(s.tod)), fiso_vox(*(s.fiso)), eext_vox(*(s.eext)),
              T(s.dwi), y(s.nrows), t(s.ncols), d(s.ncols), fk(s.nf+1), c(s.nf+1), 
              f(fk.sub(1, s.nf+1)), A(s.Ak.sub(0, s.nrows, 1, s.nf+1)), dE(0.0)
          {
            resetEnergy();
          }
          
          ExternalEnergyComputer(const ExternalEnergyComputer& E)
            : EnergyComputer(E.stats), s(E.s), dwi_vox(E.dwi_vox), tod_vox(E.tod_vox), fiso_vox(E.fiso_vox), eext_vox(E.eext_vox),
              T(E.T), y(s.nrows), t(s.ncols), d(s.ncols), fk(s.nf+1), c(s.nf+1), 
              f(fk.sub(1, s.nf+1)), A(s.Ak.sub(0, s.nrows, 1, s.nf+1)), dE(0.0)
          {  }
          
          ~ExternalEnergyComputer() { }
          
          
          void resetEnergy();
          
          double stageAdd(const Point_t& pos, const Point_t& dir)
          {
            add(pos, dir, 1.0);
            return eval();
          }
          
          double stageShift(const Particle* par, const Point_t& pos, const Point_t& dir)
          {
            add(par->getPosition(), par->getDirection(), -1.0);
            add(pos, dir, 1.0);
            return eval();
          }
          
          double stageRemove(const Particle* par)
          {
            add(par->getPosition(), par->getDirection(), -1.0);
            return eval();
          }
          
          void acceptChanges();
          
          void clearChanges();
          
          EnergyComputer* clone() const { return new ExternalEnergyComputer(*this); }
          
          
          
        protected:
          const Shared& s;
          
          Image::BufferPreload<float>::voxel_type dwi_vox;
          Image::BufferScratch<float>::voxel_type tod_vox;
          Image::BufferScratch<float>::voxel_type fiso_vox;
          Image::BufferScratch<float>::voxel_type eext_vox;
          
          Image::Transform T;
          
          Math::Vector<double> y, t, d, fk, c;
          Math::Vector<double>::View f;
          const Math::Matrix<double>::View A;
          double dE;
          
          std::vector<Point<int> > changes_vox;
          std::vector<Math::Vector<float> > changes_tod;
          std::vector<Math::Vector<float> > changes_fiso;
          std::vector<float> changes_eext;
          
          
          void add(const Point_t& pos, const Point_t& dir, const double factor = 1.);
          
          void add2vox(const Point<int> &vox, const double w);
          
          double eval();
          
          double calcEnergy();
          
          inline double hanning(const double w) const
          {
            return (w <= (1.0-s.beta)/2) ? 0.0 : (w >= (1.0+s.beta)/2) ? 1.0 : (1 - Math::cos(M_PI * (w-(1.0-s.beta)/2)/s.beta )) / 2;
          }
          
        };


      }
    }
  }
}

#endif // __gt_externalenergy_h__
