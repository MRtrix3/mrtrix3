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
#include "progressbar.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {

        MHSampler::MHSampler(const Image::Info &dwi, Properties &p, Stats &s, ParticleGrid &pgrid, 
                             EnergyComputer &e, Image::BufferPreload<bool>* m)
          : props(p), stats(s), pGrid(pgrid), E(e), T(dwi), sigpos(Particle::L / 8.), sigdir(0.2)
        {
          lock = new SpatialLock<>(3*Particle::L);  // FIXME take voxel size into account for setting lock threshold.
          if (m) {
            T = Image::Transform(*m);
            dims[0] = m->dim(0);
            dims[1] = m->dim(1);
            dims[2] = m->dim(2);
            mask = new Image::BufferPreload<bool>::voxel_type(*m);
          }
          else {
            dims[0] = dwi.dim(0);
            dims[1] = dwi.dim(1);
            dims[2] = dwi.dim(2);
          }
        }
        
        
        
        void MHSampler::execute(const int niter, const double t0, const double t1)
        {
          ProgressBar progress ("running MH sampler", niter/1000);
          // exponantial annealing factor
          double alpha = Math::pow(t1/t0, 1.0/double(niter/1000));
          for (int k = 0; k < niter; k++)
          {
            next();
            if (k % 1000 == 0) {
              progress++;
              stats.setTint(stats.getTint()*alpha);
              //std::cout << stats << std::endl;
            }
          }
        }
        
        
        void MHSampler::next()
        {
          float p = rng.uniform();
          float s = props.p_birth;
          if (p < s) {
            birth();
            return;
          }
          s += props.p_death;
          if (p < s) {
            death();
            return;
          }
          s += props.p_shift;
          if (p < s) {
            randshift();
            return;
          }
          s += props.p_optshift;
          if (p < s) {
            optshift();
            return;
          }
          s += props.p_connect;
          if (p < s) {
            connect();
            return;
          }
        }
        
        
        
        void MHSampler::birth()
        {
          //TRACE;
          stats.incN('b');
          
          Point_t pos;
          do {
            pos = getRandPosInMask();
          } while (! lock->lockIfNotLocked(pos));
          Point_t dir = getRandDir();
          
          double dE = E.stageAdd(pos, dir);
          double R = Math::exp(-dE) * props.density / (pGrid.getTotalCount()+1) * props.p_death / props.p_birth;
          if (R > rng.uniform()) {
            E.acceptChanges();
            pGrid.add(pos, dir);
            stats.incNa('b');
          }
          else {
            E.clearChanges();
          }
          
          lock->unlock(pos);
        }
        
        
        void MHSampler::death()
        {
          //TRACE;
          stats.incN('d');
          
          unsigned int idx;
          Particle* par;
           do {
            par = pGrid.getRandom(idx);
            if (par == NULL || par->hasPredecessor() || par->hasSuccessor())
              return;
          } while (! lock->lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          
          double dE = E.stageRemove(par);
          double R = Math::exp(-dE) * pGrid.getTotalCount() / props.density * props.p_birth / props.p_death;
          if (R > rng.uniform()) {
            E.acceptChanges();
            pGrid.remove(idx);
            stats.incNa('d');
          }
          else {
            E.clearChanges();
          }
          
          lock->unlock(pos0);
        }
        
        
        void MHSampler::randshift()
        {
          //TRACE;
          stats.incN('r');
          
          unsigned int idx;
          Particle* par;
          do {
            par = pGrid.getRandom(idx);
            if (par == NULL)
              return;
          } while (! lock->lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          
          Point_t pos, dir;
          moveRandom(par, pos, dir);
          
          if (!inMask(T.scanner2voxel(pos))) {
            lock->unlock(pos0);
            return;
          }
          double dE = E.stageShift(par, pos, dir);
          double R = exp(-dE);
          if (R > rng.uniform()) {
            E.acceptChanges();
            pGrid.shift(idx, pos, dir);
            stats.incNa('r');
          }
          else {
            E.clearChanges();
          }
          
          lock->unlock(pos0);
        }
        
        
        void MHSampler::optshift()
        {
          //TRACE;
          stats.incN('o');
          
          unsigned int idx;
          Particle* par;
          do {
            par = pGrid.getRandom(idx);
            if (par == NULL)
              return;
          } while (! lock->lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          
          Point_t pos, dir;
          bool moved = moveOptimal(par, pos, dir);
          if (!moved || !inMask(T.scanner2voxel(pos))) {
            lock->unlock(pos0);
            return;
          }
          
          double dE = E.stageShift(par, pos, dir);
          double p_prop = calcShiftProb(par, pos, dir);
          double R = exp(-dE) * props.p_shift * p_prop / (props.p_shift * p_prop + props.p_optshift);
          if (R > rng.uniform()) {
            E.acceptChanges();
            pGrid.shift(idx, pos, dir);
            stats.incNa('o');
          }
          else {
            E.clearChanges();
          }
          
          lock->unlock(pos0);
        }
        
        
        void MHSampler::connect()       // TODO Current implementation does not prevent loops.
        {
          //TRACE;
          stats.incN('c');
          
          unsigned int idx;
          Particle* par;
          do {
            par = pGrid.getRandom(idx);
            if (par == NULL)
              return;
          } while (! lock->lockIfNotLocked(par->getPosition()));
          Point_t pos0 = par->getPosition();
          int alpha0 = (rng.uniform() < 0.5) ? -1 : 1;
          ParticleEnd pe0;
          pe0.par = par;
          pe0.alpha = alpha0;
          
          ParticleEnd pe2;
          pe2.par = NULL;
          double dE = E.stageConnect(pe0, pe2);
          double R = exp(-dE);
          if (R > rng.uniform()) {
            E.acceptChanges();
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
            E.clearChanges();
          }
          
          lock->unlock(pos0);
        }
        
        
        
        
        Point_t MHSampler::getRandPosInMask()
        {
          Point_t p;
          do {
            p[0] = rng.uniform() * (dims[0]-1);
            p[1] = rng.uniform() * (dims[1]-1);
            p[2] = rng.uniform() * (dims[2]-1);
          } while (!inMask(p));           // FIXME Schrijf dit expliciet uit ifv random integer initialisatie,
          return T.voxel2scanner(p);      // voeg hier dan nog random ruis aan toe. Dan kan je inMask terug ifv
        }                                 // scanner positie schrijven.
        
        
        bool MHSampler::inMask(const Point_t p) const
        {
//          if ((p[0] <= -0.5) || (p[0] >= dims[0]-0.5) || 
//              (p[1] <= -0.5) || (p[1] >= dims[1]-0.5) ||
//              (p[2] <= -0.5) || (p[2] >= dims[2]-0.5))
          if ((p[0] <= 0.0) || (p[0] >= dims[0]-1.0) || 
              (p[1] <= 0.0) || (p[1] >= dims[1]-1.0) ||
              (p[2] <= 0.0) || (p[2] >= dims[2]-1.0))
            return false;
          if (mask) {
            (*mask)[0] = Math::round(p[0]);
            (*mask)[1] = Math::round(p[1]);
            (*mask)[2] = Math::round(p[2]);
            return mask->value();
          }
          return true;
        }
        
        
        Point_t MHSampler::getRandDir()
        {
          Point_t dir = Point_t(rng.normal(), rng.normal(), rng.normal());
          dir.normalise();
          return dir;
        }
        
        
        void MHSampler::moveRandom(const Particle *par, Point_t &pos, Point_t &dir)
        {
          pos = par->getPosition() + Point_t(rng.normal(sigpos), rng.normal(sigpos), rng.normal(sigpos));
          dir = par->getDirection() + Point_t(rng.normal(sigdir), rng.normal(sigdir), rng.normal(sigdir));
          dir.normalise();
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
            dir.normalise();
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

