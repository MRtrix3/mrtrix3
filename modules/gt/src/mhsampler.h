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

#include "ptr.h"
#include "thread/mutex.h"
#include "image/transform.h"
#include "image/buffer_preload.h"

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
          MHSampler(const Image::Info &dwi, Properties& p, Stats& s, ParticleGrid& pgrid, 
                    EnergyComputer& e, Image::BufferPreload<bool>* m = NULL);
                    
          void execute(const int niter, const double t0, const double t1);
          
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
          EnergyComputer& E;      // FIXME For safe copy-construction (multi-threading), this should be an lvalue instead of
                                  // an rvalue. However, in that case the dynamic type is wrong!
          Image::Transform T;
          int dims[3];
          Ptr<Image::BufferPreload<bool>::voxel_type > mask;    // Smart pointers make deep copy in copy constructor.
          
          RefPtr<SpatialLock<> > lock;
          Math::RNG rng;
          float sigpos, sigdir;
          
          
          Point_t getRandPosInMask();
          
          bool inMask(const Point_t p) const;
          
          Point_t getRandDir();
          
          void moveRandom(const Particle* par, Point_t& pos, Point_t& dir);
          
          bool moveOptimal(const Particle* par, Point_t& pos, Point_t& dir) const;
          
          double calcShiftProb(const Particle* par, const Point_t& pos, const Point_t& dir) const;
          
          
        };
        

      }
    }
  }
}


#endif // __gt_mhsampler_h__
