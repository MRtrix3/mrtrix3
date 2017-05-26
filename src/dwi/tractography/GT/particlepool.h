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


#ifndef __gt_particlepool_h__
#define __gt_particlepool_h__

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
        { MEMALIGN(ParticlePool)
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
          }
          
          /**
           * @brief Return number of Particles in the pool.
           */
          inline size_t size() const {
            return pool.size() - avail.size();
          }
          
          /**
           * @brief Select random particle from the pool (uniformly).
           */
          Particle* random() {
            std::lock_guard<std::mutex> lock (mutex);
            if (pool.size() > avail.size())
            {
              std::uniform_int_distribution<size_t> dist(0, pool.size()-1);
              for (int k = 0; k != 5; ++k) {
                Particle* p = &pool[dist(rng)];
                if (p->isAlive())
                  return p;
              }
            }
            return nullptr;
          }
          
          /**
           * @brief Clear pool.
           */
          void clear() {
            std::lock_guard<std::mutex> lock (mutex);
            pool.clear();
            std::stack<Particle*, vector<Particle*> > e {};
            avail.swap(e);
          }
          
        protected:
          std::mutex mutex;
          std::deque<Particle, Eigen::aligned_allocator<Particle> > pool;
          std::stack<Particle*, vector<Particle*> > avail;
          Math::RNG rng;
        };

      }
    }
  }
}

#endif // __gt_particlepool_h__
