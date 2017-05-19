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


#ifndef __dwi_tractography_connectome_matrix_h__
#define __dwi_tractography_connectome_matrix_h__

#include <set>

#include "types.h"

#include "connectome/connectome.h"
#include "connectome/mat2vec.h"
#include "math/math.h"

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/mapped_track.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



enum stat_edge { SUM, MEAN, MIN, MAX };
extern const char* statistics[];
extern const App::Option EdgeStatisticOption;


// The number of nodes that must be exceeded in a connectome matrix in
//   order for mechanisms relating to RAM usage reduction to be activated
constexpr node_t node_count_ram_limit = 1024;


template <typename T>
class Matrix
{ MEMALIGN(Matrix)

  public:
    using vector_type = Eigen::Matrix<T, Eigen::Dynamic, 1>;

    Matrix (const node_t max_node_index, const stat_edge stat, const bool vector_output, const bool track_assignments) :
        statistic (stat),
        vector_output (vector_output),
        track_assignments (track_assignments),
        mat2vec (vector_output ?
                 nullptr :
                 new MR::Connectome::Mat2Vec (max_node_index+1)),
        data   (vector_type::Zero (vector_output ?
                                   (max_node_index + 1) :
                                   mat2vec->vec_size())),
        counts (stat == stat_edge::MEAN ?
                vector_type::Zero (vector_output ?
                                   (max_node_index + 1) :
                                   mat2vec->vec_size()) :
                vector_type())
    {
      if (statistic == stat_edge::MIN)
        data = vector_type::Constant (vector_output ? (max_node_index + 1) : mat2vec->vec_size(), std::numeric_limits<T>::infinity());
      else if (statistic == stat_edge::MAX)
        data = vector_type::Constant (vector_output ? (max_node_index + 1) : mat2vec->vec_size(), -std::numeric_limits<T>::infinity());
    }

    bool operator() (const Mapped_track_nodepair&);
    bool operator() (const Mapped_track_nodelist&);

    void finalize();

    void error_check (const std::set<node_t>&);

    void write_assignments (const std::string&) const;

    bool is_vector() const { return (vector_output); }

    void save (const std::string&, const bool, const bool, const bool) const;


  private:
    const stat_edge statistic;
    const bool vector_output;
    const bool track_assignments;

    const std::unique_ptr<MR::Connectome::Mat2Vec> mat2vec;

    vector_type data, counts;
    vector<node_t> assignments_single;
    vector<NodePair> assignments_pairs;
    vector< vector<node_t> > assignments_lists;

    FORCE_INLINE void apply_data (const size_t, const T, const T);
    FORCE_INLINE void apply_data (const size_t, const size_t, const T, const T);
    FORCE_INLINE void apply_data (T&, const T, const T);
    FORCE_INLINE void inc_count (const size_t, const T);
    FORCE_INLINE void inc_count (const size_t, const size_t, const T);

};



extern template class Matrix<float>;
extern template class Matrix<double>;





}
}
}
}


#endif

