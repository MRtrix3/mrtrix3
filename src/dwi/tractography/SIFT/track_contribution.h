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




#ifndef __dwi_tractography_sift_track_contribution_h__
#define __dwi_tractography_sift_track_contribution_h__


#include <stdint.h>

#include "header.h"
#include "min_mem_array.h"

#include "math/math.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {




      class Track_fixel_contribution
      {
        public:
          Track_fixel_contribution (const uint32_t fixel_index, const float length)
          {
            const uint32_t length_as_int = std::min (uint32_t(255), uint32_t(std::round (scale_to_storage * length)));
            data = (fixel_index & 0x00FFFFFF) | (length_as_int << 24);
          }

          Track_fixel_contribution() :
            data (0) { }

          uint32_t get_fixel_index() const { return (data & 0x00FFFFFF); }
          float    get_length()      const { return (uint32_t((data & 0xFF000000) >> 24) * scale_from_storage); }


          bool add (const float length)
          {
            // Allow summing of multiple contributions to a fixel, UNLESS it would cause truncation, in which
            //   case keep them separate
            const uint32_t increment = std::round (scale_to_storage * length);
            const uint32_t existing = (data & 0xFF000000) >> 24;
            if (existing + increment > 255)
              return false;
            data = (data & 0x00FFFFFF) | ((existing + increment) << 24);
            return true;
          }


          static void set_scaling (const Header& H)
          {
            const float max_length = std::sqrt (Math::pow2 (H.spacing(0)) + Math::pow2 (H.spacing(1)) + Math::pow2 (H.spacing(2)));
            // TODO Newer mapping performs chordal approximation of length
            // Should technically take this into account when setting scaling
            scale_to_storage = 255.0 / max_length;
            scale_from_storage = max_length / 255.0;
            min_length_for_storage = 0.5 / scale_to_storage;
          }


          // Minimum length that will be non-zero once converted to an integer for word-sharing storage
          static float min() { return min_length_for_storage; }


        private:
          uint32_t data;

          static float scale_to_storage, scale_from_storage, min_length_for_storage;

      };




/*
// This is a 'safe' version of Track_fixel_contribution that does not use byte-sharing, but requires double the RAM
// Simply comment the class above and un-comment this one to use
class Track_fixel_contribution
{
  public:
    Track_fixel_contribution (const uint32_t fixel_index, const float length) :
      fixel (fixel_index),
      length (length) { }

    Track_fixel_contribution() :
      fixel (0),
      length (0.0) { }


    bool add (const float length) { value += length; return true; }

    uint32_t get_fixel_index() const { return fixel; }
    float    get_length()      const { return length; }


    static void set_scaling (const Image::Info& in) { min_length_for_storage = 0.0; }
    static float min() { return min_length_for_storage; }


  private:
    uint32_t fixel;
    float length;

    static float scale_to_storage, scale_from_storage, min_length_for_storage;

};
*/



      class TrackContribution : public Min_mem_array<Track_fixel_contribution>
      {

        public:
        TrackContribution (const std::vector<Track_fixel_contribution>& in, const float c, const float l) :
            Min_mem_array<Track_fixel_contribution> (in),
            total_contribution (c),
            total_length       (l) { }

        TrackContribution () :
            Min_mem_array<Track_fixel_contribution> (),
            total_contribution (0.0),
            total_length       (0.0) { }

        ~TrackContribution() { }

        float get_total_contribution() const { return total_contribution; }
        float get_total_length      () const { return total_length; }

        private:
          const float total_contribution, total_length;

      };




      }
    }
  }
}


#endif


