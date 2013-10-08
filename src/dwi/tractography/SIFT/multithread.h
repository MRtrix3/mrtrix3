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



#ifndef __dwi_tractography_sift_multithread_h__
#define __dwi_tractography_sift_multithread_h__

#include "thread/exec.h"
#include "thread/mutex.h"
#include "thread/queue.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "dwi/fmls.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/SIFT/types.h"
#include "dwi/tractography/SIFT/sifter.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



#define TRACK_INDEX_BUFFER_SIZE 10000


      // Some processes in SIFT are fast for each streamline, but there are a large number of streamlines, so
      //   if multi-threading is done on a per-track basis the I/O associated with multi-threading begins to dominate
      // Instead, the input queue for multi-threading is filled with std::pair<track_t, track_t>'s, where the values
      //   are the start and end track indices to be processed

      typedef std::pair<track_t, track_t> TrackIndexRange;
      typedef Thread::Queue< TrackIndexRange > TrackIndexRangeQueue;

      class TrackIndexRangeWriter
      {

        public:
          TrackIndexRangeWriter (const track_t, const track_t, const std::string& message = std::string ());

          bool operator() (TrackIndexRange&);

        private:
          const track_t size, end;
          track_t start;
          Ptr<ProgressBar> progress;

      };



      class SIFTer;



      class TrackGradientCalculator
      {

        public:
          TrackGradientCalculator (const SIFTer& in, std::vector<Cost_fn_gradient_sort>& v, const double mu, const double r) :
            sifter (in),
            gradient_vector (v),
            current_mu (mu),
            current_roc_cost (r) { }

          bool operator() (const TrackIndexRange&);

        private:
          const SIFTer& sifter;
          std::vector<Cost_fn_gradient_sort>& gradient_vector;
          const double current_mu, current_roc_cost;

      };




      class LobeRemapper
      {

          typedef TrackContribution<Track_lobe_contribution> TckCont;

        public:
          LobeRemapper (SIFTer& s, std::vector<size_t>& r) :
            sifter   (s),
            remapper (r) { }

          bool operator() (const TrackIndexRange&);

        private:
          SIFTer& sifter;
          std::vector<size_t>& remapper;


      };



      // Receive mapped streamlines, convert to TckCont, allocate memory, assign to contributions[] vector,
      //   once this is all completed lock a mutex and update the lobe TD's
      class MappedTrackReceiver
      {

        public:
          MappedTrackReceiver (SIFTer& s) :
            sifter (s),
            mutex (new Thread::Mutex()) { }

          MappedTrackReceiver (const MappedTrackReceiver& that) :
            sifter (that.sifter),
            mutex (that.mutex) { }

          bool operator() (const Mapping::SetDixel&);

        private:
          SIFTer& sifter;
          RefPtr<Thread::Mutex> mutex;

      };





      // Sorting of the gradient vector in SIFT is done in a multi-threaded fashion, in a number of stages:
      // * Gradient vector is split into blocks of equal size
      // * Within each block:
      //     - Non-negative gradients are pushed to the end of the block (no need to sort these)
      //     - Negative gradients within the block are sorted fully
      //     - An iterator to the first entry in the sorted block is written to a set
      // * For streamline filtering, the candidate streamline is chosen from the beginning of this set.
      //     The entry in the set corresponding to the gradient vector is removed, but the iterator WITHIN THE BLOCK
      //     is incremented and re-written to the set; this allows multiple streamlines from a single block to
      //     be filtered in a single iteration, provided the gradient is less than that of the candidate streamline
      //     from all other blocks

      class MT_gradient_vector_sorter
      {

          typedef std::vector<Cost_fn_gradient_sort> VecType;
          typedef VecType::iterator VecItType;

          class Comparator {
            public:
              bool operator() (const VecItType& a, const VecItType& b) const { return (a->get_gradient_per_unit_length() < b->get_gradient_per_unit_length()); }
          };
          typedef std::set<VecItType, Comparator> SetType;


        public:
          MT_gradient_vector_sorter (VecType& in, const track_t);

          VecItType get();

          bool operator() (const VecItType& in)
          {
            candidates.insert (in);
            return true;
          }



        private:
          VecType& data;

          SetType candidates;




          class BlockSender
          {

            public:
              BlockSender (const track_t count, const track_t size) :
                num_tracks (count),
                block_size (size),
                counter (0) { }

              bool operator() (TrackIndexRange& out)
              {
                if (counter == num_tracks) {
                  out.first = out.second = 0;
                  return false;
                }
                out.first = counter;
                counter = std::min (counter + block_size, num_tracks);
                out.second = counter;
                return true;
              }

            private:
              const track_t num_tracks, block_size;
              track_t counter;

          };

          class Sorter
          {

            public:
              Sorter (VecType& in) :
                data  (in) { }

              Sorter (const Sorter& that) :
                data  (that.data) { }

              bool operator() (const TrackIndexRange&, VecItType&) const;

            private:
              VecType& data;

          };


      };





      }
    }
  }
}


#endif

