/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
            return (w <= (1.0-beta)/2) ? 0.0 : (w >= (1.0+beta)/2) ? 1.0 : (1 - std::cos(Math::pi * (w-(1.0-beta)/2)/beta )) / 2;
          }
          
        };


      }
    }
  }
}

#endif // __gt_externalenergy_h__
