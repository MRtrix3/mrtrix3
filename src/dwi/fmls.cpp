/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/fmls.h"

#include <set>



namespace MR {
  namespace DWI {
    namespace FMLS {





      const App::OptionGroup FMLSSegmentOption = App::OptionGroup ("FOD FMLS segmenter options")

        + App::Option ("fmls_integral",
            "threshold absolute numerical integral of positive FOD lobes. "
            "Any lobe for which the integral is smaller than this threshold will be discarded. "
            "Default: " + str(FMLS_INTEGRAL_THRESHOLD_DEFAULT, 2) + ".")
        + App::Argument ("value").type_float (0.0)

        + App::Option ("fmls_peak_value",
            "threshold peak amplitude of positive FOD lobes. "
            "Any lobe for which the maximal peak amplitude is smaller than this threshold will be discarded. "
            "Default: " + str(FMLS_PEAK_VALUE_THRESHOLD_DEFAULT, 2) + ".")
        + App::Argument ("value").type_float (0.0)

        + App::Option ("fmls_no_thresholds",
            "disable all FOD lobe thresholding; every lobe where the FOD is positive will be retained.")

        + App::Option ("fmls_lobe_merge_ratio",
            "Specify the ratio between a given FOD amplitude sample between two lobes, and the smallest peak amplitude of the adjacent lobes, above which those lobes will be merged. "
            "This is the amplitude of the FOD at the 'bridge' point between the two lobes, divided by the peak amplitude of the smaller of the two adjoining lobes. "
            "A value of 1.0 will never merge two lobes into one; a value of 0.0 will always merge lobes unless they are bisected by a zero-valued crossing. "
            "Default: " + str(FMLS_MERGE_RATIO_BRIDGE_TO_PEAK_DEFAULT, 2) + ".")
        + App::Argument ("value").type_float (0.0, 1.0);




      void load_fmls_thresholds (Segmenter& segmenter)
      {
        using namespace App;

        auto opt = get_options ("fmls_no_thresholds");
        const bool no_thresholds = opt.size();
        if (no_thresholds) {
          segmenter.set_integral_threshold (0.0);
          segmenter.set_peak_value_threshold (0.0);
        }

        opt = get_options ("fmls_integral");
        if (opt.size()) {
          if (no_thresholds) {
            WARN ("Option -fmls_integral ignored: -fmls_no_thresholds overrides this");
          } else {
            segmenter.set_integral_threshold (default_type(opt[0][0]));
          }
        }

        opt = get_options ("fmls_peak_value");
        if (opt.size()) {
          if (no_thresholds) {
            WARN ("Option -fmls_peak_value ignored: -fmls_no_thresholds overrides this");
          } else {
            segmenter.set_peak_value_threshold (default_type(opt[0][0]));
          }
        }

        opt = get_options ("fmls_merge_ratio");
        if (opt.size())
          segmenter.set_lobe_merge_ratio (default_type(opt[0][0]));

      }







      Segmenter::Segmenter (const Math::Sphere::Set::Assigner& directions, const size_t lmax) :
          dirs                 (directions),
          lmax                 (lmax),
          transform            (new Math::Sphere::SH::Transform<default_type> (Math::Sphere::Set::to_spherical (directions), lmax)),
          precomputer          (new Math::Sphere::SH::PrecomputedAL<default_type> (lmax, 2 * directions.rows())),
          weights              (new Math::Sphere::Set::Weights (directions)),
          max_num_fixels       (0),
          integral_threshold   (FMLS_INTEGRAL_THRESHOLD_DEFAULT),
          peak_value_threshold (FMLS_PEAK_VALUE_THRESHOLD_DEFAULT),
          lobe_merge_ratio     (FMLS_MERGE_RATIO_BRIDGE_TO_PEAK_DEFAULT),
          calculate_lsq_dir    (false) { }




      class Max_abs {
        public:
          bool operator() (const default_type& a, const default_type& b) const { return (abs (a) > abs (b)); }
      };

