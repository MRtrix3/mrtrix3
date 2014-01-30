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



#ifndef __dwi_tractography_sift_proc_mask_h__
#define __dwi_tractography_sift_proc_mask_h__

#include "app.h"
#include "ptr.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/info.h"
#include "image/iterator.h"
#include "image/loop.h"
#include "image/threaded_loop.h"
#include "image/transform.h"
#include "image/utils.h"
#include "image/voxel.h"

#include "image/interp/linear.h"

#include "dwi/tractography/ACT/tissues.h"



namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      extern const App::OptionGroup SIFTModelProcMaskOption;


      template <class Set>
      class ResampleFunctor;


      template <class Set>
      void initialise_processing_mask (Set& in_dwi, Image::BufferScratch<float>& proc_mask, Ptr< Image::BufferScratch<float> >& act_4tt)
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
            Image::BufferScratch<float>::voxel_type v_4tt (*act_4tt);

            // Test to see if the image has already been re-gridded to match the fixel image
            // If it has, can do a direct import
            if (Image::dimensions_match (v_4tt, in_anat, 0, 3)) {
              INFO ("4TT image dimensions match fixel image - importing directly");
              Image::Buffer<float>::voxel_type v_anat (in_anat);
              Image::copy (v_anat, v_4tt);
            } else {
              Image::ThreadedLoop threaded_loop ("resampling ACT 4TT image to fixel image space...", in_dwi, 1, 0, 3);
              ResampleFunctor<Set> functor (in_dwi, in_anat, *act_4tt);
              threaded_loop.run (functor);
            }

            // Once all of the 4TT data has been read in, use it to derive the processing mask
            Image::LoopInOrder loop (v_4tt, 0, 3);
            v_4tt[3] = 2; // Access the WM fraction
            for (loop.start (v_4tt, mask); loop.ok(); loop.next (v_4tt, mask))
              mask.value() = Math::pow2<float> (v_4tt.value()); // Processing mask value is the square of the WM fraction

          } else {

            typename Set::voxel_type dwi (in_dwi);
            typedef typename Set::voxel_type::value_type value_type;
            Image::LoopInOrder loop (dwi, "Creating homogeneous processing mask...", 0, 3);
            dwi[3] = 0;
            for (loop.start (dwi, mask); loop.ok(); loop.next (dwi, mask))
              mask.value() = (dwi.value() && std::isfinite (static_cast<value_type> (dwi.value()))) ? 1.0 : 0.0;

          }

        }

      }


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

          // Helper function for doing the regridding
          ACT::Tissues ACT2pve (const Image::Iterator&);

      };



      template <class Set>
      void ResampleFunctor<Set>::operator() (const Image::Iterator& pos)
      {

        Image::voxel_assign (v_dwi, pos);
        Image::voxel_assign (v_out, pos);
        typedef typename Set::voxel_type::value_type value_type;
        if (v_dwi.value() && std::isfinite (static_cast<value_type> (v_dwi.value()))) {

          const ACT::Tissues tissues = ACT2pve (pos);
          v_out[3] = 0; v_out.value() = tissues.get_cgm();
          v_out[3] = 1; v_out.value() = tissues.get_sgm();
          v_out[3] = 2; v_out.value() = tissues.get_wm();
          v_out[3] = 3; v_out.value() = tissues.get_csf();

        } else {
          for (v_out[3] = 0; v_out[3] != 4; ++v_out[3])
            v_out.value() = 0.0;
        }

      }



      template <class Set>
      ACT::Tissues ResampleFunctor<Set>::ACT2pve (const Image::Iterator& pos)
      {

        static const int os_ratio = 10;
        static const float os_step = 1.0 / float(os_ratio);
        static const float os_offset = 0.5 * os_step;

        size_t cgm_count = 0, sgm_count = 0, wm_count = 0, csf_count = 0, total_count = 0;

        Point<int> i;
        Point<float> subvoxel_pos_dwi;
        for (i[2] = 0; i[2] != os_ratio; ++i[2]) {
          subvoxel_pos_dwi[2] = pos[2] - 0.5 + os_offset + (i[2] * os_step);
          for (i[1] = 0; i[1] != os_ratio; ++i[1]) {
            subvoxel_pos_dwi[1] = pos[1] - 0.5 + os_offset + (i[1] * os_step);
            for (i[0] = 0; i[0] != os_ratio; ++i[0]) {
              subvoxel_pos_dwi[0] = pos[0] - 0.5 + os_offset + (i[0] * os_step);

              const Point<float> p_scanner (transform_dwi->voxel2scanner (subvoxel_pos_dwi));
              if (!interp_anat.scanner (p_scanner)) {
                const Tractography::ACT::Tissues tissues (interp_anat);
                ++total_count;
                if (tissues.valid()) {
                  if (tissues.is_cgm())
                    ++cgm_count;
                  else if (tissues.is_sgm())
                    ++sgm_count;
                  else if (tissues.is_wm())
                    ++wm_count;
                  else
                    ++csf_count;
                }
              }

            } } }

        if (total_count)
          return ACT::Tissues (cgm_count / float(total_count), sgm_count / float(total_count), wm_count / float(total_count), csf_count / float(total_count));
        else
          return ACT::Tissues (0.0, 0.0, 0.0, 0.0);

      }



      }
    }
  }
}


#endif
