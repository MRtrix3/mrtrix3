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

#include "dwi/tractography/ACT/resample.h"

#include "transform.h"
#include "algo/copy.h"
#include "algo/threaded_loop.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {



        ResampleFunctor::ResampleFunctor (Image<float>& anat, Image<float>& out) :
            voxel2scanner (new transform_type (Transform(out).voxel2scanner.cast<float>())),
            interp_anat (anat),
            out (out) { }

        ResampleFunctor::ResampleFunctor (const ResampleFunctor& that) :
            voxel2scanner (that.voxel2scanner),
            interp_anat (that.interp_anat),
            out (that.out) { }



        void ResampleFunctor::operator() (const Iterator& pos)
        {
          const Tissues tissues = ACT2pve (pos);
          out.index (3) = 0; out.value() = tissues.get_cgm();
          out.index (3) = 1; out.value() = tissues.get_sgm();
          out.index (3) = 2; out.value() = tissues.get_wm();
          out.index (3) = 3; out.value() = tissues.get_csf();
          out.index (3) = 4; out.value() = tissues.get_path();
        }



        Tissues ResampleFunctor::ACT2pve (const Iterator& pos)
        {
          // TODO Move
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
                  if (tissues.valid()) {
                    ++total_count;
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
                      --total_count;
                  }
                }

          } } }

          if (total_count > Math::pow3<size_t> (os_ratio) / 2) {
            return Tissues (cgm_count  / float(total_count),
                            sgm_count  / float(total_count),
                            wm_count   / float(total_count),
                            csf_count  / float(total_count),
                            path_count / float(total_count));
          } else {
            return Tissues ();
          }
        }



      }
    }
  }
}

