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



#ifndef __dwi_tractography_mapping_fixel_td_map_h__
#define __dwi_tractography_mapping_fixel_td_map_h__

#include "app.h"
#include "point.h"

#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/iterator.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/threaded_loop.h"
#include "image/transform.h"
#include "image/utils.h"
#include "image/voxel.h"
#include "image/interp/linear.h"

#include "dwi/fmls.h"
#include "dwi/fixel_map.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/ACT/tissues.h"



namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {
    namespace Tractography
    {
      namespace Mapping
      {



      extern const App::OptionGroup FixelMapProcMaskOption;




        // Templated Fixel class MUST provide operator+= (const float) for adding streamline density

      template <class Fixel>
      class Fixel_TD_map : public Fixel_map<Fixel>
      {

        typedef typename Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef typename Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;

        public:

        template <typename Info>
        Fixel_TD_map (const Info& info, const DWI::Directions::FastLookupSet& directions) :
        Fixel_map<Fixel> (info),
        dirs (directions) { }

        virtual ~Fixel_TD_map() { }


        bool operator() (const SetDixel& in);


        protected:
        using Fixel_map<Fixel>::accessor;
        using Fixel_map<Fixel>::fixels;


        const DWI::Directions::FastLookupSet& dirs;

        size_t dixel2fixel (const Dixel&);


        Fixel_TD_map (const Fixel_TD_map& that) : Fixel_map<Fixel> (that), dirs (that.dirs) { assert (0); }

      };





      template <class Fixel>
      bool Fixel_TD_map<Fixel>::operator() (const SetDixel& in)
      {
        for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          const size_t fixel_index = dixel2fixel (*i);
          if (fixel_index)
            fixels[fixel_index] += i->get_value();
        }
        return true;
      }


      template <class Fixel>
      size_t Fixel_TD_map<Fixel>::dixel2fixel (const Dixel& in)
      {
        if (!Image::Nav::within_bounds (accessor, in))
          return 0;
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in);
        if (!v.value())
          return 0;
        const MapVoxel& map_voxel (*v.value());
        if (map_voxel.empty())
          return 0;
        return map_voxel.dir2fixel (in.get_dir());
      }




      // Templated Fixel class MUST also provide void set_weight (const float), for setting weight of processing mask

      template <class Fixel>
      class Fixel_TD_diff_map : public Fixel_TD_map<Fixel>
      {

        typedef typename Fixel_map<Fixel>::MapVoxel MapVoxel;
        typedef typename Fixel_map<Fixel>::VoxelAccessor VoxelAccessor;

        public:
          template <class Set>
          Fixel_TD_diff_map (Set& dwi, const DWI::Directions::FastLookupSet& dirs) :
          Fixel_TD_map<Fixel> (dwi, dirs),
          proc_mask (Fixel_map<Fixel>::info(), "FOD map processing mask"),
          FOD_sum (0.0),
          TD_sum (0.0)
          {
            initialise (dwi);
          }

          bool operator() (const FMLS::FOD_lobes& in);
          bool operator() (const SetDixel& in);

          double mu() const { return FOD_sum / TD_sum; }
          bool have_act_data() const { return act_4tt; }

          using Fixel_TD_map<Fixel>::begin;

        protected:
          using Fixel_map<Fixel>::accessor;
          using Fixel_map<Fixel>::fixels;
          using Fixel_TD_map<Fixel>::dirs;

          Ptr< Image::BufferScratch<float> > act_4tt;
          Image::BufferScratch<float> proc_mask;
          double FOD_sum, TD_sum;


        private:
          template <class Set>
          void initialise (Set&);


          // Private functor for performing ACT image regridding
          template <class Set>
          class ResampleFunctor
          {
            public:

              ResampleFunctor (Set& dwi, Image::Buffer<float>& anat, Image::BufferScratch<float>& out) :
                  v_dwi (dwi),
                  transform_dwi (new Image::Transform (dwi)),
                  v_anat (anat),
                  interp_anat (v_anat),
                  v_out (out) { v_dwi[3] = 0; }

              ResampleFunctor (const ResampleFunctor& that) :
                  v_dwi (that.v_dwi),
                  transform_dwi (that.transform_dwi),
                  v_anat (that.v_anat),
                  interp_anat (v_anat),
                  v_out (that.v_out) { v_dwi[3] = 0; }

              void operator() (const Image::Iterator&);

            private:
              typename Set::voxel_type v_dwi;
              RefPtr<Image::Transform> transform_dwi;
              Image::Buffer<float>::voxel_type v_anat;
              Image::Interp::Linear< Image::Buffer<float>::voxel_type > interp_anat;
              Image::BufferScratch<float>::voxel_type v_out;
          };


        protected:
          Fixel_TD_diff_map (const Fixel_TD_diff_map& that) : Fixel_TD_map<Fixel> (that), proc_mask (that.proc_mask.info()), FOD_sum (0.0), TD_sum (0.0) { assert (0); }

      };



      template <class Fixel>
      bool Fixel_TD_diff_map<Fixel>::operator() (const FMLS::FOD_lobes& in)
      {
        if (!Fixel_map<Fixel>::operator() (in))
          return false;
        Image::BufferScratch<float>::voxel_type mask (proc_mask);
        const float mask_value = Image::Nav::get_value_at_pos (mask, in.vox);
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value()) {
          for (typename Fixel_map<Fixel>::Iterator i = begin (v); i; ++i) {
            i().set_weight (mask_value); // Note: Fixel class must provide this (even if it does nothing)
            FOD_sum += i().get_FOD() * mask_value;
          }
        }
        return true;
      }

      template <class Fixel>
      bool Fixel_TD_diff_map<Fixel>::operator() (const SetDixel& in)
      {
        Image::BufferScratch<float>::voxel_type v (proc_mask);
        float total_contribution = 0.0;
        for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          const size_t fixel_index = Fixel_TD_map<Fixel>::dixel2fixel (*i);
          if (fixel_index) {
            fixels[fixel_index] += i->get_value(); // Note: Fixel class must provide operator+= (const float)
            Image::Nav::set_pos (v, *i);
            total_contribution += v.value() * i->get_value();
          }
        }
        TD_sum += total_contribution;
        return true;
      }




      template <class Fixel>
      template <class Set>
      void Fixel_TD_diff_map<Fixel>::initialise (Set& in_dwi)
      {

        Image::BufferScratch<float>::voxel_type mask (proc_mask);

        App::Options opt = App::get_options ("proc_mask");
        if (opt.size()) {

          // User-specified processing mask
          Image::Buffer<float> in_image (opt[0][0]);
          Image::Buffer<float>::voxel_type image (in_image);
          if (!Image::dimensions_match (mask, image, 0, 3))
            throw Exception ("Dimensions of processing mask image provided using -proc_mask option must match relevant fixel image");
          Image::copy_with_progress_message ("Copying processing mask to memory... ", image, mask, 0, 3);

        } else {

          App::Options opt = App::get_options ("act");
          if (opt.size()) {

            Image::Buffer<float> in_anat (opt[0][0]);
            if (in_anat.ndim() != 4 || in_anat.dim(3) != 4)
              throw Exception ("Image passed using the -act option must be in the 4TT format");

            Image::Info info_4tt (in_dwi);
            info_4tt.set_ndim (4);
            info_4tt.dim(3) = 4;
            act_4tt = new Image::BufferScratch<float> (info_4tt);

            Image::ThreadedLoop threaded_loop ("resampling ACT 4TT image to fixel image space...", in_dwi, 1, 0, 3);
            ResampleFunctor<Set> functor (in_dwi, in_anat, *act_4tt);
            threaded_loop.run (functor);

            // Once all of the 4TT data has been read in, use it to derive the processing mask
            Image::BufferScratch<float>::voxel_type v_4tt (*act_4tt);
            Image::LoopInOrder loop (v_4tt, 0, 3);
            v_4tt[3] = 2; // Access the WM fraction
            for (loop.start (v_4tt, mask); loop.ok(); loop.next (v_4tt, mask))
              mask.value() = Math::pow2<float> (v_4tt.value()); // Processing mask value is the square of the WM fraction

          } else {

            typename Set::voxel_type dwi (in_dwi);
            Image::LoopInOrder loop (dwi, "Creating homogeneous processing mask...", 0, 3);
            dwi[3] = 0;
            for (loop.start (dwi, mask); loop.ok(); loop.next (dwi, mask))
              mask.value() = (dwi.value() && finite (dwi.value())) ? 1.0 : 0.0;

          }

        }

      }




      template <class Fixel>
      template <class Set>
      void Fixel_TD_diff_map<Fixel>::ResampleFunctor<Set>::operator() (const Image::Iterator& pos)
      {

        static const int os_ratio = 10;
        static const float os_step = 1.0 / float(os_ratio);
        static const float os_offset = 0.5 * os_step;

        Image::voxel_assign (v_dwi, pos);
        Image::voxel_assign (v_out, pos);
        if (v_dwi.value() && finite (v_dwi.value())) {

          // Jump straight to the partial volume calculation
          size_t cgm_count = 0, sgm_count = 0, wm_count = 0, csf_count = 0, total_count = 0;

          Point<int> i;
          Point<float> subvoxel_pos_dwi;
          for (i[2] = 0; i[2] != os_ratio; ++i[2]) {
            subvoxel_pos_dwi[2] = v_dwi[2] - 0.5 + os_offset + (i[2] * os_step);
            for (i[1] = 0; i[1] != os_ratio; ++i[1]) {
              subvoxel_pos_dwi[1] = v_dwi[1] - 0.5 + os_offset + (i[1] * os_step);
              for (i[0] = 0; i[0] != os_ratio; ++i[0]) {
                subvoxel_pos_dwi[0] = v_dwi[0] - 0.5 + os_offset + (i[0] * os_step);

                const Point<float> p_scanner (transform_dwi->voxel2scanner (subvoxel_pos_dwi));
                if (!interp_anat.scanner (p_scanner)) {
                  ++total_count;
                  Tractography::ACT::Tissues tissues (interp_anat);
                  if (tissues.valid()) {
                    if (tissues.get_cgm() > tissues.get_sgm() + tissues.get_wm() + tissues.get_csf())
                      ++cgm_count;
                    else if (tissues.get_sgm() > tissues.get_cgm() + tissues.get_wm() + tissues.get_csf())
                      ++sgm_count;
                    else if (tissues.get_wm() > tissues.get_gm() + tissues.get_csf())
                      ++wm_count;
                    else if (tissues.get_csf() > tissues.get_gm() + tissues.get_wm())
                      ++csf_count;
                  }
                }

          } } }

          if (total_count) {
            v_out[3] = 0; v_out.value() = cgm_count / float(total_count);
            v_out[3] = 1; v_out.value() = sgm_count / float(total_count);
            v_out[3] = 2; v_out.value() =  wm_count / float(total_count);
            v_out[3] = 3; v_out.value() = csf_count / float(total_count);
          } else {
            for (v_out[3] = 0; v_out[3] != 4; ++v_out[3])
              v_out.value() = 0.0;
          }

        } else {
          for (v_out[3] = 0; v_out[3] != 4; ++v_out[3])
            v_out.value() = 0.0;
        }
      }



      }
    }
  }
}


#endif
