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

#include "thread/mutex.h"
#include "image/info.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "math/SH.h"
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
          Stats() { }
          
          double getTemperature() const {
            return t;
          }
          
          void setTemperature(double temp) {
            Thread::Mutex::Lock lock (mutex);
            t = temp;
          }
          
          void setStats(unsigned int s[]) {
            Thread::Mutex::Lock lock (mutex);
            memcpy(stats, s, 10*sizeof(int));
          }

        protected:
          Thread::Mutex mutex;
          double t;                 // Temperature
          unsigned int stats[10];   // Stats
        };
        

//        class Shared
//        {
//        public:
//          Shared(const std::string& dwi, Properties& p)
//            : props(p), stats(),
//              dwimage(dwi, Image::Stride::contiguous_along_axis(3))
//          {
//            mask = NULL;    // TODO
//            Image::Info info (dwimage);
//            info.dim(3) = Math::SH::NforL(props.Lmax);
//            TOD = Image::BufferScratch(info);
//            info.dim(3) = 2;    // TODO
//            fiso = Image::BufferScratch(info);
//            info.ndim() = 3;
//            Eext = Image::BufferScratch(info);
//          }
          
//          Properties props;
//          Stats stats;
          
//          Image::BufferPreload<float> dwimage;
//          Image::BufferPreload<float>* mask;
          
//          Image::BufferScratch<float> TOD;
//          Image::BufferScratch<float> fiso;
//          Image::BufferScratch<float> Eext;
          
//        };
        

      }
    }
  }
}

#endif // __gt_gt_h__
