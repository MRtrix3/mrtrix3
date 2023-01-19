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


#ifndef __registration_metric_linear_base_h__
#define __registration_metric_linear_base_h__

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

    class LinearBase { MEMALIGN(LinearBase)

      public:
        LinearBase ( ) : weighted (false) {}

        /** requires_precompute:
          using requires_precompute = int;
          type_trait to distinguish metric types that require a call to precompute before the operator() is called
          evaluate loops over processed_image instead of midway_image
        */
        template <class ParamType>
          default_type precompute (ParamType& parameters) { assert (0 && "FIXME: requires_precompute defined but precompute not implemented for this metric."); return 0.; }

        /** requires_initialisation:
          using requires_initialisation = int;
          type_trait to distinguish metric types that require a call to init (im1, im2) before the operator() is called */
        template <class Im1Type = Image<default_type>, class Im2Type = Image<default_type>>
          void init (const Im1Type& im1, const Im2Type& im2) { assert (0 && "FIXME: requires_initialisation defined but init not implemented for this metric."); }

        /** is_neighbourhood:
          using is_neighbourhood = int;
          type_trait to distinguish voxel-wise and neighbourhood based metric types (affects ThreadKernel) */

        // set contrast weights for 4D metrics
        void set_weights (Eigen::VectorXd weights) {
          mc_weights = weights;
          weighted = mc_weights.rows() > 0;
        }

        template <class Params>
          default_type operator() (Params& params,
                                   const Eigen::Vector3d& im1_point,
                                   const Eigen::Vector3d& im2_point,
                                   const Eigen::Vector3d& midway_point,
                                   Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient);

      protected:
        Eigen::VectorXd mc_weights;
        bool weighted;
      };

    }
  }
}
#endif
