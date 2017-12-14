/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __gt_mhsampler_h__
#define __gt_mhsampler_h__

#include "image.h"
#include "transform.h"

#include "math/rng.h"

#include "dwi/tractography/GT/gt.h"
#include "dwi/tractography/GT/particle.h"
#include "dwi/tractography/GT/particlegrid.h"
#include "dwi/tractography/GT/energy.h"
#include "dwi/tractography/GT/spatiallock.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {

        /**
         * @brief The MHSampler class
         */
        class MHSampler
        { MEMALIGN(MHSampler)
        public:
          MHSampler(const Image<float>& dwi, Properties &p, Stats &s, ParticleGrid &pgrid, 
                    EnergyComputer* e, Image<bool>& m)
            : props(p), stats(s), pGrid(pgrid), E(e), T(dwi), 
              dims{size_t(dwi.size(0)), size_t(dwi.size(1)), size_t(dwi.size(2))}, 
              mask(m), lock(make_shared<SpatialLock<float>>(5*Particle::L)), 
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
          vector<size_t> dims;
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