      bool Segmenter::operator() (const SH_coefs& in, FOD_lobes& out) const {

        assert (in.size() == ssize_t (Math::Sphere::SH::NforL (lmax)));

        out.clear();
        out.vox = in.vox;

        if (in[0] <= 0.0 || !std::isfinite (in[0]))
          return true;

        Eigen::Matrix<default_type, Eigen::Dynamic, 1> values (dirs.size());
        transform->SH2A (values, in);

        using map_type = std::multimap<default_type, index_type, Max_abs>;

        map_type dixels_in_order;
        for (size_t i = 0; i != size_t(values.size()); ++i)
          dixels_in_order.insert (std::make_pair (values[i], i));

        if (dixels_in_order.begin()->first <= 0.0)
          return true;

        auto can_add = [&] (const default_type amplitude, const index_type dixel_index, const uint32_t lobe_index) -> bool {
          return (((amplitude <= 0.0) && out[lobe_index].is_negative())
                  || ((amplitude > 0.0) && !out[lobe_index].is_negative()))
                  && (dirs.adjacent (out[lobe_index].get_mask(), dixel_index));
        };

        vector<index_type> retrospective_assignments;

        for (const auto& i : dixels_in_order) {

          vector<uint32_t> adj_lobes;
          for (uint32_t l = 0; l != out.size(); ++l) {
            if (can_add (i.first, i.second, l))
              adj_lobes.push_back (l);
          }

          if (adj_lobes.empty()) {

            out.push_back (FOD_lobe (dirs, i.second, i.first, (*weights)[i.second]));

          } else if (adj_lobes.size() == 1) {

            out[adj_lobes.front()].add (dirs, i.second, i.first, (*weights)[i.second]);

          } else if (abs (i.first) / out[adj_lobes.back()].get_max_peak_value() > lobe_merge_ratio) {

            for (size_t j = 1; j != adj_lobes.size(); ++j)
              out[adj_lobes[0]].merge (out[adj_lobes[j]]);

            out[adj_lobes[0]].add (dirs, i.second, i.first, (*weights)[i.second]);

            for (size_t j = adj_lobes.size() - 1; j; --j) {
              vector<FOD_lobe>::iterator ptr = out.begin();
              advance (ptr, adj_lobes[j]);
              out.erase (ptr);
            }

          } else {

            retrospective_assignments.emplace_back (i.second);

          }

        }

        // These dixels were adjacent to multiple fixels during segmentation;
        //   retrospectively choose which to assign them to,
        //   so that that assignment does not itself influence subsequent segmentation
        //   (produces a "seam" between two touching fixels)
        for (auto i : retrospective_assignments) {
          // Unlike previous implementations, we no longer track the fixels adjacent to each retrospective assignment
          // Instead, we will now look at the directions adjacent to this,
          //   see which fixels they were assigned to,
          //   and choose which of them we should assign this direction to.
          // Further, while previous implementations made this decision based on the
          //   fixel with the maximal peak amplitude,
          //   now this will instead be done based on the maximal _adjacent_ amplitude
          const default_type amplitude = values[i];
          default_type max_abs_adj_amplitude = 0.0;
          uint32_t lobe = out.size();
          for (uint32_t l = 0; l != out.size(); ++l) {
            if (can_add (amplitude, i, l)) {
              default_type abs_adj_amplitude = 0.0;
              for (auto d : dirs.adjacency(i))
                abs_adj_amplitude = std::max (abs_adj_amplitude, abs(out[l].get_values()[d]));
              assert (abs_adj_amplitude > 0.0);
              if (abs_adj_amplitude > max_abs_adj_amplitude) {
                max_abs_adj_amplitude = abs_adj_amplitude;
                lobe = l;
              }
            }
          }
          assert (lobe < out.size());
          out[lobe].add (dirs, i, amplitude, (*weights)[i]);
        }

        for (auto i = out.begin(); i != out.end();) { // Empty increment

          if (i->is_negative() || i->get_integral() < integral_threshold) {
            i = out.erase (i);
          } else {

            // Revise multiple peaks if present
            for (size_t peak_index = 0; peak_index != i->num_peaks(); ++peak_index) {
              Eigen::Vector3d newton_peak_dir = i->get_peak_dir (peak_index); // to be updated by subsequent Math::Sphere::SH::get_peak() call
              const default_type newton_peak_value = Math::Sphere::SH::get_peak (in, lmax, newton_peak_dir, &(*precomputer));
              if (std::isfinite (newton_peak_value) && newton_peak_dir.allFinite()) {

                // Ensure that the new peak direction found via Newton optimisation
                //   is still approximately the same direction as that found via FMLS:
                // Also needs to be closer to this peak than any other peaks within the lobe
                default_type max_dp = 0.0;
                size_t nearest_original_peak = i->num_peaks();
                for (size_t j = 0; j != i->num_peaks(); ++j) {
                  const default_type this_dp = abs (newton_peak_dir.dot (i->get_peak_dir (j)));
                  if (this_dp > max_dp) {
                    max_dp = this_dp;
                    nearest_original_peak = j;
                  }
                }
                if (nearest_original_peak == peak_index) {

                  // Needs to still lie within the lobe: Determined via mask
                  const index_type newton_peak_closest_dir_index = dirs (newton_peak_dir);
                  if (i->get_mask()[newton_peak_closest_dir_index])
                    i->revise_peak (peak_index, newton_peak_dir, newton_peak_value);

                }
              }
            }
            if (i->get_max_peak_value() < peak_value_threshold) {
              i = out.erase (i);
            } else {
              i->finalise();
              ++i;
            }
          }
        }

        std::sort (out.begin(), out.end(), [] (const FOD_lobe& a, const FOD_lobe& b) { return (a.get_integral() > b.get_integral()); } );

        if (max_num_fixels && out.size() > max_num_fixels)
          out.erase (out.begin() + max_num_fixels, out.end());

        // Only calculate the least squares direction for fixels being retained;
        //   it'd be a waste of calculations for omitted fixels
        if (calculate_lsq_dir) {
          for (auto& i : out)
            do_lsq_dir (i);
        }

        return true;
      }



