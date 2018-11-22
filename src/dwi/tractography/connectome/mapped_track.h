/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_connectome_mapped_track_h__
#define __dwi_tractography_connectome_mapped_track_h__

#include "dwi/tractography/connectome/connectome.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Connectome {



        class Mapped_track_base
        { MEMALIGN(Mapped_track_base)

          public:
            Mapped_track_base() :
              track_index (-1),
              factor (0.0),
              weight (1.0) { }

            void set_track_index (const size_t i) { track_index = i; }
            void set_factor      (const float i)  { factor = i; }
            void set_weight      (const float i)  { weight = i; }

            size_t get_track_index() const { return track_index; }
            float  get_factor()      const { return factor; }
            float  get_weight()      const { return weight; }

          private:
            size_t track_index;
            float factor, weight;
        };


        class Mapped_track_nodepair : public Mapped_track_base
        { MEMALIGN(Mapped_track_nodepair)

          public:
            Mapped_track_nodepair() :
              Mapped_track_base (),
              nodes (std::make_pair (0, 0)) { }

            void set_first_node  (const node_t i)   { nodes.first = i;  }
            void set_second_node (const node_t i)   { nodes.second = i; }
            void set_nodes       (const NodePair i) { nodes = i; }

            node_t get_first_node()     const { return nodes.first;  }
            node_t get_second_node()    const { return nodes.second; }
            const NodePair& get_nodes() const { return nodes; }

          private:
            NodePair nodes;

        };


        class Mapped_track_nodelist : public Mapped_track_base
        { MEMALIGN(Mapped_track_nodelist)

          public:
            Mapped_track_nodelist() :
              Mapped_track_base (),
              nodes () { }

            void add_node   (const node_t i)               { nodes.push_back (i);  }
            void set_nodes  (const vector<node_t>& i) { nodes = i; }
            void set_nodes  (vector<node_t>&& i)       { std::swap (nodes, i); }

            const vector<node_t>& get_nodes() const { return nodes; }

          private:
            vector<node_t> nodes;

        };




      }
    }
  }
}


#endif

