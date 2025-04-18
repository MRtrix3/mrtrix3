/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#ifndef __filter_dwi_brain_mask_h__
#define __filter_dwi_brain_mask_h__

#include "memory.h"
#include "image.h"
#include "progressbar.h"
#include "filter/base.h"
#include "filter/connected_components.h"
#include "filter/median.h"
#include "filter/optimal_threshold.h"
#include "algo/histogram.h"
#include "algo/copy.h"
#include "algo/loop.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "metadata/phase_encoding.h"


namespace MR
{
  namespace Filter
  {


    /** \addtogroup Filters
      @{ */

    //! a filter to compute a whole brain mask from a DWI image.
    /*! Both diffusion weighted and b=0 volumes are required to
     *  obtain a mask that includes both brain tissue and CSF.
     *
     * Typical usage:
     * \code
     * auto header = Header::open (argument[0]);
     * auto grad = DWI::get_DW_scheme (header);
     * auto input = Image<value_type>::open (argument[0]);
     * Filter::DWIBrainMask dwi_brain_mask_filter (input, grad);
     * auto output = Image<bool>::create (argument[1], dwi_brain_mask_filter);
     * dwi_brain_mask_filter (input, output);
     *
     * \endcode
     */
    class DWIBrainMask : public Base { MEMALIGN(DWIBrainMask)

      public:

        template <class HeaderType>
        DWIBrainMask (const HeaderType& input, const Eigen::MatrixXd& grad) :
            Base (input),
            grad (grad)
        {
          DWI::stash_DW_scheme (*this, grad);
          Metadata::PhaseEncoding::clear_scheme (keyval());
          axes_.resize(3);
          datatype_ = DataType::Bit;
        }


        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output) {
            using value_type = typename InputImageType::value_type;

            Header header (input);
            header.ndim() = 3;

            // Generate a 'master' scratch buffer mask, to which all shells will contribute
            auto mask_image = Image<bool>::scratch (header, "DWI mask");

            std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message) : nullptr);

            // Loop over each shell, including b=0, in turn
            DWI::Shells shells (grad);
            for (size_t s = 0; s != shells.count(); ++s) {
              const DWI::Shell& shell (shells[s]);

              auto shell_image = Image<value_type>::scratch (header, "mean b=" + str(size_t(std::round(shell.get_mean()))) + " image");

              for (auto l = Loop (0, 3) (input, shell_image); l; ++l) {
                default_type sum = 0.0;
                for (vector<size_t>::const_iterator v = shell.get_volumes().begin(); v != shell.get_volumes().end(); ++v) {
                  input.index(3) = *v;
                  const value_type value = input.value();
                  if (value > value_type(0))
                    sum += value;
                }
                shell_image.value() = value_type(sum / default_type(shell.count()));
              }
              if (progress)
                ++(*progress);

              // Threshold the mean intensity image for this shell
              OptimalThreshold threshold_filter (shell_image);

              auto shell_mask_voxel = Image<bool>::scratch (threshold_filter);
              threshold_filter (shell_image, shell_mask_voxel);
              if (progress)
                ++(*progress);

              // Add this mask to the master
              for (auto l = Loop (0, 3)(mask_image, shell_mask_voxel); l; ++l) {
                if (shell_mask_voxel.value())
                  mask_image.value() = true;
              }
              if (progress)
                ++(*progress);

            }

            // The following operations apply to the mask as combined from all shells
            auto temp_image = Image<bool>::scratch (header, "temporary mask");
            Median median_filter (mask_image);
            median_filter (mask_image, temp_image);
            if (progress)
              ++(*progress);

            ConnectedComponents connected_filter (temp_image);
            connected_filter.set_largest_only (true);
            connected_filter (temp_image, temp_image);
            if (progress)
              ++(*progress);

            for (auto l = Loop (0,3)(temp_image); l; ++l)
              temp_image.value() = !temp_image.value();
            if (progress)
              ++(*progress);

            connected_filter (temp_image, temp_image);
            if (progress)
              ++(*progress);

            for (auto l = Loop (0,3) (temp_image, output); l; ++l)
              output.value() = !temp_image.value();
        }

      protected:
        const Eigen::MatrixXd& grad;

    };
    //! @}
  }
}




#endif
