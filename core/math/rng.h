/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
    { NOMEMALIGN
      public:
        RNG () : std::mt19937 (get_seed()) { }
        RNG (std::mt19937::result_type seed) : std::mt19937 (seed) { }
        RNG (const RNG&) : std::mt19937 (get_seed()) { }
        template <typename ValueType> class Uniform; 
        template <typename ValueType> class Normal; 
        template <typename ValueType> class Integer;

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
      class RNG::Uniform { NOMEMALIGN
        public:
          RNG rng;
          using result_type = ValueType;
          // static const ValueType max() const { return static_cast<ValueType>(1.0); }
          // static const ValueType min() const { return static_cast<ValueType>(0.0); }
          std::uniform_real_distribution<ValueType> dist;
          ValueType operator() () { return dist (rng); }
      };

    template <typename ValueType>
      class RNG::Normal { NOMEMALIGN
        public:
          RNG rng;
          using result_type = ValueType;
          std::normal_distribution<ValueType> dist;
          ValueType operator() () { return dist (rng); }
      };

      template <typename ValueType>
        class RNG::Integer { NOMEMALIGN
          public:
            Integer (const ValueType max) :
                dist (0, max) { }
            RNG rng;
            std::uniform_int_distribution<ValueType> dist;
            ValueType operator() () { return dist (rng); }
        };

  }
}

#endif

