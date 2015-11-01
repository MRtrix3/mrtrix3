/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

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

#ifndef __math_rng_h__
#define __math_rng_h__

#include <random>
#ifdef MRTRIX_WINDOWS
#include <sys/time.h>
#endif

#include <mutex>

#include "mrtrix.h"

namespace MR
{
  namespace Math
  {

    //! random number generator
    /*! this is a thin wrapper around the standard C++11 std::mt19937 random
     * number generator. It can be used in combination with the standard C++11
     * distributions. It differs from the standard in its constructors: the
     * default constructor will seed using std::random_device, unless a seed
     * has been expicitly passed using the MRTRIX_RNG_SEED environment
     * variable. The copy constructor will seed itself using 1 + the last seed
     * used - this ensures the seeds are unique across instances in
     * multi-threading. */
    // TODO consider switch to std::mt19937_64
    class RNG : public std::mt19937
    {
      public:
        RNG () : std::mt19937 (get_seed()) { }
        RNG (std::mt19937::result_type seed) : std::mt19937 (seed) { }
        RNG (const RNG&) : std::mt19937 (get_seed()) { }
        template <typename ValueType> class Uniform; 
        template <typename ValueType> class Normal; 

        static std::mt19937::result_type get_seed () {
          static std::mutex mutex;
          std::lock_guard<std::mutex> lock (mutex);
          static std::mt19937::result_type current_seed = get_seed_private();
          return current_seed++;
        }

      private:
        static std::mt19937::result_type get_seed_private () {
          const char* from_env = getenv ("MRTRIX_RNG_SEED");
          if (from_env) 
            return to<std::mt19937::result_type> (from_env);

#ifdef MRTRIX_WINDOWS
          struct timeval tv;
          gettimeofday (&tv, nullptr);
          return tv.tv_sec ^ tv.tv_usec;
#else 
          // TODO check whether this does in fact work on Windows...
          std::random_device rd;
          return rd();
#endif
        }


    };


    template <typename ValueType>
      class RNG::Uniform {
        public:
          RNG rng;
          typedef ValueType result_type;
          // static const ValueType max() const { return static_cast<ValueType>(1.0); }
          // static const ValueType min() const { return static_cast<ValueType>(0.0); }
          std::uniform_real_distribution<ValueType> dist;
          ValueType operator() () { return dist (rng); }
      };

    template <typename ValueType>
      class RNG::Normal {
        public:
          RNG rng;
          typedef ValueType result_type;
          std::normal_distribution<ValueType> dist;
          ValueType operator() () { return dist (rng); }
      };

  }
}

#endif

