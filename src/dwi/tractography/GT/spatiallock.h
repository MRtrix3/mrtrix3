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

#ifndef __gt_spatiallock_h__
#define __gt_spatiallock_h__

#include <Eigen/Dense>
#include <mutex>
#include <set>
#include <algorithm>


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace GT {
        
        template <typename T>
        struct vec_compare
        { NOMEMALIGN
            bool operator()(const T& v, const T& w) const
            {
              for (int i = 0; i < v.size(); ++i) {
                if (v[i] < w[i]) return true;
                if (v[i] > w[i]) return false;
              }
              return false;
            }
        };

        /**
         * @brief SpatialLock manages a mutex lock on n positions in 3D space.
         */
        template <typename T = float >
        class SpatialLock
        { MEMALIGN(SpatialLock)
        public:
          typedef T value_type;
          typedef Eigen::Matrix<value_type, 3, 1> point_type;
          
          SpatialLock() : _tx(0), _ty(0), _tz(0) { }
          SpatialLock(const value_type t) : _tx(t), _ty(t), _tz(t) { }
          SpatialLock(const value_type tx, const value_type ty, const value_type tz) : _tx(tx), _ty(ty), _tz(tz) { }
          
          ~SpatialLock() {
            lockcentres.clear();
          }
          
          void setThreshold(const value_type t) {
            _tx = _ty = _tz = t;
          }
          
          void setThreshold(const value_type tx, const value_type ty, const value_type tz) {
            _tx = tx;
            _ty = ty;
            _tz = tz;
          }
          
          bool lockIfNotLocked(const point_type& pos) {
            std::lock_guard<std::mutex> lock (mutex);
            point_type d;
            for (auto& x : lockcentres) {
              d = x - pos;
              if ((std::fabs(d[0]) < _tx) && (std::fabs(d[1]) < _ty) && (std::fabs(d[2]) < _tz))
                return false;
            }
            lockcentres.insert(pos);
            return true;
          }
          
          void unlock(const point_type& pos) {
            std::lock_guard<std::mutex> lock (mutex);
            lockcentres.erase(pos);
          }
          
        protected:
          std::mutex mutex;
          std::set<point_type, vec_compare<point_type> > lockcentres;
          value_type _tx, _ty, _tz;
          
        };

      }
    }
  }
}

#endif // __gt_spatiallock_h__
