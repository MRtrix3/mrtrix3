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


#include "debug.h"

#include "math/math.h"
#include "dwi/shells.h"



namespace MR
{
  namespace DWI
  {

    const App::OptionGroup ShellsOption = App::OptionGroup ("DW shell selection options")
      + App::Option ("shells",
          "specify one or more diffusion-weighted gradient shells to use during "
          "processing, as a comma-separated list of the desired approximate b-values. "
          "Note that some commands are incompatible with multiple shells, and "
          "will throw an error if more than one b-value is provided.")
        + App::Argument ("list").type_sequence_float();






    Shell::Shell (const Eigen::MatrixXd& grad, const vector<size_t>& indices) :
        volumes (indices),
        mean (0.0),
        stdev (0.0),
        min (std::numeric_limits<default_type>::max()),
        max (0.0)
    {
      assert (volumes.size());
      for (vector<size_t>::const_iterator i = volumes.begin(); i != volumes.end(); i++) {
        const default_type b = grad (*i, 3);
        mean += b;
        min = std::min (min, b);
        max = std::max (min, b);
      }
      mean /= default_type(volumes.size());
      for (vector<size_t>::const_iterator i = volumes.begin(); i != volumes.end(); i++)
        stdev += Math::pow2 (grad (*i, 3) - mean);
      stdev = std::sqrt (stdev / (volumes.size() - 1));
    }





