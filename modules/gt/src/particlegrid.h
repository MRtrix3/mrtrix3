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

#include "thread/mutex.h"
#include "image/info.h"
#include "image/transform.h"
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
          
          ParticleGrid(const Image::Info& image);
          
          ~ParticleGrid() {
            clear();
          }
          
          unsigned int getTotalCount() const {
            return list.size();
          }
          
          void add(const Point_t& pos, const Point_t& dir);
          
          void shift(const unsigned int idx, const Point_t& pos, const Point_t& dir);
          
          void remove(const unsigned int idx);
          
          void clear();
          
          const ParticleVectorType* at(const int x, const int y, const int z) const;
          
          Particle* getRandom(unsigned int& idx);
          
          void exportTracks(Tractography::Writer<float>& writer);
          
          
        protected:
          Thread::Mutex mutex;
          ParticlePool pool;
          ParticleVectorType list;
          std::vector<ParticleVectorType> grid;
          Math::RNG rng;
          MR::Image::Transform T;
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
            Point_t gpos = T.scanner2voxel(pos);
            x = Math::round(gpos[0]);
            y = Math::round(gpos[1]);
            z = Math::round(gpos[2]);
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
