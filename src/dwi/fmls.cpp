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
namespace FMLS {





const App::OptionGroup FMLSSegmentOption = App::OptionGroup ("FOD FMLS segmenter options")

  + App::Option ("fmls_ratio_integral_to_neg",
            "threshold the ratio between the integral of a positive FOD lobe, and the integral of the largest negative lobe. "
            "Any lobe that fails to exceed the integral dictated by this ratio will be discarded.")
    + App::Argument ("value").type_float (0.0, FMLS_RATIO_TO_NEGATIVE_LOBE_INTEGRAL_DEFAULT, 1e6)

  + App::Option ("fmls_ratio_peak_to_mean_neg",
            "threshold the ratio between the peak amplitude of a positive FOD lobe, and the mean peak amplitude of all negative lobes. "
            "Any lobe that fails to exceed the peak amplitude dictated by this ratio will be discarded.")
    + App::Argument ("value").type_float (0.0, FMLS_RATIO_TO_NEGATIVE_LOBE_MEAN_PEAK_DEFAULT, 1e6)

  + App::Option ("fmls_peak_value",
            "threshold the raw peak amplitude of positive FOD lobes. "
            "Any lobe for which the peak amplitude is smaller than this threshold will be discarded.")
    + App::Argument ("value").type_float (0.0, FMLS_PEAK_VALUE_THRESHOLD, 1e6)

  + App::Option ("fmls_no_thresholds",
            "disable all FOD lobe thresholding; every lobe with a positive FOD amplitude will be retained.")

  + App::Option ("fmls_peak_ratio_to_merge",
            "specify the amplitude ratio between a sample and the smallest peak amplitude of the adjoining lobes, above which the lobes will be merged. "
            "This is the relative amplitude between the smallest of two adjoining lobes, and the 'bridge' between the two lobes. "
            "A value of 1.0 will never merge two peaks into a single lobe; a value of 0.0 will always merge lobes unless they are bisected by a zero crossing.")
    + App::Argument ("value").type_float (0.0, FMLS_RATIO_TO_PEAK_VALUE_DEFAULT, 1.0);




void load_fmls_thresholds (Segmenter& segmenter)
{

  using namespace App;

  Options opt = get_options ("fmls_no_thresholds");
  const bool no_thresholds = opt.size();
  if (no_thresholds) {
    segmenter.set_ratio_to_negative_lobe_integral (0.0f);
    segmenter.set_ratio_to_negative_lobe_mean_peak (0.0f);
    segmenter.set_peak_value_threshold (0.0f);
  }

  opt = get_options ("fmls_ratio_integral_to_neg");
  if (opt.size()) {
    if (no_thresholds) {
      WARN ("Option -fmls_ratio_integral_to_neg ignored: -fmls_no_thresholds overrides this");
    } else {
      segmenter.set_ratio_to_negative_lobe_integral (float(opt[0][0]));
    }
  }

  opt = get_options ("fmls_ratio_peak_to_mean_neg");
  if (opt.size()) {
    if (no_thresholds) {
      WARN ("Option -fmls_ratio_peak_to_mean_neg ignored: -fmls_no_thresholds overrides this");
    } else {
      segmenter.set_ratio_to_negative_lobe_mean_peak (float(opt[0][0]));
    }
  }

  opt = get_options ("fmls_peak_value");
  if (opt.size()) {
    if (no_thresholds) {
      WARN ("Option -fmls_peak_value ignored: -fmls_no_thresholds overrides this");
    } else {
      segmenter.set_peak_value_threshold (float(opt[0][0]));
    }
  }

  opt = get_options ("fmls_peak_ratio_to_merge");
  if (opt.size())
    segmenter.set_ratio_of_peak_value_to_merge (float(opt[0][0]));

}










Segmenter::Segmenter (const DWI::Directions::Set& directions, const size_t l) :
      dirs                             (directions),
      lmax                             (l),
      transform                        (NULL),
      precomputer                      (new Math::SH::PrecomputedAL<float> (lmax, 2 * dirs.size())),
      ratio_to_negative_lobe_integral  (FMLS_RATIO_TO_NEGATIVE_LOBE_INTEGRAL_DEFAULT),
      ratio_to_negative_lobe_mean_peak (FMLS_RATIO_TO_NEGATIVE_LOBE_MEAN_PEAK_DEFAULT),
      peak_value_threshold             (FMLS_PEAK_VALUE_THRESHOLD),
      ratio_of_peak_value_to_merge     (FMLS_RATIO_TO_PEAK_VALUE_DEFAULT),
      create_null_lobe                 (false),
      create_lookup_table              (true),
      dilate_lookup_table              (false)
{
  Math::Matrix<float> az_el_pairs (dirs.size(), 2);
  for (size_t row = 0; row != dirs.size(); ++row) {
    const Point<float> d (dirs.get_dir (row));
    az_el_pairs (row, 0) = std::atan2 (d[1], d[0]);
    az_el_pairs (row, 1) = std::acos  (d[2]);
  }
  transform = new Math::SH::Transform<float> (az_el_pairs, lmax);

}




class Max_abs {
  public:
    bool operator() (const float& a, const float& b) const { return (std::abs (a) > std::abs (b)); }
};

bool Segmenter::operator() (const SH_coefs& in, FOD_lobes& out) const {

  assert (in.size() == Math::SH::NforL (lmax));

  out.clear();
  out.vox = in.vox;

  if (in[0] <= 0.0 || !std::isfinite (in[0]))
    return true;

  Math::Vector<float> values (dirs.size());
  transform->SH2A (values, in);

  typedef std::multimap<float, dir_t, Max_abs> map_type;
  map_type data_in_order;
  for (size_t i = 0; i != values.size(); ++i)
    data_in_order.insert (std::make_pair (values[i], i));

  if (data_in_order.begin()->first <= 0.0)
    return true;

  std::vector< std::pair<dir_t, uint32_t> > retrospective_assignments;

  std::map<uint32_t, uint32_t> lobes_to_merge;

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

      if (std::abs (i->first) / out[adj_lobes.back()].get_peak_value() > ratio_of_peak_value_to_merge) {

        if (lobes_to_merge.find (adj_lobes.back()) == lobes_to_merge.end())
          lobes_to_merge.insert (std::make_pair (adj_lobes.back(), adj_lobes.front()));
        out[adj_lobes.front()].add (i->second, i->first);

      } else {

        retrospective_assignments.push_back (std::make_pair (i->second, adj_lobes.front()));

      }

    }

  }

  for (std::vector< std::pair<dir_t, uint32_t> >::const_iterator i = retrospective_assignments.begin(); i != retrospective_assignments.end(); ++i)
    out[i->second].add (i->first, values[i->first]);

  for (std::map<uint32_t, uint32_t>::const_reverse_iterator i = lobes_to_merge.rbegin(); i != lobes_to_merge.rend(); ++i) {
    out[i->second].merge (out[i->first]);
    std::vector<FOD_lobe>::iterator ptr = out.begin();
    advance (ptr, i->first);
    out.erase (ptr);
  }


  float mean_neg_peak = 0.0, max_neg_integral = 0.0;
  uint32_t neg_lobe_count = 0;
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
      i->finalise();
