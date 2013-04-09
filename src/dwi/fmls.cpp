/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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




#include "dwi/fmls.h"



namespace MR {
namespace DWI {




FOD_FMLS::FOD_FMLS (const DWI::Directions::Set& directions, const size_t l) :
      dirs                             (directions),
      lmax                             (l),
      transform                        (NULL),
      precomputer                      (new Math::SH::PrecomputedAL<float> (lmax, 2 * dirs.size())),
      ratio_to_negative_lobe_integral  (RATIO_TO_NEGATIVE_LOBE_INTEGRAL_DEFAULT),
      ratio_to_negative_lobe_mean_peak (RATIO_TO_NEGATIVE_LOBE_MEAN_PEAK_DEFAULT),
      ratio_to_peak_value              (RATIO_TO_PEAK_VALUE_DEFAULT),
      peak_value_threshold             (PEAK_VALUE_THRESHOLD),
      create_null_lobe                 (false),
      create_lookup_table              (true),
      dilate_lookup_table              (false)
{
  Math::Matrix<float> az_el_pairs (dirs.size(), 2);
  for (size_t row = 0; row != dirs.size(); ++row) {
    const Point<float> d (dirs.get_dir (row));
    az_el_pairs (row, 0) = Math::atan2 (d[1], d[0]);
    az_el_pairs (row, 1) = Math::acos  (d[2]);
  }
  transform = new Math::SH::Transform<float> (az_el_pairs, lmax);

}




class Max_abs {
  public:
    bool operator() (const float& a, const float& b) const { return (Math::abs (a) > Math::abs (b)); }
};

bool FOD_FMLS::operator() (const SH_coefs& in, FOD_lobes& out) const {

  assert (in.size() == Math::SH::NforL (lmax));

  out.clear();
  out.vox = in.vox;

  if (in[0] <= 0.0)
    return true;

  Math::Vector<float> values (dirs.size());
  transform->SH2A (values, in);

  typedef std::multimap<float, dir_t, Max_abs> map_type;
  map_type data_in_order;
  for (size_t i = 0; i != values.size(); ++i)
    data_in_order.insert (std::make_pair (values[i], i));

  if (data_in_order.begin()->first <= 0.0)
    return true;

  std::vector< std::pair<dir_t, uint8_t> > retrospective_assignments;

  for (map_type::const_iterator i = data_in_order.begin(); i != data_in_order.end(); ++i) {

    std::vector<uint32_t> adj_lobes;
    for (uint32_t l = 0; l != out.size(); ++l) {
      if ((((i->first <= 0.0) &&  out[l].is_negative())
        || ((i->first >  0.0) && !out[l].is_negative()))
        && (out[l].get_mask().is_adjacent (i->second))) {

        adj_lobes.push_back (l);

      }
    }

    if (adj_lobes.empty()) {

      out.push_back (FOD_lobe (dirs, i->second, i->first));

    } else if (adj_lobes.size() == 1) {

      out[adj_lobes.front()].add (i->second, i->first);

    } else {

      if (out[adj_lobes.back()].get_peak_value() / Math::abs (i->first) < ratio_to_peak_value) {

        for (size_t j = adj_lobes.size() - 1; j; --j) {
          out[adj_lobes.front()].merge (out[adj_lobes[j]]);
          std::vector<FOD_lobe>::iterator ptr = out.begin();
          advance (ptr, adj_lobes[j]);
          out.erase (ptr);
        }
        out[adj_lobes.front()].add (i->second, i->first);

      } else {

        retrospective_assignments.push_back (std::make_pair (i->second, adj_lobes.front()));

      }

    }

  }

  for (std::vector< std::pair<dir_t, uint8_t> >::const_iterator i = retrospective_assignments.begin(); i != retrospective_assignments.end(); ++i)
    out[i->second].add (i->first, values[i->first]);

  float mean_neg_peak = 0.0, max_neg_integral = 0.0;
  uint8_t neg_lobe_count = 0;
  for (std::vector<FOD_lobe>::const_iterator i = out.begin(); i != out.end(); ++i) {
    if (i->is_negative()) {
      mean_neg_peak += i->get_peak_value();
      ++neg_lobe_count;
      max_neg_integral = MAX (max_neg_integral, i->get_integral());
    }
  }

  const float min_peak_amp = ratio_to_negative_lobe_mean_peak * (mean_neg_peak / float(neg_lobe_count));
  const float min_integral = ratio_to_negative_lobe_integral  * max_neg_integral;

  for (std::vector<FOD_lobe>::iterator i = out.begin(); i != out.end();) { // Empty increment

    if (i->is_negative() || i->get_peak_value() < MAX(min_peak_amp, peak_value_threshold) || i->get_integral() < min_integral) {
      i = out.erase (i);
    } else {
      const dir_t peak_bin (i->get_peak_dir_bin());
      Point<float> newton_peak (dirs.get_dir (peak_bin));
      float new_peak_value = Math::SH::get_peak (in.ptr(), lmax, newton_peak, &(*precomputer));
      if (new_peak_value > i->get_peak_value() && newton_peak.valid())
        i->revise_peak (newton_peak, new_peak_value);
      i->normalise_integral();
      ++i;
    }
  }

  if (create_lookup_table) {

    out.lut.assign (dirs.size(), out.size());

    size_t index = 0;
    for (std::vector<FOD_lobe>::iterator i = out.begin(); i != out.end(); ++i, ++index) {
      const Mask& this_mask (i->get_mask());
      for (size_t d = 0; d != dirs.size(); ++d) {
        if (this_mask[d])
          out.lut[d] = index;
      }
    }

    if (dilate_lookup_table && out.size()) {

      Mask processed (dirs);
      for (std::vector<FOD_lobe>::iterator i = out.begin(); i != out.end(); ++i)
        processed |= i->get_mask();

      std::vector<uint8_t> new_assignments [dirs.size()];
      while (!processed.full()) {

        for (dir_t dir = 0; dir != dirs.size(); ++dir) {
          if (!processed[dir]) {
            for (std::vector<dir_t>::const_iterator neighbour = dirs.get_adj_dirs (dir).begin(); neighbour != dirs.get_adj_dirs (dir).end(); ++neighbour) {
              if (processed[*neighbour])
                new_assignments[dir].push_back (out.lut[*neighbour]);
            }
          }
        }
        for (dir_t dir = 0; dir != dirs.size(); ++dir) {
          if (new_assignments[dir].size() == 1) {

            out.lut[dir] = new_assignments[dir].front();
            processed[dir] = true;
            new_assignments[dir].clear();

          } else if (new_assignments[dir].size() > 1) {

            uint8_t best_lobe = 0;
            float max_integral = 0.0;
            for (std::vector<uint8_t>::const_iterator lobe_no = new_assignments[dir].begin(); lobe_no != new_assignments[dir].end(); ++lobe_no) {
              if (out[*lobe_no].get_integral() > max_integral) {
                best_lobe = *lobe_no;
                max_integral = out[*lobe_no].get_integral();
              }
            }
            out.lut[dir] = best_lobe;
            processed[dir] = true;
            new_assignments[dir].clear();

          }
        }

      }

    }

  }

  if (create_null_lobe) {
    Mask null_mask (dirs);
    for (std::vector<FOD_lobe>::iterator i = out.begin(); i != out.end(); ++i)
      null_mask |= i->get_mask();
    null_mask = ~null_mask;
    out.push_back (FOD_lobe (null_mask));
  }

  return true;

}




}
}