      void Segmenter::do_lsq_dir (FOD_lobe& lobe) const
      {

        // For algorithm details see:
        // Buss, Samuel R. and Fillmore, Jay P.
        // Spherical averages and applications to spherical splines and interpolation.
        // ACM Trans. Graph. 2001:20;95-126.

        Eigen::Vector3d lsq_dir = lobe.get_mean_dir(); // Initial estimate

        Eigen::Vector3d u; // Update step

        do {

          // Axes on the tangent hyperplane for this iteration of optimisation
          Eigen::Vector3d Tx, Ty, Tz;
          Tx = Eigen::Vector3d(0.0, 0.0, 1.0).cross (lsq_dir).normalized();
          if (!Tx.allFinite())
            Tx = Eigen::Vector3d(0.0, 1.0, 0.0).cross (lsq_dir).normalized();
          Ty = lsq_dir.cross (Tx).normalized();
          Tz = lsq_dir;

          default_type sum_weights = 0.0;
          u = {0.0, 0.0, 0.0};

          for (index_type d = 0; d != dirs.size(); ++d) {
            if (lobe.get_values()[d]) {

              const auto dir (dirs[d]);

              // Transform unit direction onto tangent plane defined by the current mean direction estimate
              Eigen::Vector3d p (dir[0]*Tx[0] + dir[1]*Tx[1] + dir[2]*Tx[2],
                                 dir[0]*Ty[0] + dir[1]*Ty[1] + dir[2]*Ty[2],
                                 dir[0]*Tz[0] + dir[1]*Tz[1] + dir[2]*Tz[2]);

              if (p[2] < 0.0)
                p = -p;
              p[2] = 0.0; // Force projection onto the tangent plane

              const default_type dp = abs (lsq_dir.dot (dir));
              const default_type theta = (dp < 1.0) ? std::acos (dp) : 0.0;
              const default_type log_transform = theta ? (theta / std::sin (theta)) : 1.0;
              p *= log_transform;

              u += lobe.get_values()[d] * p;
              sum_weights += lobe.get_values()[d];

            }
          }

          u *= (1.0 / sum_weights);

          const default_type r = u.norm();
          const default_type exp_transform = r ? (std::sin(r) / r) : 1.0;
          u *= exp_transform;

          // Transform the offset from the tangent plane origin to euclidean space
          u = { u[0]*Tx[0] + u[1]*Ty[0] + u[2]*Tz[0],
                u[0]*Tx[1] + u[1]*Ty[1] + u[2]*Tz[1],
                u[0]*Tx[2] + u[1]*Ty[2] + u[2]*Tz[2] };

          lsq_dir += u;
          lsq_dir.normalize();

        } while (u.norm() > 1e-6);

        lobe.set_lsq_dir (lsq_dir);

      }



    }
  }
}