    // Use to select the shells the user may have requested via the "-shell" option,
    // and additionally enforce certain constraints related to the presence of shells.
    //
    // The optional constraints are:
    // - force_singleshell: enforces the presence of exactly 1 (no more, no less) non-bzero shell;
    //                      doesn't influence the presence (or not) of bzeros, i.e., if they're present,
    //                      they're simply left in.
    // - force_with_bzero: enforces the presence of bzeros.
    // - force_without_bzero: enforces the absence of bzeros; typical usecase would be in conjunction
    //                        with "force_singleshell", to obtain a "pure" single shell.
    //
    // When the "-shell" option is present, it is first applied, and next the constraints (if any are requested)
    // are checked. If any constraint is violated, an error (exception) occurs. The logic is that the user's
    // "-shell" selection is a conscious choice that should always be honoured, and no unexpected modifications
    // (e.g. including the bzeros) are made to it.
    //
    // When the "-shell" option is not present, by default all shells are included. If constraints are requested,
    // they are made true for the convenience of the user, if possible (e.g. just selecting the highest b-value
    // shell if "force_singleshell" is requested, excluding the bzeros if "force_without_bzero" is present, etc.).
    // However, if they simply can't be made true (e.g. "force_with_bzero" when there are no bzeros in the dataset),
    // an error (exception) will still occur.
    //
    // So long story short: multi-shell (as in "all shells") is default, but constraints can be specified
    // if strictly required for certain commands or algorithms.
    Shells& Shells::select_shells (const bool force_singleshell, const bool force_with_bzero, const bool force_without_bzero)
    {

      // Easiest way to restrict processing to particular shells is to simply erase
      //   the unwanted shells; makes it command independent

      if (force_without_bzero && force_with_bzero)
        throw Exception ("Incompatible constraints: command tries to enforce proceeding both with and without b=0");

      BitSet to_retain (count(), false);

      auto opt = App::get_options ("shells");
      if (opt.size()) {

        vector<default_type> desired_bvalues = opt[0][0];
        bool bzero_selected = false;
        size_t nonbzero_selected_count = 0;

        for (vector<default_type>::const_iterator b = desired_bvalues.begin(); b != desired_bvalues.end(); ++b) {

          if (*b < 0)
            throw Exception ("Cannot select shells corresponding to negative b-values");

          // Automatically select a b=0 shell if the requested b-value is zero
          if (*b <= bzero_threshold()) {

            if (has_bzero()) {
              if (!bzero_selected) {
                to_retain[0] = true;
                bzero_selected = true;
                DEBUG ("User requested b-value " + str(*b) + "; got b=0 shell : " + str(smallest().get_mean()) + " +- " + str(smallest().get_stdev()) + " with " + str(smallest().count()) + " volumes");
              } else {
                throw Exception ("User selected b=0 shell more than once");
              }
            } else {
              throw Exception ("User selected b=0 shell, but no such data was found");
            }

          } else {

            // First, see if the b-value lies within the range of one of the shells
            // If this doesn't occur, need to make a decision based on the shell distributions
            // Few ways this could be done:
            // * Compute number of standard deviations away from each shell mean, see if there's a clear winner
            //   - Won't work if any of the standard deviations are zero
            // * Assume each is a Poisson distribution, see if there's a clear winner
            // Prompt warning if decision is slightly askew, exception if ambiguous
            bool shell_selected = false;
            for (size_t s = 0; s != count(); ++s) {
              if ((*b >= shells[s].get_min()) && (*b <= shells[s].get_max())) {
                if (!to_retain[s]) {
                  to_retain[s] = true;
                  nonbzero_selected_count++;
                  shell_selected = true;
                  DEBUG ("User requested b-value " + str(*b) + "; got shell " + str(s) + ": " + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()) + " with " + str(shells[s].count()) + " volumes");
                } else {
                  throw Exception ("User selected a shell more than once: " + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()) + " with " + str(shells[s].count()) + " volumes");
                }
              }
            }
            if (!shell_selected) {

              // Check to see if we can unambiguously select a shell based on b-value integer rounding
              size_t best_shell = 0;
              bool ambiguous = false;
              for (size_t s = 0; s != count(); ++s) {
                if (std::abs (*b - shells[s].get_mean()) <= 1.0) {
                  if (shell_selected) {
                    ambiguous = true;
                  } else {
                    best_shell = s;
                    shell_selected = true;
                  }
                }
              }
              if (shell_selected && !ambiguous) {
                if (!to_retain[best_shell]) {
                  to_retain[best_shell] = true;
                  nonbzero_selected_count++;
                  DEBUG ("User requested b-value " + str(*b) + "; got shell " + str(best_shell) + ": " + str(shells[best_shell].get_mean()) + " +- " + str(shells[best_shell].get_stdev()) + " with " + str(shells[best_shell].count()) + " volumes");
                } else {
                  throw Exception ("User selected a shell more than once: " + str(shells[best_shell].get_mean()) + " +- " + str(shells[best_shell].get_stdev()) + " with " + str(shells[best_shell].count()) + " volumes");
                }
              } else {

                // First, check to see if all non-zero shells have (effectively) non-zero standard deviation
                // (If one non-zero shell has negligible standard deviation, assume a Poisson distribution for all shells)
                bool zero_stdev = false;
                for (vector<Shell>::const_iterator s = shells.begin(); s != shells.end(); ++s) {
                  if (!s->is_bzero() && s->get_stdev() < 1.0) {
                    zero_stdev = true;
                    break;
                  }
                }

                size_t best_shell = 0;
                default_type best_num_stdevs = std::numeric_limits<default_type>::max();
                bool ambiguous = false;
                for (size_t s = 0; s != count(); ++s) {
                  const default_type stdev = (shells[s].is_bzero() ? 0.5 * bzero_threshold() : (zero_stdev ? std::sqrt (shells[s].get_mean()) : shells[s].get_stdev()));
                  const default_type num_stdev = std::abs ((*b - shells[s].get_mean()) / stdev);
                  if (num_stdev < best_num_stdevs) {
                    ambiguous = (num_stdev >= 0.1 * best_num_stdevs);
                    best_shell = s;
                    best_num_stdevs = num_stdev;
                  } else {
                    ambiguous = (num_stdev < 10.0 * best_num_stdevs);
                  }
                }

                if (ambiguous) {
                  std::string bvalues;
                  for (size_t s = 0; s != count(); ++s) {
                    if (bvalues.size())
                      bvalues += ", ";
                    bvalues += str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev());
                  }
                  throw Exception ("Unable to robustly select desired shell b=" + str(*b) + " (detected shells are: " + bvalues + ")");
                } else {
                  WARN ("User requested shell b=" + str(*b) + "; have selected nearby shell " + str(shells[best_shell].get_mean()) + " +- " + str(shells[best_shell].get_stdev()));
                  if (!to_retain[best_shell]) {
                    to_retain[best_shell] = true;
                    nonbzero_selected_count++;
                  } else {
                    throw Exception ("User selected a shell more than once: " + str(shells[best_shell].get_mean()) + " +- " + str(shells[best_shell].get_stdev()) + " with " + str(shells[best_shell].count()) + " volumes");
                  }
                }

              } // End checking if the requested b-value is within 1.0 of a shell mean

            } // End checking if the shell can be selected because of lying within the numerical range of a shell

          } // End checking to see if requested shell is b=0

        } // End looping over list of requested b-value shells

        if (force_singleshell && nonbzero_selected_count != 1)
          throw Exception ("User selected " + str(nonbzero_selected_count) + " non b=0 shells, but the command requires single-shell data");

        if (force_with_bzero && !bzero_selected)
          throw Exception ("User did not select b=0 shell, but the command requires the presence of b=0 data");

