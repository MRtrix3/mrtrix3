/* Copyright (c) 2008-2021 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/GT/particlegrid.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        
        void ParticleGrid::add(const Point_t &pos, const Point_t &dir)
        {
          Particle* p = pool.create(pos, dir);
          size_t gidx = pos2idx(pos);
          grid[gidx].push_back(p);
        }
        
        void ParticleGrid::shift(Particle *p, const Point_t& pos, const Point_t& dir)
        {
          size_t gidx0 = pos2idx(p->getPosition());
          size_t gidx1 = pos2idx(pos);
          grid[gidx0].erase(std::remove(grid[gidx0].begin(), grid[gidx0].end(), p), grid[gidx0].end());
          p->setPosition(pos);
          p->setDirection(dir);
          grid[gidx1].push_back(p);
        }
        
        void ParticleGrid::remove(Particle* p)
        {
          size_t gidx0 = pos2idx(p->getPosition());
          grid[gidx0].erase(std::remove(grid[gidx0].begin(), grid[gidx0].end(), p), grid[gidx0].end());
          pool.destroy(p);
        }
        
        void ParticleGrid::clear()
        {
          grid.clear();
          pool.clear();
        }
        
        const ParticleGrid::ParticleVectorType* ParticleGrid::at(const ssize_t x, const ssize_t y, const ssize_t z) const
        {
          if ((x < 0) || (size_t(x) >= dims[0]) || (y < 0) || (size_t(y) >= dims[1]) || (z < 0) || (size_t(z) >= dims[2]))  // out of bounds
            return nullptr;
          return &grid[xyz2idx(x, y, z)];
        }
        
        void ParticleGrid::exportTracks(Tractography::Writer<float> &writer)
        {
          std::lock_guard<std::mutex> lock (mutex);
          // Initialise
          Particle* par;
          Particle* nextpar;
          int alpha = 0;
          vector<Point_t> track;
          // Loop through all unvisited particles
          for (ParticleVectorType& gridvox : grid)
          {
            for (Particle* par0 : gridvox) 
            {
              par = par0;
              if (!par->isVisited())
              {
                par->setVisited(true);
                // forward
                track.push_back(par->getPosition());
                alpha = +1;
                while ((alpha == +1) ? par->hasSuccessor() : par->hasPredecessor())
                {
                  nextpar = (alpha == +1) ? par->getSuccessor() : par->getPredecessor();
                  alpha = (nextpar->getPredecessor() == par) ? +1 : -1;
                  track.push_back(nextpar->getPosition());
                  nextpar->setVisited(true);
                  par = nextpar;
                }
                track.push_back(par->getEndPoint(alpha));
                // backward
                par = par0;
                std::reverse(track.begin(), track.end());
                alpha = -1;
                while ((alpha == +1) ? par->hasSuccessor() : par->hasPredecessor())
                {
                  nextpar = (alpha == +1) ? par->getSuccessor() : par->getPredecessor();
                  alpha = (nextpar->getPredecessor() == par) ? +1 : -1;
                  track.push_back(nextpar->getPosition());
                  nextpar->setVisited(true);
                  par = nextpar;
                }
                track.push_back(par->getEndPoint(alpha));
                if (track.size() > 1)
                  writer(track);
                track.clear();
              }
            }
          }
          // Free all particle locks
          for (ParticleVectorType& gridvox : grid) {
            for (Particle* par : gridvox) {
                par->setVisited(false);
            }
          }
        }
        

      }
    }
  }
}
