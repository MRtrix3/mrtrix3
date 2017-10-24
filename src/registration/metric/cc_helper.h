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


#ifndef __registration_metric_cc_helper_h__
#define __registration_metric_cc_helper_h__

#include "debug.h"
#include "math/math.h"
#include "image_helpers.h"
#include "algo/neighbourhooditerator.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {

      template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType, class DerivedImageType>
        void cc_precompute (Im1ImageType& im1_image,
                            Im2ImageType& im2_image,
                            Im1MaskType& im1_mask,
                            Im2MaskType& im2_mask,
                            DerivedImageType& A,
                            DerivedImageType& B,
                            DerivedImageType& C,
                            DerivedImageType& im1_meansubtr,
                            DerivedImageType& im2_meansubtr,
                            const vector<size_t>& extent) {
          // TODO check extent
          int nmax = extent[0] * extent[1] * extent[2];
          Eigen::VectorXd n1 = Eigen::VectorXd(nmax);
          Eigen::VectorXd n2 = Eigen::VectorXd(nmax);
          Eigen::Vector3 pos;
          for (auto l = Loop("precomputing cross correlation values") (im1_image); l; ++l) {
            pos[0] = im1_image.index(0);
            pos[1] = im1_image.index(1);
            pos[2] = im1_image.index(2);
            assign_pos_of(im1_image, 0, 3).to(A, B, C, im1_meansubtr, im2_meansubtr);
            default_type m1(0.0);
            default_type m2(0.0);

            n1.setZero();
            n2.setZero();

            int nvox(0);
            auto niter = NeighbourhoodIterator(im1_image, extent);
            while (niter.loop()) {
              if (im1_mask.valid()) {
                assign_pos_of(niter, 0, 3).to(im1_mask);
                if (!im1_mask.value())
                  continue;
              }
              if (im2_mask.valid()) {
                assign_pos_of(niter, 0, 3).to(im2_mask);
                if (!im2_mask.value())
                  continue;
              }
              assign_pos_of(niter, 0, 3).to(im1_image);
              assign_pos_of(niter, 0, 3).to(im2_image);

              n1[nvox] = im1_image.value();
              n2[nvox] = im2_image.value();

              m1 += n1[nvox];
              m2 += n2[nvox];
              nvox++;
            }

            if (nvox == 0) {
              A.value() = NaN;
              C.value() = NaN;
              B.value() = NaN;
              im1_meansubtr.value() = NaN;
              im2_meansubtr.value() = NaN;
              continue;
            }

            // local mean subtracted
            n1.array() -= n1.sum() / nvox;
            n2.array() -= n2.sum() / nvox;
            A.value() = n1.adjoint() * n2;
            B.value() = n1.adjoint() * n1;
            C.value() = n2.adjoint() * n2;

            // reset image positions
            im1_image.index(0) = pos[0];
            im1_image.index(1) = pos[1];
            im1_image.index(2) = pos[2];

            im2_image.index(0) = pos[0];
            im2_image.index(1) = pos[1];
            im2_image.index(2) = pos[2];

            im1_meansubtr.value() = im1_image.value() - m1 / nvox;
            im2_meansubtr.value() = im2_image.value() - m2 / nvox;
          }

        }

    }
  }
}
#endif