        if (force_without_bzero && bzero_selected)
          throw Exception ("User selected b=0 shell, but the command is not compatible with b=0 data");

      } else {

        if (force_singleshell && !is_single_shell()) {
          if (count() == 1 && has_bzero())
            throw Exception ("No non b=0 data found, but the command requires a non b=0 shell");
          WARN ("Multiple non-zero b-value shells detected, automatically selecting largest b-value: b=" + str(largest().get_mean()) + " with " + str(largest().count()) + " volumes");
          to_retain[count()-1] = true;
          if (has_bzero())
            to_retain[0] = true;
        } else {
          // default: keep everything
          to_retain.clear (true);
        }

        if (force_with_bzero && !has_bzero())
          throw Exception ("No b=0 data found, but the command requires the presence of b=0 data");

        if (force_without_bzero && has_bzero())
          to_retain[0] = false;

      }

      if (to_retain.full()) {
        DEBUG ("No DW shells to be removed");
        return *this;
      }

      // Erase the unwanted shells
      vector<Shell> new_shells;
      for (size_t s = 0; s != count(); ++s) {
        if (to_retain[s])
          new_shells.push_back (shells[s]);
      }
      shells.swap (new_shells);

      return *this;
    }




    Shells& Shells::reject_small_shells (const size_t min_volumes)
    {
      for (vector<Shell>::iterator s = shells.begin(); s != shells.end();) {
        if (!s->is_bzero() && s->count() < min_volumes)
          s = shells.erase (s);
        else
          ++s;
      }
      return *this;
    }




    Shells::Shells (const Eigen::MatrixXd& grad)
    {
      BValueList bvals = grad.col (3);
      vector<size_t> clusters (bvals.size(), 0);
      const size_t num_shells = clusterBvalues (bvals, clusters);

      if ((num_shells < 1) || (num_shells > std::sqrt (default_type(grad.rows()))))
        throw Exception ("DWI volumes could not be classified into b-value shells; gradient encoding may not represent a HARDI sequence");

      for (size_t shellIdx = 0; shellIdx <= num_shells; shellIdx++) {

        vector<size_t> volumes;
        for (size_t volumeIdx = 0; volumeIdx != clusters.size(); ++volumeIdx) {
          if (clusters[volumeIdx] == shellIdx)
            volumes.push_back (volumeIdx);
        }

        if (shellIdx) {
          shells.push_back (Shell (grad, volumes));
        } else if (volumes.size()) {
          std::string unassigned;
          for (size_t i = 0; i != volumes.size(); ++i) {
            if (unassigned.size())
              unassigned += ", ";
            unassigned += str(volumes[i]) + " (" + str(bvals[volumes[i]]) + ")";
          }
          WARN ("The following image volumes were not successfully assigned to a b-value shell:");
          WARN (unassigned);
        }

      }

      std::sort (shells.begin(), shells.end());

      if (smallest().is_bzero()) {
        INFO ("Diffusion gradient encoding data clustered into " + str(num_shells - 1) + " non-zero shells and " + str(smallest().count()) + " b=0 volumes");
      } else {
        INFO ("Diffusion gradient encoding data clustered into " + str(num_shells) + " shells (no b=0 volumes)");
      }
      DEBUG ("Shells: b = { " +
          str ([&]{ std::string m; for (auto& s : shells) m += str(s.get_mean()) + "(" + str(s.count()) + ") "; return m; }())
          + "}");
    }




    size_t Shells::clusterBvalues (const BValueList& bvals, vector<size_t>& clusters) const
    {
      BitSet visited (bvals.size(), false);
      size_t clusterIdx = 0;

      for (ssize_t ii = 0; ii != bvals.size(); ii++) {
        if (!visited[ii]) {

          visited[ii] = true;
          const default_type b = bvals[ii];
          vector<size_t> neighborIdx;
          regionQuery (bvals, b, neighborIdx);

          if (b > bzero_threshold() && neighborIdx.size() < DWI_SHELLS_MIN_LINKAGE) {

            clusters[ii] = 0;

          } else {

            clusters[ii] = ++clusterIdx;
            for (size_t i = 0; i < neighborIdx.size(); i++) {
              if (!visited[neighborIdx[i]]) {
                visited[neighborIdx[i]] = true;
                vector<size_t> neighborIdx2;
                regionQuery (bvals, bvals[neighborIdx[i]], neighborIdx2);
                if (neighborIdx2.size() >= DWI_SHELLS_MIN_LINKAGE)
                  for (size_t j = 0; j != neighborIdx2.size(); j++)
                    neighborIdx.push_back (neighborIdx2[j]);
              }
              if (clusters[neighborIdx[i]] == 0)
                clusters[neighborIdx[i]] = clusterIdx;
            }

          }

        }
      }

      return clusterIdx;
    }



    void Shells::regionQuery (const BValueList& bvals, const default_type b, vector<size_t>& idx) const
    {
      for (ssize_t i = 0; i < bvals.size(); i++) {
        if (std::abs (b - bvals[i]) < DWI_SHELLS_EPSILON)
          idx.push_back (i);
      }
    }



  }
}


