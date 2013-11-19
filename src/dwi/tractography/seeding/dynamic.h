/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2012.

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

#ifndef __dwi_tractography_seeding_dynamic_h__
#define __dwi_tractography_seeding_dynamic_h__


#include "image/transform.h"

#include "thread/mutex.h"
#include "thread/queue.h"

#include "dwi/fmls.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/ACT/tissues.h"
#include "dwi/tractography/ACT/gmwmi.h"

#include "dwi/tractography/mapping/fixel_td_map.h"

#include "dwi/tractography/seeding/base.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

#include <fstream>
#include <queue>




//#define DYNAMIC_SEED_DEBUGGING



// TD_sum is set to this at the start of execution to prevent a divide_by_zero error
#define DYNAMIC_SEED_INITIAL_TD_SUM 0.01



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      namespace ACT { class GMWMI_finder; }

      namespace Seeding
      {




      class Fixel_TD_seed
      {

        public:
          Fixel_TD_seed (const FMLS::FOD_lobe& lobe, const float proc_mask_value, const Point<int>& vox) :
            FOD (lobe.get_integral()),
            TD (0.0),
            weight (proc_mask_value),
            dir (lobe.get_peak_dir()),
            voxel (vox) { }

          Fixel_TD_seed() :
            FOD (0.0),
            TD (0.0),
            weight (0.0),
            dir (),
            voxel (0, 0, 0) { }


          double get_FOD()    const { return FOD; }
          double get_TD()     const { return TD; }
          float  get_weight() const { return weight; }
          const Point<float>& get_dir()   const { return dir; }
          const Point<int>&   get_voxel() const { return voxel; }

          Fixel_TD_seed& operator+= (const float i) { TD += i; return *this; }

          float get_diff (const double mu) const { return ((TD * mu) - FOD); }
          float get_cost (const double mu) const { return (weight * Math::pow2 (get_diff (mu))); }

          float get_seed_prob (const double mu) const
          {
            const float diff = get_diff (mu);
            if (diff >= 0.0)
              return 0.0;
            return (Math::pow2 (diff / FOD));
          }


        private:
          double FOD;
          double TD;
          float weight;
          Point<float> dir;
          Point<int> voxel;


      };




      class Dynamic_ACT_additions
      {

          typedef Image::Interp::Linear< Image::Buffer<float>::voxel_type > Interp;

        public:
          Dynamic_ACT_additions (const std::string& path) :
            data (path),
            interp_template (Image::Buffer<float>::voxel_type (data)),
            gmwmi_finder (data) { }

          bool check_seed (Point<float>&);

        private:
          Image::Buffer<float> data;
          const Interp interp_template;
          ACT::GMWMI_finder gmwmi_finder;


      };





      class Dynamic : public Base, public Mapping::Fixel_TD_diff_map<Fixel_TD_seed>
      {

        typedef Fixel_TD_seed Fixel;

        typedef Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;


        public:
        Dynamic (const std::string&, Image::Buffer<float>&, const Math::RNG&, const DWI::Directions::FastLookupSet&);

        ~Dynamic();

        bool get_seed (Point<float>&, Point<float>&);

        // Although the FOD_TD_diff_map versions of these functions are OK, the FOD_TD_seed fixel class
        //   includes the processing mask weight, which is faster to access than the mask image
        bool operator() (const FMLS::FOD_lobes&);
        bool operator() (const Mapping::SetDixel&);


        private:
        using Fixel_map<Fixel>::accessor;
        using Fixel_map<Fixel>::fixels;

        using Mapping::Fixel_TD_diff_map<Fixel>::mu;
        using Mapping::Fixel_TD_diff_map<Fixel>::proc_mask;


        // Want to know statistics on dynamic seeding sampling
        uint64_t total_samples, total_seeds;
        Thread::Mutex mutex;


#ifdef DYNAMIC_SEED_DEBUGGING
        Tractography::Writer<float> seed_output;
        void write_seed (const Point<float>&);
#endif

        Image::Transform transform;

        Ptr<Dynamic_ACT_additions> act;

      };




      }
    }
  }
}

#endif

