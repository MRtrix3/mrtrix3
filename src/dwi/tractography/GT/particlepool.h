/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gt_particlepool_h__
#define __gt_particlepool_h__

#define PAGESIZE 10000

#include <deque>
#include <stack>
#include <mutex>

#include "math/rng.h"

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
//              // Create new particles
//              pool.resize(pool.size() + PAGESIZE);
//              std::deque<Particle>::reverse_iterator it = pool.rbegin();
//              for (unsigned int k = 0; k < PAGESIZE; ++it, ++k) {
//                avail.push( &(*it) );
//              }
              pool.emplace_back(pos, dir);
              return &pool.back();
            } else {
              Particle* p = avail.top();
              p->init(pos, dir);
              avail.pop();
              return p;
            }
          }
          
          /**
           * @brief Destroys the particle at pointer p.
           */
          void destroy(Particle* p) {
            std::lock_guard<std::mutex> lock (mutex);
            p->finalize();
            avail.push(p);
//            VAR(avail.size());
          }
          
          inline size_t size() const {
            return pool.size() - avail.size();
          }
          
          Particle* getRandom() {
            std::lock_guard<std::mutex> lock (mutex);
            Particle* p = nullptr;
            if (pool.size() > avail.size())
            {
              std::uniform_int_distribution<size_t> dist(0, pool.size()-1);
              do {
                p = &pool[dist(rng)];
              } while (!p->isAlive());
            }
            return p;
          }
          
          void clear() {
            std::lock_guard<std::mutex> lock (mutex);
            pool.clear();
            std::stack<Particle*> e = {};
            avail.swap(e);
          }
          
        protected:
          std::mutex mutex;
          std::deque<Particle> pool;
          std::stack<Particle*> avail;
          Math::RNG rng;
        };

      }
    }
  }
}

#endif // __gt_particlepool_h__
