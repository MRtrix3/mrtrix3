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



#ifndef __dwi_tractography_sift2_fixel_h__
#define __dwi_tractography_sift2_fixel_h__


#include <limits>

#include "dwi/tractography/SIFT/model_base.h"
#include "dwi/tractography/SIFT/types.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT2
      {



      class Fixel : public SIFT::FixelBase
      {

        typedef SIFT::track_t track_t;

        public:
          Fixel () :
              SIFT::FixelBase (),
              excluded    (false),
              count       (0),
              orig_TD     (0.0),
              mean_coeff  (0.0) { }

          Fixel (const FMLS::FOD_lobe& lobe) :
              SIFT::FixelBase (lobe),
              excluded    (false),
              count       (0),
              orig_TD     (0.0),
              mean_coeff  (0.0) { }

          Fixel (const Fixel& that) :
              SIFT::FixelBase (that),
              excluded    (false),
              count       (0),
              orig_TD     (that.orig_TD),
              mean_coeff  (0.0) { }


          // Overloaded += operator; want to track the number of streamlines as well as the sum of lengths
          Fixel& operator+= (const double length) { TD += length; ++count; return *this; }

          // This is for the multi-threaded fixel updater - only gets one update per thread
          void add_TD (const double sum_lengths, const track_t num) { TD += sum_lengths; count += num; }

          // Overloaded clear_TD(); latch the previous TD, and clear the count field also
          void clear_TD() { TD = 0.0; count = 0; }

          // Functions for altering the state of this more advanced fixel class
          void exclude()          { excluded = true; }
          void store_orig_TD()    { orig_TD = get_TD(); }
          void clear_mean_coeff() { mean_coeff = 0.0; }

          void add_to_mean_coeff (const double i) { mean_coeff += i; }
          void normalise_mean_coeff()             { if (orig_TD) mean_coeff /= orig_TD; if (count < 2) mean_coeff = 0.0; }

          // get() functions
          bool    is_excluded()     const { return excluded; }
          track_t get_count()       const { return count; }
          double  get_orig_TD()     const { return orig_TD; }
          double  get_mean_coeff()  const { return mean_coeff; }


        private:
          bool excluded;
          track_t count;
          double orig_TD, mean_coeff;

      };




      }
    }
  }
}


#endif


