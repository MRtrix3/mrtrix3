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

#include "cmdline_option.h"
#include "memory.h"

#include "image.h"
#include "algo/copy.h"
#include "algo/iterator.h"
#include "algo/threaded_loop.h"
#include "transform.h"
#include "interp/linear.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



      extern const App::OptionGroup SIFTModelProcMaskOption;


      template <class InputType, class AnatType, class OutputType>
        class ResampleFunctor;


      template <class DWIType, class MaskType, class ACTType>
        void initialise_processing_mask (DWIType& in_dwi, MaskType& mask, std::unique_ptr<ACTType>& act_5tt)
        {
          // User-specified processing mask
          auto opt = App::get_options ("proc_mask");
          if (opt.size()) {
            auto image = Image<float>::open (opt[0][0]);
            if (!dimensions_match (mask, image, 0, 3))
              throw Exception ("Dimensions of processing mask image provided using -proc_mask option must match relevant fixel image");
            copy_with_progress_message ("Copying processing mask to memory... ", image, mask, 0, 3);

          } 
          else {
            auto opt = App::get_options ("act");
            if (opt.size()) {

              auto in_anat = Header::open (opt[0][0]);
              ACT::verify_5TT_image (in_anat);

              Header info_5tt = in_dwi;
              info_5tt.set_ndim (4);
              info_5tt.size(3) = 5;
              act_5tt.reset (new Image<float> (Image<float>::scratch (info_5tt, "5TT BufferScratch")));

              // Test to see if the image has already been re-gridded to match the fixel image
              // If it has, can do a direct import
              if (dimensions_match (*act_5tt, in_anat, 0, 3)) {
                INFO ("5TT image dimensions match fixel image - importing directly");
                copy (in_anat.get_image<float>(), *act_5tt);
              } else {
                auto threaded_loop  = ThreadedLoop ("resampling ACT 5TT image to fixel image space...", in_dwi, 0, 3);
                auto v_anat = in_anat.get_image<float>();
                ResampleFunctor<DWIType, decltype(v_anat), ACTType> functor (in_dwi, v_anat, *act_5tt);
                threaded_loop.run (functor);
              }

              // Once all of the 5TT data has been read in, use it to derive the processing mask
              act_5tt->index(3) = 2; // Access the WM fraction
              for (auto l = Loop (*act_5tt,0,3) (*act_5tt, mask); l; ++l)
                mask.value() = Math::pow2<float> (act_5tt->value()); // Processing mask value is the square of the WM fraction

            } else {

              auto f = [] (decltype(in_dwi)& dwi, decltype(mask)& mask) {
                typedef typename std::remove_reference<decltype(in_dwi)>::type::value_type value_type;
                mask.value() = (dwi.value() && std::isfinite (static_cast<value_type> (dwi.value()))) ? 1.0 : 0.0;
              };
              ThreadedLoop ("Creating homogeneous processing mask...", in_dwi, 0, 3).run (f, in_dwi, mask);
            }

          }

        }


      // Private functor for performing ACT image regridding
      template <class InputType, class AnatType, class OutputType>
        class ResampleFunctor
        {
          public:

            ResampleFunctor (InputType& dwi, AnatType& anat, OutputType& out) :
              dwi (dwi),
              voxel2scanner (Transform(dwi).voxel2scanner.cast<float>()),
              interp_anat (anat),
              out (out) { dwi.index(3) = 0; }

            ResampleFunctor (const ResampleFunctor& that) :
              dwi (that.dwi),
              voxel2scanner (that.voxel2scanner),
              interp_anat (that.interp_anat),
              out (that.out) { dwi.index(3) = 0; }

            void operator() (const Iterator&);

          private:
            InputType dwi;
            Eigen::Transform<float,3,Eigen::AffineCompact> voxel2scanner;
            Interp::Linear<AnatType> interp_anat;
            OutputType out;

            // Helper function for doing the regridding
            ACT::Tissues ACT2pve (const Iterator&);

        };



      template <class InputType, class AnatType, class OutputType>
        void ResampleFunctor<InputType,AnatType,OutputType>::operator() (const Iterator& pos)
        {

          assign_pos_of (pos).to (dwi, out);
          typedef typename decltype(dwi)::value_type value_type;
          if (dwi.value() && std::isfinite (static_cast<value_type> (dwi.value()))) {

            const ACT::Tissues tissues = ACT2pve (pos);
            out.index (3) = 0; out.value() = tissues.get_cgm();
            out.index (3) = 1; out.value() = tissues.get_sgm();
            out.index (3) = 2; out.value() = tissues.get_wm();
            out.index (3) = 3; out.value() = tissues.get_csf();
            out.index (3) = 4; out.value() = tissues.get_path();

          } else {
            for (out.index(3) = 0; out.index(3) != 5; ++out.index(3))
              out.value() = 0.0;
          }

        }



      template <class InputType, class AnatType, class OutputType>
        ACT::Tissues ResampleFunctor<InputType,AnatType,OutputType>::ACT2pve (const Iterator& pos)
        {

          static const int os_ratio = 10;
          static const float os_step = 1.0 / float(os_ratio);
          static const float os_offset = 0.5 * os_step;

          size_t cgm_count = 0, sgm_count = 0, wm_count = 0, csf_count = 0, path_count = 0, total_count = 0;

          Eigen::Vector3i i;
          Eigen::Vector3f subvoxel_pos_dwi;
          for (i[2] = 0; i[2] != os_ratio; ++i[2]) {
            subvoxel_pos_dwi[2] = pos.index(2) - 0.5 + os_offset + (i[2] * os_step);
            for (i[1] = 0; i[1] != os_ratio; ++i[1]) {
              subvoxel_pos_dwi[1] = pos.index(1) - 0.5 + os_offset + (i[1] * os_step);
              for (i[0] = 0; i[0] != os_ratio; ++i[0]) {
                subvoxel_pos_dwi[0] = pos.index(0) - 0.5 + os_offset + (i[0] * os_step);

                const auto p_scanner (voxel2scanner * subvoxel_pos_dwi);
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
                    else if (tissues.is_csf())
                      ++csf_count;
                    else if (tissues.is_path())
                      ++path_count;
                    else
                      --total_count; // Should ideally never happen...
                  }
                }

              } } }

          if (total_count) {
            return ACT::Tissues (cgm_count  / float(total_count),
                sgm_count  / float(total_count),
                wm_count   / float(total_count),
                csf_count  / float(total_count),
                path_count / float(total_count));
          } else {
            return ACT::Tissues ();
          }

        }



      }
    }
  }
}


#endif
