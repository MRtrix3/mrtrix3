/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 04/06/13.
    
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

#ifndef __gt_particlegrid_h__
#define __gt_particlegrid_h__

#include <mutex>

#include "header.h"
#include "transform.h"
#include "dwi/tractography/file.h"
#include "math/rng.h"

#include "particle.h"
#include "particlepool.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        /**
         * @brief The ParticleGrid class
         */
        class ParticleGrid
        {
        public:
          
          typedef std::vector<Particle*> ParticleVectorType;
          
          template <class HeaderType>
          ParticleGrid(const HeaderType& image)
          {
            DEBUG("Initialise particle grid.");
            n[0] = Math::ceil<size_t>( image.size(0) * image.spacing(0) / (2*Particle::L) );
            n[1] = Math::ceil<size_t>( image.size(1) * image.spacing(1) / (2*Particle::L) );
            n[2] = Math::ceil<size_t>( image.size(2) * image.spacing(2) / (2*Particle::L) );
            grid.resize(n[0]*n[1]*n[2]);
            
            Eigen::DiagonalMatrix<default_type, 3> newspacing (2*Particle::L, 2*Particle::L, 2*Particle::L);
            T = image.transform() * newspacing;
            T = T.inverse();
          }
          
          ParticleGrid(const ParticleGrid& other)
            : mutex(), pool(), list(other.list), grid(other.grid), rng(), T(other.T)
          {
            DEBUG("Copy particle grid.");    // Needed for C++11 clang compilation (?). Should not be called though.
            n[0] = other.n[0];
            n[1] = other.n[1];
            n[2] = other.n[2];
          }
          
          ~ParticleGrid() {
            clear();
          }
          
          unsigned int getTotalCount() const {
            return list.size();
          }
          
          void add(const Point_t& pos, const Point_t& dir);
          
          void shift(Particle* p, const Point_t& pos, const Point_t& dir);
          
          void remove(const size_t idx);
          
          void clear();
          
          const ParticleVectorType* at(const int x, const int y, const int z) const;
          
          Particle* getRandom(size_t& idx);
          
          void exportTracks(Tractography::Writer<float>& writer);
          
          
        protected:
          std::mutex mutex;
          ParticlePool pool;
          ParticleVectorType list;
          std::vector<ParticleVectorType> grid;
          Math::RNG rng;
          transform_type T;
          size_t n[3];     // grid dimensions
          
          
          inline size_t pos2idx(const Point_t& pos) const
          {
            size_t x, y, z;
            pos2xyz(pos, x, y, z);
            return xyz2idx(x, y, z);
          }
          
        public:
          inline void pos2xyz(const Point_t& pos, size_t& x, size_t& y, size_t& z) const
          {
            Point_t gpos = T.cast<float>() * pos;
            x = Math::round<size_t>(gpos[0]);
            y = Math::round<size_t>(gpos[1]);
            z = Math::round<size_t>(gpos[2]);
          }
          
        protected:
          inline size_t xyz2idx(const size_t x, const size_t y, const size_t z) const
          {
            return z + n[2] * (y + n[1] * x);
          }
          
        };

      }
    }
  }
}

#endif // __gt_particlegrid_h__
