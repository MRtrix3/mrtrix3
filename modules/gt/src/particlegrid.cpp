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

        ParticleGrid::ParticleGrid(const Image::Info& image)
          : T(image)
        {
          n[0] = Math::ceil<size_t>( image.dim(0) * image.vox(0) / (2*Particle::L) );
          n[1] = Math::ceil<size_t>( image.dim(1) * image.vox(1) / (2*Particle::L) );
          n[2] = Math::ceil<size_t>( image.dim(2) * image.vox(2) / (2*Particle::L) );
          grid.resize(n[0]*n[1]*n[2]);
          
          Image::Info info (image);
          info.vox(0) = info.vox(1) = info.vox(2) = 2*Particle::L;
          info.dim(0) = n[0];
          info.dim(1) = n[1];
          info.dim(2) = n[2];
          info.sanitise();
          T = Image::Transform(info);
        }
        
        void ParticleGrid::add(const Point_t &pos, const Point_t &dir)
        {
          Particle* p = pool.create(pos, dir);
          unsigned int gidx = pos2idx(pos);
          grid[gidx].push_back(p);
          std::lock_guard<std::mutex> lock (mutex);
          list.push_back(p);
        }
        
        void ParticleGrid::shift(const unsigned int idx, const Point_t& pos, const Point_t& dir)
        {
          Particle* p = list[idx];
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
        
        void ParticleGrid::remove(const unsigned int idx)
        {
          Particle* p = list[idx];
          unsigned int gidx0 = pos2idx(p->getPosition());
          for (ParticleVectorType::iterator it = grid[gidx0].begin(); it != grid[gidx0].end(); ++it) {
            if (*it == p) {
              grid[gidx0].erase(it);
              break;
            }
          }
          std::lock_guard<std::mutex> lock (mutex);
          list[idx] = list.back();    // FIXME Not thread safe if last element is in use !  May corrupt datastructure,
          list.pop_back();            // but program won't crash. Ignore for now.
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
          if ((x < 0) || (x >= n[0]) || (y < 0) || (y >= n[1]) || (z < 0) || (z >= n[2]))   // if out of bounds
            return NULL;                                                                    // return empty vector
          return &grid[xyz2idx(x, y, z)];
        }
        
        Particle* ParticleGrid::getRandom(unsigned int &idx)
        {
          if (list.empty())
            return NULL;
          idx = rng.uniform_int(list.size());
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
