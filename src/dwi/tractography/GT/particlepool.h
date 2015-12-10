/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 17/03/14.
    
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

#ifndef __gt_particlepool_h__
#define __gt_particlepool_h__

#define PAGESIZE 10000

#include <deque>
#include <stack>

#include <mutex>

#include "dwi/tractography/GT/particle.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        /**
         * @brief ParticlePool manages creation and deletion of particles,
         *        minimizing the no. calls to new/delete.
         */
        class ParticlePool
        {
        public:
          ParticlePool() { }
          
          ParticlePool(const ParticlePool&) = delete;
          ParticlePool& operator=(const ParticlePool&) = delete;
          ~ParticlePool() { }
          
          /**
           * @brief Creates a new particle and returns a pointer to its address.
           */
          Particle* create(const Point_t& pos, const Point_t& dir)
          {
            std::lock_guard<std::mutex> lock (mutex);
            if (avail.empty()) {
              // Create new particles
              pool.resize(pool.size() + PAGESIZE);
              std::deque<Particle>::reverse_iterator it = pool.rbegin();
              for (unsigned int k = 0; k < PAGESIZE; ++it, ++k) {
                avail.push( &(*it) );
              }
            }
            Particle* p = avail.top();
            p->init(pos, dir);
            avail.pop();
            return p;
          }
          
          /**
           * @brief Destroys the particle at pointer p.
           */
          void destroy(Particle* p) {
            std::lock_guard<std::mutex> lock (mutex);
            p->finalize();
            avail.push(p);
          }
          
        protected:
          std::mutex mutex;
          std::deque<Particle> pool;
          std::stack<Particle*> avail;
        };

      }
    }
  }
}

#endif // __gt_particlepool_h__
