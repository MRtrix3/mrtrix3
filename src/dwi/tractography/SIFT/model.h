/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

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



#ifndef __dwi_tractography_sift_model_h__
#define __dwi_tractography_sift_model_h__


#include <vector>

#include "app.h"

#include "dwi/fixel_map.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/track_data.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/SIFT/model_base.h"
#include "dwi/tractography/SIFT/track_contribution.h"
#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"

#include "image/loop.h"

#include "thread/queue.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {


      template <class Fixel>
      class Model : public ModelBase<Fixel>
      {

        protected:
        typedef typename Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef typename Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;

        public:
          template <class Set>
          Model (Set& dwi, const DWI::Directions::FastLookupSet& dirs) :
              ModelBase<Fixel> (dwi, dirs)
          {
            Track_fixel_contribution::set_scaling (dwi);
          }

          virtual ~Model ();


          void map_streamlines (const std::string&);

          void remove_excluded_fixels ();

          // For debugging purposes - make sure the sum of TD in the fixels is equal to the sum of TD in the streamlines
          void check_TD();

          track_t num_tracks() const { return contributions.size(); }

          void output_non_contributing_streamlines (const std::string&) const;


          using ModelBase<Fixel>::mu;


        protected:
          std::string tck_file_path;
          std::vector<TrackContribution*> contributions;

          using Fixel_map<Fixel>::accessor;
          using Fixel_map<Fixel>::begin;

          using ModelBase<Fixel>::dirs;
          using ModelBase<Fixel>::fixels;
          using ModelBase<Fixel>::FOD_sum;
          using ModelBase<Fixel>::H;
          using ModelBase<Fixel>::TD_sum;


          Model (const Model& that) : ModelBase<Fixel> (that) { assert (0); }


        private:
          // Some member classes to support multi-threaded processes
          class MappedTrackReceiver
          {
            public:
              MappedTrackReceiver (Model& i) :
                master (i),
                mutex (new Thread::Mutex()),
                TD_sum (0.0),
                fixel_TDs (master.fixels.size(), 0.0) { }
              MappedTrackReceiver (const MappedTrackReceiver& that) :
                master (that.master),
                mutex (that.mutex),
                TD_sum (0.0),
                fixel_TDs (master.fixels.size(), 0.0) { }
              ~MappedTrackReceiver();
              bool operator() (const Mapping::SetDixel&);
            private:
              Model& master;
              RefPtr<Thread::Mutex> mutex;
              double TD_sum;
              std::vector<double> fixel_TDs;
          };

          class FixelRemapper
          {
            public:
              FixelRemapper (Model& i, std::vector<size_t>& r) :
                master   (i),
                remapper (r) { }
              bool operator() (const TrackIndexRange&);
            private:
              Model& master;
              std::vector<size_t>& remapper;
          };

      };





      template <class Fixel>
      Model<Fixel>::~Model ()
      {
        for (std::vector<TrackContribution*>::iterator i = contributions.begin(); i != contributions.end(); ++i) {
          if (*i) {
            delete *i;
            *i = NULL;
          }
        }
      }




      template <class Fixel>
      void Model<Fixel>::map_streamlines (const std::string& path)
      {
        Tractography::Properties properties;
        Tractography::Reader<float> file (path, properties);

        if (properties.find ("count") == properties.end())
          throw Exception ("Input .tck file does not specify number of streamlines (run tckfixcount on your .tck file!)");
        const track_t count = to<track_t>(properties["count"]);

        contributions.assign (count, NULL);

        // Determine appropriate upsampling ratio for mapping
        // In this particular context want the calculation of length to be precise - 1/10th voxel size at worst
        float step_size = 0.0;
        if (properties.find ("output_step_size") != properties.end())
          step_size = to<float> (properties["output_step_size"]);
        else
          step_size = to<float> (properties["step_size"]);
        if (!step_size || !std::isfinite (step_size))
          throw Exception ("Cannot perform appropriate streamline mapping without knowledge of track step size!");
        const float upsample_ratio = Math::ceil<size_t> (step_size / (minvalue (H.vox(0), H.vox(1), H.vox(2)) * 0.1));

        Mapping::TrackLoader loader (file, count);
        Mapping::TrackMapperDixel mapper (H, upsample_ratio, true, dirs);
        MappedTrackReceiver receiver (*this);
        Thread::run_batched_queue_custom_threading (loader, 1, Tractography::TrackData<float>(), 100, mapper, 0, Mapping::SetDixel(), 100, receiver, 0);

        if (!contributions.back()) {
          track_t num_tracks = 0, max_index = 0;
          for (track_t i = 0; i != contributions.size(); ++i) {
            if (contributions[i]) {
              ++num_tracks;
              max_index = std::max (max_index, i);
            }
            WARN ("Only " + str (num_tracks) + " tracks read from input track file; expected " + str (contributions.size()));
            WARN ("(suggest running command tckfixcount on file " + path + ")");
            contributions.resize (max_index + 1);
          }
        }

        tck_file_path = path;

        INFO ("Proportionality coefficient after streamline mapping is " + str (mu()));
      }





      template <class Fixel>
      void Model<Fixel>::remove_excluded_fixels ()
      {

        const bool remove_untracked_fixels = App::get_options ("remove_untracked").size();
        App::Options opt = App::get_options ("min_fod_integral");
        const float min_FOD_integral = opt.size() ? float(opt[0][0]) : 0.0;

        if (!remove_untracked_fixels && !min_FOD_integral)
          return;

        std::vector<size_t> fixel_index_mapping (fixels.size(), 0);
        typename Fixel_map<Fixel>::VoxelAccessor v (accessor);
        Image::LoopInOrder loop (v);

        std::vector<Fixel> new_fixels;
        new_fixels.push_back (Fixel());
        FOD_sum = 0.0;

        for (loop.start (v); loop.ok(); loop.next (v)) {
          if (v.value()) {

            size_t new_start_index = new_fixels.size();

            for (typename Fixel_map<Fixel>::ConstIterator i = begin(v); i; ++i) {
              if ((!remove_untracked_fixels || i().get_TD()) && (i().get_FOD() > min_FOD_integral)) {
                fixel_index_mapping [size_t (i)] = new_fixels.size();
                new_fixels.push_back (i());
                FOD_sum += i().get_weight() * i().get_FOD();
              }
            }

            delete v.value();

            if (new_fixels.size() == new_start_index)
              v.value() = NULL;
            else
              v.value() = new MapVoxel (new_start_index, new_fixels.size() - new_start_index);

          }
        }

        INFO (str (fixels.size() - new_fixels.size()) + " out of " + str(fixels.size()) + " fixels removed from reconstruction (" + str(new_fixels.size()) + ") remaining)");

        fixels.swap (new_fixels);

        TrackIndexRangeWriter writer (SIFT_TRACK_INDEX_BUFFER_SIZE, num_tracks(), "Removing excluded fixels...");
        FixelRemapper remapper (*this, fixel_index_mapping);
        Thread::run_queue_threaded_sink (writer, TrackIndexRange(), remapper);

        TD_sum = 0.0;
        for (typename std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i)
          TD_sum += i->get_weight() * i->get_TD();

        INFO ("After fixel removal, the proportionality coefficient is " + str(mu()));

      }







      template <class Fixel>
      void Model<Fixel>::check_TD()
      {
        VAR (TD_sum);
        double sum_from_fixels = 0.0, sum_from_fixels_weighted = 0.0;
        for (typename std::vector<Fixel>::const_iterator i = fixels.begin(); i != fixels.end(); ++i) {
          sum_from_fixels          += i->get_TD();
          sum_from_fixels_weighted += i->get_TD() * i->get_weight();
        }
        VAR (sum_from_fixels);
        VAR (sum_from_fixels_weighted);
        double sum_from_tracks = 0.0;
        for (std::vector<TrackContribution*>::const_iterator i = contributions.begin(); i != contributions.end(); ++i) {
          if (*i)
            sum_from_tracks += (*i)->get_total_contribution();
        }
        VAR (sum_from_tracks);
      }





      template <class Fixel>
      void Model<Fixel>::output_non_contributing_streamlines (const std::string& output_path) const
      {
        Tractography::Properties p;
        Tractography::Reader<float> reader (tck_file_path,  p);
        Tractography::Writer<float> writer (output_path, p);
        std::vector< Point<float> > tck, null_tck;
        ProgressBar progress ("Writing non-contributing streamlines output file...", contributions.size());
        track_t tck_counter = 0;
        while (reader.next (tck) && tck_counter < contributions.size()) {
          if (contributions[tck_counter] && !contributions[tck_counter++]->get_total_contribution())
            writer.append (tck);
          else
            writer.append (null_tck);
          ++progress;
        }
        reader.close();
      }








      template <class Fixel>
      Model<Fixel>::MappedTrackReceiver::~MappedTrackReceiver()
      {
        Thread::Mutex::Lock lock (*mutex);
        master.TD_sum += TD_sum;
        for (size_t i = 0; i != fixel_TDs.size(); ++i)
          master.fixels[i] += fixel_TDs[i];
      }



      template <class Fixel>
      bool Model<Fixel>::MappedTrackReceiver::operator() (const Mapping::SetDixel& in)
      {

        if (in.index >= master.contributions.size())
          throw Exception ("Received mapped streamline beyond the expected number of streamlines (run tckfixcount on your .tck file!)");
        if (master.contributions[in.index])
          throw Exception ("FIXME: Same streamline has been mapped multiple times! (?)");

        std::vector<Track_fixel_contribution> masked_contributions;
        double total_contribution = 0.0, total_length = 0.0;

        for (Mapping::SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          total_length += i->get_value();
          const size_t fixel_index = master.dixel2fixel (*i);
          if (fixel_index && (i->get_value() > Track_fixel_contribution::min())) {
            total_contribution += i->get_value() * master.fixels[fixel_index].get_weight();
            bool incremented = false;
            for (std::vector<Track_fixel_contribution>::iterator c = masked_contributions.begin(); !incremented && c != masked_contributions.end(); ++c) {
              if ((c->get_fixel_index() == fixel_index) && c->add (i->get_value()))
                incremented = true;
            }
            if (!incremented)
              masked_contributions.push_back (Track_fixel_contribution (fixel_index, i->get_value()));
          }
        }

        master.contributions[in.index] = new TrackContribution (masked_contributions, total_contribution, total_length);

        TD_sum += total_contribution;
        for (std::vector<Track_fixel_contribution>::const_iterator i = masked_contributions.begin(); i != masked_contributions.end(); ++i)
          fixel_TDs [i->get_fixel_index()] += i->get_value();

        return true;

      }





      template <class Fixel>
      bool Model<Fixel>::FixelRemapper::operator() (const TrackIndexRange& in)
      {
        for (track_t track_index = in.first; track_index != in.second; ++track_index) {
          if (master.contributions[track_index]) {
            TrackContribution& this_cont (*master.contributions[track_index]);
            std::vector<Track_fixel_contribution> new_cont;
            double total_contribution = 0.0;
            for (size_t i = 0; i != this_cont.dim(); ++i) {
              const size_t new_index = remapper[this_cont[i].get_fixel_index()];
              if (new_index) {
                new_cont.push_back (Track_fixel_contribution (new_index, this_cont[i].get_value()));
                total_contribution += this_cont[i].get_value() * master[new_index].get_weight();
              }
            }
            TrackContribution* new_contribution = new TrackContribution (new_cont, total_contribution, this_cont.get_total_length());
            delete master.contributions[track_index];
            master.contributions[track_index] = new_contribution;
          }
        }
        return true;
      }






      }
    }
  }
}


#endif
