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

#include "dwi/tractography/GT/mhsampler.h"

#include "math/math.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        // RUNTIME METHODS --------------------------------------------------------------
        
        void MHSampler::execute()
        {          
          do {
            next();
          } while (stats.next());
          
        }
        
        
        void MHSampler::next()
        {
          float p = rng_uniform();
          float s = props.p_birth;
          if (p < s)
            return birth();
          s += props.p_death;
          if (p < s)
            return death();
          s += props.p_shift;
          if (p < s)
            return randshift();
          s += props.p_optshift;
          if (p < s)
            return optshift();
          s += props.p_connect;
          if (p < s)
            return connect();
        }
        
        
        // PROPOSAL DISTRIBUTIONS -------------------------------------------------------
        
        void MHSampler::birth()
        {
          //TRACE;
          stats.incN('b');
          
          Point_t pos;
          SpatialLock<float>::Guard spatial_guard (*lock);
          do {
            pos = getRandPosInMask();
          } while (! spatial_guard.try_lock(pos));
          Point_t dir = getRandDir();
          
          double dE = E->stageAdd(pos, dir);
          double R = std::exp(-dE) * props.density / (pGrid.getTotalCount()+1) * props.p_death / props.p_birth;
          if (R > rng_uniform()) {
            E->acceptChanges();
            pGrid.add(pos, dir);
            stats.incNa('b');
          }
          else {
            E->clearChanges();
          }
        }
        
        
        void MHSampler::death()
        {
          //TRACE;
          stats.incN('d');
          
          Particle* par;
          SpatialLock<float>::Guard spatial_guard (*lock);
          do {
            par = pGrid.getRandom();
            if (par == NULL || par->hasPredecessor() || par->hasSuccessor())
              return;
          } while (! spatial_guard.try_lock(par->getPosition()));
          
          double dE = E->stageRemove(par);
          double R = std::exp(-dE) * pGrid.getTotalCount() / props.density * props.p_birth / props.p_death;
          if (R > rng_uniform()) {
            E->acceptChanges();
            pGrid.remove(par);
            stats.incNa('d');
          }
          else {
            E->clearChanges();
          }
        }
        
        
        void MHSampler::randshift()
        {
          //TRACE;
          stats.incN('r');
          
          Particle* par;
          SpatialLock<float>::Guard spatial_guard (*lock);
          do {
            par = pGrid.getRandom();
            if (par == NULL)
              return;
          } while (! spatial_guard.try_lock(par->getPosition()));

          Point_t pos, dir;
          moveRandom(par, pos, dir);
          
          if (!inMask(T.scanner2voxel.cast<float>() * pos)) {
            return;
          }
          double dE = E->stageShift(par, pos, dir);
          double R = exp(-dE);
          if (R > rng_uniform()) {
            E->acceptChanges();
            pGrid.shift(par, pos, dir);
            stats.incNa('r');
          }
          else {
            E->clearChanges();
          }
        }
        
        
        void MHSampler::optshift()
        {
          //TRACE;
          stats.incN('o');
          
          Particle* par;
          SpatialLock<float>::Guard spatial_guard (*lock);
          do {
            par = pGrid.getRandom();
            if (par == NULL)
              return;
          } while (! spatial_guard.try_lock(par->getPosition()));

          Point_t pos, dir;
          bool moved = moveOptimal(par, pos, dir);
          if (!moved || !inMask(T.scanner2voxel.cast<float>() * pos)) {
            return;
          }
          
          double dE = E->stageShift(par, pos, dir);
          double p_prop = calcShiftProb(par, pos, dir);
          double R = exp(-dE) * props.p_shift * p_prop / (props.p_shift * p_prop + props.p_optshift);
          if (R > rng_uniform()) {
            E->acceptChanges();
            pGrid.shift(par, pos, dir);
            stats.incNa('o');
          }
          else {
            E->clearChanges();
          }
        }
        
        
        void MHSampler::connect()       // TODO Current implementation does not prevent loops.
        {
          //TRACE;
          stats.incN('c');
          
          Particle* par;
          SpatialLock<float>::Guard spatial_guard (*lock);
          do {
            par = pGrid.getRandom();
            if (par == NULL)
              return;
          } while (! spatial_guard.try_lock(par->getPosition()));

          int alpha0 = (rng_uniform() < 0.5) ? -1 : 1;
          ParticleEnd pe0;
          pe0.par = par;
          pe0.alpha = alpha0;
          
          ParticleEnd pe2;
          pe2.par = NULL;
          double dE = E->stageConnect(pe0, pe2);
          double R = exp(-dE);
          if (R > rng_uniform()) {
            E->acceptChanges();
            if (pe2.par) {
              if (alpha0 == -1)
                par->connectPredecessor(pe2.par, pe2.alpha);
              else
                par->connectSuccessor(pe2.par, pe2.alpha);
            } else {
              if ((alpha0 == -1) && par->hasPredecessor())
                par->removePredecessor();
              else if ((alpha0 == +1) && par->hasSuccessor())
                par->removeSuccessor();
            }
            stats.incNa('c');
          }
          else {
            E->clearChanges();
          }
        }
        
        
        // SUPPORTING METHODS -----------------------------------------------------------
        
        Point_t MHSampler::getRandPosInMask()
        {
          Point_t p;
          do {
            p[0] = rng_uniform() * dims[0] - 0.5;
            p[1] = rng_uniform() * dims[1] - 0.5;
            p[2] = rng_uniform() * dims[2] - 0.5;
          } while (!inMask(p));
          return T.voxel2scanner.cast<float>() * p;
        }
        
        
        bool MHSampler::inMask(const Point_t p)
        {
          if ((p[0] <= -0.5) || (p[0] >= dims[0]-0.5) || 
              (p[1] <= -0.5) || (p[1] >= dims[1]-0.5) ||
              (p[2] <= -0.5) || (p[2] >= dims[2]-0.5))
            return false;
          if (mask.valid()) {
            mask.index(0) = Math::round<size_t>(p[0]);
            mask.index(1) = Math::round<size_t>(p[1]);
            mask.index(2) = Math::round<size_t>(p[2]);
            return mask.value();
          }
          return true;
        }
        
        
        Point_t MHSampler::getRandDir()
        {
          Point_t dir = Point_t(rng_normal(), rng_normal(), rng_normal());
          dir.normalize();
          return dir;
        }
        
        
        void MHSampler::moveRandom(const Particle *par, Point_t &pos, Point_t &dir)
        {
          pos = par->getPosition() + Point_t(rng_normal()*sigpos, rng_normal()*sigpos, rng_normal()*sigpos);
          dir = par->getDirection() + Point_t(rng_normal()*sigdir, rng_normal()*sigdir, rng_normal()*sigdir);
          dir.normalize();
        }
        
        
        bool MHSampler::moveOptimal(const Particle *par, Point_t &pos, Point_t &dir) const
        {
          // assert(par != NULL);
          if (par->hasPredecessor() && par->hasSuccessor())
          {
            int a1 = (par->getPredecessor()->getPredecessor() == par) ? -1 : 1;
            int a3 = (par->getSuccessor()->getPredecessor() == par) ? -1 : 1;
            pos = (par->getPredecessor()->getEndPoint(a1) + par->getSuccessor()->getEndPoint(a3)) / 2;
            dir = par->getSuccessor()->getPosition() - par->getPredecessor()->getPosition();
            dir.normalize();
            return true;
          }
          else if (par->hasPredecessor())
          {
            int a = (par->getPredecessor()->getPredecessor() == par) ? -1 : 1;
            pos = par->getPredecessor()->getEndPoint(2*a);
            dir = par->getPredecessor()->getDirection() * a;
            return true;
          }
          else if (par->hasSuccessor())
          {
            int a = (par->getSuccessor()->getPredecessor() == par) ? -1 : 1;
            pos = par->getSuccessor()->getEndPoint(2*a);
            dir = par->getSuccessor()->getDirection() * (-a);
            return true;
          }
          else
          {
            return false;
          }
        }
        
        

      }
    }
  }
}

