/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "dwi/tractography/SIFT/proc_mask.h"

#include "transform.h"
#include "algo/copy.h"
#include "algo/threaded_loop.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {

        

        const App::OptionGroup SIFTModelProcMaskOption = App::OptionGroup ("Options for setting the processing mask for the SIFT fixel-streamlines comparison model")

          + App::Option ("proc_mask", "provide an image containing the processing mask weights for the model; image spatial dimensions must match the fixel image")
            + App::Argument ("image").type_image_in()

          + App::Option ("act", "use an ACT five-tissue-type segmented anatomical image to derive the processing mask")
            + App::Argument ("image").type_image_in();




        void initialise_processing_mask (Image<float>& in_dwi, Image<float>& out_mask, Image<float>& out_5tt)
        {
          // User-specified processing mask
          auto opt = App::get_options ("proc_mask");
          if (opt.size()) {
            auto image = Image<float>::open (opt[0][0]);
            if (!dimensions_match (out_mask, image, 0, 3))
              throw Exception ("Dimensions of processing mask image provided using -proc_mask option must match relevant fixel image");
            copy_with_progress_message ("Copying processing mask to memory", image, out_mask, 0, 3);

          } else {
            auto opt = App::get_options ("act");
            if (opt.size()) {

              auto in_5tt = Image<float>::open (opt[0][0]);
              ACT::verify_5TT_image (in_5tt);

              Header H_5tt (in_dwi);
              H_5tt.ndim() = 4;
              H_5tt.size(3) = 5;
              assert (!out_5tt.valid());
              out_5tt = Image<float>::scratch (H_5tt, "5TT scratch buffer");

              // Test to see if the image has already been re-gridded to match the fixel image
              // If it has, can do a direct import
              if (dimensions_match (out_5tt, in_5tt, 0, 3)) {
                INFO ("5TT image dimensions match fixel image - importing directly");
                copy (in_5tt, out_5tt);
              } else {
                auto threaded_loop  = ThreadedLoop ("resampling ACT 5TT image to fixel image space", in_dwi, 0, 3);
                ResampleFunctor functor (in_dwi, in_5tt, out_5tt);
                threaded_loop.run (functor);
              }

              // Once all of the 5TT data has been read in, use it to derive the processing mask
              out_5tt.index(3) = 2; // Access the WM fraction
              float integral = 0.0f;
              for (auto l = Loop (out_5tt, 0, 3) (out_5tt, out_mask); l; ++l) {
                const float value = Math::pow2<float> (out_5tt.value()); // Processing mask value is the square of the WM fraction
                if (std::isfinite (value)) {
                  out_mask.value() = value;
                  integral += value;
                } else {
                  out_mask.value() = 0.0f;
                }
              }
              if (!integral)
                throw Exception ("Processing mask is empty; check input images / registration");

            } else {

              auto f = [] (Image<float>& dwi, Image<float>& mask) {
                mask.value() = (dwi.value() && std::isfinite ((float) dwi.value())) ? 1.0 : 0.0;
              };
              ThreadedLoop ("Creating homogeneous processing mask", in_dwi, 0, 3).run (f, in_dwi, out_mask);

            }

          }

        }


        ResampleFunctor::ResampleFunctor (Image<float>& dwi, Image<float>& anat, Image<float>& out) :
            dwi (dwi),
            voxel2scanner (new transform_type (Transform(dwi).voxel2scanner.cast<float>())),
            interp_anat (anat),
            out (out)
        {
          dwi.index(3) = 0;
        }

        ResampleFunctor::ResampleFunctor (const ResampleFunctor& that) :
            dwi (that.dwi),
            voxel2scanner (that.voxel2scanner),
            interp_anat (that.interp_anat),
            out (that.out)
        {
          dwi.index(3) = 0;
        }



        void ResampleFunctor::operator() (const Iterator& pos)
        {
          assign_pos_of (pos).to (dwi, out);
          if (dwi.value() && std::isfinite ((float) dwi.value())) {
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



        ACT::Tissues ResampleFunctor::ACT2pve (const Iterator& pos)
        {
          static const int os_ratio = 10;
          static const float os_step = 1.0 / float(os_ratio);
          static const float os_offset = 0.5 * os_step;

          size_t cgm_count = 0, sgm_count = 0, wm_count = 0, csf_count = 0, path_count = 0, total_count = 0;

          Eigen::Array3i i;
          Eigen::Vector3f subvoxel_pos_dwi;
          for (i[2] = 0; i[2] != os_ratio; ++i[2]) {
            subvoxel_pos_dwi[2] = pos.index(2) - 0.5 + os_offset + (i[2] * os_step);
            for (i[1] = 0; i[1] != os_ratio; ++i[1]) {
              subvoxel_pos_dwi[1] = pos.index(1) - 0.5 + os_offset + (i[1] * os_step);
              for (i[0] = 0; i[0] != os_ratio; ++i[0]) {
                subvoxel_pos_dwi[0] = pos.index(0) - 0.5 + os_offset + (i[0] * os_step);

                const auto p_scanner (*voxel2scanner * subvoxel_pos_dwi);
                if (interp_anat.scanner (p_scanner)) {
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

