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


#ifndef __image_registration_metric_local_cross_correlation_h__
#define __image_registration_metric_local_cross_correlation_h__

#include "transform.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "adapter/reslice.h"
#include "filter/reslice.h"
#include "algo/neighbourhooditerator.h"

namespace MR
{
  namespace Registration
  {
    namespace Metric
    {
      template <typename ImageType1, typename ImageType2>
      struct LCCPrecomputeFunctorMasked_Naive { MEMALIGN(LCCPrecomputeFunctorMasked_Naive<ImageType1,ImageType2>)
        template <typename MaskType, typename ImageType3>
        void operator() (MaskType& mask, ImageType3& out) {
          if (!mask.value())
            return;
          Eigen::Vector3 pos (mask.index(0), mask.index(1), mask.index(2));
          out.index(0) = pos[0];
          out.index(1) = pos[1];
          out.index(2) = pos[2];
          out.index(3) = 0;

          int nmax = extent[0] * extent[1] * extent[2];
          Eigen::VectorXd n1 = Eigen::VectorXd(nmax);
          Eigen::VectorXd n2 = Eigen::VectorXd(nmax);

          using value_type = typename ImageType3::value_type;
          in1.index(0) = pos[0];
          in1.index(1) = pos[1];
          in1.index(2) = pos[2];
          value_type value_in1 =  in1.value();
          if (value_in1 != value_in1){ // nan in image 1, update mask
            mask.value() = false;
            out.row(3) = 0.0;
            return;
          }
          in2.index(0) = pos[0];
          in2.index(1) = pos[1];
          in2.index(2) = pos[2];
          value_type value_in2 = in2.value();
          if (value_in2 != value_in2){ // nan in image 2, update mask
            mask.value() = false;
            out.row(3) = 0.0;
            return;
          }

          n1.setZero();
          n2.setZero();
          auto niter = NeighbourhoodIterator(mask, extent);
          size_t cnt = 0;
          while(niter.loop()) {
            mask.index(0) = niter.index(0);
            mask.index(1) = niter.index(1);
            mask.index(2) = niter.index(2);
            if (!mask.value())
              continue;
            in1.index(0) = niter.index(0);
            in1.index(1) = niter.index(1);
            in1.index(2) = niter.index(2);
            value_type val1 = in1.value();
            if (val1 != val1){
              continue;
            }
            in2.index(0) = niter.index(0);
            in2.index(1) = niter.index(1);
            in2.index(2) = niter.index(2);
            value_type val2 = in2.value();
            if (val2 != val2){
              continue;
            }

            n1[cnt] = in1.value();
            n2[cnt] = in2.value();

            cnt++;
          }
          // reset the mask index
          mask.index(0) = out.index(0);
          mask.index(1) = out.index(1);
          mask.index(2) = out.index(2);

          if (cnt <= 0)
            throw Exception ("FIXME: neighbourhood does not contain centre");

          // local mean subtracted
          default_type m1 = n1.sum() / cnt;
          default_type m2 = n2.sum() / cnt;
          n1.array() -= m1;
          n2.array() -= m2;
          out.row(3) = ( Eigen::Matrix<default_type,5,1>() << value_in1 - m1, value_in2 - m2, n1.adjoint() * n2, n1.adjoint() * n1, n2.adjoint() * n2 ).finished();
        }

        LCCPrecomputeFunctorMasked_Naive (const vector<size_t>& ext, ImageType1& adapter1, ImageType2& adapter2) :
          extent(ext),
          in1(adapter1),
          in2(adapter2) { /* TODO check dimensions and extent */ }

        protected:
          vector<size_t> extent;
          ImageType1 in1; // store reslice adapter in functor to avoid iterating over it when mask is false
          ImageType2 in2; // TODO: cache interpolated values for neighbourhood iteration
      };

      class LocalCrossCorrelation { MEMALIGN(LocalCrossCorrelation)
          private:
            transform_type midway_v2s;

          public:
            /** typedef int is_neighbourhood: type_trait to distinguish voxel-wise and neighbourhood based metric types */
            using is_neighbourhood = int;
            /** requires_precompute int is_neighbourhood: type_trait to distinguish metric types that require a call to precompute before the operator() is called */
            using requires_precompute = int;

