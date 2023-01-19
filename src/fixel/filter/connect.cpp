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


#include "fixel/filter/connect.h"

#include <stack>

#include "types.h"
#include "algo/loop.h"
#include "fixel/helpers.h"
#include "misc/bitset.h"

namespace MR
{
  namespace Fixel
  {
    namespace Filter
    {



      void Connect::operator() (Image<float>& input, Image<float>& output) const
      {
        Fixel::check_data_file (input);
        Fixel::check_data_file (output);

        check_dimensions (input, output);

        if (size_t (input.size(0)) != matrix.size())
          throw Exception ("Size of fixel data file \"" + input.name() + "\" (" + str(input.size(0)) +
                           ") does not match fixel connectivity matrix (" + str(matrix.size()) + ")");

        if (input.ndim() > 1 && input.size(1) != 1)
          throw Exception ("Fixel connected component filter cannot be applied to fixel data files with more than one parameter");

        // Need to provide a manual implementation of the connected-component filter here;
        //   don't want to re-format the data to use an existing algorithm as that could result in
        //   duplication of memory requirements

        for (auto l = Loop(0) (output); l; ++l)
          output.value() = 0.0f;

        BitSet processed (input.size (0));
        using IndexAndSize = std::pair<size_t, size_t>;
        vector<IndexAndSize> cluster_sizes;
        for (index_type seed = 0; seed != input.size (0); ++seed) {
          if (!processed[seed]) {
            input.index (0) = seed;
            if (input.value() >= value_threshold) {
              processed[seed] = true;
              size_t cluster_size = 1;
              std::stack<index_type, vector<index_type>> to_expand;
              to_expand.push (seed);
              const size_t cluster_index = cluster_sizes.size()+1;
              while (to_expand.size()) {
                const index_type index = to_expand.top();
                to_expand.pop();
                output.index(0) = index;
                output.value() = cluster_index;
                const auto connections = matrix[index];
                for (const auto& c : connections) {
                  input.index (0) = c.index();
                  if (!processed[c.index()] && c.value() >= connectivity_threshold && input.value() >= value_threshold) {
                    ++cluster_size;
                    processed[c.index()] = true;
                    to_expand.push (c.index());
                  } // Finish adding the other fixel in this connection to the list of fixels to expand the cluster
                } // Finish looping over fixels connected to "index"
              } // Finish waiting for the list of fixels to expand the cluster to has been exhausted
              cluster_sizes.push_back (std::make_pair (cluster_index, cluster_size));
            } // Finish testing to see if seed fixel is above the value threshold
          } // Finish thesting whether the seed fixel has already been processed
        } // Finish looping over all plausible cluster seed fixels

        // Now we just want to change the values in the output image so that the cluster
        //   indices are in order of cluster size

        // Sort clusters by size (largest first)
        std::sort (cluster_sizes.begin(),
                   cluster_sizes.end(),
                   [] (const IndexAndSize& a, const IndexAndSize& b) -> bool
                   {
                     return (a.second > b.second);
                   });
        // For a given value in the output image (as determined by the order in which the
        //   clusters were segmented), what should the new value be, such that the clusters
        //   are indexed sequentially according to size, with 1 being the biggest cluster?
        vector<size_t> index_remapper (cluster_sizes.size() + 1);
        index_remapper[0] = 0;
        for (size_t index = 0; index != cluster_sizes.size(); ++index)
          index_remapper[cluster_sizes[index].first] = index + 1;

        for (auto l = Loop(0) (output); l; ++l)
          output.value() = index_remapper[output.value()];
      }



    }
  }
}

