/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __image_filter_anisotropic_h__
#define __image_filter_anisotropic_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/buffer_scratch.h"
#include "image/buffer.h"
#include "image/threaded_loop.h"
#include "image/adapter/info.h"
#include "image/nav.h"
#include "math/least_squares.h"
#include <cmath>

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      namespace {

        typedef struct {
          int offset[3];
          float weight;
        } KernelWeight;

        template <class InputVoxelType, class OutputVoxelType>
          class __AnisotropicCopyKernel {
            public:
            __AnisotropicCopyKernel (InputVoxelType& input, OutputVoxelType& output, std::vector<KernelWeight> & kernel) :
                in_ (input),
                out_ (output),
                kernel_ (kernel) { }

              void operator () (const Iterator& pos) {
                voxel_assign (out_, pos, 0, 3);
                float val = 0;
                for (size_t w = 0; w < kernel_.size(); ++w) {
                  voxel_assign (in_, pos, 0, 3);
                  in_[0] += kernel_[w].offset[0];
                  in_[1] += kernel_[w].offset[1];
                  in_[2] += kernel_[w].offset[2];
                  if (Image::Nav::within_bounds(in_))
                    val += kernel_[w].weight * in_.value();
                }
                out_.value() = val;
              }

            protected:
              InputVoxelType in_;
              OutputVoxelType out_;
              std::vector<KernelWeight> kernel_;
        };
      }


      class AnisotropicSmooth : public ConstInfo
      {

      public:
          template <class InputVoxelType>
            AnisotropicSmooth (const InputVoxelType& in) :
              ConstInfo (in),
              stdev_ (2, 1),
              directions_ (0,0) {
              stdev_[0] = 3;
              stdev_[1] = 1;
          }

          template <class InputVoxelType>
            AnisotropicSmooth (const InputVoxelType& in,
                               const std::vector<float>& stdev,
                               const Math::Matrix<float>& directions) :
              ConstInfo (in),
              stdev_ (stdev),
              directions_ (directions) {
              set_stdev (stdev);
          }

          //! Set the standard deviation of the anisotropic gaussian
          //  The first element corresponds to the stdev along the primary eigenvector
          //  The second element corresponds to the stdev along the other two eigenvectors
          void set_stdev (const std::vector<float>& stdev) {
            stdev_ = stdev;
          }

          void set_directions (const Math::Matrix<float>& directions) {
            directions_ = directions;
          }

          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output) {
              if (stdev_.size() != 2)
                throw Exception ("Anisotropic Gaussian smoothing requires two stdev values, one for the primary "
                                 "eigenvector of the Gaussian kernel, and another for the second and third eigenvector");
              if (directions_.columns() != 2)
                throw Exception ("unexpected number of elements defining the directions. Expecting az el pairs.");
              unsigned int num_volumes = 1;
              if (input.ndim() == 3) {
                if (directions_.rows() > 1)
                  throw Exception ("more than one direction has been set for anisotropic smoothing of a 3D volume.");
              } else {
                if (int(directions_.rows()) != input.dim(3))
                  throw Exception ("the number of directions does not equal the number of volumes along axis 3.");
                num_volumes = input.dim(3);
              }

              std::vector<int> radius(3);
              radius[0] = ceil((2 * stdev_[0]) / input.vox(0));
              radius[1] = ceil((2 * stdev_[0]) / input.vox(1));
              radius[2] = ceil((2 * stdev_[0]) / input.vox(2));

              ProgressBar progress ("smoothing image...", num_volumes);
              for (unsigned int vol = 0; vol < num_volumes; ++vol) {

                Math::Matrix<float> covariance (3, 3);
                covariance.zero();
                covariance(0,0) = stdev_[1] * stdev_[1];
                covariance(1,1) = stdev_[1] * stdev_[1];
                covariance(2,2) = stdev_[0] * stdev_[0];

                Math::Matrix<float> R_az (3, 3);
                R_az.identity();
                R_az (0,0) = cos (directions_.row (vol)[0]);
                R_az (0,1) = -sin (directions_.row (vol)[0]);
                R_az (1,0) = sin (directions_.row (vol)[0]);
                R_az (1,1) = cos (directions_.row (vol)[0]);

                Math::Matrix<float> R_el (3, 3);
                R_el.identity();
                R_el (0,0) = cos (directions_.row (vol)[1]);
                R_el (0,2) = sin (directions_.row (vol)[1]);
                R_el (2,0) = -sin (directions_.row (vol)[1]);
                R_el (2,2) = cos (directions_.row (vol)[1]);

                Math::Matrix<float> R_az_t (3, 3);
                Math::Matrix<float> R_el_t (3, 3);
                Math::transpose (R_az_t, R_az);
                Math::transpose (R_el_t, R_el);
                Math::Matrix<float> temp (3, 3);
                Math::Matrix<float> temp2 (3, 3);
                Math::mult (temp, R_el_t, R_az_t);
                Math::mult (temp2, covariance, temp);
                Math::mult (temp, R_el, temp2);
                Math::mult (temp2, R_az, temp);
                Math::pinv(temp, temp2);

                std::vector<KernelWeight> kernel;
                float sum = 0;
                int count = 0;
                for (int x = -radius[0]; x <= radius[0]; x++) {
                  for (int y = -radius[1]; y <= radius[1]; y++) {
                    for (int z = -radius[2]; z <= radius[2]; z++) {
                      Math::Vector<float> vec (3);
                      vec[0] = x * input.vox(0);
                      vec[1] = y * input.vox(1);
                      vec[2] = z * input.vox(2);
                      Math::Vector<float> temp_vec;
                      Math::mult(temp_vec, temp, vec);
                      float weight = exp (-0.5 * Math::dot (vec, temp_vec));
                      if (weight > 0.01) {
                        sum += weight;
                        KernelWeight kernel_weight;
                        kernel_weight.weight = weight;
                        kernel_weight.offset[0] = x;
                        kernel_weight.offset[1] = y;
                        kernel_weight.offset[2] = z;
                        kernel.push_back (kernel_weight);
                        count++;
                      }
                    }
                  }
                }
                for (size_t w = 0; w < kernel.size(); ++w)
                  kernel[w].weight /= sum;

                input[3] = vol;
                output[3] = vol;
                __AnisotropicCopyKernel <InputVoxelType, OutputVoxelType> copy_kernel (input, output, kernel);
                ThreadedLoop (input, 1, 0, 3).run (copy_kernel);
                progress++;
              }
            }

      protected:
          std::vector<float> stdev_;
          Math::Matrix<float> directions_;
      };
      //! @}
    }
  }
}


#endif
