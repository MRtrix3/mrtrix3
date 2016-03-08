/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

#ifndef __registration_metric_cross_correlation_h__
#define __registration_metric_cross_correlation_h__

#include "transform.h"
#include "interp/linear.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      class CrossCorrelationNoGradient {

        private:
          default_type mean1, mean2;
          default_type denom;

          template <
            typename ImageType1,
            typename ImageType2,
            typename MaskType1,
            typename MaskType2,
            typename Im1ImageInterpolatorType,
            typename Im2ImageInterpolatorType,
            typename Im1MaskInterpolatorType,
            typename Im2MaskInterpolatorType
            >
            struct CCNoGradientPrecomputeFunctor {
              CCNoGradientPrecomputeFunctor (
                ImageType1& im1,
                ImageType2& im2,
                MaskType1& mask1,
                MaskType2& mask2,
                MR::Transform& transform,
                default_type& sum_im1,
                default_type& sum_im2,
                size_t& overlap):
                  in1 (im1),
                  in2 (im2),
                  msk1 (mask1),
                  msk2 (mask2),
                  v2s (transform.voxel2scanner),
                  global_s1 (sum_im1),
                  global_s2 (sum_im2),
                  global_cnt (overlap),
                  s1 (0.0),
                  s2 (0.0),
                  cnt (0) {
                    im1_image_interp.reset (new Im1ImageInterpolatorType (in1));
                    im1_image_interp.reset (new Im1ImageInterpolatorType (in2));
                    im1_mask_interp.reset (new Im1MaskInterpolatorType (msk1));
                    im2_mask_interp.reset (new Im2MaskInterpolatorType (msk2));
                  }

              ~CCNoGradientPrecomputeFunctor () {
                global_s1 += s1;
                global_s2 += s2;
                global_cnt += cnt;
              }

              template <typename ProcessedImageType, typename MaskImageType>
                void operator() (ProcessedImageType& pimage, MaskImageType& mask) {
                  vox =  Eigen::Vector3d (default_type(pimage.index(0)), default_type(pimage.index(1)), default_type(pimage.index(2)));
                  pos = v2s * pos;
                  im1_mask_interp->scanner(pos);
                  if (im1_mask_interp.value() < 0.5)
                    return;

                  im2_mask_interp->scanner(pos);
                  if (im2_mask_interp.value() < 0.5)
                    return;

                  assert(mask.index(0) == pimage.index(0));
                  assert(mask.index(1) == pimage.index(1));
                  assert(mask.index(2) == pimage.index(2));
                  mask.value() = 1;

                  im1_image_interp->scanner(pos);
                  im2_image_interp->scanner(pos);

                  v1 = im1_image_interp->value();
                  v2 = im2_image_interp->value();
                  s1 += v1;
                  s2 += v2;

                  pimage.value() = v1;
                  ++pimage.index(3);
                  pimage.value() = v2;
                  --pimage.index(3);
                  ++cnt;
                }

              private:
                ImageType1 in1;
                ImageType2 in2;
                MaskType1 msk1;
                MaskType2 msk2;
                transform_type v2s;
                default_type& global_s1, global_s2;
                size_t& global_cnt;
                default_type s1, s2;
                size_t cnt;
                Eigen::Vector3d vox, pos;
                default_type v1, v2;
                MR::copy_ptr<Im1ImageInterpolatorType> im1_image_interp;
                MR::copy_ptr<Im2ImageInterpolatorType> im2_image_interp;
                MR::copy_ptr<Im1MaskInterpolatorType> im1_mask_interp;
                MR::copy_ptr<Im2MaskInterpolatorType> im2_mask_interp;
            };

        public:
          /** requires_precompute:
          type_trait to distinguish metric types that require a call to precompute before the operator() is called */
          typedef int requires_precompute;

          template <class Params>
            default_type operator() (Params& params,
                                     const Iterator& iter,
                                     Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

              const Eigen::Vector3 pos = Eigen::Vector3 (iter.index(0), iter.index(1), iter.index(2));

              assert (params.processed_mask.valid());
              assign_pos_of (iter, 0, 3). to (params.processed_mask);
              if (!params.processed_mask.value())
                return 0.0;

              default_type val1 = params.processed_image.value();
              ++params.processed_image.index(3);
              default_type val2 = params.processed_image.value();
              --params.processed_image.index(3);

              return (mean1 - val1) * (val2 - mean2); // negative cross correlation
          }

          template <class ParamType>
            default_type precompute (ParamType& parameters) {
              DEBUG ("precomputing cross correlation data...");

              typedef decltype(parameters.im1_image) Im1Type;
              typedef decltype(parameters.im2_image) Im2Type;
              typedef decltype(parameters.im1_mask) Im1MaskType;
              typedef decltype(parameters.im2_mask) Im2MaskType;
              typedef typename ParamType::Im1InterpType Im1ImageInterpolatorType;
              typedef typename ParamType::Im2InterpType Im2ImageInterpolatorType;
              // typedef typename ParamType::ImProcessedMaskInterpolatorType ProcessedMaskInterpolatorType;
              typedef typename ParamType::ProcessedImageType PImageType;
              // typedef typename ParamType::ImProcessedImageInterpolatorType PImageInterpType;
              typedef Interp::LinearInterp<Im1Type, Interp::LinearInterpProcessingType::Value> Im1MaskInterpolatorType;
              typedef Interp::LinearInterp<Im2Type, Interp::LinearInterpProcessingType::Value> Im2MaskInterpolatorType;

              // const size_t volumes = (parameters.midway_image.ndim() > 3) ? parameters.midway_image.size(3) : 1;
              assert (parameters.midway_image.ndim() == 3);
              // const size_t volumes = 1;
              mean1 = 0.0;
              mean2 = 0.0;
              size_t overlap (0);

              Header midway_header (parameters.midway_image_header);
              MR::Transform transform (midway_header);

              auto cc_mask = Header::scratch (midway_header).template get_image<bool>();
              parameters.processed_mask = cc_mask;
              // cc_image: 2 volumes: interpolated image1 value, interpolated image2 value if both mask's values >= 0.5
              auto cc_header = Header::scratch (parameters.midway_image);
              cc_header.set_ndim(4);
              cc_header.size(3) = 2;
              PImageType cc_image (cc_header);

              // {
              //   LogLevelLatch log_level (0);
              //   if (parameters.im1_mask.valid() or parameters.im2_mask.valid())
              //     cc_mask = cc_mask_header.template get_image<bool>();
              //   else if (parameters.im1_mask.valid() and !parameters.im2_mask.valid())
              //     Filter::reslice <Interp::Nearest> (parameters.im1_mask, cc_mask, parameters.transformation.get_transform_half(), Adapter::AutoOverSample);
              //   else if (!parameters.im1_mask.valid() and parameters.im2_mask.valid())
              //     Filter::reslice <Interp::Nearest> (parameters.im2_mask, cc_mask, parameters.transformation.get_transform_half_inverse(), Adapter::AutoOverSample);
              //   else if (parameters.im1_mask.valid() and parameters.im2_mask.valid()) {
              //     Adapter::Reslice<Interp::Linear, Im1MaskType> mask_reslicer1 (
              //       parameters.im1_mask, cc_mask_header, parameters.transformation.get_transform_half(), Adapter::AutoOverSample);
              //     Adapter::Reslice<Interp::Linear, Im2MaskType> mask_reslicer2 (
              //       parameters.im2_mask, cc_mask_header, parameters.transformation.get_transform_half_inverse(), Adapter::AutoOverSample);
              //     // TODO should be faster to just loop over m1:
              //     //    if (m1.value())
              //     //     assign_pos_of(m1).to(m2); cc_mask.value() = m2.value()
              //     auto both = [](decltype(cc_mask)& cc_mask, decltype(mask_reslicer1)& m1, decltype(mask_reslicer2)& m2) {
              //       cc_mask.value() = ( m1.value() >= 0.5 and m2.value() >= 0.5);
              //     };
              //     ThreadedLoop (cc_mask).run (both, cc_mask, mask_reslicer1, mask_reslicer2);
              //   } else {
              //     cc_mask = cc_mask_header.template get_image<bool>(); // todo: change precompute functor loop over midspace image
              //     ThreadedLoop (cc_mask).run([](decltype(cc_mask)& m) {m.value()=true;}, cc_mask);
              //   }
              // }

              auto loop = ThreadedLoop ("precomputing cross correlation data...", cc_image, 0, 3);
              loop.run (CCNoGradientPrecomputeFunctor<Im1Type,
                                                        Im2Type,
                                                        Im1MaskType,
                                                        Im2MaskType,
                                                        Im1ImageInterpolatorType,
                                                        Im2ImageInterpolatorType,
                                                        Im1MaskInterpolatorType,
                                                        Im2MaskInterpolatorType> (
                                                          parameters.im1_image,
                                                          parameters.im2_image,
                                                          parameters.im1_mask,
                                                          parameters.im2_mask,
                                                          transform,
                                                          mean1,
                                                          mean2,
                                                          overlap), cc_image, cc_mask);
              mean1 /= static_cast<default_type>(overlap);
              mean2 /= static_cast<default_type>(overlap);

              // ThreadedLoop (cc_image, 0, 3).run (CCPrecomputeFunctor_Bogus(), interp1, interp2, cc_image);
              // parameters.processed_mask = cc_mask;
              // parameters.processed_mask_interp.reset (new ProcessedMaskInterpolatorType (parameters.processed_mask));

              return 0;
            }
      };
    }
  }
}
#endif
