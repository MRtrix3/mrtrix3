/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __image_registration_metric_norm_cross_correlation_h__
#define __image_registration_metric_norm_cross_correlation_h__

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
      struct NCCPrecomputeFunctorMasked_DEBUG { MEMALIGN(NCCPrecomputeFunctorMasked_DEBUG<ImageType1,ImageType2>)
        template <typename MaskType, typename ImageType3>
        void operator() (MaskType& mask, ImageType3& out) {
          out.index(0) = mask.index(0);
          out.index(1) = mask.index(1);
          out.index(2) = mask.index(2);
          out.index(3) = 0;
          out.value() = 100.0 * mask.value();
          // DEBUG(str(mask));

          size_t cnt = 1;
          auto niter = NeighbourhoodIterator(mask, extent);
          // DEBUG(str(niter));
          while(niter.loop()){
            mask.index(0) = niter.index(0);
            mask.index(1) = niter.index(1);
            mask.index(2) = niter.index(2);
            // DEBUG("      "+str(mask));
            if (!mask.value())
              continue;
            cnt += 1;
          }
          in1.index(0) = out.index(0);
          in1.index(1) = out.index(1);
          in1.index(2) = out.index(2);

          in2.index(0) = out.index(0);
          in2.index(1) = out.index(1);
          in2.index(2) = out.index(2);

          out.index(3) = 1;
          out.value() = in1.value();
          out.index(3) = 2;
          out.value() = 100.0 * mask.value();
          out.index(3) = 3;
          out.value() = in2.value();
          out.index(3) = 4;
          out.value() = cnt;

          // reset mask indices
          mask.index(0) = out.index(0);
          mask.index(1) = out.index(1);
          mask.index(2) = out.index(2);
        }

        NCCPrecomputeFunctorMasked_DEBUG(const vector<size_t>& ext, ImageType1& adapter1, ImageType2& adapter2) :
          extent(ext),
          in1(adapter1),
          in2(adapter2) { /* TODO check dimensions and extent */ }

        protected:
          vector<size_t> extent;
          ImageType1 in1; // Adapter::Reslice<Interp::Cubic, Image<float>>) :
          ImageType2 in2;
      };

      template <typename ImageType1, typename ImageType2>
      struct NCCPrecomputeFunctorMasked_Naive { MEMALIGN(NCCPrecomputeFunctorMasked_Naive<ImageType1,ImageType2>)
        template <typename MaskType, typename ImageType3>
        void operator() (MaskType& mask, ImageType3& out) {
          if (!mask.value())
            return;
          out.index(0) = mask.index(0);
          out.index(1) = mask.index(1);
          out.index(2) = mask.index(2);
          out.index(3) = 0;
          typedef typename ImageType3::value_type value_type;
          in1.index(0) = mask.index(0);
          in1.index(1) = mask.index(1);
          in1.index(2) = mask.index(2);
          value_type value_in1 =  in1.value();
          if (value_in1 != value_in1){ // nan in image 1, update mask
            DEBUG("nan in image 1.");
            mask.value() = false;
            out.row(3) = 0.0;
            return;
          }
          in2.index(0) = mask.index(0);
          in2.index(1) = mask.index(1);
          in2.index(2) = mask.index(2);
          value_type value_in2 = in2.value();
          if (value_in2 != value_in2){ // nan in image 2, update mask
            DEBUG("nan in image 2.");
            mask.value() = false;
            out.row(3) = 0.0;
            return;
          }
          auto niter = NeighbourhoodIterator(mask, extent);
          value_type v1 = 0, v1_2 = 0, v2 = 0, v2_2 = 0 , v1_v2 = 0, mean1, mean2;
          size_t cnt = 0;
          // DEBUG(str(niter));
          while(niter.loop()){
            mask.index(0) = niter.index(0);
            mask.index(1) = niter.index(1);
            mask.index(2) = niter.index(2);
            // DEBUG("      "+str(mask));
            if (!mask.value())
              continue;
            in1.index(0) = niter.index(0);
            in1.index(1) = niter.index(1);
            in1.index(2) = niter.index(2);
            value_type val1 = in1.value();
            if (val1 != val1){
              DEBUG("nan in image 1");
              continue;
            }
            in2.index(0) = niter.index(0);
            in2.index(1) = niter.index(1);
            in2.index(2) = niter.index(2);
            value_type val2 = in2.value();
            if (val2 != val2){
              DEBUG("nan in image 2");
              continue;
            }
            v1    += val1;
            v1_2  += val1 * val1;
            v2    += val2;
            v2_2  += val2 * val2;
            v1_v2 += val1 * val2;
            cnt   += 1;
          }
          // reset the mask index
          mask.index(0) = out.index(0);
          mask.index(1) = out.index(1);
          mask.index(2) = out.index(2);

          if (cnt <= 0)
            throw Exception ("FIXME: neighbourhood does not contain centre");

          // subtract mean
          mean1 = v1 / cnt;
          mean2 = v2 / cnt;
          v1_2  -= (v1 * mean1);
          v2_2  -= (v2 * mean2);
          v1_v2 -= (v2 * mean1);

          assert(mean1 == mean1);
          assert(mean2 == mean2);
          assert(v1_2 == v1_2);
          assert(v2_2 == v2_2);
          assert(v1_v2 == v1_v2);

          // out.value() = value_in1 - mean1;
          // out.index(3) = 1;
          // out.value() = value_in2 - mean2;
          // out.index(3) = 2;
          // out.value() = v1_v2;
          // out.index(3) = 3;
          // out.value() = v1_2;
          // out.index(3) = 4;
          // out.value() = v2_2;

          out.row(3) = ( Eigen::Matrix<default_type,5,1>() << value_in1 - mean1, value_in2 - mean2, v1_v2, v1_2, v2_2 ).finished();
          // HACK for debug:
          // in2.index(0) = out.index(0);
          // in2.index(1) = out.index(1);
          // in2.index(2) = out.index(2);
          // in1.index(0) = out.index(0);
          // in1.index(1) = out.index(1);
          // in1.index(2) = out.index(2);
          // out.row(3) << value_in1 - mean1, value_in2 - mean2, in1.value(), in2.value(), v2_2;
        }

        NCCPrecomputeFunctorMasked_Naive (const vector<size_t>& ext, ImageType1& adapter1, ImageType2& adapter2) :
          extent(ext),
          in1(adapter1),
          in2(adapter2) { /* TODO check dimensions and extent */ }

        protected:
          vector<size_t> extent;
          ImageType1 in1; // store reslice adapter in functor to avoid iterating over it when mask is false
          ImageType2 in2;
      };

      class NormalisedCrossCorrelation { MEMALIGN(NormalisedCrossCorrelation)
          private:
            transform_type midway_v2s;

          public:
            /** typedef int is_neighbourhood: type_trait to distinguish voxel-wise and neighbourhood based metric types */
            typedef int is_neighbourhood;
            /** requires_precompute int is_neighbourhood: type_trait to distinguish metric types that require a call to precompute before the operator() is called */
            typedef int requires_precompute;

            template <class ParamType>
              default_type precompute(ParamType& parameters) {
                INFO("precomputing cross correlation data...");
                throw Exception ("TODO");

                typedef decltype(parameters.im1_image) Im1Type;
                typedef decltype(parameters.im2_image) Im2Type;
                typedef decltype(parameters.im1_mask) Im1MaskType;
                typedef decltype(parameters.im2_mask) Im2MaskType;
                typedef typename ParamType::ProcessedValueType ProcessedImageValueType;
                typedef typename ParamType::ProcessedMaskType ProcessedMaskType;
                typedef typename ParamType::ProcessedMaskInterpType ProcessedMaskInterpolatorType;
                typedef typename ParamType::ProcessedImageInterpType CCInterpType;

                Header midway_header (parameters.midway_image);
                midway_v2s = MR::Transform (midway_header).voxel2scanner;

                // store precomputed values in cc_image:
                // volumes 0 and 1: normalised intensities of both images (Im1 and Im2)
                // vlumes 2 to 4: neighbourhood dot products Im1.dot(Im2), Im1.dot(Im1), Im2.dot(Im2)
                auto cc_image_header = Header::scratch (midway_header);
                cc_image_header.ndim() = 4;
                cc_image_header.size(3) = 5;
                ProcessedMaskType cc_mask;
                auto cc_mask_header = Header::scratch (parameters.midway_image);

                auto cc_image = cc_image_header.template get_image <ProcessedImageValueType>().with_direct_io(Stride::contiguous_along_axis(3));
                {
                  LogLevelLatch log_level (0);
                  if (parameters.im1_mask.valid() or parameters.im2_mask.valid())
                    cc_mask = cc_mask_header.template get_image<bool>();
                  if (parameters.im1_mask.valid() and !parameters.im2_mask.valid())
                    Filter::reslice <Interp::Nearest> (parameters.im1_mask, cc_mask, parameters.transformation.get_transform_half());
                  else if (!parameters.im1_mask.valid() and parameters.im2_mask.valid())
                    Filter::reslice <Interp::Nearest> (parameters.im2_mask, cc_mask, parameters.transformation.get_transform_half_inverse(), Adapter::AutoOverSample);
                  else if (parameters.im1_mask.valid() and parameters.im2_mask.valid()){
                    Adapter::Reslice<Interp::Linear, Im1MaskType> mask_reslicer1 (parameters.im1_mask, cc_mask_header, parameters.transformation.get_transform_half());
                    Adapter::Reslice<Interp::Linear, Im2MaskType> mask_reslicer2 (parameters.im2_mask, cc_mask_header, parameters.transformation.get_transform_half_inverse());
                    // TODO should be faster to just loop over m1:
                    //    if (m1.value())
                    //     assign_pos_of(m1).to(m2); cc_mask.value() = m2.value()
                    auto both = [](decltype(cc_mask)& cc_mask, decltype(mask_reslicer1)& m1, decltype(mask_reslicer2)& m2) {
                      cc_mask.value() = ((m1.value() + m2.value()) / 2.0) > 0.5 ? true : false;
                    };
                    ThreadedLoop (cc_mask).run (both, cc_mask, mask_reslicer1, mask_reslicer2);
                  }
                }

                Adapter::Reslice<Interp::Cubic, Im1Type> interp1 (parameters.im1_image, cc_image_header, parameters.transformation.get_transform_half(), Adapter::AutoOverSample, std::numeric_limits<typename Im1Type::value_type>::quiet_NaN());

                Adapter::Reslice<Interp::Cubic, Im2Type> interp2 (parameters.im2_image, cc_image_header, parameters.transformation.get_transform_half_inverse(), Adapter::AutoOverSample, std::numeric_limits<typename Im2Type::value_type>::quiet_NaN());

                const auto extent = parameters.get_extent();

                // TODO unmasked CCPrecomputeFunctor. create a mask (all voxels true) if none given.
                // ThreadedLoop (cc_image, 0, 3).run (CCPrecomputeFunctor_Bogus(), interp1, interp2, cc_image);
                if (!cc_mask.valid()){
                  cc_mask = cc_mask_header.template get_image<bool>();
                  ThreadedLoop (cc_mask).run([](decltype(cc_mask)& m) {m.value()=true;}, cc_mask);
                }
                parameters.processed_mask = cc_mask;
                parameters.processed_mask_interp.reset (new ProcessedMaskInterpolatorType (parameters.processed_mask));
                auto loop = ThreadedLoop ("precomputing cross correlation data...", parameters.processed_mask);
                loop.run (NCCPrecomputeFunctorMasked_Naive<decltype(interp1), decltype(interp2)>(extent, interp1, interp2), parameters.processed_mask, cc_image);
                parameters.processed_image = cc_image;
                parameters.processed_image_interp.reset (new CCInterpType (parameters.processed_image));
                // display<Image<float>>(parameters.processed_image);
                return 0;
              }

            template <class Params>
              default_type operator() (Params& params,
                                       const Iterator& iter,
                                       Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
                assert (params.processed_image.index(0) == iter.index(0)); // iterates over processed image rather than midway image
                assert (params.processed_image.index(1) == iter.index(1));
                assert (params.processed_image.index(2) == iter.index(2));

                if (params.processed_mask.valid()) {
                  params.processed_mask.index(0) = iter.index(0);
                  params.processed_mask.index(1) = iter.index(1);
                  params.processed_mask.index(2) = iter.index(2);
                  if (!params.processed_mask.value())
                    return 0.0;
                }

                const Eigen::Vector3 pos = Eigen::Vector3(default_type(iter.index(0)), default_type(iter.index(0)), default_type(iter.index(0)));

                params.processed_image.index(3) = 2;
                default_type A = params.processed_image.value();
                params.processed_image.index(3) = 3;
                default_type B = params.processed_image.value();
                params.processed_image.index(3) = 4;
                default_type C = params.processed_image.value();
                default_type A_BC = A / (B * C);

                if (A_BC != A_BC){
                  DEBUG("A_BC is NAN");
                  return 0.0;
                }

                params.processed_image_interp->voxel(pos);
                params.processed_image_interp->index (3) = 0;
                typename Params::Im1ValueType val1;
                typename Params::Im2ValueType val2;
                Eigen::Matrix<typename Params::Im1ValueType, 1, 3> grad1;
                Eigen::Matrix<typename Params::Im2ValueType, 1, 3> grad2;

                // gradient calculation
                params.processed_image_interp->index(3) = 0;
                params.processed_image_interp->value_and_gradient_wrt_scanner(val1, grad1);
                if (val1 != val1){
                  // this should not happen as the precompute should have changed the mask
                  WARN("FIXME: val1 is nan");
                  return 0.0;
                }
                params.processed_image_interp->index(3) = 1;
                params.processed_image_interp->value_and_gradient_wrt_scanner(val2, grad2);
                if (val2 != val2){
                  // this should not happen as the precompute should have changed the mask
                  WARN("FIXME: val2 is nan");
                  return 0.0;
                }

                // {
                  // typename Params::Im1ValueType val1_scanner;
                  // typename Params::Im2ValueType val2_scanner;
                  // Eigen::Matrix<typename Params::Im1ValueType, 1, 3> grad1_scanner;
                  // Eigen::Matrix<typename Params::Im2ValueType, 1, 3> grad2_scanner;
                  // params.processed_image_interp->index(3) = 0;
                  // params.processed_image_interp->scanner(midway_point);
                  // params.processed_image_interp->value_and_gradient(val1_scanner, grad1_scanner);
                  // params.processed_image_interp->index(3) = 1;
                  // params.processed_image_interp->value_and_gradient(val2_scanner, grad2_scanner);
                  // if (!grad1.isApprox(grad1_scanner))
                  //   WARN("gradient: " + str(grad1) + " " + str(grad1_scanner));
                  // if (!grad2.isApprox(grad2_scanner))
                  //   WARN("gradient: " + str(grad2) + " " + str(grad2_scanner));
                  // params.processed_image.index(3) = 0;
                  // std::cerr << "val1: " << params.processed_image.value() << ", " << val1_scanner << ", " << val1 << std::endl;
                  // params.processed_image.index(3) = 1;
                  // std::cerr << "val2: " << params.processed_image.value() << ", " << val2_scanner << ", " << val2 << std::endl;
                // }
                const Eigen::Vector3 midway_point = midway_v2s * pos;
                Eigen::MatrixXd jacobian = params.transformation.get_jacobian_wrt_params (midway_point);
                for (ssize_t par = 0; par < gradient.size(); par++) {
                  default_type sum = 0.0;
                  for ( size_t dim = 0; dim < 3; dim++){
                    sum -= A_BC * jacobian (dim, par) * (val2 - A/B * val1) * grad1[dim];
                    sum -= A_BC * jacobian (dim, par) * (val1 - A/C * val2) * grad2[dim];
                  }
                  gradient[par] += sum;
                }

                return A * A_BC;
            }
      };
    }
  }
}
#endif
