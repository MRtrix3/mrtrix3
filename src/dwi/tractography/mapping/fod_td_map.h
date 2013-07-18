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



#ifndef __dwi_tractography_mapping_fod_td_map_h__
#define __dwi_tractography_mapping_fod_td_map_h__


#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/nav.h"
#include "image/transform.h"
#include "image/utils.h"
#include "image/voxel.h"
#include "image/interp/linear.h"

#include "dwi/fmls.h"
#include "dwi/fod_map.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/ACT/tissues.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Mapping
      {



        // Templated Lobe class MUST provide operator+= (const float) for adding streamline density

      template <class Lobe>
      class FOD_TD_map : public FOD_map<Lobe>
      {

        typedef typename FOD_map<Lobe>::MapVoxel MapVoxel;
        typedef typename FOD_map<Lobe>::VoxelAccessor VoxelAccessor;

        public:

        template <typename Info>
        FOD_TD_map (const Info& info, const DWI::Directions::FastLookupSet& directions) :
        FOD_map<Lobe> (info),
        dirs (directions) { }

        virtual ~FOD_TD_map() { }


        bool operator() (const SetDixel& in);


        protected:
        using FOD_map<Lobe>::accessor;
        using FOD_map<Lobe>::lobes;


        const DWI::Directions::FastLookupSet& dirs;

        size_t dix2lobe (const Dixel&);


        FOD_TD_map (const FOD_TD_map& that) : FOD_map<Lobe> (that), dirs (that.dirs) { assert (0); }

      };





      template <class Lobe>
      bool FOD_TD_map<Lobe>::operator() (const SetDixel& in)
      {
        for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          const size_t lobe_index = dix2lobe (*i);
          if (lobe_index)
            lobes[lobe_index] += i->get_value();
        }
        return true;
      }


      template <class Lobe>
      size_t FOD_TD_map<Lobe>::dix2lobe (const Dixel& in)
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
        return map_voxel.dir2lobe (in.get_dir());
      }




      // Templated Lobe class MUST also provide void set_weight (const float), for setting weight of processing mask

      template <class Lobe>
      class FOD_TD_diff_map : public FOD_TD_map<Lobe>
      {

        typedef typename FOD_map<Lobe>::MapVoxel MapVoxel;
        typedef typename FOD_map<Lobe>::VoxelAccessor VoxelAccessor;

        public:
          template <class Set>
          FOD_TD_diff_map (Set& dwi, const DWI::Directions::FastLookupSet& dirs) :
          FOD_TD_map<Lobe> (dwi, dirs),
          proc_mask (FOD_map<Lobe>::info(), "FOD map processing mask"),
          FOD_sum (0.0),
          TD_sum (0.0)
          {
            fill_proc_mask (dwi, proc_mask);
          }

          bool operator() (const FMLS::FOD_lobes& in);
          bool operator() (const SetDixel& in);

          double mu() const { return FOD_sum / TD_sum; }

          using FOD_TD_map<Lobe>::begin;

        protected:
          using FOD_map<Lobe>::accessor;
          using FOD_map<Lobe>::lobes;
          using FOD_TD_map<Lobe>::dirs;

          Image::BufferScratch<float> proc_mask;
          double FOD_sum, TD_sum;


        private:
          template <class Set>
          void fill_proc_mask (Set&, Image::BufferScratch<float>&);


        protected:
          FOD_TD_diff_map (const FOD_TD_diff_map& that) : FOD_TD_map<Lobe> (that), proc_mask (that.proc_mask.info()), FOD_sum (0.0), TD_sum (0.0) { assert (0); }

      };



      template <class Lobe>
      bool FOD_TD_diff_map<Lobe>::operator() (const FMLS::FOD_lobes& in)
      {
        if (!FOD_map<Lobe>::operator() (in))
          return false;
        Image::BufferScratch<float>::voxel_type mask (proc_mask);
        const float mask_value = Image::Nav::get_value_at_pos (mask, in.vox);
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value()) {
          for (typename FOD_map<Lobe>::Iterator i = begin (v); i; ++i) {
            i().set_weight (mask_value); // Note: Lobe class must provide this (even if it does nothing)
            FOD_sum += i().get_FOD() * mask_value;
          }
        }
        return true;
      }

      template <class Lobe>
      bool FOD_TD_diff_map<Lobe>::operator() (const SetDixel& in)
      {
        Image::BufferScratch<float>::voxel_type v (proc_mask);
        float total_contribution = 0.0;
        for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
          const size_t lobe_index = FOD_TD_map<Lobe>::dix2lobe (*i);
          if (lobe_index) {
            lobes[lobe_index] += i->get_value(); // Note: Lobe class must provide operator+= (const float)
            Image::Nav::set_pos (v, *i);
            total_contribution += v.value() * i->get_value();
          }
        }
        TD_sum += total_contribution;
        return true;
      }




      template <class Lobe>
      template <class Set>
      void FOD_TD_diff_map<Lobe>::fill_proc_mask (Set& in_dwi, Image::BufferScratch<float>& out_mask)
      {

        Image::Buffer<float>::voxel_type dwi (in_dwi);
        dwi[3] = 0;
        Image::Transform transform_dwi (dwi);
        Image::BufferScratch<float>::voxel_type mask (out_mask);

        App::Options opt = App::get_options ("proc_mask");
        if (opt.size()) {

          // User-specified processing mask
          Image::Buffer<float> in_image (opt[0][0]);
          Image::Buffer<float>::voxel_type image (in_image);
          if (!Image::dimensions_match (proc_mask, image, 0, 3))
            throw Exception ("Dimensions of processing mask image provided using -proc_mask option must match relevant FOD image");
          Image::copy_with_progress_message ("Copying processing mask to memory... ", image, mask, 0, 3);

        } else {

          App::Options opt = App::get_options ("act");
          if (opt.size()) {

            Image::Buffer<float> in_anat (opt[0][0]);
            if (in_anat.ndim() != 4 || in_anat.dim(3) != 4)
              throw Exception ("Image passed using the -act option must be in the 4TT format");

            const int os_ratio = 10;
            const float os_step = 1.0 / float(os_ratio);
            const float os_offset = 0.5 * os_step;

            // TODO Could multi-thread this

            Image::Buffer<float>::voxel_type anat (in_anat);
            Image::Interp::Linear< Image::Buffer<float>::voxel_type > interp (anat);

            Image::LoopInOrder loop (dwi, "Creating processing mask from anatomical image...", 0, 3);
            for (loop.start (dwi, mask); loop.ok(); loop.next (dwi, mask)) {
              if (dwi.value()) {

                // Get the upper & lower bounds of the diffusion voxel, in ACT image voxel space
                ssize_t limits[3][2] = {{anat.dim(0)-1, 0}, {anat.dim(1)-1, 0}, {anat.dim(2)-1, 0}};
                Point<int> edges;
                Point<float> v_dwi;
                for (edges[2] = 0; edges[2] != 2; ++edges[2]) {
                  v_dwi[2] = dwi[2] - 0.5 + edges[2];
                  for (edges[1] = 0; edges[1] != 2; ++edges[1]) {
                    v_dwi[1] = dwi[1] - 0.5 + edges[1];
                    for (edges[0] = 0; edges[0] != 2; ++edges[0]) {
                      v_dwi[0] = dwi[0] - 0.5 + edges[0];

                      const Point<float> p_scanner (transform_dwi.voxel2scanner (v_dwi));
                      const Point<float> v_anat (interp.scanner2voxel (p_scanner));

                      for (size_t axis = 0; axis != 3; ++axis) {
                        limits[axis][0] = std::min (limits[axis][0], ssize_t(Math::floor (v_anat[axis])));
                        limits[axis][1] = std::max (limits[axis][1], ssize_t(Math::ceil  (v_anat[axis])));
                      }

                } } }

                // Ensure that these bounds are actually within the anatomical image
                for (size_t axis = 0; axis != 3; ++axis) {
                  limits[axis][0] = std::max (limits[axis][0], ssize_t(0));
                  limits[axis][1] = std::min (limits[axis][1], ssize_t(anat.dim (axis) - 1));
                }

                // Test ALL ACT voxels within this cube; want to know if there is any chance of
                //   non-zero and non-unity WM partial volume within this diffusion voxel
                bool WM_lt_rest = false, WM_gt_rest = false;
                for (anat[2] = limits[2][0]; anat[2] <= limits[2][1]; ++anat[2]) {
                  for (anat[1] = limits[1][0]; anat[1] <= limits[1][1]; ++anat[1]) {
                    for (anat[0] = limits[0][0]; anat[0] <= limits[0][1]; ++anat[0]) {

                      Tractography::ACT::Tissues tissues (anat);
                      if (tissues.get_wm() < tissues.get_gm() + tissues.get_csf())
                        WM_lt_rest = true;
                      else if (tissues.get_wm() > tissues.get_gm() + tissues.get_csf())
                        WM_gt_rest = true;

                } } }

                // Determine whether or not this voxel needs to be tested using a full partial-volume approach
                if (!WM_gt_rest) {
                  mask.value() = 0.0;
                } else if (!WM_lt_rest) {
                  mask.value() = 1.0;
                } else {

                  // Full partial volume test
                  size_t wm_count = 0, total_count = 0;
                  Point<int> i;
                  Point<float> v_dwi;
                  for (i[2] = 0; i[2] != os_ratio; ++i[2]) {
                    v_dwi[2] = dwi[2] - 0.5 + os_offset + (i[2] * os_step);
                    for (i[1] = 0; i[1] != os_ratio; ++i[1]) {
                      v_dwi[1] = dwi[1] - 0.5 + os_offset + (i[1] * os_step);
                      for (i[0] = 0; i[0] != os_ratio; ++i[0]) {
                        v_dwi[0] = dwi[0] - 0.5 + os_offset + (i[0] * os_step);

                        const Point<float> p_scanner (transform_dwi.voxel2scanner (v_dwi));
                        if (!interp.scanner (p_scanner)) {
                          ++total_count;
                          Tractography::ACT::Tissues tissues (interp);
                          if (tissues.valid() && (tissues.get_wm() > tissues.get_gm() + tissues.get_csf()))
                            ++wm_count;

                        }

                  } } }

                  if (total_count)
                    mask.value() = Math::pow2 (float(wm_count) / float(total_count));
                  else
                    mask.value() = 0.0;

                }

              } else { // No DWI data in this voxel
                mask.value() = 0.0;
              }
            }

          } else {

            Image::LoopInOrder loop (dwi, "Creating homogeneous processing mask...", 0, 3);
            dwi[3] = 0;
            for (loop.start (dwi, mask); loop.ok(); loop.next (dwi, mask))
              mask.value() = (dwi.value() && finite (dwi.value())) ? 1.0 : 0.0;

          }

        }

      }





      }
    }
  }
}


#endif
