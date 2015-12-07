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

#include "particlegrid.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        
        void ParticleGrid::add(const Point_t &pos, const Point_t &dir)
        {
          Particle* p = pool.create(pos, dir);
          unsigned int gidx = pos2idx(pos);
          grid[gidx].push_back(p);
          std::lock_guard<std::mutex> lock (mutex);
          list.push_back(p);
        }
        
        void ParticleGrid::shift(Particle *p, const Point_t& pos, const Point_t& dir)
        {
          unsigned int gidx0 = pos2idx(p->getPosition());
          unsigned int gidx1 = pos2idx(pos);
          for (ParticleVectorType::iterator it = grid[gidx0].begin(); it != grid[gidx0].end(); ++it) {
            if (*it == p) {
              grid[gidx0].erase(it);
              break;
            }
          }
          p->setPosition(pos);
          p->setDirection(dir);
          grid[gidx1].push_back(p);
        }
        
        void ParticleGrid::remove(const size_t idx)
        {
          std::lock_guard<std::mutex> lock (mutex);
          Particle* p = list[idx];
          unsigned int gidx0 = pos2idx(p->getPosition());
          for (ParticleVectorType::iterator it = grid[gidx0].begin(); it != grid[gidx0].end(); ++it) {
            if (*it == p) {
              grid[gidx0].erase(it);
              break;
            }
          }
          list[idx] = list.back();    // FIXME Not thread safe if last element is in use by another _delete_ proposal!  
          list.pop_back();            // May corrupt datastructure, but program won't crash. Ignore for now.
          pool.destroy(p);
        }
        
        void ParticleGrid::clear()
        {
          grid.clear();
          std::lock_guard<std::mutex> lock (mutex);
          for (ParticleVectorType::iterator it = list.begin(); it != list.end(); ++it)
            pool.destroy(*it);
          list.clear();
        }
        
        const ParticleGrid::ParticleVectorType* ParticleGrid::at(const int x, const int y, const int z) const
        {
          if ((x < 0) || (x >= dims[0]) || (y < 0) || (y >= dims[1]) || (z < 0) || (z >= dims[2]))  // out of bounds
            return nullptr;
          return &grid[xyz2idx(x, y, z)];
        }
        
        Particle* ParticleGrid::getRandom(size_t& idx)
        {
          if (list.empty())
            return nullptr;
          std::uniform_int_distribution<size_t> dist(0, list.size()-1);
          idx = dist(rng);
          return list[idx];
        }
        
        void ParticleGrid::exportTracks(Tractography::Writer<float> &writer)
        {
          std::lock_guard<std::mutex> lock (mutex);
          // Initialise
          Particle* par;
          Particle* nextpar;
          int alpha = 0;
          std::vector<Point_t> track;
          // Loop through all unvisited particles
          for (ParticleVectorType::iterator it = list.begin(); it != list.end(); ++it) 
          {
            par = (*it);
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
              par = (*it);
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
          // Free all particle locks
          for (ParticleVectorType::iterator it = list.begin(); it != list.end(); ++it)
            (*it)->setVisited(false);
        }
        

      }
    }
  }
}
