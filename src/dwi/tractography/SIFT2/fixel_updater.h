/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_sift2_fixel_updater_h__
#define __dwi_tractography_sift2_fixel_updater_h__


#include "types.h"

#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT2/types.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace SIFT2 {


      class TckFactor;



      class FixelUpdaterBase
      {

        public:
          using value_type = SIFT::value_type;

          FixelUpdaterBase (TckFactor&);

          virtual bool operator() (const SIFT::TrackIndexRange& range) = 0;

        protected:
          TckFactor& master;

          // Each thread needs a local copy of these
          vector<value_type> local_sum_contributions;
      };



      class FixelUpdaterAbsolute : public FixelUpdaterBase
      {

        public:
          FixelUpdaterAbsolute (TckFactor&);
          ~FixelUpdaterAbsolute();

          bool operator() (const SIFT::TrackIndexRange& range) final;

        private:
          vector<value_type> local_sum_coeffs;
          vector<SIFT::track_t> local_counts;

      };



      class FixelUpdaterDifferential : public FixelUpdaterBase
      {
        public:
          FixelUpdaterDifferential (TckFactor&);
          ~FixelUpdaterDifferential();

          bool operator() (const SIFT::TrackIndexRange& range) final;
      };



      // TODO SFINAE a FixelUpdaterSelector that can yield the type based on template operation_mode_t
      // template <class T>
      // struct FixelUpdaterSelector<T, std::enable_if<std::equal_to<operation_mode_t::ABSOLUTE, T>::type>> {
      //   using type = FixelUpdaterAbsolute;
      // }
      // template <class T>
      // struct FixelUpdaterSelector<T, std::enable_if<std::equal_to<operation_mode_t::DIFFERENTIAL, T>::type>> {
      //   using type = FixelUpdaterDifferential;
      // }
      template <operation_mode_t Mode>
      struct FixelUpdaterSelector;
      template <>
      struct FixelUpdaterSelector<operation_mode_t::ABSOLUTE> {
        using type = FixelUpdaterAbsolute;
      };
      template <>
      struct FixelUpdaterSelector<operation_mode_t::DIFFERENTIAL> {
        using type = FixelUpdaterDifferential;
      };




      }
    }
  }
}



#endif

