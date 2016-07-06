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
          default_type mean1;
          default_type mean2;
          default_type denom; // TODO: denominator for normalisation

          template <
            typename LinearTrafoType,
            typename ImageType1,
            typename ImageType2,
            typename MidwayImageType,
            typename MaskType1,
            typename MaskType2,
            typename Im1ImageInterpolatorType,
            typename Im2ImageInterpolatorType,
            typename Im1MaskInterpolatorType,
            typename Im2MaskInterpolatorType
            >
            struct CCNoGradientPrecomputeFunctor {
              CCNoGradientPrecomputeFunctor (
                const LinearTrafoType& transformation,
                ImageType1& im1,
                ImageType2& im2,
                const MidwayImageType& midway,
                MaskType1& mask1,
                MaskType2& mask2,
                default_type& sum_im1,
                default_type& sum_im2,
                size_t& overlap):
                  trafo_half (transformation.get_transform_half()),
                  trafo_half_inverse (transformation.get_transform_half_inverse()),
                  v2s (MR::Transform(midway).voxel2scanner),
                  in1 (im1),
                  in2 (im2),
                  msk1 (mask1),
                  msk2 (mask2),
                  global_s1 (sum_im1),
                  global_s2 (sum_im2),
                  global_cnt (overlap),
                  s1 (0.0),
                  s2 (0.0),
                  cnt (0) {
                    assert (in1.valid());
                    assert (in2.valid());
                    im1_image_interp.reset (new Im1ImageInterpolatorType (in1));
                    im2_image_interp.reset (new Im2ImageInterpolatorType (in2));
                    if (msk1.valid())
                      im1_mask_interp.reset (new Im1MaskInterpolatorType (msk1));
                    if (msk2.valid())
                      im2_mask_interp.reset (new Im2MaskInterpolatorType (msk2));
                  }

              ~CCNoGradientPrecomputeFunctor () {
                global_s1 += s1;
                global_s2 += s2;
                global_cnt += cnt;
              }

              template <typename ProcessedImageType, typename MaskImageType>
                void operator() (ProcessedImageType& pimage, MaskImageType& mask) {
                  assert(mask.index(0) == pimage.index(0));
                  assert(mask.index(1) == pimage.index(1));
                  assert(mask.index(2) == pimage.index(2));
                  assert(pimage.index(3) == 0);
                  vox =  Eigen::Vector3d (default_type(pimage.index(0)), default_type(pimage.index(1)), default_type(pimage.index(2)));
                  pos = v2s * vox;

                  pos1 = trafo_half * pos;
                  if (msk1.valid()) {
                    im1_mask_interp->scanner(pos1);
                    if (!(*im1_mask_interp))
                      return;
                    if (im1_mask_interp->value() < 0.5)
                      return;
                  }

                  pos2 = trafo_half_inverse * pos;
                  if (msk2.valid()) {
                    im2_mask_interp->scanner(pos2);
                    if (!(*im2_mask_interp))
                      return;
                    if (im2_mask_interp->value() < 0.5)
                      return;
                  }

                  im1_image_interp->scanner(pos1);
                  if (!(*im1_image_interp))
                    return;
                  v1 = im1_image_interp->value();
                  if (v1 != v1)
                    return;

                  im2_image_interp->scanner(pos2);
                  if (!(*im2_image_interp))
                    return;
                  v2 = im2_image_interp->value();
                  if (v2 != v2)
                    return;

                  mask.value() = 1;
                  s1 += v1;
                  s2 += v2;

                  pimage.value() = v1;
                  ++pimage.index(3);
                  pimage.value() = v2;
                  --pimage.index(3);
                  ++cnt;
                }

              private:
                const Eigen::Transform<default_type, 3, Eigen::AffineCompact> trafo_half, trafo_half_inverse;
                const transform_type v2s;
                ImageType1 in1;
                ImageType2 in2;
                MaskType1 msk1;
                MaskType2 msk2;
                default_type &global_s1, &global_s2;
                size_t& global_cnt;
                default_type s1, s2;
                size_t cnt;
                Eigen::Vector3d vox, pos, pos1, pos2;
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

              // const Eigen::Vector3 pos = Eigen::Vector3 (iter.index(0), iter.index(1), iter.index(2));

              assert (params.processed_mask.valid());
              assert (params.processed_image.valid());
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
              typedef decltype(parameters.midway_image) MidwayImageType;
              typedef decltype(parameters.im1_mask) Im1MaskType;
              typedef decltype(parameters.im2_mask) Im2MaskType;
              typedef typename ParamType::Im1InterpType Im1ImageInterpolatorType;
              typedef typename ParamType::Im2InterpType Im2ImageInterpolatorType;
              typedef typename ParamType::ProcessedImageType PImageType;
              // typedef Interp::LinearInterp<Im1MaskType, Interp::LinearInterpProcessingType::Value> Im1MaskInterpolatorType;
              // typedef Interp::LinearInterp<Im2MaskType, Interp::LinearInterpProcessingType::Value> Im2MaskInterpolatorType;
              typedef typename ParamType::Mask1InterpolatorType Im1MaskInterpolatorType;
              typedef typename ParamType::Mask2InterpolatorType Im2MaskInterpolatorType;

              assert (parameters.midway_image.ndim() == 3);
              mean1 = 0.0;
              mean2 = 0.0;
              size_t overlap (0);

              Header midway_header (parameters.midway_image);
              MR::Transform transform (midway_header);

              parameters.processed_mask = Header::scratch (midway_header).template get_image<bool>();
              // processed_image: 2 volumes: interpolated image1 value, interpolated image2 value if both masks' values are >= 0.5
              auto cc_header = Header::scratch (parameters.midway_image);
              cc_header.ndim() = 4;
              cc_header.size(3) = 2;
              // PImageType cc_image = PImageType::scratch (cc_header);
              parameters.processed_image = PImageType::scratch (cc_header);

              auto loop = ThreadedLoop ("precomputing cross correlation data...", parameters.processed_image, 0, 3);
              loop.run (CCNoGradientPrecomputeFunctor<decltype(parameters.transformation),
                                                      Im1Type,
                                                      Im2Type,
                                                      MidwayImageType,
                                                      Im1MaskType,
                                                      Im2MaskType,
                                                      Im1ImageInterpolatorType,
                                                      Im2ImageInterpolatorType,
                                                      Im1MaskInterpolatorType,
                                                      Im2MaskInterpolatorType> (
                                                        parameters.transformation,
                                                        parameters.im1_image,
                                                        parameters.im2_image,
                                                        parameters.midway_image,
                                                        parameters.im1_mask,
                                                        parameters.im2_mask,
                                                        mean1,
                                                        mean2,
                                                        overlap), parameters.processed_image, parameters.processed_mask);
              // display<Im1Type>(parameters.im1_image);
              // display<Im2Type>(parameters.im2_image);
              // display<Image<bool>>(parameters.processed_mask);
              // VAR(overlap);
              // VAR(mean1);
              // VAR(mean2);
              if (overlap > 0 ) {
                mean1 /= static_cast<default_type>(overlap);
                mean2 /= static_cast<default_type>(overlap);
              } else {
                DEBUG ("Cross Correlation metric: zero overlap");
              }

              return 0;
            }
      };
    }
  }
}
#endif
