/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __gt_energy_h__
#define __gt_energy_h__

#include "dwi/tractography/GT/particle.h"
#include "dwi/tractography/GT/gt.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        class EnergyComputer
        { MEMALIGN(EnergyComputer)
        public:
          EnergyComputer(Stats& s) : stats(s) { }
          
          virtual ~EnergyComputer() { }
          
          virtual double stageAdd(const Point_t& pos, const Point_t& dir) { return 0.0; }
          
          virtual double stageShift(const Particle* par, const Point_t& pos, const Point_t& dir) { return 0.0; }
          
          virtual double stageRemove(const Particle* par) { return 0.0; }
                    
          virtual double stageConnect(const ParticleEnd& pe1, ParticleEnd& pe2) { return 0.0; }
          
          virtual void acceptChanges() { }
          
          virtual void clearChanges() { }
          
          virtual EnergyComputer* clone() const = 0;
          
        protected:
          Stats& stats;
          
          
        };
        
        
        class EnergySumComputer : public EnergyComputer
        { MEMALIGN(EnergySumComputer)
        public:
          
          // Copy-constructable via clone method
          
          EnergySumComputer(Stats& stat, EnergyComputer* e1, double lam1, EnergyComputer* e2, double lam2)
            : EnergyComputer(stat), _e1(e1), _e2(e2), l1(lam1), l2(lam2)
          {  }
          
          ~EnergySumComputer()
          {
            delete _e1;
            delete _e2;
          }
          
          double stageAdd(const Point_t& pos, const Point_t& dir) 
          { 
            return l1 * _e1->stageAdd(pos, dir) + l2 * _e2->stageAdd(pos, dir);
          }
          
          double stageShift(const Particle *par, const Point_t &pos, const Point_t &dir)
          {
            return l1 * _e1->stageShift(par, pos, dir) + l2 * _e2->stageShift(par, pos, dir);
          }
          
          double stageRemove(const Particle *par)
          {
            return l1 * _e1->stageRemove(par) + l2 * _e2->stageRemove(par);
          }
                    
          double stageConnect(const ParticleEnd& pe1, ParticleEnd& pe2)
          {
            double eint = _e1->stageConnect(pe1, pe2);   // Warning: not symmetric due to output variable!
            double eext = _e2->stageConnect(pe1, pe2);
            return l1 * eint + l2 * eext;
          }
          
          void acceptChanges()
          {
            _e1->acceptChanges();
            _e2->acceptChanges();
          }
          
          void clearChanges()
          {
            _e1->clearChanges();
            _e2->clearChanges();
          }
          
          EnergyComputer* clone() const { return new EnergySumComputer(stats, _e1->clone(), l1, _e2->clone(), l2); }
          
        protected:
          EnergyComputer* _e1;
          EnergyComputer* _e2;
          double l1, l2;
          
          
        };
        


      }
    }
  }
}

#endif // __gt_energy_h__