            void set_weights (Eigen::Matrix<default_type, Eigen::Dynamic, 1> weights) {
              assert ("FIXME: set_weights not implemented");
            }

            template <class ParamType>
              default_type precompute(ParamType& parameters) {
                INFO ("precomputing cross correlation data...");

                using Im1Type = decltype(parameters.im1_image);
                using Im2Type = decltype(parameters.im2_image);
                using Im1MaskType = decltype(parameters.im1_mask);
                using Im2MaskType = decltype(parameters.im2_mask);
                using ProcessedImageValueType = typename ParamType::ProcessedValueType;
                using ProcessedMaskType = typename ParamType::ProcessedMaskType;
                using ProcessedMaskInterpolatorType = typename ParamType::ProcessedMaskInterpType;
                using CCInterpType = typename ParamType::ProcessedImageInterpType;

                Header midway_header (parameters.midway_image);
                midway_v2s = MR::Transform (midway_header).voxel2scanner;

                // store precomputed values in cc_image:
                // volumes 0 and 1: normalised intensities of both images (Im1 and Im2)
                // volumes 2 to 4: neighbourhood dot products Im1.dot(Im2), Im1.dot(Im1), Im2.dot(Im2)
                auto cc_image_header = Header::scratch (midway_header);
                cc_image_header.ndim() = 4;
                cc_image_header.size(3) = 5;
                ProcessedMaskType cc_mask;
                auto cc_mask_header = Header::scratch (parameters.midway_image);

                auto cc_image = cc_image_header.template get_image <ProcessedImageValueType>().with_direct_io(Stride::contiguous_along_axis(3));
                vector<int> NoOversample;
                {
                  LogLevelLatch log_level (0);
                  if (parameters.im1_mask.valid() or parameters.im2_mask.valid())
                    cc_mask = cc_mask_header.template get_image<bool>();
                  if (parameters.im1_mask.valid() and !parameters.im2_mask.valid())
                    Filter::reslice <Interp::Nearest> (parameters.im1_mask, cc_mask, parameters.transformation.get_transform_half());
                  else if (!parameters.im1_mask.valid() and parameters.im2_mask.valid())
                    Filter::reslice <Interp::Nearest> (parameters.im2_mask, cc_mask, parameters.transformation.get_transform_half_inverse(), Adapter::AutoOverSample);
                  else if (parameters.im1_mask.valid() and parameters.im2_mask.valid()){
                    Adapter::Reslice<Interp::Nearest, Im1MaskType> mask_reslicer1 (parameters.im1_mask, cc_mask_header, parameters.transformation.get_transform_half(), NoOversample);
                    Adapter::Reslice<Interp::Nearest, Im2MaskType> mask_reslicer2 (parameters.im2_mask, cc_mask_header, parameters.transformation.get_transform_half_inverse(), NoOversample);
                    // TODO should be faster to just loop over m1:
                    //    if (m1.value())
                    //     assign_pos_of(m1).to(m2); cc_mask.value() = m2.value()
                    auto both = [](decltype(cc_mask)& cc_mask, decltype(mask_reslicer1)& m1, decltype(mask_reslicer2)& m2) {
                      cc_mask.value() = ((m1.value() + m2.value()) / 2.0) > 0.5 ? true : false;
                    };
                    ThreadedLoop (cc_mask).run (both, cc_mask, mask_reslicer1, mask_reslicer2);
                  }
                }
                Adapter::Reslice<Interp::Linear, Im1Type> interp1 (parameters.im1_image, midway_header, parameters.transformation.get_transform_half(), NoOversample, std::numeric_limits<typename Im1Type::value_type>::quiet_NaN());

                Adapter::Reslice<Interp::Linear, Im2Type> interp2 (parameters.im2_image, midway_header, parameters.transformation.get_transform_half_inverse(), NoOversample, std::numeric_limits<typename Im2Type::value_type>::quiet_NaN());

                const auto extent = parameters.get_extent();

                // TODO unmasked CCPrecomputeFunctor.
                // ThreadedLoop (cc_image, 0, 3).run (CCPrecomputeFunctor_Bogus(), interp1, interp2, cc_image);
                // create a mask (all voxels true) if none given.
                if (!cc_mask.valid()){
                  cc_mask = cc_mask_header.template get_image<bool>();
                  ThreadedLoop (cc_mask).run([](decltype(cc_mask)& m) {m.value()=true;}, cc_mask);
                }
                parameters.processed_mask = cc_mask;
                parameters.processed_mask_interp.reset (new ProcessedMaskInterpolatorType (parameters.processed_mask));
                auto loop = ThreadedLoop ("precomputing cross correlation data...", parameters.processed_mask);
                loop.run (LCCPrecomputeFunctorMasked_Naive<decltype(interp1), decltype(interp2)>(extent, interp1, interp2), parameters.processed_mask, cc_image);
                parameters.processed_image = cc_image;
                parameters.processed_image_interp.reset (new CCInterpType (parameters.processed_image));
                // display<Image<default_type>>(parameters.processed_image);
                return 0.0;
              }