#ifdef FMLS_OPTIMISE_MEAN_DIR
      optimise_mean_dir (*i);
#endif
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

      NON_POD_VLA (new_assignments, std::vector<uint32_t>, dirs.size());
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

            uint32_t best_lobe = 0;
            float max_integral = 0.0;
            for (std::vector<uint32_t>::const_iterator lobe_no = new_assignments[dir].begin(); lobe_no != new_assignments[dir].end(); ++lobe_no) {
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



#ifdef FMLS_OPTIMISE_MEAN_DIR
void Segmenter::optimise_mean_dir (FOD_lobe& lobe) const
{

  // For algorithm details see:
  // Buss, Samuel R. and Fillmore, Jay P.
  // Spherical averages and applications to spherical splines and interpolation.
  // ACM Trans. Graph. 2001:20;95-126.

  Point<float> mean_dir = lobe.get_mean_dir(); // Initial estimate

  Point<float> u; // Update step

  do {

    // Axes on the tangent hyperplane for this iteration of optimisation
    Point<float> Tx, Ty, Tz;
    Tx = Point<float>(0.0, 0.0, 1.0).cross (mean_dir);
    Tx.normalise();
    if (!Tx) {
      Tx = Point<float>(0.0, 1.0, 0.0).cross (mean_dir);
      Tx.normalise();
    }
    Ty = mean_dir.cross (Tx);
    Ty.normalise();
    Tz = mean_dir;

    float sum_weights = 0.0;
    u.set (float(0.0), float(0.0), float(0.0));

    for (dir_t d = 0; d != dirs.size(); ++d) {
      if (lobe.get_values()[d]) {

        const Point<float>& dir (dirs[d]);

        // Transform unit direction onto tangent plane defined by the current mean direction estimate
        Point<float> p (dir[0]*Tx[0] + dir[1]*Tx[1] + dir[2]*Tx[2],
                        dir[0]*Ty[0] + dir[1]*Ty[1] + dir[2]*Ty[2],
                        dir[0]*Tz[0] + dir[1]*Tz[1] + dir[2]*Tz[2]);

        if (p[2] < 0.0)
          p = -p;
        p[2] = 0.0; // Force projection onto the tangent plane

        const float dp = std::abs (mean_dir.dot (dir));
        const float theta = (dp < 1.0) ? std::acos (dp) : 0.0;
        const float log_transform = theta ? (theta / std::sin (theta)) : 1.0;
        p *= log_transform;

        u += lobe.get_values()[d] * p;
        sum_weights += lobe.get_values()[d];

      }
    }

    u *= (1.0 / sum_weights);

    const float r = u.norm();
    const float exp_transform = r ? (std::sin(r) / r) : 1.0;
    u *= exp_transform;

    // Transform the offset from the tangent plane origin to euclidean space
    u.set (u[0]*Tx[0] + u[1]*Ty[0] + u[2]*Tz[0],
           u[0]*Tx[1] + u[1]*Ty[1] + u[2]*Tz[1],
           u[0]*Tx[2] + u[1]*Ty[2] + u[2]*Tz[2]);

    mean_dir += u;
    mean_dir.normalise();

  } while (u.norm() > 1e-6);

  lobe.revise_mean_dir (mean_dir);

}
#endif



}
}
}

