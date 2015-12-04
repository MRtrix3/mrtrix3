/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 06/06/13.
    
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

#ifndef __gt_mhsampler_h__
#define __gt_mhsampler_h__

#include "image.h"
#include "transform.h"

#include "math/rng.h"

#include "gt.h"
#include "particle.h"
#include "particlegrid.h"
#include "energy.h"
#include "spatiallock.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {

        /**
         * @brief The MHSampler class
         */
        class MHSampler
        {
        public:
          MHSampler(const Image<float>& dwi, Properties &p, Stats &s, ParticleGrid &pgrid, 
                    EnergyComputer* e, Image<bool>& m)
            : props(p), stats(s), pGrid(pgrid), E(e), T(dwi), 
              dims{size_t(dwi.size(0)), size_t(dwi.size(1)), size_t(dwi.size(2))}, 
              mask(m), lock(std::make_shared<SpatialLock<float>>(5*Particle::L)), 
              sigpos(Particle::L / 8.), sigdir(0.2)
          {
            DEBUG("Initialise Metropolis Hastings sampler.");
          }
          
          MHSampler(const MHSampler& other)
            : props(other.props), stats(other.stats), pGrid(other.pGrid), E(other.E->clone()), 
              T(other.T), dims(other.dims), mask(other.mask), lock(other.lock), rng_uniform(), rng_normal(), sigpos(other.sigpos), sigdir(other.sigdir)
          {
            DEBUG("Copy Metropolis Hastings sampler.");
          }
          
          ~MHSampler() { delete E; }
                    
          void execute();
          
          void next();
          
          void birth();
          void death();
          void randshift();
          void optshift();
          void connect();
          
          
        protected:
          
          Properties& props;
          Stats& stats;
          ParticleGrid& pGrid;
          EnergyComputer* E;      // Polymorphic copy requires call to EnergyComputer::clone(), hence references or smart pointers won't do.
          
          Transform T;
          std::vector<size_t> dims;
          Image<bool> mask;
          
          std::shared_ptr< SpatialLock<float> > lock;
          Math::RNG::Uniform<float> rng_uniform;
          Math::RNG::Normal<float> rng_normal;
          float sigpos, sigdir;
          
          
          Point_t getRandPosInMask();
          
          bool inMask(const Point_t p);
          
          Point_t getRandDir();
          
          void moveRandom(const Particle* par, Point_t& pos, Point_t& dir);
          
          bool moveOptimal(const Particle* par, Point_t& pos, Point_t& dir) const;
          
          inline double calcShiftProb(const Particle* par, const Point_t& pos, const Point_t& dir) const
          {
            Point_t Dpos = par->getPosition() - pos;
            Point_t Ddir = par->getDirection() - dir;
            return gaussian_pdf(Dpos, sigpos) * gaussian_pdf(Ddir, sigdir);
          }
          
          inline double gaussian_pdf(const Point_t& x, double sigma) const {
            return std::exp( -x.squaredNorm() / (2*sigma) ) / std::sqrt( 2*Math::pi * sigma*sigma);
          }
          
          
        };
        

      }
    }
  }
}


#endif // __gt_mhsampler_h__
