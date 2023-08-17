/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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

#ifndef __fixel_correspondence_matcher_h__
#define __fixel_correspondence_matcher_h__

#include <mutex>
#include <string>

#include "image.h"

#include "fixel/correspondence/correspondence.h"
#include "fixel/correspondence/mapping.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {



      namespace Algorithms
      {
        class Base;
      }




      // Functor is safe to copy-construct for multi-threading:
      //   correspondence data are stored in a std::shared_ptr<>
      class Matcher
      {

        public:

          Matcher (const std::string& source_file,
                   const std::string& target_file,
                   std::shared_ptr<Algorithms::Base>& algorithm);



          // Input is just a dummy iterator that provides the location
          void operator() (Image<uint32_t>& voxel);


          // Use this to get a template image in order to loop over voxels
          Image<uint32_t> get_template() const { return Image<uint32_t> (target_index); }

          const MR::Fixel::Correspondence::Mapping& get_mapping() const { assert (mapping); return *mapping; }

          size_t num_source_fixels() const { return source_data.size(0); }
          size_t num_target_fixels() const { return target_data.size(0); }

          void export_remapped (const std::string& dirname);


        private:
          std::shared_ptr<Algorithms::Base> algorithm;

          Image<uint32_t> source_index, target_index;
          Image<float> source_directions, target_directions, remapped_directions;
          Image<float> source_data, target_data, remapped_data;

          std::shared_ptr<MR::Fixel::Correspondence::Mapping> mapping;

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
          std::shared_ptr<std::mutex> mutex;
          static uint64_t max_computed_combinations;
#endif

      };

#ifdef FIXELCORRESPONDENCE_TEST_COMBINATORICS
      uint64_t Matcher::max_computed_combinations = 0;
#endif




    }
  }
}

#endif
