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


#ifndef __gt_particlegrid_h__
#define __gt_particlegrid_h__

#include <mutex>

#include "header.h"
#include "transform.h"
#include "dwi/tractography/file.h"
#include "math/rng.h"

#include "dwi/tractography/GT/particle.h"
#include "dwi/tractography/GT/particlepool.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        /**
         * @brief The ParticleGrid class
         */
        class ParticleGrid
        { MEMALIGN(ParticleGrid)
        public:
          
          typedef vector<Particle*> ParticleVectorType;
          
          template <class HeaderType>
          ParticleGrid(const HeaderType& image)
          {
            DEBUG("Initialise particle grid.");
            dims[0] = Math::ceil<size_t>( image.size(0) * image.spacing(0) / (2*Particle::L) );
            dims[1] = Math::ceil<size_t>( image.size(1) * image.spacing(1) / (2*Particle::L) );
            dims[2] = Math::ceil<size_t>( image.size(2) * image.spacing(2) / (2*Particle::L) );
            grid.resize(dims[0]*dims[1]*dims[2]);
            
            // Initialise scanner-to-grid transform
            Eigen::DiagonalMatrix<default_type, 3> newspacing (2*Particle::L, 2*Particle::L, 2*Particle::L);
            Eigen::Vector3 shift (image.spacing(0)/2 - Particle::L, 
                                  image.spacing(1)/2 - Particle::L, 
                                  image.spacing(2)/2 - Particle::L);
            T_s2g = image.transform() * newspacing;
            T_s2g = T_s2g.inverse().translate(shift);
          }
          
          ParticleGrid(const ParticleGrid&) = delete;
          ParticleGrid& operator=(const ParticleGrid&) = delete;
          
          ~ParticleGrid() {
            clear();
          }
          
          inline unsigned int getTotalCount() const {
            return pool.size();
          }
          
          void add(const Point_t& pos, const Point_t& dir);
          
          void shift(Particle* p, const Point_t& pos, const Point_t& dir);
          
          void remove(Particle* p);
          
          void clear();
          
          const ParticleVectorType* at(const ssize_t x, const ssize_t y, const ssize_t z) const;
          
          inline Particle* getRandom() {
            return pool.random();
          }
          
          void exportTracks(Tractography::Writer<float>& writer);
          
          
        protected:
          std::mutex mutex;
          ParticlePool pool;
          vector<ParticleVectorType> grid;
          Math::RNG rng;
          transform_type T_s2g;
          size_t dims[3];
          
          
          inline size_t pos2idx(const Point_t& pos) const
          {
            size_t x, y, z;
            pos2xyz(pos, x, y, z);
            return xyz2idx(x, y, z);
          }
          
        public:
          inline void pos2xyz(const Point_t& pos, size_t& x, size_t& y, size_t& z) const
          {
            Point_t gpos = T_s2g.cast<float>() * pos;
            x = Math::round<size_t>(gpos[0]);
            y = Math::round<size_t>(gpos[1]);
            z = Math::round<size_t>(gpos[2]);
          }
          
        protected:
          inline size_t xyz2idx(const size_t x, const size_t y, const size_t z) const
          {
            return z + dims[2] * (y + dims[1] * x);
          }
          
        };

      }
    }
  }
}

#endif // __gt_particlegrid_h__
