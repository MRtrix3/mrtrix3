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


#include "dwi/tractography/connectome/matrix.h"

#include "bitset.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



const char* statistics[] = { "sum", "mean", "min", "max", NULL };

const App::Option EdgeStatisticOption

  = App::Option ("stat_edge",
                 "statistic for combining the values from all streamlines in an edge "
                 "into a single scale value for that edge "
                 "(options are: " + join(statistics, ",") + "; default=sum)")
    + App::Argument("statistic").type_choice (statistics);




template <typename T>
bool Matrix<T>::operator() (const Mapped_track_nodepair& in)
{
  assert (in.get_first_node()  < mat2vec->mat_size());
  assert (in.get_second_node() < mat2vec->mat_size());
  assert (assignments_lists.empty());
  if (is_vector()) {
    assert (assignments_pairs.empty());
    apply_data (in.get_second_node(), in.get_factor(), in.get_weight());
    inc_count (in.get_second_node(), in.get_weight());
    if (track_assignments) {
      if (in.get_track_index() == assignments_single.size()) {
        assignments_single.push_back (in.get_second_node());
      } else if (in.get_track_index() < assignments_single.size()) {
        assignments_single[in.get_track_index()] = in.get_second_node();
      } else {
        assignments_single.resize (in.get_track_index() + 1, 0);
        assignments_single[in.get_track_index()] = in.get_second_node();
      }
    }
  } else {
    assert (assignments_single.empty());
    apply_data (in.get_first_node(), in.get_second_node(), in.get_factor(), in.get_weight());
    inc_count (in.get_first_node(), in.get_second_node(), in.get_weight());
    if (track_assignments) {
      if (in.get_track_index() == assignments_pairs.size()) {
        assignments_pairs.push_back (in.get_nodes());
      } else if (in.get_track_index() < assignments_pairs.size()) {
        assignments_pairs[in.get_track_index()] = in.get_nodes();
      } else {
        assignments_pairs.resize (in.get_track_index() + 1, std::make_pair<size_t, size_t> (0, 0));
        assignments_pairs[in.get_track_index()] = in.get_nodes();
      }
    }
  }
  return true;
}



template <typename T>
bool Matrix<T>::operator() (const Mapped_track_nodelist& in)
{
  assert (assignments_single.empty());
  assert (assignments_pairs.empty());
  vector<node_t> list (in.get_nodes());
  for (vector<node_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
    assert (*i < data.rows());
  }
  if (is_vector()) {
    if (list.empty()) {
      apply_data (0, in.get_factor(), in.get_weight());
      inc_count (0, in.get_weight());
      list.push_back (0);
    } else {
      for (vector<node_t>::const_iterator n = list.begin(); n != list.end(); ++n) {
        apply_data (*n, in.get_factor(), in.get_weight());
        inc_count (*n, in.get_weight());
      }
    }
  } else { // Matrix output
    if (list.empty()) {
      apply_data (0, 0, in.get_factor(), in.get_weight());
      inc_count (0, 0, in.get_weight());
      list.push_back (0);
    } else if (list.size() == 1) {
      apply_data (0, list.front(), in.get_factor(), in.get_weight());
      inc_count (0, list.front(), in.get_weight());
    } else {
      for (size_t i = 0; i != list.size(); ++i) {
        for (size_t j = i; j != list.size(); ++j) {
          apply_data (list[i], list[j], in.get_factor(), in.get_weight());
          inc_count (list[i], list[j], in.get_weight());
        }
      }
    }
  }
  if (track_assignments) {
    std::sort (list.begin(), list.end());
    if (in.get_track_index() == assignments_lists.size()) {
      assignments_lists.push_back (std::move (list));
    } else if (in.get_track_index() < assignments_lists.size()) {
      assignments_lists[in.get_track_index()] = std::move (list);
    } else {
      assignments_lists.resize (in.get_track_index() + 1, vector<node_t>());
      assignments_lists[in.get_track_index()] = std::move (list);
    }
  }
  return true;
}



template <typename T>
void Matrix<T>::finalize()
{
  switch (statistic) {
    case stat_edge::SUM:
      return;
    case stat_edge::MEAN:
      assert (counts.size());
      for (ssize_t i = 0; i != data.size(); ++i) {
        if (counts[i]) {
          data[i] /= counts[i];
          counts[i] = T(1.0);
        }
      }
      return;
    case stat_edge::MIN:
    case stat_edge::MAX:
      for (ssize_t i = 0; i != data.size(); ++i) {
        if (!std::isfinite (data[i]))
          data[i] = std::numeric_limits<T>::quiet_NaN();
      }
      return;
  }
}




