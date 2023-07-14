/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __gt_particle_h__
#define __gt_particle_h__

#include "types.h"



namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        using Point_t = Eigen::Vector3f;
        
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
            alive = false;
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
            alive = true;
          }
          
          inline void finalize()
          {
            if (predecessor)
              removePredecessor();
            if (successor)
              removeSuccessor();
            alive = false;
          }

          // disable copy and assignment
          Particle(const Particle&) = delete;
          Particle& operator=(const Particle&) = delete;
          
          // default move constructor and assignment
          Particle(Particle&&) = default;
          Particle& operator=(Particle&&) = default;

          
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
          
          bool isAlive() const
          {
            return alive;
          }
          

        protected:
          
          Point_t pos, dir;
          Particle* predecessor;
          Particle* successor;
          bool visited;
          bool alive;
          
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
          Particle* par = nullptr;
          int alpha = 0;
          float e_conn = 0.0;
          double p_suc = 1.0;
        };
        
        
        
      }
    }
  }
}

#endif // __gt_particle_h__
