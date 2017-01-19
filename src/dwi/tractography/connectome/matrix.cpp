/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#include "dwi/tractography/connectome/matrix.h"


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





bool Matrix::operator() (const Mapped_track_nodepair& in)
{
  assert (in.get_first_node()  < data.rows());
  assert (in.get_second_node() < data.rows());
  assert (assignments_lists.empty());
  if (is_vector()) {
    apply (data (0, in.get_first_node()), in.get_factor(), in.get_weight());
    counts (0, in.get_first_node()) += in.get_weight();
    apply (data (0, in.get_second_node()), in.get_factor(), in.get_weight());
    counts (0, in.get_second_node()) += in.get_weight();
  } else {
    const node_t row    = std::min (in.get_first_node(), in.get_second_node());
    const node_t column = std::max (in.get_first_node(), in.get_second_node());
    apply (data (row, column), in.get_factor(), in.get_weight());
    counts (row, column) += in.get_weight();
  }
  if (in.get_track_index() == assignments_pairs.size()) {
    assignments_pairs.push_back (in.get_nodes());
  } else if (in.get_track_index() < assignments_pairs.size()) {
    assignments_pairs[in.get_track_index()] = in.get_nodes();
  } else {
    assignments_pairs.resize (in.get_track_index() + 1, std::make_pair<size_t, size_t> (0, 0));
    assignments_pairs[in.get_track_index()] = in.get_nodes();
  }
  return true;
}



bool Matrix::operator() (const Mapped_track_nodelist& in)
{
  assert (assignments_pairs.empty());
  vector<node_t> list (in.get_nodes());
  for (vector<node_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
    assert (*i < data.rows());
  }
  if (is_vector()) {
    if (list.empty()) {
      apply (data (0, 0), in.get_factor(), in.get_weight());
      counts (0, 0) += in.get_weight();
      list.push_back (0);
    } else {
      for (vector<node_t>::const_iterator n = list.begin(); n != list.end(); ++n) {
        apply (data (0, *n), in.get_factor(), in.get_weight());
        counts (0, *n) += in.get_weight();
      }
    }
  } else { // Matrix output
    if (list.empty()) {
      apply (data (0, 0), in.get_factor(), in.get_weight());
      counts (0, 0) += in.get_weight();
      list.push_back (0);
    } else if (list.size() == 1) {
      apply (data (0, list.front()), in.get_factor(), in.get_weight());
      counts (0, list.front()) += in.get_weight();
    } else {
      for (size_t i = 0; i != list.size(); ++i) {
        for (size_t j = i; j != list.size(); ++j) {
          apply (data (list[i], list[j]), in.get_factor(), in.get_weight());
          counts (list[i], list[j]) += in.get_weight();
        }
      }
    }
  }
  std::sort (list.begin(), list.end());
  if (in.get_track_index() == assignments_lists.size()) {
    assignments_lists.push_back (std::move (list));
  } else if (in.get_track_index() < assignments_lists.size()) {
    assignments_lists[in.get_track_index()] = std::move (list);
  } else {
    assignments_lists.resize (in.get_track_index() + 1, vector<node_t>());
    assignments_lists[in.get_track_index()] = std::move (list);
  }
  return true;
}




void Matrix::finalize()
{
  switch (statistic) {
    case stat_edge::SUM:
      return;
    case stat_edge::MEAN:
      for (node_t i = 0; i != counts.rows(); ++i) {
        for (node_t j = i; j != counts.cols(); ++j) {
          if (counts (i, j)) {
            data (i, j) /= counts (i, j);
            counts (i, j) = 1;
          }
        }
      }
      return;
    case stat_edge::MIN:
    case stat_edge::MAX:
      for (node_t i = 0; i != counts.rows(); ++i) {
        for (node_t j = i; j != counts.cols(); ++j) {
          if (!std::isfinite (data (i, j)))
            data (i, j) = std::numeric_limits<default_type>::quiet_NaN();
        }
      }
      return;
  }
}



void Matrix::remove_unassigned()
{
  if (is_vector()) {
    for (node_t i = 0; i != data.cols() - 1; ++i) {
      data   (0, i) = data   (0, i+1);
      counts (0, i) = counts (0, i+1);
    }
    data  .conservativeResize (1, data  .cols() - 1);
    counts.conservativeResize (1, counts.cols() - 1);
  } else {
    for (node_t i = 0; i != data.rows() - 1; ++i) {
      for (node_t j = i; j != data.cols() - 1; ++j) {
        data   (i, j) = data   (i+1, j+1);
        counts (i, j) = counts (i+1, j+1);
      }
    }
    data  .conservativeResize (data  .rows() - 1, data  .cols() - 1);
    counts.conservativeResize (counts.rows() - 1, counts.cols() - 1);
  }
}



void Matrix::zero_diagonal()
{
  if (is_vector()) return;
  for (node_t i = 0; i != data.rows(); ++i)
    data (i, i) = counts (i, i) = 0.0;
}



void Matrix::error_check (const std::set<node_t>& missing_nodes)
{
  vector<default_type> node_counts (data.cols(), 0);
  for (node_t i = 0; i != counts.rows(); ++i) {
    for (node_t j = i; j != counts.cols(); ++j) {
      node_counts[i] += counts (i, j);
      node_counts[j] += counts (i, j);
    }
  }
  vector<node_t> empty_nodes;
  for (size_t i = 1; i != node_counts.size(); ++i) {
    if (!node_counts[i] && missing_nodes.find (i) == missing_nodes.end())
      empty_nodes.push_back (i);
  }
  if (empty_nodes.size()) {
    WARN ("The following nodes do not have any streamlines assigned:");
    std::string list = str(empty_nodes.front());
    for (size_t i = 1; i != empty_nodes.size(); ++i)
      list += ", " + str(empty_nodes[i]);
    WARN (list);
    WARN ("(This may indicate a poor registration)");
  }
}



void Matrix::write (const std::string& path) const
{
  MR::save_matrix (data, path);
}

void Matrix::write_assignments (const std::string& path) const
{
  File::OFStream stream (path);
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




void Matrix::apply (double& target, const double value, const double weight)
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






}
}
}
}