            template <class Params>
              default_type operator() (Params& params,
                                       const Iterator& iter,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                 // iterates over processed image rather than midway image

                if (params.processed_mask.valid()) {
                  assign_pos_of(iter, 0, 3).to(params.processed_mask);
                  if (!params.processed_mask.value())
                    return 0.0;
                }

                assign_pos_of(iter).to(params.processed_image); // TODO why do we need this?
                assert (params.processed_image.index(0) == iter.index(0));
                assert (params.processed_image.index(1) == iter.index(1));
                assert (params.processed_image.index(2) == iter.index(2));

                params.processed_image.index(3) = 2;
                default_type A = params.processed_image.value();
                params.processed_image.index(3) = 3;
                default_type B = params.processed_image.value();
                params.processed_image.index(3) = 4;
                default_type C = params.processed_image.value();
                default_type A_BC = A / (B * C);
                params.processed_image.index(3) = 0;

                if (A_BC != A_BC || A_BC == 0.0) {
                  return 0.0;
                }

                const Eigen::Vector3 pos = Eigen::Vector3(default_type(iter.index(0)), default_type(iter.index(0)), default_type(iter.index(0)));
                params.processed_image_interp->voxel(pos);
                typename Params::Im1ValueType val1;
                typename Params::Im2ValueType val2;
                Eigen::Matrix<typename Params::Im1ValueType, 1, 3> grad1;
                Eigen::Matrix<typename Params::Im2ValueType, 1, 3> grad2;

                // gradient calculation
                params.processed_image_interp->index(3) = 0;
                params.processed_image_interp->value_and_gradient_wrt_scanner(val1, grad1);

                if (val1 != val1){
                  // this should not happen as the precompute should have changed the mask
                  WARN ("FIXME: val1 is nan");
                  return 0.0;
                }

                params.processed_image_interp->index(3) = 1;
                params.processed_image_interp->value_and_gradient_wrt_scanner(val2, grad2);
                if (val2 != val2){
                  // this should not happen as the precompute should have changed the mask
                  WARN ("FIXME: val2 is nan");
                  return 0.0;
                }

                // 2.0 * sfm / (sff * smm) * ((i2 - sfm / smm * i1 ) * im1_gradient.value() - (i1 - sfm / sff * i2 ) * im2_gradient.value());

                // ITK:
                // derivWRTImage[dim] = 2.0 * sFixedMoving / (sFixedFixed_sMovingMoving) * (fixedI - sFixedMoving / sMovingMoving * movingI) * movingImageGradient[dim];
                Eigen::Vector3 derivWRTImage = - A_BC * ((val2 - A/B * val1) * grad1 - 0.0 * (val1 - A/C * val2) * grad2);

                const Eigen::Vector3 midway_point = midway_v2s * pos;
                const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);
                gradient.segment<4>(0) += derivWRTImage(0) * jacobian_vec;
                gradient.segment<4>(4) += derivWRTImage(1) * jacobian_vec;
                gradient.segment<4>(8) += derivWRTImage(2) * jacobian_vec;

                return A * A_BC;
            }
      };
    }
  }
}
#endif
