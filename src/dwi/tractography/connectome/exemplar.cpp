/* Copyright (c) 2008-2017 the MRtrix3 contributors
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




#include "dwi/tractography/connectome/exemplar.h"


// Fraction of the streamline length at each end that will be pulled toward the node centre-of-mass
// TODO Make this a fraction of length, rather than fraction of points?
#define EXEMPLAR_ENDPOINT_CONVERGE_FRACTION 0.25



namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



Exemplar& Exemplar::operator= (const Exemplar& that)
{
  Tractography::Streamline<float>(*this) = that;
  nodes = that.nodes;
  node_COMs = that.node_COMs;
  is_finalized = that.is_finalized;
  return *this;
}

void Exemplar::add (const Tractography::Connectome::Streamline_nodepair& in)
{
  // Determine whether or not this streamline is reversed w.r.t. the exemplar
  // Now the ordering of the nodes is retained; use those
  bool is_reversed = false;
  if (in.get_nodes() != nodes) {
    is_reversed = true;
    assert (in.get_nodes().first == nodes.second && in.get_nodes().second == nodes.first);
  }
  // Contribute this streamline toward the mean exemplar streamline
  add (in, is_reversed);
}

void Exemplar::add (const Connectome::Streamline_nodelist& in)
{
  // In this case, the streamline hasn't been neatly assigned to a node pair,
  //   but has potentially traversed many nodes.
  // To try to make the exemplars sensible, find the two points along the streamline
  //   that are closest to the node COMs, and truncate the track to that range,
  //   before contributing to the exemplar
  size_t index_closest_to_first_node = 0, index_closest_to_second_node = 0;
  float min_distance_to_first_node  = (node_COMs.first  - in[0]).squaredNorm();
  float min_distance_to_second_node = (node_COMs.second - in[0]).squaredNorm();
  for (size_t i = 1; i != in.size(); ++i) {
    const float distance_to_first_node = (node_COMs.first - in[i]).squaredNorm();
    if (distance_to_first_node < min_distance_to_first_node) {
      min_distance_to_first_node = distance_to_first_node;
      index_closest_to_first_node = i;
    }
    const float distance_to_second_node = (node_COMs.second - in[i]).squaredNorm();
    if (distance_to_second_node < min_distance_to_second_node) {
      min_distance_to_second_node = distance_to_second_node;
      index_closest_to_second_node = i;
    }
  }
  if (index_closest_to_first_node == index_closest_to_second_node)
    return;
  const bool is_reversed = (index_closest_to_second_node < index_closest_to_first_node);
  const size_t first = std::min (index_closest_to_first_node, index_closest_to_second_node);
  const size_t last  = std::max (index_closest_to_first_node, index_closest_to_second_node);
  Tractography::Streamline<float> subtck;
  for (size_t i = first; i <= last; ++i)
    subtck.push_back (in[i]);
  subtck.index  = in.index;
  subtck.weight = in.weight;
  add (in, is_reversed);
}

void Exemplar::add (const Tractography::Streamline<float>& in, const bool is_reversed)
{
  assert (!is_finalized);
  std::lock_guard<std::mutex> lock (mutex);

  for (size_t i = 0; i != size(); ++i) {
    float interp_pos = (in.size() - 1) * i / float(size());
    if (is_reversed)
      interp_pos = in.size() - 1 - interp_pos;
    const size_t lower = std::floor (interp_pos), upper (lower + 1);
    const float mu = interp_pos - lower;
    point_type pos;
    if (lower == in.size() - 1)
      pos = in.back();
    else
      pos = ((1.0f-mu) * in[lower]) + (mu * in[upper]);
    (*this)[i] += (pos * in.weight);
  }

  weight += in.weight;
}




void Exemplar::finalize (const float step_size)
{
  assert (!is_finalized);
  std::lock_guard<std::mutex> lock (mutex);

  if (!weight || is_diagonal()) {
    // No streamlines assigned, or a diagonal in the matrix; generate a straight line between the two nodes
    // FIXME Is there an error in the new tractography tool that is causing omission of the first track segment?
    clear();
    push_back (node_COMs.first);
    push_back (node_COMs.second);
    is_finalized = true;
    return;
  }

  const float multiplier = 1.0f / weight;
  for (auto i = begin(); i != end(); ++i)
    *i *= multiplier;

  // Constrain endpoints to the node centres of mass
  size_t num_converging_points = EXEMPLAR_ENDPOINT_CONVERGE_FRACTION * size();
  for (size_t i = 0; i != num_converging_points; ++i) {
    const float mu = i / float(num_converging_points);
    (*this)[i] = (mu * (*this)[i]) + ((1.0f-mu) * node_COMs.first);
  }
  for (size_t i = size() - 1; i != size() - 1 - num_converging_points; --i) {
    const float mu = (size() - 1 - i) / float(num_converging_points);
    (*this)[i] = (mu * (*this)[i]) + ((1.0f-mu) * node_COMs.second);
  }

  // Resample to fixed step size
  // Start from the midpoint, resample backwards to the start of the exemplar,
  //   reverse the data, then do the second half of the exemplar
  int32_t index = (size() + 1) / 2;
  std::vector<point_type> vertices (1, (*this)[index]);
  const float step_sq = Math::pow2 (step_size);
  for (int32_t step = -1; step <= 1; step += 2) {
    if (step == 1) {
      std::reverse (vertices.begin(), vertices.end());
      index = (size() + 1) / 2;
    }
    do {
      while ((index+step) >= 0 && (index+step) < int32_t(size()) && ((*this)[index+step] - vertices.back()).squaredNorm() < step_sq)
        index += step;
      // Ideal point for fixed step size lies somewhere between [index] and [index+step]
      // Do a binary search to find this point
      // Unless we're at an endpoint...
      if (index == 0 || index == int32_t(size())-1) {
        vertices.push_back ((*this)[index]);
      } else {
        float lower = 0.0f, mu = 0.5f, upper = 1.0f;
        point_type p (((*this)[index] + (*this)[index+step]) * 0.5f);
        for (uint32_t iter = 0; iter != 6; ++iter) {
          if ((p - vertices.back()).squaredNorm() > step_sq)
            upper = mu;
          else
            lower = mu;
          mu = 0.5 * (lower + upper);
          p = ((*this)[index] * (1.0-mu)) + ((*this)[index+step] * mu);
        }
        vertices.push_back (p);
      }
    } while (index != 0 && index != int32_t(size())-1);
  }
  std::swap (*this, vertices);

  is_finalized = true;
}


}
}
}
}


