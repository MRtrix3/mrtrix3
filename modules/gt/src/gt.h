/*
    Copyright 2013 KU Leuven, Dept. Electrical Engineering, Medical Image Computing
    Herestraat 49 box 7003, 3000 Leuven, Belgium
    
    Written by Daan Christiaens, 19/03/14.
    
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

#ifndef __gt_gt_h__
#define __gt_gt_h__

#include <iostream>

#include "thread/mutex.h"
#include "math/matrix.h"
#include "math/vector.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        
        struct Properties
        {
          float p_birth;
          float p_death;
          float p_shift;
          float p_optshift;
          float p_connect;
          
          double density;
          double weight;
          int Lmax;
          
          double lam_ext;
          double lam_int;
          
          Math::Matrix<float> resp_WM;
          Math::Vector<float>* resp_CSF;
          Math::Vector<float>* resp_GM;
        };
        
        
        
        class Stats
        {
        public:
          
          Stats(double T0, double T1) 
            : Text(T1), Tint(T0), EextTot(0.0), EintTot(0.0), n_total(0) 
          {
            for (int k = 0; k != 5; k++)
              n_gen[k] = n_acc[k] = 0;
          }
          
          
          // getters and setters ----------------------------------------------
          
          double getText() const {
            return Text;
          }
          
          double getTint() const {
            return Tint;
          }
          
          void setTint(double temp) {
            Thread::Mutex::Lock lock (mutex);
            Tint = temp;
          }
          
          
          double getEextTotal() const {
            return EextTot;
          }
          
          double getEintTotal() const {
            return EintTot;
          }
          
          void incEextTotal(double d) {
            Thread::Mutex::Lock lock (mutex);
            EextTot += d;
          }
          
          void incEintTotal(double d) {
            Thread::Mutex::Lock lock (mutex);
            EintTot += d;
          }          
          
          
          unsigned int getN(const char p) const {
            switch (p) {
              case 'b': return n_gen[0];
              case 'd': return n_gen[1];
              case 'r': return n_gen[2];
              case 'o': return n_gen[3];
              case 'c': return n_gen[4];
              default: return 0;
            }
          }
          
          unsigned int getNa(const char p) const {
            switch (p) {
              case 'b': return n_acc[0];
              case 'd': return n_acc[1];
              case 'r': return n_acc[2];
              case 'o': return n_acc[3];
              case 'c': return n_acc[4];
              default: return 0;
            }
          }
          
          void incN(const char p, unsigned int i = 1) {
            Thread::Mutex::Lock lock (mutex);
            switch (p) {
              case 'b': n_gen[0] += i; break;
              case 'd': n_gen[1] += i; break;
              case 'r': n_gen[2] += i; break;
              case 'o': n_gen[3] += i; break;
              case 'c': n_gen[4] += i; break;
              default: return;
            }
            n_total += i;
          }
          
          void incNa(const char p, unsigned int i = 1) {
            Thread::Mutex::Lock lock (mutex);
            switch (p) {
              case 'b': n_acc[0] += i; break;
              case 'd': n_acc[1] += i; break;
              case 'r': n_acc[2] += i; break;
              case 'o': n_acc[3] += i; break;
              case 'c': n_acc[4] += i; break;
            }
          }
          
          double getAcceptanceRate(const char p) const {
            switch (p) {
              case 'b': return double(n_acc[0]) / double(n_gen[0]);
              case 'd': return double(n_acc[1]) / double(n_gen[1]);
              case 'r': return double(n_acc[2]) / double(n_gen[2]);
              case 'o': return double(n_acc[3]) / double(n_gen[3]);
              case 'c': return double(n_acc[4]) / double(n_gen[4]);
              default: return 0.0;
            }
          }
          
          
          friend std::ostream& operator<< (std::ostream& o, Stats const& stats);
          

        protected:
          Thread::Mutex mutex;
          double Text, Tint;
          double EextTot, EintTot;

          unsigned int n_gen[5];
          unsigned int n_acc[5];
          unsigned int n_total;
          
          
        };
        
        
        
        
        

      }
    }
  }
}

#endif // __gt_gt_h__
