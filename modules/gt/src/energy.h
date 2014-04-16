/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 19/12/13.
    
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

#ifndef __gt_energy_h__
#define __gt_energy_h__

#include <vector>

#include "particle.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        class EnergyComputer
        {
        public:
          
          virtual double stageAdd(const Point_t& pos, const Point_t& dir) { return 0.0; }
          
          virtual double stageShift(const Particle* par, const Point_t& pos, const Point_t& dir) { return 0.0; }
          
          virtual double stageRemove(const Particle* par) { return 0.0; }
          
//          virtual double stageConnect(const Particle* par, const int ep1, Particle* par2, int& ep2) { return 0.0; }
          
          virtual double stageConnect(const ParticleEnd& pe1, ParticleEnd& pe2) { return 0.0; }
          
          virtual void acceptChanges() { }
          
          virtual void clearChanges() { }
          
          
        };
        
        
        class EnergySumComputer : public EnergyComputer
        {
        public:
          
          // FIXME: copy-constructable ?
          
          EnergySumComputer(EnergyComputer& e1, double lam1, EnergyComputer& e2, double lam2)
            : EnergyComputer(), _e1(e1), _e2(e2), l1(lam1), l2(lam2)
          { }
          
          double stageAdd(const Point_t& pos, const Point_t& dir) 
          { 
            return l1 * _e1.stageAdd(pos, dir) + l2 * _e2.stageAdd(pos, dir);
          }
          
          double stageShift(const Particle *par, const Point_t &pos, const Point_t &dir)
          {
            return l1 * _e1.stageShift(par, pos, dir) + l2 * _e2.stageShift(par, pos, dir);
          }
          
          double stageRemove(const Particle *par)
          {
            return l1 * _e1.stageRemove(par) + l2 * _e2.stageRemove(par);
          }
          
//          double stageConnect(const Particle *par, const int ep1, Particle *par2, int &ep2)
//          {
//            Particle* tmp = par2;
//            VAR(tmp);
//            double eint = _e1.stageConnect(par, ep1, tmp, ep2);
//            VAR(tmp);
//            par2 = tmp;
//            double eext = _e2.stageConnect(par, ep1, par2, ep2);
//            VAR(par2);
//            return l1 * eint + l2 * eext;
//            // Warning: not symmetric due to output variable!
//          }
          
          double stageConnect(const ParticleEnd& pe1, ParticleEnd& pe2)
          {
            double eint = _e1.stageConnect(pe1, pe2);
            double eext = _e2.stageConnect(pe1, pe2);
            return l1 * eint + l2 * eext;
          }
          
          void acceptChanges()
          {
            _e1.acceptChanges();
            _e2.acceptChanges();
          }
          
          void clearChanges()
          {
            _e1.clearChanges();
            _e2.clearChanges();
          }
          
        protected:
          EnergyComputer& _e1;
          EnergyComputer& _e2;
          double l1, l2;
          
          
        };
        


      }
    }
  }
}

#endif // __gt_energy_h__
