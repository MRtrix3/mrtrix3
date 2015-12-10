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

#include "image.h"
#include "transform.h"
#include "math/constrained_least_squares.h"

#include "dwi/tractography/GT/particle.h"
#include "dwi/tractography/GT/gt.h"
#include "dwi/tractography/GT/energy.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        class ExternalEnergyComputer : public EnergyComputer
        {
        public:
          
          ExternalEnergyComputer(Stats& stat, const Image<float>& dwimage, const Properties& props);
          
          
          Image<float>& getTOD() { return tod; }
          Image<float>& getFiso() { return fiso; }
          Image<float>& getEext() { return eext; }
          
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
          
          Image<float> dwi;
          Image<float> tod;
          Image<float> fiso;
          Image<float> eext;
          
          transform_type T;
          
          int lmax; 
          size_t nrows, ncols, nf;
          double beta, mu, dE;
          Eigen::MatrixXd K, Ak;
          Eigen::VectorXd y, t, d, fk;
          
          Math::ICLS::Problem<double> nnls;
          
          std::vector<Eigen::Vector3i > changes_vox;
          std::vector<Eigen::VectorXd > changes_tod;
          std::vector<Eigen::VectorXd > changes_fiso;
          std::vector<double> changes_eext;
          
          
          void add(const Point_t& pos, const Point_t& dir, const double factor = 1.);
          
          void add2vox(const Eigen::Vector3i& vox, const double w);
          
          double eval();
          
          double calcEnergy();
          
          inline double hanning(const double w) const
          {
            return (w <= (1.0-beta)/2) ? 0.0 : (w >= (1.0+beta)/2) ? 1.0 : (1 - std::cos(M_PI * (w-(1.0-beta)/2)/beta )) / 2;
          }
          
        };


      }
    }
  }
}

#endif // __gt_externalenergy_h__
