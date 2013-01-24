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



#ifndef __dwi_tractography_sift_sifter_h__
#define __dwi_tractography_sift_sifter_h__

#include <fstream>

#include "progressbar.h"

#include "file/utils.h"

#include "image/buffer.h"
#include "image/copy.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/voxel.h"

#include "math/rng.h"
#include "math/SH.h"

#include "thread/exec.h"
#include "thread/queue.h"

#include "dwi/fmls.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/mapping/common.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/writer.h"

#include "dwi/tractography/SIFT/types.h"
#include "dwi/tractography/SIFT/multithread.h"


#include <iomanip>
#include <algorithm>


#include "dwi/tractography/mapping/fod_td_map.h"




#define SIFT_SORT_BLOCK_SIZE 1024



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {




      class SIFTer : public Mapping::FOD_TD_diff_map<FOD_TD_weighted>
      {

        typedef FOD_TD_weighted Lobe;

        typedef Mapping::FOD_TD_diff_map<Lobe> MapType;
        typedef FOD_map<Lobe>::MapVoxel MapVoxel;
        typedef FOD_map<Lobe>::VoxelAccessor VoxelAccessor;
        typedef Mapping::Dixel Dixel;
        typedef Mapping::SetDixel SetDixel;
        typedef TrackContribution<Track_lobe_contribution> TckCont;


        public:
        SIFTer (Image::Buffer<float>&, const DWI::Directions::FastLookupSet&);

        ~SIFTer();


        // CORE OPERATIONS
        void perform_FOD_segmentation ();
        void map_streamlines (const std::string&);
        void remove_excluded_lobes();
        void perform_filtering();
        void output_filtered_tracks (const std::string&, const std::string&) const;


        // OUTPUT FUNCTIONS
        void output_all_debug_images (const std::string&) const;
        void output_proc_mask (const std::string&);


        // CONFIGURATION OPTIONS

        // Functions for setting up the FOD segmentation before it is run
        void FMLS_set_peak_value_threshold (const float i) { fmls.set_peak_value_threshold (i); }
        void FMLS_set_dilate_lookup_table  ()              { fmls.set_dilate_lookup_table (true); }
        void FMLS_set_create_null_lobe     ()              { fmls.set_create_null_lobe (true); }

        // These configure the filtering before it is run
        void set_remove_untracked_lobes (const bool i)         { remove_untracked_lobes = i; }
        void set_min_FOD_integral       (const float i)        { min_FOD_integral = i; }
        void set_term_number            (const track_t i)      { term_number = i; }
        void set_term_ratio             (const float i)        { term_ratio = i; }
        void set_term_mu                (const float i)        { term_mu = i; }
        void set_csv_path               (const std::string& i) { csv_path = i; }

        void set_regular_outputs (const std::vector<int>&, const bool);


        bool operator() (const FOD_lobes& in) { return MapType::operator() (in); }

        size_t num_tracks() const { return contributions.size(); }


        // DEBUGGING
        void check_TD();
        void test_sorting_block_size (const size_t) const;


        protected:
        using FOD_map<Lobe>::accessor;
        using FOD_map<Lobe>::lobes;

        using MapType::FOD_sum;
        using MapType::TD_sum;
        using MapType::mu;
        using MapType::proc_mask;


        // Members required at construction
        Image::Buffer<float>& fod_data;
        Image::Header H;
        const DWI::Directions::FastLookupSet& dirs;
        DWI::FOD_FMLS fmls;


        std::string tck_file_path;


        // Dynamic member for filtering
        std::vector<TckCont*> contributions;


        // User-controllable settings
        std::vector<track_t> output_at_counts;
        bool    output_debug;
        bool    remove_untracked_lobes;
        float   min_FOD_integral;
        track_t term_number;
        float   term_ratio;
        double  term_mu;
        std::string csv_path;


        // Convenience functions
        double calc_cost_function() const;
        double calc_roc_cost_function() const;

        double calc_gradient (const track_t, const double, const double) const;


        // Output functions - non-essential, mostly debugging outputs
        void output_target_image (const std::string&) const;
        void output_target_image_sh (const std::string&) const;
        void output_tdi (const std::string& path) const;
        void output_tdi_null_lobes (const std::string& path) const;
        void output_tdi_sh (const std::string&) const;
        void output_error_images (const std::string&, const std::string&, const std::string&) const;
        void output_scatterplot (const std::string&) const;
        void output_lobe_count_image (const std::string&) const;
        void output_non_contributing_streamlines (const std::string&, const std::string&) const;
        void output_untracked_lobes (const std::string&, const std::string&) const;


        friend class TrackGradientCalculator;
        friend class LobeRemapper;
        friend class MappedTrackReceiver;


        SIFTer (const SIFTer& that) :
          MapType (that),
          fod_data (that.fod_data),
          dirs (that.dirs),
          fmls (that.fmls) { assert (0); }


      };




      }
    }
  }
}


#endif


