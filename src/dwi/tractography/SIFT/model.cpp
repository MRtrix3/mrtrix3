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

#include "dwi/tractography/SIFT/model.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      Model::Model (const std::string& fd_path) :
          ModelBase (fd_path)
      {
        Track_fixel_contribution::set_scaling (*this);
      }



      Model::~Model ()
      {
        for (vector<TrackContribution*>::iterator i = contributions.begin(); i != contributions.end(); ++i) {
          if (*i) {
            delete *i;
            *i = nullptr;
          }
        }
      }




      void Model::map_streamlines (const std::string& path)
      {
        Tractography::Properties properties;
        Tractography::Reader<> file (path, properties);

        const track_t count = (properties.find ("count") == properties.end()) ? 0 : to<track_t>(properties["count"]);
        if (!count)
          throw Exception ("Cannot map streamlines: track file " + Path::basename(path) + " is empty");

        contributions.assign (count, nullptr);
        TD_sum = 0.0;

        {
          Mapping::TrackLoader loader (file, count);
          TrackMappingWorker worker (*this, Mapping::determine_upsample_ratio (*this, properties, 0.1));
          Thread::run_queue (loader,
                             Thread::batch (Tractography::Streamline<>()),
                             Thread::multi (worker));
        }

        if (!contributions.back()) {
          track_t num_tracks = 0, max_index = 0;
          for (track_t i = 0; i != contributions.size(); ++i) {
            if (contributions[i]) {
              ++num_tracks;
              max_index = std::max (max_index, i);
            }
            WARN ("Only " + str (num_tracks) + " tracks read from input track file; expected " + str (contributions.size()));
            contributions.resize (max_index + 1);
          }
        }

        INFO ("Proportionality coefficient after streamline mapping is " + str (mu()));
        check_TD();

        tractogram_path = path;
      }




      void Model::exclude_fixels ()
      {

        // TODO Consider changing default behaviour
        // Previously, preference was to retain untracked fixels,
        //   as it simplified comparison between tractograms with the same set of fixels
        // Whereas now, there is greater advocacy for scaling by mu,
        //   which is a more explicit control for the potential source of the concern
        const bool exclude_untracked = App::get_options ("exclude_untracked").size();
        auto opt = App::get_options ("fd_thresh");
        const value_type min_fibre_density = opt.size() ? value_type(opt[0][0]) : 0.0;

        if (!exclude_untracked && !min_fibre_density)
          return;

        MR::Fixel::index_type exclude_untracked_count = 0;
        MR::Fixel::index_type below_fd_threshold_count = 0;
        FD_sum = 0.0;
        TD_sum = 0.0;
        for (size_t i = 0; i != nfixels(); ++i) {
          auto fixel ((*this)[i]);
          if (fixel.weight() && exclude_untracked && !fixel.td())
            ++exclude_untracked_count;
          if (fixel.weight() && fixel.fd() < min_fibre_density)
            ++below_fd_threshold_count;
          if ((exclude_untracked && !fixel.td()) || (fixel.fd() < min_fibre_density)) {
            fixel.set_weight(0.0);
          } else {
            FD_sum += fixel.weight() * fixel.fd();
            TD_sum += fixel.weight() * fixel.td();
          }
        }

        INFO (str(exclude_untracked_count) + " fixels had weight reset to zero due to not being tracked");
        INFO (str(below_fd_threshold_count) + " fixels had weight reset to zero due to FD being below threshold");
        INFO ("After fixel exclusion, the proportionality coefficient is " + str(mu()));

      }






      void Model::check_TD()
      {
        // TODO Some small discrepancy;
        //   is this due to an imbalance in the track fixel contribution compression?
        //   Or perhaps truncation?
        VAR (TD_sum);
        value_type sum_from_fixels = 0.0, sum_from_fixels_weighted = 0.0;

        for (size_t i = 0; i != nfixels(); ++i) {
          const auto fixel ((*this)[i]);
          sum_from_fixels          += fixel.td();
          sum_from_fixels_weighted += fixel.td() * fixel.weight();
        }
        VAR (sum_from_fixels);
        VAR (sum_from_fixels_weighted);
        value_type sum_from_tracks = 0.0;
        for (vector<TrackContribution*>::const_iterator i = contributions.begin(); i != contributions.end(); ++i) {
          if (*i)
            sum_from_tracks += (*i)->get_total_contribution();
        }
        VAR (sum_from_tracks);
      }





      void Model::output_non_contributing_streamlines (const std::string& output_path) const
      {
        Tractography::Properties p;
        Tractography::Reader<float> reader (tractogram_path, p);
        Tractography::Writer<float> writer (output_path, p);
        Tractography::Streamline<> tck;
        ProgressBar progress ("Writing non-contributing streamlines output file", contributions.size());
        track_t tck_counter = 0;
        while (reader (tck) && tck_counter < contributions.size()) {
          if (contributions[tck_counter] && !contributions[tck_counter++]->get_total_contribution())
            writer (tck);
          else
            writer.skip();
          ++progress;
        }
        reader.close();
      }



      Model::TrackMappingWorker::TrackMappingWorker (Model& i, const default_type upsample_ratio) :
          master (i),
          mapper (i, i),
          mutex (new std::mutex),
          TD_sum (0.0),
          fixel_TDs (master.nfixels(), 0.0),
          fixel_counts (master.nfixels(), 0)
      {
        mapper.set_upsample_ratio (upsample_ratio);
        mapper.set_use_precise_mapping (true);
      }


      Model::TrackMappingWorker::TrackMappingWorker (const TrackMappingWorker& that) :
          master (that.master),
          mapper (that.mapper),
          mutex (that.mutex),
          TD_sum (0.0),
          fixel_TDs (master.nfixels(), 0.0),
          fixel_counts (master.nfixels(), 0) { }



      Model::TrackMappingWorker::~TrackMappingWorker()
      {
        std::lock_guard<std::mutex> lock (*mutex);
        master.TD_sum += TD_sum;
        for (MR::Fixel::index_type i = 0; i != master.nfixels(); ++i)
          master[i].add(fixel_TDs[i], fixel_counts[i]);
      }



      bool Model::TrackMappingWorker::operator() (const Tractography::Streamline<>& in)
      {
        assert (in.get_index() < master.contributions.size());
        assert (!master.contributions[in.get_index()]);

        try {

          Mapping::Set<Mapping::Fixel> fixels;
          mapper (in, fixels);

          vector<Track_fixel_contribution> masked_contributions;
          value_type total_contribution = 0.0, total_length = 0.0;

          for (const auto& i : fixels) {
            total_length += i.get_length();
            if (i.get_length() > Track_fixel_contribution::min()) {
              total_contribution += i.get_length() * master.fixels(MR::Fixel::index_type(i), model_weight_column);
              bool incremented = false;
              for (vector<Track_fixel_contribution>::iterator c = masked_contributions.begin(); !incremented && c != masked_contributions.end(); ++c) {
                if ((c->get_fixel_index() == MR::Fixel::index_type(i)) && c->add (i.get_length()))
                  incremented = true;
              }
              if (!incremented)
                masked_contributions.push_back (Track_fixel_contribution (MR::Fixel::index_type(i), i.get_length()));
            }
          }

          master.contributions[in.get_index()] = new TrackContribution (masked_contributions, total_contribution, total_length);

          TD_sum += total_contribution;
          for (const auto& i : masked_contributions) {
            fixel_TDs [i.get_fixel_index()] += i.get_length();
            ++fixel_counts [i.get_fixel_index()];
          }

          return true;

        } catch (...) {
          throw Exception ("Error allocating memory for streamline visitations");
          return false;
        }
      }



      }
    }
  }
}
