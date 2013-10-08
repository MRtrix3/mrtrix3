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


#include "dwi/tractography/SIFT/multithread.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {


      TrackIndexRangeWriter::TrackIndexRangeWriter (const track_t buffer_size, const track_t num_tracks, const std::string& message) :
        size  (buffer_size),
        end   (num_tracks),
        start (0),
        progress (message.empty() ? NULL : new ProgressBar (message, ceil (float(end) / float(size)))) { }


      bool TrackIndexRangeWriter::operator() (TrackIndexRange& out)
      {
        if (start >= end)
          return false;
        out.first = start;
        const track_t last = MIN (start + size, end);
        out.second = last;
        start = last;
        if (progress)
          ++*progress;
        return true;
      }






      bool TrackGradientCalculator::operator() (const TrackIndexRange& in)
      {
        for (track_t track_index = in.first; track_index != in.second; ++track_index) {
          if (sifter.contributions[track_index]) {
            const double gradient = sifter.calc_gradient (track_index, current_mu, current_roc_cost);
            const double grad_per_unit_length = sifter.contributions[track_index]->get_total_contribution() ? (gradient / sifter.contributions[track_index]->get_total_contribution()) : 0.0;
            gradient_vector[track_index].set (track_index, gradient, grad_per_unit_length);
          } else {
            gradient_vector[track_index].set (sifter.num_tracks(), 0.0, 0.0);
          }
        }
        return true;
      }




      bool LobeRemapper::operator() (const TrackIndexRange& in)
      {
        for (track_t track_index = in.first; track_index != in.second; ++track_index) {
          TckCont& this_cont (*sifter.contributions[track_index]);
          std::vector<Track_lobe_contribution> new_cont;
          double total_contribution = 0.0;
          for (size_t i = 0; i != this_cont.dim(); ++i) {
            const size_t new_index = remapper[this_cont[i].get_lobe_index()];
            if (new_index) {
              new_cont.push_back (Track_lobe_contribution (new_index, this_cont[i].get_value()));
              total_contribution += this_cont[i].get_value() * sifter[new_index].get_weight();
            }
          }
          TckCont* new_contribution = new TckCont (new_cont, total_contribution, this_cont.get_total_length());
          delete sifter.contributions[track_index];
          sifter.contributions[track_index] = new_contribution;
        }
        return true;
      }





      bool MappedTrackReceiver::operator() (const Mapping::SetDixel& in)
      {

        if (in.index >= sifter.contributions.size())
          throw Exception ("Received mapped streamline beyond the expected number of streamlines (run tckfixcount on your .tck file!)");
        if (sifter.contributions[in.index])
          throw Exception ("FIXME: Same streamline has been mapped multiple times! (?)");

        std::vector<Track_lobe_contribution> masked_contributions;
        double total_contribution = 0.0, total_length = 0.0;

        for (Mapping::SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          total_length += i->get_value();
          const size_t lobe_index = sifter.dix2lobe (*i);
          if (lobe_index) {
            total_contribution += i->get_value() * sifter.lobes[lobe_index].get_weight();
            if (i->get_value() > Track_lobe_contribution::min()) {
              bool incremented = false;
              for (std::vector<Track_lobe_contribution>::iterator c = masked_contributions.begin(); !incremented && c != masked_contributions.end(); ++c) {
                if ((c->get_lobe_index() == lobe_index) && c->add (i->get_value()))
                  incremented = true;
              }
              if (!incremented)
                masked_contributions.push_back (Track_lobe_contribution (lobe_index, i->get_value()));
            }
          }
        }

        sifter.contributions[in.index] = new TrackContribution<Track_lobe_contribution> (masked_contributions, total_contribution, total_length);

        {
          Thread::Mutex::Lock lock (*mutex);
          sifter.TD_sum += total_contribution;
          for (std::vector<Track_lobe_contribution>::const_iterator i = masked_contributions.begin(); i != masked_contributions.end(); ++i)
            sifter.lobes[i->get_lobe_index()] += i->get_value();
        }

        return true;

      }









      MT_gradient_vector_sorter::MT_gradient_vector_sorter (MT_gradient_vector_sorter::VecType& in, const track_t block_size) :
        data (in)
      {

        BlockSender source (in.size(), block_size);
        Sorter      pipe   (in);

        Thread::run_queue_threaded_pipe (source, TrackIndexRange(), pipe, VecItType(), *this);

      }




      MT_gradient_vector_sorter::VecItType MT_gradient_vector_sorter::get()
      {

        VecItType return_iterator (*candidates.begin());

        VecItType incremented (*candidates.begin());
        ++incremented;
        candidates.erase (candidates.begin());
        candidates.insert (incremented);

        return return_iterator;

      }



      bool MT_gradient_vector_sorter::Sorter::operator() (const TrackIndexRange& in, VecItType& out) const
      {

        VecItType start      (data.begin() + in.first);
        VecItType from_end   (data.begin() + in.second);
        VecItType from_start (start);

        for (; from_start < from_end; ++from_start) {
          if (from_start->get_gradient_per_unit_length() >= 0.0) {
            for (--from_end; from_end > from_start && from_end->get_gradient_per_unit_length() >= 0.0; --from_end);
            std::swap (*from_start, *from_end);
          }
        }

        std::sort (start, from_start);

        out = start;
        return true;

      }






      }
    }
  }
}


