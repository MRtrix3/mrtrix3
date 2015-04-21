/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 06/07/11.

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

#ifndef __image_matrix_multiply_h__
#define __image_matrix_multiply_h__

#include "math/matrix.h"
#include "image/voxel.h"
#include "image/threaded_loop.h"

namespace MR 
{
  namespace Image 
  {

    //! a no-operation function for use with the Image::matrix_multiply() functions
    template <typename ValueType>
      ValueType NoOp (ValueType x) { return x; }


    //! the functor used as the backend for the Image::matrix_multiply() functions
    template <class InputVoxelType, class OutputVoxelType, class PreFunctor, class PostFunctor, typename value_type>
      class MatrixMultiply {
        public:
          MatrixMultiply (const InputVoxelType& in, 
              const OutputVoxelType& out, 
              const Math::Matrix<value_type>& matrix,
              size_t val_axis,
              PreFunctor func_pre,
              PostFunctor func_post) :
            in (in),
            out (out),
            matrix (matrix),
            vals_out (matrix.rows()),
            vals_in (matrix.columns()),
            val_loop (val_axis, val_axis+1),
            val_axis (val_axis),
            func_pre (func_pre),
            func_post (func_post) { }


          void operator () (const Image::Iterator& pos) {
            Image::voxel_assign (in, pos);
            Image::voxel_assign (out, pos);

            // load input values into matrix:
            for (auto l = val_loop (in); l; ++l) 
              vals_in [in[val_axis]] = func_pre (in.value());

            // apply matrix:
            Math::mult (vals_out, matrix, vals_in);

            // write back:
            for (auto l = val_loop (out); l; ++l) 
              out.value() = func_post (vals_out[out[val_axis]]);
          }

        protected:
          InputVoxelType in;
          OutputVoxelType out;
          const Math::Matrix<value_type>& matrix;
          Math::Vector<value_type> vals_out, vals_in;
          Image::Loop val_loop;
          const size_t val_axis;
          PreFunctor func_pre;
          PostFunctor func_post;
      };



    //! perform a multi-threaded matrix-vector multiply per voxel
    /*! perform a multi-threaded matrix-vector multiply of the constant matrix
     * \a matrix for each vector along axis \a val_axis, read from the
     * InputVoxelType object \a in, storing the resulting vector of values in
     * the OutputVoxelType object \a out. Each value is processed by the \a
     * func_pre functor as it is read in, and by \a func_post as it written
     * out. The Image::NoOp() function can be supplied when no operation is to
     * be performed on the values before or after the matrix operation. 
     *
     * For example:
     * \code
     * Image::Buffer<float>::voxel_type in;
     * Image::Buffer<float>::voxel_type out;
     *
     * Math::Matrix<float> matrix (out.dim(3), in.dim(3));
     * 
     * ...
     *
     * Image::matrix_multiply ("performing matrix-vector multiply...", 
     *                         matrix, in, out, 
     *                         Image::NoOp<float>, Image::NoOp<float>);
     * \endcode
     *
     * The \a func_pre and \a func_post operations can be a simple function or
     * a functor, depending on what needs to be done. For example, the
     * following will take the log on the input data before the matrix
     * multiply, and clamp the output at a pre-specified maximum value:
     * \code
     * float take_log (float val) { return std::log (val); }
     * 
     * class clamp_to_max {
     *   public:
     *     clamp_to_max (float maxval) : maxval (maxval) { }
     *     float operator() (float val) const { return std::min (val, maxval); }
     *   protected:
     *     const float maxval;
     * };
     * 
     * ...
     *
     * float maxval = get_max_val();
     * 
     * Image::matrix_multiply ("performing clamped log-transformed matrix-vector multiply...", 
     *                         matrix, in, out, 
     *                         take_log, clamp_to_max (maxval));
     * \endcode
     */
    template <class InputVoxelType, class OutputVoxelType, class PreFunctor, class PostFunctor, typename value_type>
      inline void matrix_multiply (
          const Math::Matrix<value_type>& matrix,
          const InputVoxelType& in, 
          const OutputVoxelType& out, 
          PreFunctor func_pre,
          PostFunctor func_post,
          size_t val_axis = 3)
      {
        std::vector<size_t> axes = Image::Stride::order (in);
        axes.erase (std::find (axes.begin(), axes.end(), val_axis));
        Image::ThreadedLoop (in, axes, 1)
          .run (MatrixMultiply<InputVoxelType,OutputVoxelType,PreFunctor,PostFunctor,value_type> (in, out, matrix, val_axis, func_pre, func_post));
      }

    //! @copydoc matrix_multiply()
    template <class InputVoxelType, class OutputVoxelType, class PreFunctor, class PostFunctor, typename value_type>
      inline void matrix_multiply (
          const std::string& progress_message,
          const Math::Matrix<value_type>& matrix,
          const InputVoxelType& in, 
          const OutputVoxelType& out, 
          PreFunctor func_pre,
          PostFunctor func_post,
          size_t val_axis = 3)
      {
        std::vector<size_t> axes = Image::Stride::order (in);
        axes.erase (std::find (axes.begin(), axes.end(), val_axis));
        Image::ThreadedLoop (progress_message, in, axes, 1)
          .run (MatrixMultiply<InputVoxelType,OutputVoxelType,PreFunctor,PostFunctor,value_type> (in, out, matrix, val_axis, func_pre, func_post));
      }


  }
}


#endif

