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

#define ITER_BIGSTEP 10000
#define FRAC_BURNIN 10
#define FRAC_PHASEOUT 10

#include <iostream>
#include <vector>
#include <mutex>

#include "math/matrix.h"
#include "math/vector.h"
#include "progressbar.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        const double M_4PI = 4 * M_PI;
        const double M_sqrt4PI = std::sqrt(M_4PI);
        
        
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
          
          double beta;
          double ppot;
          
          Math::Matrix<float> resp_WM;
          std::vector< Math::Vector<float> > resp_ISO;
          
        };
        
        
        
        class Stats
        {
        public:
          
          Stats(const double T0, const double T1, const float maxiter) 
            : Text(T1), Tint(T0), EextTot(0.0), EintTot(0.0), n_iter(0), n_max(maxiter), 
              progress("running MH sampler", n_max/ITER_BIGSTEP)
          {
            for (int k = 0; k != 5; k++)
              n_gen[k] = n_acc[k] = 0;
            alpha = std::pow(T1/T0, double(ITER_BIGSTEP)/double(n_max - n_max/FRAC_BURNIN - n_max/FRAC_PHASEOUT));
          }
          
          ~Stats() {
            out.close();
          }
          
          
          void open_stream(const std::string& file) {
            out.close();
            out.open(file.c_str(), std::ofstream::out);
          }
          
          
          bool next() {
            std::lock_guard<std::mutex> lock (mutex);
            ++n_iter;
            if (n_iter % ITER_BIGSTEP == 0) {
              if ((n_iter >= n_max/FRAC_BURNIN) && (n_iter < n_max - n_max/FRAC_PHASEOUT))
                Tint *= alpha;
              progress++;
              out << *this << std::endl;
            }
            return (n_iter < n_max);
          }
          
          
          // getters and setters ----------------------------------------------
          
          double getText() const {
            return Text;
          }
          
          double getTint() const {
            return Tint;
          }
          
          void setTint(double temp) {
            std::lock_guard<std::mutex> lock (mutex);
            Tint = temp;
          }
          
          
          double getEextTotal() const {
            return EextTot;
          }
          
          double getEintTotal() const {
            return EintTot;
          }
          
          void incEextTotal(double d) {
            std::lock_guard<std::mutex> lock (mutex);
            EextTot += d;
          }
          
          void incEintTotal(double d) {
            std::lock_guard<std::mutex> lock (mutex);
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
            std::lock_guard<std::mutex> lock (mutex);
            switch (p) {
              case 'b': n_gen[0] += i; break;
              case 'd': n_gen[1] += i; break;
              case 'r': n_gen[2] += i; break;
              case 'o': n_gen[3] += i; break;
              case 'c': n_gen[4] += i; break;
              default: return;
            }
          }
          
          void incNa(const char p, unsigned int i = 1) {
            std::lock_guard<std::mutex> lock (mutex);
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
          std::mutex mutex;
          double Text, Tint;
          double EextTot, EintTot;
          double alpha;

          unsigned long n_gen[5];
          unsigned long n_acc[5];
          unsigned long n_iter;
          const unsigned long n_max;
          
          ProgressBar progress;
          std::ofstream out;
          
        };
        
        
        
        
        

      }
    }
  }
}

#endif // __gt_gt_h__
