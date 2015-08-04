/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */



#include "dwi/tractography/connectome/matrix.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {





bool Matrix::operator() (const Mapped_track_nodepair& in)
{
  assert (in.get_first_node()  < data.rows());
  assert (in.get_second_node() < data.rows());
  assert (assignments_lists.empty());
  if (is_vector()) {
    data   (0, in.get_first_node())  += in.get_factor() * in.get_weight();
    counts (0, in.get_first_node())  += in.get_weight();
    data   (0, in.get_second_node()) += in.get_factor() * in.get_weight();
    counts (0, in.get_second_node()) += in.get_weight();
  } else {
    const node_t row    = std::min (in.get_first_node(), in.get_second_node());
    const node_t column = std::max (in.get_first_node(), in.get_second_node());
    data   (row, column) += in.get_factor() * in.get_weight();
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
  assert (in.get_first_node()  < data.rows());
  assert (in.get_second_node() < data.rows());
  assert (assignments_pairs.empty());
  std::vector<node_t> list (in.get_nodes());
  if (is_vector()) {
    if (list.empty()) {
      data   (0, 0) += in.get_factor() * in.get_weight();
      counts (0, 0) += in.get_weight();
      list.push_back (0);
    } else {
      for (std::vector<node_t>::const_iterator n = list.begin(); n != list.end(); ++n) {
        data   (0, *n) += in.get_factor() * in.get_weight();
        counts (0, *n) += in.get_weight();
      }
    }
  } else { // Matrix output
    if (list.empty()) {
      data   (0, 0) += in.get_factor() * in.get_weight();
      counts (0, 0) += in.get_weight();
      list.push_back (0);
    } else if (list.size() == 1) {
      data   (0, list.front()) += in.get_factor() * in.get_weight();
      counts (0, list.front()) += in.get_weight();
    } else {
      for (size_t i = 0; i != list.size(); ++i) {
        for (size_t j = i; j != list.size(); ++j) {
          data   (list[i], list[j]) += in.get_factor() * in.get_weight();
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
    assignments_lists.resize (in.get_track_index() + 1, std::vector<node_t>());
    assignments_lists[in.get_track_index()] = std::move (list);
  }
  return true;
}




void Matrix::scale_by_streamline_count()
{
  for (node_t i = 0; i != counts.rows(); ++i) {
    for (node_t j = i; j != counts.columns(); ++j) {
      if (counts (i, j)) {
        data (i, j) /= counts (i, j);
        counts (i, j) = 1;
      }
    }
  }
}



void Matrix::remove_unassigned()
{
  if (is_vector()) {
    for (node_t i = 0; i != data.columns() - 1; ++i) {
      data   (0, i) = data   (0, i+1);
      counts (0, i) = counts (0, i+1);
    }
    data  .resize (1, data  .columns() - 1);
    counts.resize (1, counts.columns() - 1);
  } else {
    for (node_t i = 0; i != data.rows() - 1; ++i) {
      for (node_t j = i; j != data.columns() - 1; ++j) {
        data   (i, j) = data   (i+1, j+1);
        counts (i, j) = counts (i+1, j+1);
      }
    }
    data  .resize (data  .rows() - 1, data  .columns() - 1);
    counts.resize (counts.rows() - 1, counts.columns() - 1);
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
  std::vector<uint32_t> node_counts (data.columns(), 0);
  for (node_t i = 0; i != counts.rows(); ++i) {
    for (node_t j = i; j != counts.columns(); ++j) {
      node_counts[i] += counts (i, j);
      node_counts[j] += counts (i, j);
    }
  }
  std::vector<node_t> empty_nodes;
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



void Matrix::write_assignments (const std::string& path)
{
  File::OFStream stream (path);
  for (auto i = assignments_pairs.begin(); i != assignments_pairs.end(); ++i)
    stream << str(i->first) << " " << str(i->second) << "\n";
}






}
}
}
}


