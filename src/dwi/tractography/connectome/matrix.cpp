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




bool Matrix::operator() (const Mapped_track& in)
{
  assert (in.get_first_node()  < data.rows());
  assert (in.get_second_node() < data.rows());
  if (in.get_second_node() < in.get_first_node()) {
    data   (in.get_second_node(), in.get_first_node()) += in.get_factor() * in.get_weight();
    counts (in.get_second_node(), in.get_first_node()) += in.get_weight();
  } else {
    data   (in.get_first_node(), in.get_second_node()) += in.get_factor() * in.get_weight();
    counts (in.get_first_node(), in.get_second_node()) += in.get_weight();
  }
  if (in.get_track_index() == assignments.size()) {
    assignments.push_back (in.get_nodes());
  } else if (in.get_track_index() < assignments.size()) {
    assignments[in.get_track_index()] = in.get_nodes();
  } else {
    assignments.resize (in.get_track_index() + 1, std::make_pair<size_t, size_t> (0, 0));
    assignments[in.get_track_index()] = in.get_nodes();
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
  for (node_t i = 0; i != data.rows() - 1; ++i) {
    for (node_t j = i; j != data.columns() - 1; ++j) {
      data   (i, j) = data   (i+1, j+1);
      counts (i, j) = counts (i+1, j+1);
    }
  }
  data  .resize (data  .rows() - 1, data  .columns() - 1);
  counts.resize (counts.rows() - 1, counts.columns() - 1);
}



void Matrix::zero_diagonal()
{
  for (node_t i = 0; i != data.rows(); ++i)
    data (i, i) = counts (i, i) = 0.0;
}



void Matrix::error_check (const std::set<node_t>& missing_nodes)
{
  std::vector<uint32_t> node_counts (num_nodes(), 0);
  for (node_t i = 0; i != counts.rows() - 1; ++i) {
    for (node_t j = i; j != counts.columns() - 1; ++j) {
      node_counts[i] += counts (i, j);
      node_counts[j] += counts (i, j);
    }
  }
  std::vector<node_t> empty_nodes;
  for (size_t i = 0; i != node_counts.size(); ++i) {
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
  for (auto i = assignments.begin(); i != assignments.end(); ++i)
    stream << str(i->first) << " " << str(i->second) << "\n";
}






}
}
}
}


