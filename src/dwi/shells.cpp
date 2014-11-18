
#include "dwi/shells.h"

#include "file/config.h"


namespace MR
{
  namespace DWI
  {



    using namespace App;

    const OptionGroup ShellOption = OptionGroup ("DW Shell selection options")
      + Option ("shell",
          "specify one or more diffusion-weighted gradient shells to use during "
          "processing, as a comma-separated list of the desired approximate b-values. "
          "Note that some commands are incompatible with multiple shells, and "
          "will throw an error if more than one b-value are provided.")
        + Argument ("list").type_sequence_float();



    const float bzero_threshold = File::Config::get_float ("BZeroThreshold", 10.0);




    Shell::Shell (const Math::Matrix<float>& grad, const std::vector<size_t>& indices) :
        volumes (indices),
        mean (0.0),
        stdev (0.0),
        min (std::numeric_limits<float>::max()),
        max (0.0)
    {
      assert (volumes.size());
      for (std::vector<size_t>::const_iterator i = volumes.begin(); i != volumes.end(); i++) {
        const float b = grad (*i, 3);
        mean += b;
        min = std::min (min, b);
        max = std::max (min, b);
      }
      mean /= float(volumes.size());
      for (std::vector<size_t>::const_iterator i = volumes.begin(); i != volumes.end(); i++)
        stdev += Math::pow2 (grad (*i, 3) - mean);
      stdev = std::sqrt (stdev / (volumes.size() - 1));
    }






    void Shells::select_shells (const bool keep_bzero, const bool force_single_shell)
    {

      // Easiest way to restrict processing to particular shells is to simply erase
      //   the unwanted shells; makes it command independent

      BitSet to_retain (shells.size(), false);
      if (keep_bzero && smallest().is_bzero())
        to_retain[0] = true;

      App::Options opt = App::get_options ("shell");
      if (opt.size()) {

        std::vector<float> desired_bvalues = opt[0][0];
        if (desired_bvalues.size() > 1 && !(desired_bvalues.front() == 0) && force_single_shell)
          throw Exception ("Command not compatible with multiple non-zero b-shells");

        for (std::vector<float>::const_iterator b = desired_bvalues.begin(); b != desired_bvalues.end(); ++b) {

          if (*b < 0)
            throw Exception ("Cannot select shells corresponding to negative b-values");

          // Automatically select a b=0 shell if the requested b-value is zero
          if (*b <= bzero_threshold) {

            if (smallest().is_bzero()) {
              to_retain[0] = true;
              DEBUG ("User requested b-value " + str(*b) + "; got b=0 shell : " + str(smallest().get_mean()) + " +- " + str(smallest().get_stdev()) + " with " + str(smallest().count()) + " volumes");
            } else {
              throw Exception ("User selected b=0 shell, but no such data exists");
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
            for (size_t s = 0; s != shells.size(); ++s) {
              if ((*b >= shells[s].get_min()) && (*b <= shells[s].get_max())) {
                to_retain[s] = true;
                shell_selected = true;
                DEBUG ("User requested b-value " + str(*b) + "; got shell " + str(s) + ": " + str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev()) + " with " + str(shells[s].count()) + " volumes");
              }
            }
            if (!shell_selected) {

              // First, check to see if all non-zero shells have a non-zero standard deviation
              bool zero_stdev = false;
              for (std::vector<Shell>::const_iterator s = shells.begin(); s != shells.end(); ++s) {
                if (!s->is_bzero() && !s->get_stdev()) {
                  zero_stdev = true;
                  break;
                }
              }

              size_t best_shell = 0;
              float best_num_stdevs = std::numeric_limits<float>::max();
              bool ambiguous = false;
              for (size_t s = 0; s != shells.size(); ++s) {
                const float num_stdev = std::abs ((*b - shells[s].get_mean()) / (zero_stdev ? std::sqrt (shells[s].get_mean()) : shells[s].get_stdev()));
                const bool within_range = (num_stdev < (zero_stdev ? 1.0 : 5.0));
                if (within_range) {
                  if (!shell_selected) {
                    best_shell = s;
                    best_num_stdevs = num_stdev;
                  } else {
                    // More than one shell plausible; decide whether or not the decision is unambiguous
                    if (num_stdev < 0.1 * best_num_stdevs) {
                      best_shell = s;
                      best_num_stdevs = num_stdev;
                    } else if (num_stdev < 10.0 * best_num_stdevs) {
                      ambiguous = true;
                    } // No need to do anything if the existing selection is significantly better than this shell
                  }
                }
              }

              if (ambiguous) {
                std::string bvalues;
                for (size_t s = 0; s != shells.size(); ++s) {
                  if (bvalues.size())
                    bvalues += ", ";
                  bvalues += str(shells[s].get_mean()) + " +- " + str(shells[s].get_stdev());
                }
                throw Exception ("Unable to robustly select desired shell b=" + str(*b) + " (detected shells are: " + bvalues + ")");
              }

              WARN ("User requested shell b=" + str(*b) + "; have selected shell " + str(shells[best_shell].get_mean()) + " +- " + str(shells[best_shell].get_stdev()));

              to_retain[best_shell] = true;

            }

          } // End checking to see if requested shell is b=0

        } // End looping over list of requested b-value shells

      } else {

        if (force_single_shell && !is_single_shell())
          WARN ("Multiple non-zero b-value shells detected; automatically selecting b=" + str(largest().get_mean()) + " with " + str(largest().count()) + " volumes");
        to_retain[shells.size()-1] = true;

      }

      if (to_retain.full()) {
        DEBUG ("No DW shells to be removed");
        return;
      }

      // Erase the unwanted shells
      std::vector<Shell> new_shells;
      for (size_t s = 0; s != shells.size(); ++s) {
        if (to_retain[s])
          new_shells.push_back (shells[s]);
      }
      shells.swap (new_shells);

    }




