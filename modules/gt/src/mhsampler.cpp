/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 06/06/13.
    
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

#include "mhsampler.h"

#include "math/math.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {

        MHSampler::MHSampler(const Image<float>& dwi, Properties &p, Stats &s, ParticleGrid &pgrid, 
                             EnergyComputer* e, Image<bool>& m)
          : props(p), stats(s), pGrid(pgrid), E(e), T(dwi), lock(5*Particle::L), sigpos(Particle::L / 8.), sigdir(0.2)
        {
          if (m.valid()) {
            T = Transform(m);
            dims[0] = m.size(0);
            dims[1] = m.size(1);
            dims[2] = m.size(2);
            mask = m;
          }
          else {
            dims[0] = dwi.size(0);
            dims[1] = dwi.size(1);
            dims[2] = dwi.size(2);
          }
        }
        
        
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
          do {
            pos = getRandPosInMask();
          } while (! lock.lockIfNotLocked(pos));
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
          lock.unlock(pos);
        }
        
        
        void MHSampler::death()
        {
          //TRACE;
          stats.incN('d');
          
          size_t idx;
          Particle* par;
           do {
            par = pGrid.getRandom(idx);
            if (par == NULL || par->hasPredecessor() || par->hasSuccessor())
              return;
          } while (! lock.lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          
          double dE = E->stageRemove(par);
          double R = std::exp(-dE) * pGrid.getTotalCount() / props.density * props.p_birth / props.p_death;
          if (R > rng_uniform()) {
            E->acceptChanges();
            pGrid.remove(idx);
            stats.incNa('d');
          }
          else {
            E->clearChanges();
          }
          
          lock.unlock(pos0);
        }
        
        
        void MHSampler::randshift()
        {
          //TRACE;
          stats.incN('r');
          
          size_t idx;
          Particle* par;
          do {
            par = pGrid.getRandom(idx);
            if (par == NULL)
              return;
          } while (! lock.lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          
          Point_t pos, dir;
          moveRandom(par, pos, dir);
          
          if (!inMask(T.scanner2voxel.cast<float>() * pos)) {
            lock.unlock(pos0);
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
          
          lock.unlock(pos0);
        }
        
        
        void MHSampler::optshift()
        {
          //TRACE;
          stats.incN('o');
          
          size_t idx;
          Particle* par;
          do {
            par = pGrid.getRandom(idx);
            if (par == NULL)
              return;
          } while (! lock.lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          
          Point_t pos, dir;
          bool moved = moveOptimal(par, pos, dir);
          if (!moved || !inMask(T.scanner2voxel.cast<float>() * pos)) {
            lock.unlock(pos0);
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
          
          lock.unlock(pos0);
        }
        
        
        void MHSampler::connect()       // TODO Current implementation does not prevent loops.
        {
          //TRACE;
          stats.incN('c');
          
          size_t idx;
          Particle* par;
          do {
            par = pGrid.getRandom(idx);
            if (par == NULL)
              return;
          } while (! lock.lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
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
          
          lock.unlock(pos0);
        }
        
        
        // SUPPORTING METHODS -----------------------------------------------------------
        
        Point_t MHSampler::getRandPosInMask()
        {
          Point_t p;
          do {
            p[0] = rng_uniform() * (dims[0]-1);
            p[1] = rng_uniform() * (dims[1]-1);
            p[2] = rng_uniform() * (dims[2]-1);
          } while (!inMask(p));           // FIXME Schrijf dit expliciet uit ifv random integer initialisatie,
          return T.voxel2scanner.cast<float>() * p;      // voeg hier dan nog random ruis aan toe. Dan kan je inMask terug ifv
        }                                 // scanner positie schrijven.
        
        
        bool MHSampler::inMask(const Point_t p) const
        {
          if ((p[0] <= -0.5) || (p[0] >= dims[0]-0.5) || 
              (p[1] <= -0.5) || (p[1] >= dims[1]-0.5) ||
              (p[2] <= -0.5) || (p[2] >= dims[2]-0.5))
//          if ((p[0] <= 0.0) || (p[0] >= dims[0]-1.0) || 
//              (p[1] <= 0.0) || (p[1] >= dims[1]-1.0) ||
//              (p[2] <= 0.0) || (p[2] >= dims[2]-1.0))
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
          // assert(par != NULL)
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
        
        
        double MHSampler::calcShiftProb(const Particle *par, const Point_t &pos, const Point_t &dir) const
        {
          Point_t Dpos = par->getPosition() - pos;
          Point_t Ddir = par->getDirection() - dir;
          return gsl_ran_gaussian_pdf(Dpos[0], sigpos) * gsl_ran_gaussian_pdf(Dpos[1], sigpos) * gsl_ran_gaussian_pdf(Dpos[2], sigpos) *
              gsl_ran_gaussian_pdf(Ddir[0], sigdir) * gsl_ran_gaussian_pdf(Ddir[1], sigdir) * gsl_ran_gaussian_pdf(Ddir[2], sigdir);
//	  Point_t Dep1 = par->getEndPoint(-1) - (pos - Particle::L * dir);
//	  Point_t Dep2 = par->getEndPoint(1) - (pos + Particle::L * dir);
//	  double sigma = Particle::L/2;
//	  return gsl_ran_gaussian_pdf(Dep1[0], sigma) * gsl_ran_gaussian_pdf(Dep1[1], sigma) * gsl_ran_gaussian_pdf(Dep1[2], sigma) *
//		 gsl_ran_gaussian_pdf(Dep2[0], sigma) * gsl_ran_gaussian_pdf(Dep2[1], sigma) * gsl_ran_gaussian_pdf(Dep2[2], sigma);
        }
        
        
        

      }
    }
  }
}

