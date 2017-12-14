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


#include "connectome/enhance.h"
#include "connectome/mat2vec.h"

#include "progressbar.h"

#include "misc/bitset.h"


namespace MR {
  namespace Connectome {
    namespace Enhance {



      void PassThrough::operator() (in_column_type in, out_column_type out) const
      {
        out = in;
      }



      void NBS::operator() (in_column_type in, const value_type T, out_column_type out) const
      {
        out.setZero();

        for (ssize_t seed = 0; seed != in.size(); ++seed) {
          if (std::isfinite (in[seed]) && in[seed] >= T && !out[seed]) {

            BitSet visited (in.size());
            visited[seed] = true;
            vector<size_t> to_expand (1, seed);
            size_t cluster_size = 0;

            while (to_expand.size()) {

              const uint32_t index = to_expand.back();
              to_expand.pop_back();
              cluster_size++;

              for (vector<size_t>::const_iterator i = (*adjacency)[index].begin(); i != (*adjacency)[index].end(); ++i) {
                if (!visited[*i] && std::isfinite(in[*i]) && in[*i] >= T) {
                  visited[*i] = true;
                  to_expand.push_back (*i);
                }
              }

            }

            for (ssize_t i = 0; i != in.size(); ++i)
              out[i] += (visited[i] ? 1.0 : 0.0) * cluster_size;

          }
        }
      }



      void NBS::initialise (const node_t num_nodes)
      {
        const Mat2Vec mat2vec (num_nodes);
        const size_t num_edges = mat2vec.vec_size();
        ProgressBar progress ("Pre-computing statistical correlation matrix...", num_edges);
        adjacency.reset (new vector< vector<size_t> > (num_edges, vector<size_t>()));
        for (node_t row = 0; row != num_nodes; ++row) {
          for (node_t column = row; column != num_nodes; ++column) {

            const size_t index = mat2vec (row, column);
            vector<size_t>& vector = (*adjacency)[index];
            vector.reserve (2 * (num_nodes-1));
            // Should be able to expand from this edge to any other edge connected to either row or column
            for (node_t r = 0; r != num_nodes; ++r) {
              if (r != row)
                vector.push_back (mat2vec (r, column));
            }
            for (node_t c = 0; c != num_nodes; ++c) {
              if (c != column)
                vector.push_back (mat2vec (row, c));
            }
            ++progress;
          }
        }
      }



    }
  }
}



