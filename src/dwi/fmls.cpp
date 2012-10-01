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




FOD_FMLS::FOD_FMLS (const Math::Hemisphere::Directions& directions, const size_t l) :
      dirs                             (directions),
      lmax                             (l),
      transform                        (new Math::SH::Transform<float> (dirs.get_az_el_pairs(), lmax)),
      precomputer                      (new Math::SH::PrecomputedAL<float> (lmax, 2 * dirs.get_num_dirs())),
      ratio_to_negative_lobe_integral  (RATIO_TO_NEGATIVE_LOBE_INTEGRAL_DEFAULT),
      ratio_to_negative_lobe_mean_peak (RATIO_TO_NEGATIVE_LOBE_MEAN_PEAK_DEFAULT),
      ratio_to_peak_value              (RATIO_TO_PEAK_VALUE_DEFAULT),
      peak_value_threshold             (PEAK_VALUE_THRESHOLD) { }




class Max_abs {
  public:
    bool operator() (const float& a, const float& b) const { return (Math::abs (a) > Math::abs (b)); }
};

bool FOD_FMLS::operator() (const SH_coefs& in, FOD_lobes& out) const {

  assert (in.size() == Math::SH::NforL (lmax));

  out.clear();
  out.vox = in.vox;

  Math::Vector<float> values (dirs.get_num_dirs());
  transform->SH2A (values, in);

  typedef std::multimap<float, dir_t, Max_abs> map_type;
  map_type data_in_order;
  for (size_t i = 0; i != values.size(); ++i)
    data_in_order.insert (std::make_pair (values[i], i));

  std::vector< std::pair<dir_t, uint8_t> > retrospective_assignments;

  for (map_type::const_iterator i = data_in_order.begin(); i != data_in_order.end(); ++i) {

    std::vector<lobe_t> adj_lobes;
    for (lobe_t l = 0; l != out.size(); ++l) {
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

    if (i->is_negative() || i->get_peak_value() < MIN(min_peak_amp, peak_value_threshold) || i->get_integral() < min_integral) {
      i = out.erase (i);
    } else {
      const dir_t peak_bin (i->get_peak_dir_bin());
      Point<float> newton_peak (dirs.get_dir (peak_bin));
      float new_peak_value = Math::SH::get_peak (in.ptr(), lmax, newton_peak, &(*precomputer));
      if (new_peak_value > i->get_peak_value() && newton_peak.valid())
        i->revise_peak (newton_peak, new_peak_value);
      else
        i->revise_peak (dirs.get_dir (peak_bin), i->get_peak_value());
      i->normalise_integral();
      ++i;

    }

  }

  return true;

}




}
}
