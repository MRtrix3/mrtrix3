/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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



#include "dwi/tractography/SIFT/sifter.h"

#include "dwi/tractography/mapping/loader.h"

#include "timer.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {







      SIFTer::SIFTer (Image::Buffer<float>& i, const DWI::Directions::FastLookupSet& d) :
        MapType (i, d),
        fod_data (i),
        dirs (d),
        fmls (dirs, Math::SH::LforN (fod_data.dim (3))),
        output_debug (false),
        remove_untracked_lobes (false),
        min_FOD_integral (0.0),
        term_number (0),
        term_ratio (0),
        term_mu (std::numeric_limits<double>::max())
      {
        Track_lobe_contribution::set_scaling (fod_data);
        H.info() = fod_data.info();
        H.set_ndim (3);
      }



      SIFTer::~SIFTer ()
      {
        for (std::vector<TckCont*>::iterator i = contributions.begin(); i != contributions.end(); ++i) {
          if (*i) {
            delete *i;
            *i = NULL;
          }
        }
      }



      void SIFTer::perform_FOD_segmentation ()
      {
        FODQueueWriter<Image::Buffer<float>, Image::BufferScratch<float> > writer (fod_data, proc_mask);
        Thread::run_queue_threaded_pipe (writer, DWI::SH_coefs(), fmls, DWI::FOD_lobes(), *this);
      }



      void SIFTer::map_streamlines (const std::string& path)
      {
        Tractography::Properties properties;
        Tractography::Reader<float> file (path, properties);

        if (properties.find ("count") == properties.end())
          throw Exception ("Input .tck file does not specify number of streamlines (run tckfixcount on your .tck file!)");
        const track_t num_tracks = to<track_t>(properties["count"]);

        contributions.assign (num_tracks, NULL);

        Mapping::TrackLoader loader (file, num_tracks);
        Mapping::TrackMapperDixel mapper (H, true, dirs);
        MappedTrackReceiver receiver (*this);
        Thread::run_batched_queue_custom_threading (loader, 1, Mapping::TrackAndIndex(), 100, mapper, 0, SetDixel(), 100, receiver, 0);

        tck_file_path = path;
      }



      void SIFTer::remove_excluded_lobes()
      {

        if (!remove_untracked_lobes && !min_FOD_integral)
          return;

        std::vector<size_t> lobe_index_mapping (lobes.size(), 0);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v);

        lobes.clear();
        lobes.push_back (Lobe());
        FOD_sum = 0.0;

        for (loop.start (v); loop.ok(); loop.next (v)) {
          if (v.value()) {

            size_t new_start_index = lobes.size();

            for (FOD_map<Lobe>::ConstIterator i = begin(v); i; ++i) {
              if ((!remove_untracked_lobes || i().get_TD()) && (i().get_FOD() > min_FOD_integral)) {
                lobe_index_mapping [size_t (i)] = lobes.size();
                lobes.push_back (i());
                FOD_sum += i().get_FOD();
              }
            }

            delete v.value();

            if (lobes.size() == new_start_index)
              v.value() = NULL;
            else
              v.value() = new MapVoxel (new_start_index, lobes.size() - new_start_index);

          }
        }

        TrackIndexRangeWriter writer (TRACK_INDEX_BUFFER_SIZE, num_tracks(), "Removing excluded FOD lobes...");
        LobeRemapper remapper (*this, lobe_index_mapping);
        Thread::run_queue_threaded_sink (writer, TrackIndexRange(), remapper);

        TD_sum = 0.0;
        for (std::vector<TckCont*>::const_iterator i = contributions.begin(); i != contributions.end(); ++i)
          TD_sum += (*i)->get_total_contribution();

      }






      void SIFTer::perform_filtering()
      {

        enum recalc_reason { UNDEFINED, NONLINEARITY, QUANTISATION, TERM_COUNT, TERM_RATIO, TERM_MU, POS_GRADIENT };

        const track_t num_tracks = contributions.size();

        INFO ("Proportionality coefficient at start of filtering is " + str (mu()));

        Ptr<std::ofstream> csv_out;
        if (!csv_path.empty()) {
          csv_out = new std::ofstream();
          csv_out->open (csv_path.c_str(), std::ios_base::trunc);
          (*csv_out) << "Iteration,Removed this iteration,Total removed,Remaining,Cost,TD,Mu,Recalculation,\n";
        }

        // For streamlines that do not contribute to the map, remove an equivalent proportion of length to those that do contribute
        double sum_contributing_length = 0.0, sum_noncontributing_length = 0.0;
        std::vector<track_t> noncontributing_indices;
        for (track_t i = 0; i != contributions.size(); ++i) {
          if (contributions[i]->get_total_contribution()) {
            sum_contributing_length    += contributions[i]->get_total_length();
          } else {
            sum_noncontributing_length += contributions[i]->get_total_length();
            noncontributing_indices.push_back (i);
          }
        }
        double contributing_length_removed = 0.0, noncontributing_length_removed = 0.0;
        // Randomise the order or removal here; faster than trying to select at random later
        std::random_shuffle (noncontributing_indices.begin(), noncontributing_indices.end());

        std::vector<Cost_fn_gradient_sort> gradient_vector;
        gradient_vector.assign (num_tracks, Cost_fn_gradient_sort (num_tracks, std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
        unsigned int tracks_remaining = num_tracks;

        if (tracks_remaining < term_number)
          throw Exception ("Filtering failed; desired number of filtered streamlines is greater than or equal to the size of the input dataset");

        const double init_cf = calc_cost_function();

        if (csv_out)
          (*csv_out) << "0,0,0," << str (tracks_remaining) << "," << str (init_cf) << "," << str (TD_sum) << "," << str (mu()) << ",Start,\n";

        if (App::log_level) {
          fprintf (stderr, "%s:   Iteration     Tracks removed      Tracks remaining      Cost function %%\n", App::NAME.c_str());
          fprintf (stderr, "%s:        0                 0             %9u               %.2f%%   ", App::NAME.c_str(), tracks_remaining, 100.0);
        }

        bool another_iteration = true;
        unsigned int iteration = 0;
        recalc_reason recalculate (UNDEFINED);

        do {

          ++iteration;

          const double current_mu     = mu();
          const double current_cf     = calc_cost_function();
          const double current_roc_cf = calc_roc_cost_function();


          TrackIndexRangeWriter range_writer (TRACK_INDEX_BUFFER_SIZE, num_tracks);
          TrackGradientCalculator gradient_calculator (*this, gradient_vector, current_mu, current_roc_cf);
          Thread::run_queue_threaded_sink (range_writer, TrackIndexRange(), gradient_calculator);


          // Theoretically possible to optimise the sorting block size at execution time
          // * Estimate the smallest possible block size that will not overload the std::set
          // * Simulate a gradient vector using a Gaussian distribution of gradients (ignore length dependence)
          // * Simulate sorting this gradient vector; need to simulate both the sort, and some number of get() calls
          // * Perform a golden section search to find the optimal block size
          // This wasn't implemented as the optimal block size seems pretty stable regardless of gradient vector size

          // Ideally the sorting block size should change dynamically as streamlines are filtered
          // This is to reduce the load on the single-threaded section as the multi-threaded sorting complexity declines
          //   (as more streamlines are no longer present, hence have a null gradient and are excluded from the full sort)
          // Tried for an algebraic solution but the numbers didn't line up with my experiments
          // Trying a heuristic for now; go for a sort size of 1000 following initial sort, assuming half of all
          //   remaining streamlines have a negative gradient

          const track_t sort_size = std::min (num_tracks / double(Thread::number_of_threads()), Math::round (2000.0 * double(num_tracks) / double(tracks_remaining)));
          MT_gradient_vector_sorter sorter (gradient_vector, sort_size);

          // Remove candidate streamlines one at a time, and correspondingly modify the lobes to which they were attributed
          unsigned int removed_this_iteration = 0;
          recalculate = UNDEFINED;
          std::vector<Cost_fn_gradient_sort>::const_iterator candidate (gradient_vector.begin());
          do {

            if (!output_at_counts.empty() && (tracks_remaining == output_at_counts.back())) {
              const std::string prefix = str (tracks_remaining);
              output_filtered_tracks (tck_file_path, prefix + "_tracks.tck");
              if (output_debug)
                output_all_debug_images (prefix);
              INFO ("\nProportionality coefficient at " + str (tracks_remaining) + " streamlines is " + str (mu()));
              output_at_counts.pop_back();
            }

            if (tracks_remaining == term_number) {
              another_iteration = false;
              recalculate = TERM_COUNT;
              goto end_iteration;
            }

            if (mu() > term_mu) {
              another_iteration = false;
              recalculate = TERM_MU;
              goto end_iteration;
            }

            // Determine whether or not it is appropriate to remove a non-contributing streamline at this point
            if (sum_noncontributing_length && ((contributing_length_removed / sum_contributing_length) > (noncontributing_length_removed / sum_noncontributing_length))) {

              // Select a non-contributing streamline at random
              const track_t to_remove = noncontributing_indices.back();
              noncontributing_indices.pop_back();

              // Remove this streamline, and adjust all of the relevant quantities
              noncontributing_length_removed += contributions[to_remove]->get_total_length();
              delete contributions[to_remove];
              contributions[to_remove] = NULL;
              ++removed_this_iteration;
              --tracks_remaining;

            } else { // Proceed as normal

              const std::vector<Cost_fn_gradient_sort>::iterator candidate = sorter.get();

              const track_t candidate_index = candidate->get_tck_index();

              assert (candidate_index != num_tracks);
              assert (contributions[candidate_index]);

              if (candidate->get_cost_gradient() >= 0.0) {
                recalculate = POS_GRADIENT;
                if (!removed_this_iteration)
                  another_iteration = false;
                goto end_iteration;
              }

              const double streamline_density_ratio = candidate->get_cost_gradient() / (sum_contributing_length - contributing_length_removed);
              const double required_cf_change_ratio = - term_ratio * streamline_density_ratio * current_cf;

              const TckCont& candidate_contribution (*contributions[candidate_index]);

              const double old_mu = mu();
              const double new_mu = FOD_sum / (TD_sum - candidate_contribution.get_total_contribution());
              const double mu_change = new_mu - old_mu;

              // Initial estimate of cost change knowing only the change to the normalisation coefficient
              double this_actual_cf_change = current_roc_cf * mu_change;
              double quantisation = 0.0;

              for (size_t l = 0; l != candidate_contribution.dim(); ++l) {
                const Track_lobe_contribution& lobe_cont = candidate_contribution[l];
                const float length = lobe_cont.get_value();
                Lobe& this_lobe = lobes[lobe_cont.get_lobe_index()];
                quantisation += this_lobe.calc_quantisation (old_mu, length);
                const float delta_gradient = (this_lobe.remove_TD (length, new_mu, old_mu) - (this_lobe.get_d_cost_d_mu (old_mu) * mu_change));
                this_actual_cf_change += delta_gradient;
              }

              const double required_cf_change_quantisation = -0.5 * quantisation;
              const double this_nonlinearity = (candidate->get_cost_gradient() - this_actual_cf_change);

              if (this_actual_cf_change < minvalue (required_cf_change_ratio, required_cf_change_quantisation, this_nonlinearity)) {

                // Candidate streamline removal meets all criteria

                TD_sum -= candidate_contribution.get_total_contribution();
                contributing_length_removed += candidate_contribution.get_total_length();
                delete contributions[candidate_index];
                contributions[candidate_index] = NULL;
                ++removed_this_iteration;
                --tracks_remaining;

              } else {

                // Removal doesn't meet all criteria; put this track back into the reconstruction
                for (size_t l = 0; l != candidate_contribution.dim(); ++l) {
                  const Track_lobe_contribution& lobe_cont = candidate_contribution[l];
                  lobes[lobe_cont.get_lobe_index()] += lobe_cont.get_value();
                }

                if (this_actual_cf_change >= this_nonlinearity)
                  recalculate = NONLINEARITY;
                else if (term_ratio && this_actual_cf_change >= required_cf_change_ratio)
                  recalculate = TERM_RATIO;
                else
                  recalculate = QUANTISATION;
                if (!removed_this_iteration)
                  another_iteration = false;
                goto end_iteration;

              } // End checking validity of candidate streamline removal

            } // End switching between removal of zero-contribution or nonzero-contribution streamline

          } while (!recalculate); // End removing streamlines in this iteration

          end_iteration:

          const float cf_end_iteration = calc_cost_function();

          if (App::log_level)
            fprintf (stderr, "\r%s:     %4u            %6u             %9u               %.2f%%   ", App::NAME.c_str(), iteration, removed_this_iteration, tracks_remaining, 100.0 * cf_end_iteration / init_cf);

          if (csv_out) {
            (*csv_out) << str (iteration) << "," << str (removed_this_iteration) << "," << str (num_tracks - tracks_remaining) << "," << str (tracks_remaining) << "," << str (cf_end_iteration) << "," << str (TD_sum) << "," << str (mu()) << ",";
            switch (recalculate) {
              case UNDEFINED:    throw Exception ("Encountered undefined recalculation at end of iteration!");
              case NONLINEARITY: (*csv_out) << "Non-linearity"; break;
              case QUANTISATION: (*csv_out) << "Quantisation"; break;
              case TERM_COUNT:   (*csv_out) << "Target streamline count"; break;
              case TERM_RATIO:   (*csv_out) << "Termination ratio"; break;
              case TERM_MU:      (*csv_out) << "Target proportionality coefficient"; break;
              case POS_GRADIENT: (*csv_out) << "Positive gradient"; break;
            }
            (*csv_out) << ",\n";
            csv_out->flush();
          }

        } while (another_iteration);

        if (csv_out)
          csv_out->close();

        if (App::log_level)
          fprintf (stderr, "\n");

        switch (recalculate) {
          case UNDEFINED:    throw Exception ("Encountered undefined recalculation at end of iteration!");
          case NONLINEARITY: INFO ("Filtering terminated due to instability in cost function gradients"); break;
          case QUANTISATION: INFO ("Filtering terminated due to candidate streamline failing to exceed quantisation"); break;
          case TERM_COUNT:   INFO ("Filtering terminated due to reaching desired streamline count"); break;
          case TERM_RATIO:   INFO ("Filtering terminated due to cost function / streamline density decrease ratio"); break;
          case TERM_MU:      INFO ("Filtering terminated due to reaching desired proportionality coefficient"); break;
          case POS_GRADIENT: INFO ("Filtering terminated due to candidate streamline having positive gradient"); break;
        }

        INFO ("Proportionality coefficient at end of filtering is " + str (mu()));

      }





      void SIFTer::output_filtered_tracks (const std::string& input_path, const std::string& output_path) const
      {
        Tractography::Properties p;
        Tractography::Reader<float> reader (input_path, p);
        // "count" and "total_count" should be dealt with by the writer
        p["SIFT_mu"] = str (mu());
        Tractography::Writer<float> writer (output_path, p);
        track_t tck_counter = 0;
        std::vector< Point<float> > tck;
        ProgressBar progress ("Writing filtered tracks output file...", contributions.size());
        std::vector< Point<float> > empty_tck;
        while (reader.next (tck)) {
          if (contributions[tck_counter++])
            writer.append (tck);
          else
            writer.append (empty_tck);
          ++progress;
        }
        reader.close();
      }






      void SIFTer::output_all_debug_images (const std::string& prefix) const
      {
        output_target_image (prefix + "_target.mif");
#ifdef SIFTER_OUTPUT_SH_IMAGES
        output_target_image_sh (prefix + "_target_sh.mif");
#endif
        output_tdi (prefix + "_tdi.mif");
        if (fmls.get_create_null_lobe())
          output_tdi_null_lobes (prefix + "_tdi_null_lobes.mif");
#ifdef SIFTER_OUTPUT_SH_IMAGES
        output_tdi_sh (prefix + "_tdi_sh.mif");
#endif
        output_error_images (prefix + "_max_abs_diff.mif", prefix + "_diff.mif", prefix + "_cost.mif");
        output_scatterplot (prefix + "_scatterplot.csv");
        output_lobe_count_image (prefix + "_lobe_count.mif");
        output_untracked_lobes (prefix + "_untracked_count.mif", prefix + "_untracked_amps.mif");
      }




      void SIFTer::set_regular_outputs (const std::vector<int>& in, const bool b)
      {
        for (std::vector<int>::const_iterator i = in.begin(); i != in.end(); ++i) {
          if (*i > 0 && *i <= (int)contributions.size())
            output_at_counts.push_back (*i);
        }
        sort (output_at_counts.begin(), output_at_counts.end());
        output_debug = b;
      }




      void SIFTer::check_TD()
      {

        VAR (TD_sum);

        double sum_from_lobes = 0.0, sum_from_lobes_weighted = 0.0;
        for (std::vector<Lobe>::const_iterator i = lobes.begin(); i != lobes.end(); ++i) {
          sum_from_lobes          += i->get_TD();
          sum_from_lobes_weighted += i->get_TD() * i->get_weight();
        }
        VAR (sum_from_lobes);
        VAR (sum_from_lobes_weighted);
        double sum_from_tracks = 0.0;
        for (std::vector<TckCont*>::const_iterator i = contributions.begin(); i != contributions.end(); ++i) {
          if (*i)
            sum_from_tracks += (*i)->get_total_contribution();
        }
        VAR (sum_from_tracks);

      }





      void SIFTer::test_sorting_block_size (const size_t num_tracks) const
      {

        Math::RNG rng;

        std::vector<Cost_fn_gradient_sort> gradient_vector;
        gradient_vector.assign (num_tracks, Cost_fn_gradient_sort (num_tracks, 0.0, 0.0));
        // Fill the gradient vector with random Gaussian data
        for (track_t index = 0; index != num_tracks; ++index) {
          const float value = rng.normal();
          gradient_vector[index].set (index, value, value);
        }

        std::vector<size_t> block_sizes;
        for (size_t i = 16; i < num_tracks; i *= 2)
          block_sizes.push_back (i);
        block_sizes.push_back (num_tracks);

        for (std::vector<size_t>::const_iterator i = block_sizes.begin(); i != block_sizes.end(); ++i) {
          const size_t block_size = *i;

          // Make a copy of the gradient vector, so the same data is sorted each time
          std::vector<Cost_fn_gradient_sort> temp_gv (gradient_vector);

          Timer timer;
          // Simulate sorting and filtering
          try {
            MT_gradient_vector_sorter sorter (temp_gv, block_size);
            for (size_t candidate_count = 0; candidate_count < num_tracks / 1000; ++candidate_count)
              const std::vector<Cost_fn_gradient_sort>::iterator candidate = sorter.get();
            std::cerr << "Time required for sorting " << num_tracks << " tracks, block size " << block_size << " = " << timer.elapsed() * 1000.0 << "ms\n";
          } catch (...) {
            std::cerr << "Could not sort " << num_tracks << "tracks with block size " << block_size << "\n";
          }



        }

      }






      // Convenience functions

      double SIFTer::calc_cost_function() const
      {
        const double current_mu = mu();
        double cost = 0.0;
        std::vector<Lobe>::const_iterator i = lobes.begin();
        for (++i; i != lobes.end(); ++i)
          cost += i->get_cost (current_mu);
        return cost;
      }

      double SIFTer::calc_roc_cost_function() const
      {
        const double current_mu = mu();
        double roc_cost = 0.0;
        std::vector<Lobe>::const_iterator i = lobes.begin();
        for (++i; i != lobes.end(); ++i)
          roc_cost += i->get_d_cost_d_mu (current_mu);
        return roc_cost;
      }

      double SIFTer::calc_gradient (const track_t index, const double current_mu, const double current_roc_cost) const
      {
        if (!contributions[index])
          return std::numeric_limits<double>::max();
        const TckCont& tck_cont = *contributions[index];
        const double TD_sum_if_removed = TD_sum - tck_cont.get_total_contribution();
        const double mu_if_removed = FOD_sum / TD_sum_if_removed;
        const double mu_change_if_removed = mu_if_removed - current_mu;
        double gradient = current_roc_cost * mu_change_if_removed;
        for (size_t v = 0; v != tck_cont.dim(); ++v) {
          const Lobe& lobe = lobes[tck_cont[v].get_lobe_index()];
          const double undo_gradient_mu_only = lobe.get_d_cost_d_mu (current_mu) * mu_change_if_removed;
          gradient -= undo_gradient_mu_only;
          const double gradient_remove_tck = lobe.get_cost_manual_TD (mu_if_removed, lobe.get_TD() - tck_cont[v].get_value()) - lobe.get_cost (current_mu);
          gradient += gradient_remove_tck;
        }
        return gradient;
      }








      // Output functions - non-essential, mostly debugging outputs

      void SIFTer::output_proc_mask (const std::string& path)
      {
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        Image::BufferScratch<float>::voxel_type v_mask (proc_mask);
        Image::copy (v_mask, v_out);
      }

      void SIFTer::output_target_image (const std::string& path) const
      {
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            float value = 0.0;
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i)
              value += i().get_FOD();
            v_out.value() = value;
          } else {
            v_out.value() = NAN;
          }
        }
      }

      void SIFTer::output_target_image_sh (const std::string& path) const
      {
        const size_t L = 8;
        const size_t N = Math::SH::NforL (L);
        Math::SH::aPSF<float> aPSF (L);
        Image::Header H_sh (H);
        H_sh.set_ndim (4);
        H_sh.dim(3) = N;
        H_sh.stride (3) = 0;
        Image::Buffer<float> out (path, H_sh);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out, 0, 3);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            Math::Vector<float> sum;
            sum.resize (N, 0.0);
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
              if (i().get_FOD()) {
                Math::Vector<float> this_lobe;
                aPSF (this_lobe, dirs.get_dir (i().get_dir()));
                for (size_t c = 0; c != N; ++c)
                  sum[c] += i().get_FOD() * this_lobe[c];
              }
            }
            for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
              v_out.value() = sum[v_out[3]];
          } else {
            for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
              v_out.value() = NAN;
          }
        }
      }

      void SIFTer::output_tdi (const std::string& path) const
      {
        const double current_mu = mu();
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            float value = 0.0;
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i)
              value += i().get_TD();
            v_out.value() = value * current_mu;
          } else {
            v_out.value() = NAN;
          }
        }
      }

      void SIFTer::output_tdi_null_lobes (const std::string& path) const
      {
        const double current_mu = mu();
        Image::Buffer<float> out (path, H);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            float value = 0.0;
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
              if (!i().get_FOD())
                value += i().get_TD();
            }
            v_out.value() = value * current_mu;
          } else {
            v_out.value() = NAN;
          }
        }
      }

      void SIFTer::output_tdi_sh (const std::string& path) const
      {
        const double current_mu = mu();
        const size_t L = 8;
        const size_t N = Math::SH::NforL (L);
        Math::SH::aPSF<float> aPSF (L);
        Image::Header H_sh (H);
        H_sh.set_ndim (4);
        H_sh.dim(3) = N;
        H_sh.stride (3) = 0;
        Image::Buffer<float> out (path, H_sh);
        Image::Buffer<float>::voxel_type v_out (out);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out, 0, 3);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value()) {
            Math::Vector<float> sum;
            sum.resize (N, 0.0);
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
              if (i().get_FOD()) {
                Math::Vector<float> this_lobe;
                aPSF (this_lobe, dirs.get_dir (i().get_dir()));
                for (size_t c = 0; c != N; ++c)
                  sum[c] += i().get_TD() * this_lobe[c];
              }
              for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
                v_out.value() = sum[v_out[3]] * current_mu;
            }
          } else {
            for (v_out[3] = 0; v_out[3] != (int)N; ++v_out[3])
              v_out.value() = NAN;
          }
        }
      }

      void SIFTer::output_error_images (const std::string& max_abs_diff_path, const std::string& diff_path, const std::string& cost_path) const
      {
        const double current_mu = mu();
        Image::Buffer<float> max_abs_diff (max_abs_diff_path, H), diff (diff_path, H), cost (cost_path, H);
        Image::Buffer<float>::voxel_type v_max_abs_diff (max_abs_diff), v_diff (diff), v_cost (cost);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v);
        for (loop.start (v); loop.ok(); loop.next (v)) {
          v_max_abs_diff[2] = v_diff[2] = v_cost[2] = v[2];
          v_max_abs_diff[1] = v_diff[1] = v_cost[1] = v[1];
          v_max_abs_diff[0] = v_diff[0] = v_cost[0] = v[0];
          if (v.value()) {
            double max_abs_diff = 0.0, diff = 0.0, cost = 0.0;
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
              const double this_diff = i().get_diff (current_mu);
              max_abs_diff = MAX (max_abs_diff, fabs (this_diff));
              diff += this_diff;
              cost += i().get_cost (current_mu) * i().get_weight();
            }
            v_max_abs_diff.value() = max_abs_diff;
            v_diff.value() = diff;
            v_cost.value() = cost;
          } else {
            v_max_abs_diff.value() = NAN;
            v_diff.value() = NAN;
            v_cost.value() = NAN;
          }
        }
      }

      void SIFTer::output_scatterplot (const std::string& path) const
      {
        std::ofstream out (path.c_str(), std::ios_base::trunc);
        const double current_mu = mu();
        out << "FOD amplitude,Track density (unscaled),Track density (scaled),Weight,\n";
        for (std::vector<Lobe>::const_iterator i = lobes.begin(); i != lobes.end(); ++i)
          out << str (i->get_FOD()) << "," << str (i->get_TD()) << "," << str (i->get_TD() * current_mu) << "," << str (i->get_weight()) << ",\n";
        out.close();
      }

      void SIFTer::output_lobe_count_image (const std::string& path) const
      {
        Image::Header H_out (H);
        H_out.datatype() = DataType::UInt8;
        Image::Buffer<uint8_t> image (path, H_out);
        Image::Buffer<uint8_t>::voxel_type v_out (image);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out);
        for (loop.start (v_out, v); loop.ok(); loop.next (v_out, v)) {
          if (v.value())
            v_out.value() = (*v.value()).num_lobes();
          else
            v_out.value() = 0;
        }
      }

      void SIFTer::output_non_contributing_streamlines (const std::string& input_path, const std::string& output_path) const
      {
        Tractography::Properties p;
        Tractography::Reader<float> reader (input_path, p);
        p["total_count"] = contributions.size();
        Tractography::Writer<float> writer (output_path, p);
        std::vector< Point<float> > tck;
        ProgressBar progress ("Writing non-contributing streamlines output file...", contributions.size());
        track_t tck_counter = 0;
        while (reader.next (tck)) {
          if (contributions[tck_counter] && !contributions[tck_counter++]->get_total_contribution())
            writer.append (tck);
          ++progress;
        }
        reader.close();
      }

      void SIFTer::output_untracked_lobes (const std::string& path_count, const std::string& path_amps) const
      {
        Image::Header H_uint8_t (H);
        H_uint8_t.datatype() = DataType::UInt8;
        Image::Buffer<uint8_t> out_count (path_count, H_uint8_t);
        Image::Buffer<float>   out_amps (path_amps, H);
        Image::Buffer<uint8_t>::voxel_type v_out_count (out_count);
        Image::Buffer<float>  ::voxel_type v_out_amps  (out_amps);
        VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v_out_count);
        for (loop.start (v_out_count, v_out_amps, v); loop.ok(); loop.next (v_out_count, v_out_amps, v)) {
          if (v.value()) {
            uint8_t count = 0;
            float sum = 0.0;
            for (FOD_map<Lobe>::ConstIterator i = begin (v); i; ++i) {
              if (!i().get_TD()) {
                ++count;
                sum += i().get_FOD();
              }
            }
            v_out_count.value() = count;
            v_out_amps .value() = sum;
          }
        }
      }




      }
    }
  }
}



