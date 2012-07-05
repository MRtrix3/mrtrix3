/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 05/06/12

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef __image_filter_anisotropic_h__
#define __image_filter_anisotropic_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/buffer_scratch.h"
#include "image/threaded_loop.h"
#include "image/adapter/info.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      namespace {

        template <class InputVoxelType, class OutputVoxelType, class KernelVoxelType>
          class __AnisotropicCopyKernel {
            public:
            __AnisotropicCopyKernel (InputVoxelType& input, OutputVoxelType output, KernelVoxelType kernel) :
                in_ (input),
                out_ (output),
                kernel_ (kernel)
                { }

              void operator () (const Iterator& pos) {
                voxel_assign (in_, pos);
                voxel_assign (out_, pos);
                out_.value() = in_.value();
              }

            protected:
              InputVoxelType in_;
              OutputVoxelType out_;
              KernelVoxelType kernel_;
          };
      }


      class AnisotropicSmooth : public ConstInfo
      {

      public:
          template <class InputVoxelType>
            AnisotropicSmooth (const InputVoxelType& in) :
              ConstInfo (in),
              stdev_ (in.ndim(), 1),
              directions_(0,0) {
            for (unsigned int i = 0; i < in.ndim(); i++)
              stdev_[i] = in.vox(i);
          }

          template <class InputVoxelType>
            AnisotropicSmooth (const InputVoxelType& in,
                               const std::vector<float>& stdev,
                               const Math::Matrix<float>& directions) :
              ConstInfo (in),
              stdev_(in.ndim()),
              directions_(directions) {
              set_stdev(stdev);
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
              if (extent_.size() != this->ndim())
                throw Exception ("The number of extent values supplied does not correspond to the number of dimensions");
              if (directions_.columns() != 2)
                throw Exception ("unexpected number of elements defining the directions. Expecting az el pairs.");
              int num_volumes = 1;
              if (input.ndim() == 3) {
                if (directions_.rows() > 1)
                  throw Exception ("more than one direction has been set for anisotropic smoothing of a 3D volume.");
              } else {
                if (directions_.rows() != input.dim(3))
                  throw Exception ("the number of directions does not equal the number of volumes along axis 3.");
                int num_volumes = input.dim(3);
              }

              Image::Header kernel_header;
              kernel_header.set_ndim(3);
              kernel_header.ndim() = 3;
              kernel_header.datatype() = DataType::Float32;
              float radius = ceil(4 * stdev_[0]);
              float extent = 2 * radius + 1;
              kernel_header.dim(0) = extent_;
              kernel_header.dim(1) = extent_;
              kernel_header.dim(2) = extent_;

              Image::BufferScratch<float> kernel_data (kernel_header);
              Image::BufferScratch<float>::voxel_type kernel_vox (kernel_data);

              for (size_t vol = 0; vol < num_volumes; ++vol) {

                Math::Matrix<float> tensor (3, 3, 0);
                tensor(0,0) = stdev_[0];
                tensor(1,1) = stdev_[1];
                tensor(2,2) = stdev_[1];

                Math::Matrix<float> R_az (3, 3, 0);
                R_az (0,0) = cos (dir[0]);
                R_az (0,1) = -sin (dir[0]);
                R_az (1,0) = sin (dir[0]);
                R_az (1,1) = cos (dir[0]);

                Math::Matrix<float> R_el (3, 3, 0);
                R_el (0,0) = cos (dir[1]);
                R_el (0,2) = sin (dir[1]);
                R_el (2,0) = -sin (dir[1]);
                R_el (2,2) = cos (dir[1]);

                Math::Matrix<float> R_az_t (3, 3, 0);
                Math::Matrix<float> R_el_t (3, 3, 0);
                Math::transpose (R_az_t, R_az);
                Math::transpose (R_el_t, R_el);

                Math::Matrix<float> temp (3, 3, 0);
                Math::Matrix<float> temp2 (3, 3, 0);
                Math::mult(temp, R_el_t, R_az_t);
                Math::mult(temp2, D, temp);
                Math::mult(temp, R_el, temp2);
                Math::mult(temp2, R_az, temp);
                Math::pinv(temp, temp2);

                Math::Matrix<float> vox2real (3, 3);

                Loop loop (0,3);
                for (loop.start(kernel_vox); loop.ok(); loop.next(kernel_vox)) {
                  Math::Vector<float> vec(3);
                  vec[0] = (kernel_vox[0] - radius) * input.vox(0);
                  vec[1] = (kernel_vox[1] - radius) * input.vox(1);
                  vec[2] = (kernel_vox[2] - radius) * input.vox(2);
                  Math::Vector<float> temp_vec;
                  Math::mult(temp_vec, temp, vec);
                  kernel_vox.value() = exp (-0.5 * Math::dot(vec, temp_vec));
                }

                __AnisotropicCopyKernel <InputVoxelType, OutputVoxelType, Image::BufferScratch<float>::voxel_type> copy_kernel (input, output, kernel);
                ThreadedLoop (input, 1, 0, 3).run (copy_kernel);
              }
            }

      protected:
          Math::Matrix<float> directions_;
          std::vector<int> extent_;
          std::vector<float> stdev_;
      };
      //! @}
    }
  }
}


#endif
