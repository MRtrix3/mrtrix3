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

#ifndef __gt_particle_h__
#define __gt_particle_h__

#include <Eigen/Dense>


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        typedef Eigen::Vector3f Point_t;
        
        /**
         * A particle is a segment of a track and consists of a position and a direction.
         */
        class Particle
        {
        public:
          
          // Particle length
          static float L;
          
          // Constructors and destructor --------------------------------------------------
          
          Particle()
          {
            predecessor = nullptr;
            successor = nullptr;
            visited = false;
          }
          
          Particle(const Point_t& p, const Point_t& d)
          {
            init(p, d);
          }
          
          ~Particle()
          {
            finalize();
          }
          
          inline void init(const Point_t& p, const Point_t& d)
          {
            setPosition(p);
            setDirection(d);
            predecessor = nullptr;
            successor = nullptr;
            visited = false;
          }
          
          inline void finalize()
          {
            if (predecessor)
              removePredecessor();
            if (successor)
              removeSuccessor();
          }
          
          
          // Getters and setters ----------------------------------------------------------
          
          Point_t getPosition() const
          {
            return pos;
          }
          
          void setPosition(const Point_t& p)
          {
            pos = p;
          }
          
          Point_t getDirection() const
          {
            return dir;
          }
          
          void setDirection(const Point_t& d)
          {
            dir = d;
            dir.normalize();
          }
          
          Point_t getEndPoint(const int a) const
          {
            return (pos + a*L*dir);
          }
          
          bool hasPredecessor() const
          {
            return (predecessor != nullptr);
          }
          
          Particle* getPredecessor() const
          {
            return predecessor;
          }
          
          void connectPredecessor(Particle* p1, const int a1)
          {
            assert(p1 != nullptr);
            setPredecessor(p1);
            if (a1 == 1)
              p1->setSuccessor(this);
            if (a1 == -1)
              p1->setPredecessor(this);
          }
          
          void removePredecessor()
          {
            assert(predecessor != nullptr);
            assert(predecessor->predecessor == this || predecessor->successor == this);
            if (predecessor->predecessor == this)
              predecessor->predecessor = nullptr;
            if (predecessor->successor == this)
              predecessor->successor = nullptr;
            predecessor = nullptr;
          }
          
          bool hasSuccessor() const
          {
            return (successor != nullptr);
          }
          
          Particle* getSuccessor() const
          {
            return successor;
          }
          
          void connectSuccessor(Particle* p1, const int a1)
          {
            assert(p1 != nullptr);
            setSuccessor(p1);
            if (a1 == 1)
              p1->setSuccessor(this);
            if (a1 == -1)
              p1->setPredecessor(this);
          }
          
          void removeSuccessor()
          {
            assert(successor != nullptr);
            assert(successor->predecessor == this || successor->successor == this);
            if (successor->predecessor == this)
              successor->predecessor = nullptr;
            if (successor->successor == this)
              successor->successor = nullptr;
            successor = nullptr;
          }
          
          bool isVisited() const
          {
            return visited;
          }
          
          void setVisited(const bool v)
          {
            visited = v;
          }
          

        protected:
          
          Point_t pos, dir;
          Particle* predecessor;
          Particle* successor;
          bool visited;
          
          void setPredecessor(Particle* p1)
          {
            if (predecessor)
              removePredecessor();
            predecessor = p1;
          }
          
          void setSuccessor(Particle* p1)
          {
            if (successor)
              removeSuccessor();
            successor = p1;
          }
          
        };

        
        
        /**
         * Small data structure that refers to one end of a particle.
         * It is used to represent candidate neighbours of a given particle,
         * and to represent a pending fibre track.
         */
        struct ParticleEnd
        {
          Particle* par;
          int alpha;
          float e_conn;
          double p_suc;
        };
        
        
        
      }
    }
  }
}

#endif // __gt_particle_h__
