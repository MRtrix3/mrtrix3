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

#ifndef __fixel_correspondence_mapping_h__
#define __fixel_correspondence_mapping_h__

#include <string>

#include "types.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {




      // TODO Do we want to store a fractional weight for each projection as part of the mapping,
      //   both in the internal RAM representation and on the filesystem?
      //
      // TODO Change from a vector< vector<uint32_t> > (vector of fixel indices per fixel)
      //   to something more faithful to what will be stored on disk (and have less overhead):
      //   Forward:
      //     Eigen::Array<index_type, Nt, 2, RowMajor>
      //     Eigen::Array<index_type, C, 1>
      //     Eigen::Array<float, C, 1>  (maybe)
      //   Note that this is not something that can be trivially dynamically expanded...
      //
      // TODO Consider automatically computing for every source fixel the number of template fixels
      //   to which it is attributed
      //   (in the case of explicitly storing attribution weights,
      //   this would need to be a count of non-zero attribution weights and a sum of such)
      //   This would not need to be stored on disk,
      //   but would be used for various calculations
      //
      // TODO Consider (both here and potentially for fixel dataset also) a pair of classes where
      //   one uses a data representation that clearly comes straight from disk and is read-only,
      //   and one is intended to be dynamically resizable,
      //   but they operate using the same interface (perhaps using CRTP)
      // In this way the interface for fixelcorrespondence and fixel2fixel could look identical,
      //   even though the underlying data structures would be different
      class Mapping
      {
        public:
          Mapping (const uint32_t source_fixels, const uint32_t target_fixels);
          Mapping (const std::string& directory);

          void load (const std::string& directory, const bool import_inverse = false);

          // Save to new format:
          // - Create directory based on user input
          // - index_forward.mif: Nt x 2 x 1, containing count and offset for each target fixel
          // - fixels_forward.mif: C x 1 x 1, containing source fixel indices to pull into target fixels
          // - index_inverse.mif: Ns x 2 x 1, containing count and offset for each source fixel in the inverse mapping
          // - fixels_inverse.mif: C x 1 x 1, containing target fixel indices to pull into source fixels
          void save (const std::string& directory) const;

          const vector<uint32_t>& operator[] (const size_t index) const { return M[index]; }

          class Value
          {
            public:
              Value (vector<vector<uint32_t>>& M, const size_t index) :
                  M (M),
                  index (index)
              {
                assert (index < M.size());
              }
              const vector<uint32_t>& operator() () const { return M[index]; }
              const vector<uint32_t>& operator= (const vector<uint32_t>& data) { M[index] = data; return M[index]; }
              uint32_t operator[] (const size_t i) const { assert (i < M[index].size()); return M[index][i]; }
            private:
              vector<vector<uint32_t>>& M;
              const size_t index;
          };
          Value operator[] (const size_t index) { return Value (M, index); }

          size_t size() const { return M.size(); }

          // TODO Modify to yield a complete instance of the Mapping class?
          vector< vector<uint32_t> > inverse() const;

        private:
          uint32_t source_fixels, target_fixels;
          vector< vector<uint32_t> > M;


          void save (const std::string& directory, const bool export_inverse) const;

      };



    }
  }
}

#endif
