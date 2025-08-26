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

#ifndef __dwi_tractography_sift_track_index_range_h__
#define __dwi_tractography_sift_track_index_range_h__

#include "progressbar.h"
#include "dwi/tractography/SIFT/types.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



#define SIFT_TRACK_INDEX_BUFFER_SIZE 10000



      class TrackIndexRange
      {
        public:
          class iterator : public std::iterator<std::forward_iterator_tag, value_type, std::ptrdiff_t, const value_type*, value_type>
          {
            public:
              explicit iterator (const track_t value) : index (value) { }
              iterator& operator++() { ++index; return *this; }
              iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }
              bool operator== (const iterator& other) const { return index == other.index; }
              bool operator!= (const iterator& other) const { return !(*this == other); }
              track_t operator*() { return index; }
            private:
              track_t index;
          };

          TrackIndexRange (const track_t index_start, const track_t index_end) :
              index_start (index_start),
              index_end (index_end) { }
          TrackIndexRange (const TrackIndexRange& that) = default;
          TrackIndexRange() : index_start (0), index_end (0) { }

          iterator begin() const { return iterator(index_start); }
          iterator end()   const { return iterator(index_end); }

          void invalidate() { index_start = index_end = 0; }
          void set (const track_t start, const track_t end) { index_start = start; index_end = end; }

          std::pair<track_t, track_t> operator() () const { return std::make_pair (index_start, index_end); }

        private:
          track_t index_start, index_end;
      };


      using TrackIndexRangeQueue = Thread::Queue<TrackIndexRange>;



      // Some processes in SIFT are fast for each streamline, but there are a large number of streamlines, so
      //   if multi-threading is done on a per-track basis the I/O associated with multi-threading begins to dominate
      // Instead, the input queue for multi-threading is filled with std::pair<track_t, track_t>'s, where the values
      //   are the start and end track indices to be processed
      class TrackIndexRangeWriter
      {

        public:
          TrackIndexRangeWriter (const track_t buffer_size, const track_t num_tracks, const std::string& message = std::string ());

          bool operator() (TrackIndexRange&);

        private:
          const track_t size, end;
          track_t start;
          std::unique_ptr<ProgressBar> progress;

      };




      }
    }
  }
}


#endif

