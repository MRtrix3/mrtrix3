#ifndef __image_filter_gradient_h__
#define __image_filter_gradient_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/adapter/gradient1D.h"
#include "image/buffer_scratch.h"
#include "image/transform.h"
#include "image/loop.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {
      /** \addtogroup Filters
      @{ */

      /*! Compute the image gradients of a 3D or 4D image.
       *
       * Typical usage:
       * \code
       * Image::BufferPreload<float> src_data (argument[0]);
       * Image::BufferPreload<float>::voxel_type src (src_data);
       * Image::Filter::Gradient gradient_filter (src);
       *
       * Image::Header header (src_data);
       * header.info() = gradient_filter.info();
       * header.datatype() = src_data.datatype();
       *
       * Image::Buffer<float> dest_data (argument[1], src_data);
       * Image::Buffer<float>::voxel_type dest (dest_data);
       *
       * smooth_filter (src, dest);
       *
       * \endcode
       */
      class Gradient : public ConstInfo
      {
        public:
          template <class InputVoxelType>
            Gradient (const InputVoxelType& in) :
              ConstInfo (in),
              wrt_scanner_(true) {
                if (in.ndim() == 4) {
                  axes_.resize(5);
                  axes_[3].dim = 3;
                  axes_[4].dim = in.dim(3);
                  axes_[0].stride = 2;
                  axes_[1].stride = 3;
                  axes_[2].stride = 4;
                  axes_[3].stride = 1;
                  axes_[4].stride = 5;
                } else if (in.ndim() == 3) {
                  axes_.resize(4);
                  axes_[3].dim = 3;
                  axes_[0].stride = 2;
                  axes_[1].stride = 3;
                  axes_[2].stride = 4;
                  axes_[3].stride = 1;
                } else {
                  throw Exception("input image must be 3D or 4D");
                }
              datatype_ = DataType::Float32;
          }

          void compute_wrt_scanner(bool wrt_scanner) {
            wrt_scanner_ = wrt_scanner;
          }


          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& in, OutputVoxelType& out) {

              size_t num_volumes;
              (in.ndim() == 3) ? num_volumes = 1 : num_volumes = in.dim(3);

              for (size_t vol = 0; vol < num_volumes; ++vol) {
                if (in.ndim() == 4) {
                  in[3] = vol;
                  out[4] = vol;
                }
                Adapter::Gradient1D<InputVoxelType> gradient1D (in);
                out[3] = 0;
                threaded_copy (gradient1D, out, 2, 0, 3);
                out[3] = 1;
                gradient1D.set_axis (1);
                threaded_copy (gradient1D, out, 2, 0, 3);
                out[3] = 2;
                gradient1D.set_axis (2);
                threaded_copy (gradient1D, out, 2, 0, 3);

                if (wrt_scanner_) {
                  Image::Transform transform (in);

                  Math::Vector<float> gradient (3);
                  Math::Vector<float> gradient_wrt_scanner (3);

                  Image::Loop loop (0, 3);
                  for (loop.start (out); loop.ok(); loop.next (out)) {
                    for (size_t dim = 0; dim < 3; dim++) {
                      out[3] = dim;
                      gradient[dim] = out.value() / in.vox(dim);
                    }
                    transform.image2scanner_dir (gradient, gradient_wrt_scanner);
                    for (size_t dim = 0; dim < 3; dim++) {
                      out[3] = dim;
                      out.value() = gradient_wrt_scanner[dim];
                    }
                  }
                }
              }
            }

        protected:
          bool wrt_scanner_;
      };
      //! @}
    }
  }
}


#endif