    void Shells::reject_small_shells (const size_t min_volumes)
    {
      for (std::vector<Shell>::iterator s = shells.begin(); s != shells.end();) {
        if (!s->is_bzero() && s->count() < min_volumes)
          s = shells.erase (s);
        else
          ++s;
      }
    }




    void Shells::initialise (const Math::Matrix<float>& grad)
    {
      BValueList bvals (grad.column (3));
      std::vector<size_t> clusters (bvals.size(), 0);
      const size_t num_shells = clusterBvalues (bvals, clusters);

      if ((num_shells < 1) || (num_shells > std::sqrt (float(grad.rows()))))
        throw Exception ("Gradient encoding matrix does not represent a HARDI sequence");

      for (size_t shellIdx = 0; shellIdx <= num_shells; shellIdx++) {

        std::vector<size_t> volumes;
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

      INFO ("Diffusion gradient encoding data clustered into " + str(count()) + " shells with volumes counts = " + str(get_counts()) + ", b-values = " + str(get_bvalues()) + "");
    }




    size_t Shells::clusterBvalues (const BValueList& bvals, std::vector<size_t>& clusters) const
    {
      BitSet visited (bvals.size(), false);
      size_t clusterIdx = 0;

      for (size_t ii = 0; ii != bvals.size(); ii++) {
        if (!visited[ii]) {

          visited[ii] = true;
          const float b = bvals[ii];
          std::vector<size_t> neighborIdx;
          regionQuery (bvals, b, neighborIdx);

          if (b > bzero_threshold && neighborIdx.size() < DWI_SHELLS_MIN_LINKAGE) {

            clusters[ii] = 0;

          } else {

            clusters[ii] = ++clusterIdx;
            for (size_t i = 0; i < neighborIdx.size(); i++) {
              if (!visited[neighborIdx[i]]) {
                visited[neighborIdx[i]] = true;
                std::vector<size_t> neighborIdx2;
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



    void Shells::regionQuery (const BValueList& bvals, const float b, std::vector<size_t>& idx) const
    {
      for (size_t i = 0; i < bvals.size(); i++) {
        if (std::abs (b - bvals[i]) < DWI_SHELLS_EPSILON)
          idx.push_back (i);
      }
    }



  }
}