template <typename T>
void Matrix<T>::error_check (const std::set<node_t>& missing_nodes)
{
  // Don't bother looking for empty nodes if we're generating a
  //   connectivity vector from a seed region rather than a
  //   connectome from a whole-brain tractogram
  if (vector_output)
    return;
  assert (mat2vec);
  BitSet visited (mat2vec->mat_size());
  for (ssize_t i = 0; i != data.size(); ++i) {
    if (std::isfinite(data[i]) && data[i]) {
      auto nodes = (*mat2vec) (i);
      visited[nodes.first]  = true;
      visited[nodes.second] = true;
    }
  }
  vector<std::string> empty_nodes;
  for (node_t i = 1; i != visited.size(); ++i) {
    if (!visited[i] && missing_nodes.find (i) == missing_nodes.end())
      empty_nodes.push_back (str(i));
  }
  if (empty_nodes.size()) {
    WARN ("The following nodes do not have any streamlines assigned:");
    WARN (join (empty_nodes, ", "));
    WARN ("(This may indicate a poor registration)");
  }
}



template <typename T>
void Matrix<T>::write_assignments (const std::string& path) const
{
  if (!track_assignments)
    throw Exception ("Cannot write streamline assignments to file as they were not stored during processing");
  File::OFStream stream (path);
  for (auto i = assignments_single.begin(); i != assignments_single.end(); ++i)
    stream << str(*i) << "\n";
  for (auto i = assignments_pairs.begin(); i != assignments_pairs.end(); ++i)
    stream << str(i->first) << " " << str(i->second) << "\n";
  for (auto i = assignments_lists.begin(); i != assignments_lists.end(); ++i) {
    assert (i->size());
    stream << str((*i)[0]);
    for (size_t j = 1; j != i->size(); ++j)
      stream << " " << str((*i)[j]);
    stream << "\n";
  }
}



template <typename T>
void Matrix<T>::save (const std::string& path,
                      const bool keep_unassigned,
                      const bool symmetric,
                      const bool zero_diagonal) const
{
  // Write the output file one line at a time
  // No point in keeping a dense matrix version of this function;
  //   it would just increase code management
  if (vector_output) {
    if (symmetric)
      WARN ("Option -symmetric not applicable when generating connectivity vector; ignored");
    if (zero_diagonal)
      WARN ("Option -zero_diagonal not applicable when generating connectivity vector; ignored");
    if (keep_unassigned)
      save_vector (data, path);
    else
      save_vector (data.tail(data.size()-1), path);
    return;
  }

  assert (mat2vec);

  File::OFStream out (path);
  Eigen::IOFormat fmt (Eigen::FullPrecision, Eigen::DontAlignCols, " ", "\n", "", "", "", "");
  for (node_t row = 0; row != mat2vec->mat_size(); ++row) {
    if (!row && !keep_unassigned)
      continue;
    vector_type temp (vector_type::Zero (mat2vec->mat_size()));
    for (node_t col = 0; col != mat2vec->mat_size(); ++col) {
      if (symmetric || col >= row)
        temp[col] = data[(*mat2vec) (row, col)];
    }
    if (zero_diagonal)
      temp[row] = T(0.0);
    if (keep_unassigned)
      out << temp.transpose().format (fmt) << "\n";
    else
      out << temp.tail (temp.size()-1).transpose().format (fmt) << "\n";
  }
}



template <typename T>
void Matrix<T>::apply_data (const size_t index, const T value, const T weight)
{
  T& target = data[index];
  apply_data (target, value, weight);
}

template <typename T>
void Matrix<T>::apply_data (const size_t node_one, const size_t node_two, const T value, const T weight)
{
  assert (mat2vec);
  T& target = data[(*mat2vec) (node_one, node_two)];
  apply_data (target, value, weight);
}

template <typename T>
void Matrix<T>::apply_data (T& target, const T value, const T weight)
{
  switch (statistic) {
    case stat_edge::SUM:
    case stat_edge::MEAN:
      target += value * weight;
      return;
    case stat_edge::MIN:
      target = std::min (target, value);
      break;
    case stat_edge::MAX:
      target = std::max (target, value);
      break;
  }
}

template <typename T>
void Matrix<T>::inc_count (const size_t index, const T weight)
{
  if (statistic != stat_edge::MEAN)
    return;
  assert (counts.size());
  counts[index] += weight;
}

template <typename T>
void Matrix<T>::inc_count (const size_t node_one, const size_t node_two, const T weight)
{
  if (statistic != stat_edge::MEAN)
    return;
  assert (counts.size());
  assert (mat2vec);
  counts[(*mat2vec) (node_one, node_two)] += weight;
}



template class Matrix<float>;
template class Matrix<double>;



}
}
}
}


