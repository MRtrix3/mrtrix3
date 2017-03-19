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


#ifndef __dwi_tractography_seeding_base_h__
#define __dwi_tractography_seeding_base_h__


#include "image.h"
#include "file/path.h"
#include "algo/threaded_loop.h"



// These constants set how many times a tracking algorithm should attempt to propagate
//   from a given seed point, based on the mechanism used to provide the seed point
//
// Update 12/03/2017: By default a greater number of attempts will be made to
//   find an appropriate direction in which to initiate tracking from all
//   seeding mechanisms
//
// Mechanisms that provide random seed locations
#define MAX_TRACKING_SEED_ATTEMPTS_RANDOM 1000
//
// Dynamic seeding also provides the mean direction of the fixel, so only a small number of
//   attempts should be required to find a direction above the FOD amplitude threshold;
//   this will however depend on this threshold as well as the angular threshold
#define MAX_TRACKING_SEED_ATTEMPTS_DYNAMIC 1000
//
// GM-WM interface seeding incurs a decent overhead when generating the seed points;
//   therefore want to make maximal use of each seed point generated, bearing in mind that
//   the FOD amplitudes may be small there.
#define MAX_TRACKING_SEED_ATTEMPTS_GMWMI 1000
//
// Mechanisms that provide a fixed number of seed points; hence the maximum effort should
//   be made to find an appropriate tracking direction from every seed point provided
#define MAX_TRACKING_SEED_ATTEMPTS_FIXED 1000




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      template <class ImageType>
      uint32_t get_count (ImageType& data)
      {
        std::atomic<uint32_t> count (0);
        ThreadedLoop (data).run ([&] (ImageType& v) { if (v.value()) count.fetch_add (1, std::memory_order_relaxed); }, data);
        return count;
      }


      template <class ImageType>
      float get_volume (ImageType& data)
      {
        std::atomic<default_type> volume (0.0);
        ThreadedLoop (data).run (
            [&] (decltype(data)& v) {
              const typename ImageType::value_type value = v.value();
              if (value) {
                default_type current = volume.load (std::memory_order_relaxed);
                default_type target;
                do {
                  target = current + value;
                } while (!volume.compare_exchange_weak (current, target, std::memory_order_relaxed));
              }
            }, data);
        return volume;
      }




      // Common interface for providing streamline seeds
      class Base { MEMALIGN(Base)

        public:
          Base (const std::string& in, const std::string& desc, const size_t attempts) :
            volume (0.0),
            count (0),
            type (desc),
            name (Path::exists (in) ? Path::basename (in) : in),
            max_attempts (attempts) { }

          virtual ~Base() { }

          default_type vol() const { return volume; }
          uint32_t num() const { return count; }
          bool is_finite() const { return count; }
          const std::string& get_type() const { return type; }
          const std::string& get_name() const { return name; }
          size_t get_max_attempts() const { return max_attempts; }

          virtual bool get_seed (Eigen::Vector3f&) const = 0;
          virtual bool get_seed (Eigen::Vector3f& p, Eigen::Vector3f&) { return get_seed (p); }

          friend inline std::ostream& operator<< (std::ostream& stream, const Base& B) {
            stream << B.name;
            return (stream);
          }


        protected:
          // Finite seeds are defined by the number of seeds; non-limited are defined by volume
          float volume;
          uint32_t count;
          mutable std::mutex mutex;
          const std::string type; // Text describing the type of seed this is

        private:
          const std::string name; // Could be an image path, or spherical coordinates
          const size_t max_attempts; // Maximum number of times the tracking algorithm should attempt to start from each provided seed point

      };





      }
    }
  }
}

#endif

