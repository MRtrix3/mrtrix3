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

#include <fstream>
#include <queue>

#include "transform.h"
#include "thread_queue.h"
#include "dwi/fmls.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/ACT/tissues.h"
#include "dwi/tractography/ACT/gmwmi.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/seeding/base.h"
#include "dwi/tractography/SIFT/model_base.h"
#include "dwi/tractography/tracking/generated_track.h"
#include "dwi/tractography/tracking/write_kernel.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"




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



      class Fixel_TD_seed : public SIFT::FixelBase
      {

        public:
          Fixel_TD_seed (const FMLS::FOD_lobe& lobe) :
            SIFT::FixelBase (lobe),
            voxel (-1, -1, -1) { }

          Fixel_TD_seed() :
            SIFT::FixelBase (),
            voxel (-1, -1, -1) { }


          void set_voxel (const Point<int>& i) { voxel = i; }
          const Point<int>&   get_voxel() const { return voxel; }


          float get_seed_prob (const double mu) const
          {
            const float diff = get_diff (mu);
            if (diff >= 0.0)
              return 0.0;
            return (Math::pow2 (diff / FOD));
          }


        private:
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





      class Dynamic : public Base, public SIFT::ModelBase<Fixel_TD_seed>
      {

        typedef Fixel_TD_seed Fixel;

        typedef Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;


        public:
        Dynamic (const std::string&, Image::Buffer<float>&, const DWI::Directions::FastLookupSet&);

        ~Dynamic();

        bool get_seed (Point<float>&, Point<float>&);

        // Although the ModelBase version of this function is OK, the Fixel_TD_seed class
        //   includes the voxel location for easier determination of seed location
        bool operator() (const FMLS::FOD_lobes&);

        bool operator() (const Mapping::SetDixel& i)
        {
          if (!i.weight) // Flags that tracking should terminate
            return false;
          return SIFT::ModelBase<Fixel_TD_seed>::operator() (i);
        }


        private:
        using Fixel_map<Fixel>::accessor;
        using Fixel_map<Fixel>::fixels;

        using SIFT::ModelBase<Fixel>::mu;
        using SIFT::ModelBase<Fixel>::proc_mask;


        // Want to know statistics on dynamic seeding sampling
        uint64_t total_samples, total_seeds;
        std::mutex mutex;


#ifdef DYNAMIC_SEED_DEBUGGING
        Tractography::Writer<float> seed_output;
        void write_seed (const Point<float>&);
#endif

        Image::Transform transform;

        std::unique_ptr<Dynamic_ACT_additions> act;

      };





      class WriteKernelDynamic : public Tracking::WriteKernel
      {
        public:
          WriteKernelDynamic (const Tracking::SharedBase& shared, const std::string& output_file, const DWI::Tractography::Properties& properties) :
              Tracking::WriteKernel (shared, output_file, properties) { }
          bool operator() (const Tracking::GeneratedTrack&, Tractography::Streamline<>&);
      };





      }
    }
  }
}

#endif

