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





      IntegrationWeights::IntegrationWeights (const Eigen::MatrixXd& dirs) :
          data (dirs.size())
      {
        const size_t calibration_lmax = Math::SH::LforN (dirs.rows()) + 2;
        auto calibration_SH2A = Math::SH::init_transform (Math::Sphere::to_spherical (dirs), calibration_lmax);
        const size_t num_basis_fns = calibration_SH2A.cols();

        // Integrating an FOD with constant amplitude 1 (l=0 term = sqrt(4pi) should produce a value of 4pi
        // Every other integral should produce zero
        Eigen::Matrix<default_type, Eigen::Dynamic, 1> integral_results = Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero (num_basis_fns);
        integral_results[0] = 2.0 * sqrt(Math::pi);

        // Problem matrix: One row for each SH basis function, one column for each samping direction
        Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> A;
        A.resize (num_basis_fns, dirs.rows());

        for (size_t basis_fn_index = 0; basis_fn_index != num_basis_fns; ++basis_fn_index) {
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> SH_in = Eigen::Matrix<default_type, Eigen::Dynamic, 1>::Zero (num_basis_fns);
          SH_in[basis_fn_index] = 1.0;
          A.row (basis_fn_index) = calibration_SH2A * SH_in;
        }

        data = A.householderQr().solve (integral_results);
      }










      Segmenter::Segmenter (const DWI::Directions::Assigner& directions, const size_t lmax) :
          dirs                 (directions),
          lmax                 (lmax),
          transform            (new Math::SH::Transform<default_type> (Math::Sphere::to_spherical (directions), lmax)),
          precomputer          (new Math::SH::PrecomputedAL<default_type> (lmax, 2 * directions.rows())),
          weights              (new IntegrationWeights (directions)),
          max_num_fixels       (0),
          integral_threshold   (FMLS_INTEGRAL_THRESHOLD_DEFAULT),
          peak_value_threshold (FMLS_PEAK_VALUE_THRESHOLD_DEFAULT),
          lobe_merge_ratio     (FMLS_MERGE_RATIO_BRIDGE_TO_PEAK_DEFAULT),
          calculate_lsq_dir    (false),
          create_lookup_table  (true) {}




      class Max_abs {
        public:
          bool operator() (const default_type& a, const default_type& b) const { return (abs (a) > abs (b)); }
      };

      bool Segmenter::operator() (const SH_coefs& in, FOD_lobes& out) const {

        assert (in.size() == ssize_t (Math::SH::NforL (lmax)));

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

        vector< std::pair<index_type, uint32_t> > retrospective_assignments;

        for (const auto& i : dixels_in_order) {

          vector<uint32_t> adj_lobes;
          for (uint32_t l = 0; l != out.size(); ++l) {
            if ((((i.first <= 0.0) &&  out[l].is_negative())
                  || ((i.first >  0.0) && !out[l].is_negative()))
                  && (dirs.adjacency (out[l].get_mask(), i.second))) {

              adj_lobes.push_back (l);

            }
          }

          if (adj_lobes.empty()) {

            out.push_back (FOD_lobe (dirs, i.second, i.first, (*weights)[i.second]));

          } else if (adj_lobes.size() == 1) {

            out[adj_lobes.front()].add (dirs, i.second, i.first, (*weights)[i.second]);

          } else {

            // Changed handling of lobe merges
            // Merge lobes as they appear to be merged, but update the
            //   contents of retrospective_assignments accordingly
            if (abs (i.first) / out[adj_lobes.back()].get_max_peak_value() > lobe_merge_ratio) {

              std::sort (adj_lobes.begin(), adj_lobes.end());
              for (size_t j = 1; j != adj_lobes.size(); ++j)
                out[adj_lobes[0]].merge (out[adj_lobes[j]]);
              out[adj_lobes[0]].add (dirs, i.second, i.first, (*weights)[i.second]);
              for (auto j = retrospective_assignments.begin(); j != retrospective_assignments.end(); ++j) {
                bool modified = false;
                for (size_t k = 1; k != adj_lobes.size(); ++k) {
                  if (j->second == adj_lobes[k]) {
                    j->second = adj_lobes[0];
                    modified = true;
                  }
                }
                if (!modified) {
                  // Compensate for impending deletion of elements from the vector
                  uint32_t lobe_index = j->second;
                  for (size_t k = adj_lobes.size() - 1; k; --k) {
                    if (adj_lobes[k] < lobe_index)
                      --lobe_index;
                  }
                  j->second = lobe_index;
                }
              }
              for (size_t j = adj_lobes.size() - 1; j; --j) {
                vector<FOD_lobe>::iterator ptr = out.begin();
                advance (ptr, adj_lobes[j]);
                out.erase (ptr);
              }

            } else {

              retrospective_assignments.push_back (std::make_pair (i.second, adj_lobes.front()));

            }

          }

        }

        for (const auto& i : retrospective_assignments)
          out[i.second].add (dirs, i.first, values[i.first], (*weights)[i.first]);

        for (auto i = out.begin(); i != out.end();) { // Empty increment

          if (i->is_negative() || i->get_integral() < integral_threshold) {
            i = out.erase (i);
          } else {

            // Revise multiple peaks if present
            for (size_t peak_index = 0; peak_index != i->num_peaks(); ++peak_index) {
              Eigen::Vector3d newton_peak_dir = i->get_peak_dir (peak_index); // to be updated by subsequent Math::SH::get_peak() call
              const default_type newton_peak_value = Math::SH::get_peak (in, lmax, newton_peak_dir, &(*precomputer));
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

        // If omitting some number of fixels, should be doing that before the LUT is constructed;
        //   would also be necessary if dilating the LUT
        if (max_num_fixels)
          out.erase (out.begin() + max_num_fixels, out.end());

        // Only calculate the least squares direction for fixels being retained;
        //   it'd be a waste of calculations for omitted fixels
        if (calculate_lsq_dir) {
          for (auto& i : out)
            do_lsq_dir (i);
        }

        if (create_lookup_table) {
          out.lut = lookup_type::Constant (dirs.size(), out.size());
          size_t index = 0;
          for (auto i = out.begin(); i != out.end(); ++i, ++index) {
            const BitSet& this_mask (i->get_mask());
            for (size_t d = 0; d != dirs.size(); ++d) {
              if (this_mask[d])
                out.lut[d] = index;
            }
          }
        } else {
          out.lut.resize (0, 0);
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
