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


#ifndef __interp_linear_h__
#define __interp_linear_h__

#include <complex>
#include <type_traits>

#include "datatype.h"
#include "types.h"
#include "interp/base.h"


namespace MR
{
  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! This class provides access to the voxel intensities of a data set, using tri-linear interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes.
     * The (integer) position along the remaining axes should be set using the
     * template DataSet class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner().
     * For example:
     * \code
     * auto voxel = image_buffer.voxel();
     *
     * auto input = Image<float>::create (argument[0]);
     *
     * // create an Interp::Linear object using input as the parent data set:
     * Interp::Linear<decltype(input) > interp (input);
     *
     * // set the scanner-space position to [ 10.2 3.59 54.1 ]:
     * interp.scanner (10.2, 3.59, 54.1);
     *
     * // get the value at this position:
     * float value = interp.value();
     * \endcode
     *
     * The template \a input class must be usable with this type of syntax:
     * \code
     * int xsize = input.size(0);    // return the dimension
     * int ysize = input.size(1);    // along the x, y & z dimensions
     * int zsize = input.size(2);
     * float v[] = { input.spacing(0), input.spacing(1), input.spacing(2) };  // return voxel dimensions
     * input.index(0) = 0;               // these lines are used to
     * input.index(1)--;                 // set the current position
     * input.index(2)++;                 // within the data set
     * float f = input.value();
     * transform_type M = input.transform(); // a valid 4x4 transformation matrix
     * \endcode
     */

    template <class C>
    struct value_type_of
    { NOMEMALIGN
      using type = C;
    };

    template <class X>
    struct value_type_of<std::complex<X>>
    { NOMEMALIGN
      using type = X;
    };


    enum LinearInterpProcessingType
    {
      Value = 1,
      Derivative = 2,
      ValueAndDerivative = Value | Derivative
    };

    // To avoid unnecessary computation, we want to partially specialize our template based
    // on processing type (value/gradient or both), however each specialization has common core logic
    // which we store in LinearInterpBase

    template <class ImageType, LinearInterpProcessingType PType>
    class LinearInterpBase : public Base<ImageType> { MEMALIGN(LinearInterpBase<ImageType, PType>)
      public:
        using typename Base<ImageType>::value_type;
        using coef_type = typename value_type_of<value_type>::type;


        LinearInterpBase (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
            Base<ImageType> (parent, value_when_out_of_bounds),
            zero (0.0),
            eps (1.0e-6) { }

      protected:
        const coef_type zero, eps;
        Eigen::Vector3 P;

        ssize_t clamp (ssize_t x, ssize_t dim) const {
          if (x < 0) return 0;
          if (x >= dim) return (dim-1);
          return x;
        }
    };


    template <class ImageType, LinearInterpProcessingType PType>
    class LinearInterp : public LinearInterpBase <ImageType, PType> { MEMALIGN(LinearInterp<ImageType,PType>)
      private:
        LinearInterp ();
    };


    // Specialization of LinearInterp when we're only after interpolated values

    template <class ImageType>
    class LinearInterp<ImageType, LinearInterpProcessingType::Value> :
        public LinearInterpBase<ImageType, LinearInterpProcessingType::Value> 
    { MEMALIGN(LinearInterp<ImageType,LinearInterpProcessingType::Value>)
      public:
        using LinearBase = LinearInterpBase<ImageType, LinearInterpProcessingType::Value>;

        using value_type = typename LinearBase::value_type;
        using coef_type = typename LinearBase::coef_type;
        using LinearBase::P;
        using LinearBase::clamp;
        using LinearBase::bounds;
        using LinearBase::eps;

        LinearInterp (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
            LinearInterpBase <ImageType, LinearInterpProcessingType::Value> (parent, value_when_out_of_bounds)
        { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos);
          if (Base<ImageType>::out_of_bounds)
            return false;
          P = pos;
          for (size_t i = 0; i < 3; ++i) {
            if (pos[i] < 0.0 || pos[i] > bounds[i]-0.5)
              f[i] = 0.0;
          }

          coef_type x_weights[2] = { coef_type(1 - f[0]), coef_type(f[0]) };
          coef_type y_weights[2] = { coef_type(1 - f[1]), coef_type(f[1]) };
          coef_type z_weights[2] = { coef_type(1 - f[2]), coef_type(f[2]) };

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            for (ssize_t y = 0; y < 2; ++y) {
              coef_type partial_weight = y_weights[y] * z_weights[z];
              for (ssize_t x = 0; x < 2; ++x) {
                factors[i] = x_weights[x] * partial_weight;

                if (factors[i] < eps)
                  factors[i] = 0.0;

                ++i;
              }
            }
          }

          return true;
        }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool image (const VectorType& pos) {
          return voxel (Transform::voxelsize.inverse() * pos.template cast<default_type>());
        }

        //! Set the current position to <b>scanner space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool scanner (const VectorType& pos) {
          return voxel (Transform::scanner2voxel * pos.template cast<default_type>());
        }

        FORCE_INLINE value_type value () {
          if (Base<ImageType>::out_of_bounds)
            return Base<ImageType>::out_of_bounds_value;

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          Eigen::Matrix<value_type, 8, 1> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_vec[i++] = ImageType::value ();
              }
            }
          }

          return coeff_vec.dot (factors);
        }

        //! Read interpolated values from volumes along axis >= 3
        /*! See file interp/base.h for details. */
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis) {
          if (Base<ImageType>::out_of_bounds) {
            Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_row (ImageType::size(axis));
            out_of_bounds_row.setOnes();
            out_of_bounds_row *= Base<ImageType>::out_of_bounds_value;
            return out_of_bounds_row;
          }

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 8> coeff_matrix ( ImageType::size(3), 8 );

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_matrix.col (i++) = ImageType::row (axis);
              }
            }
          }

          return coeff_matrix * factors;
        }

      protected:
        Eigen::Matrix<coef_type, 8, 1> factors;
    };


    // Specialization of LinearInterp when we're only after interpolated gradients

    template <class ImageType>
    class LinearInterp<ImageType, LinearInterpProcessingType::Derivative> :
        public LinearInterpBase<ImageType, LinearInterpProcessingType::Derivative>
    { MEMALIGN(LinearInterp<ImageType,LinearInterpProcessingType::Derivative>)
      public:
        using LinearBase = LinearInterpBase<ImageType, LinearInterpProcessingType::Derivative>;

        using value_type = typename LinearBase::value_type;
        using coef_type = typename LinearBase::coef_type;
        using LinearBase::P;
        using LinearBase::clamp;
        using LinearBase::bounds;
        using LinearBase::voxelsize;

        LinearInterp (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
            LinearInterpBase <ImageType, LinearInterpProcessingType::Derivative> (parent, value_when_out_of_bounds),
            out_of_bounds_vec (value_when_out_of_bounds, value_when_out_of_bounds, value_when_out_of_bounds),
            wrt_scanner_transform (Transform::scanner2image.linear() * voxelsize.inverse())
        { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos.template cast<default_type>());
          if (Base<ImageType>::out_of_bounds)
            return false;
          P = pos;
          for (size_t i = 0; i < 3; ++i) {
            if (pos[i] < 0.0 || pos[i] > bounds[i]-0.5)
              f[i] = 0.0;
          }

          coef_type x_weights[2] = {coef_type(1 - f[0]), coef_type(f[0])};
          coef_type y_weights[2] = {coef_type(1 - f[1]), coef_type(f[1])};
          coef_type z_weights[2] = {coef_type(1 - f[2]), coef_type(f[2])};

          // For linear interpolation gradient weighting is independent of direction
          // i.e. Simply looking at finite difference
          coef_type diff_weights[2] = {-0.5, 0.5};

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            for (ssize_t y = 0; y < 2; ++y) {
              coef_type partial_weight = y_weights[y] * z_weights[z];
              coef_type partial_weight_dy = diff_weights[y] * z_weights[z];
              coef_type partial_weight_dz = y_weights[y] * diff_weights[z];

              for (ssize_t x = 0; x < 2; ++x) {
                weights_matrix(i,0) = diff_weights[x] * partial_weight;
                weights_matrix(i,1) = x_weights[x] * partial_weight_dy;
                weights_matrix(i,2) = x_weights[x] * partial_weight_dz;

                ++i;
              }
            }
          }

          return true;
        }

        //! Set the current position to <b>image space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool image (const VectorType& pos) {
          return voxel (Transform::voxelsize.inverse() * pos.template cast<default_type>());
        }

        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool scanner (const VectorType& pos) {
          return voxel (Transform::scanner2voxel * pos.template cast<default_type>());
        }

        //! Returns the image gradient at the current position
        FORCE_INLINE Eigen::Matrix<value_type, 1, 3> gradient () {
          if (Base<ImageType>::out_of_bounds)
            return out_of_bounds_vec;

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          Eigen::Matrix<coef_type, 1, 8> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_vec[i++] = ImageType::value ();
              }
            }
          }

          return coeff_vec * weights_matrix;
        }

        //! Returns the image gradient at the current position, defined with respect to the scanner coordinate frame of reference.
        Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> gradient_wrt_scanner() {
          return gradient().template cast<default_type>() * wrt_scanner_transform;
        }

        // Collectively interpolates gradients along axis 3
        Eigen::Matrix<coef_type, Eigen::Dynamic, 3> gradient_row() {
          if (Base<ImageType>::out_of_bounds) {
            Eigen::Matrix<coef_type, Eigen::Dynamic, 3> out_of_bounds_matrix (ImageType::size(3), 3);
            out_of_bounds_matrix.setOnes();
            out_of_bounds_matrix *= Base<ImageType>::out_of_bounds_value;
            return out_of_bounds_matrix;
          }

          assert (ImageType::ndim() == 4);

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 8> coeff_matrix (ImageType::size(3), 8);

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_matrix.col (i++) = ImageType::row (3);
              }
            }
          }

          return coeff_matrix * weights_matrix;
        }


        //! Collectively interpolates gradients along axis 3, defined with respect to the scanner coordinate frame of reference.
        Eigen::Matrix<default_type, Eigen::Dynamic, 3> gradient_row_wrt_scanner() {
          return gradient_row().template cast<default_type>() * wrt_scanner_transform;
        }

      protected:
        const Eigen::Matrix<coef_type, 1, 3> out_of_bounds_vec;
        Eigen::Matrix<coef_type, 8, 3> weights_matrix;
        const Eigen::Matrix<default_type, 3, 3> wrt_scanner_transform;
    };



    // Specialization of LinearInterp when we're after both interpolated gradients and values
    template <class ImageType>
    class LinearInterp<ImageType, LinearInterpProcessingType::ValueAndDerivative> :
        public LinearInterpBase <ImageType, LinearInterpProcessingType::ValueAndDerivative>
    { MEMALIGN(LinearInterp<ImageType,LinearInterpProcessingType::ValueAndDerivative>)
      public:
        using LinearBase = LinearInterpBase<ImageType, LinearInterpProcessingType::ValueAndDerivative>;

        using value_type = typename LinearBase::value_type;
        using coef_type = typename LinearBase::coef_type;
        using LinearBase::P;
        using LinearBase::clamp;
        using LinearBase::bounds;
        using LinearBase::voxelsize;

        LinearInterp (const ImageType& parent, coef_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
          LinearInterpBase <ImageType, LinearInterpProcessingType::ValueAndDerivative> (parent, value_when_out_of_bounds),
          wrt_scanner_transform (Transform::scanner2image.linear() * voxelsize.inverse())
        {
          if (ImageType::ndim() == 4) {
            out_of_bounds_vec.resize(ImageType::size(3), 1);
            out_of_bounds_matrix.resize(ImageType::size(3), 3);
          } else {
            out_of_bounds_vec.resize(1, 1);
            out_of_bounds_matrix.resize(1, 3);
          }
          out_of_bounds_vec.fill(value_when_out_of_bounds);
          out_of_bounds_matrix.fill(value_when_out_of_bounds);
        }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos.template cast<default_type>());
          if (Base<ImageType>::out_of_bounds)
            return false;
          P = pos;
          for (size_t i = 0; i < 3; ++i) {
            if (pos[i] < 0.0 || pos[i] > bounds[i]-0.5)
              f[i] = 0.0;
          }

          coef_type x_weights[2] = { coef_type(1 - f[0]), coef_type(f[0]) };
          coef_type y_weights[2] = { coef_type(1 - f[1]), coef_type(f[1]) };
          coef_type z_weights[2] = { coef_type(1 - f[2]), coef_type(f[2]) };

          // For linear interpolation gradient weighting is independent of direction
          // i.e. Simply looking at finite difference
          coef_type diff_weights[2] = {coef_type(-0.5), coef_type(0.5) };

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            for (ssize_t y = 0; y < 2; ++y) {
              coef_type partial_weight = y_weights[y] * z_weights[z];
              coef_type partial_weight_dy = diff_weights[y] * z_weights[z];
              coef_type partial_weight_dz = y_weights[y] * diff_weights[z];

              for (ssize_t x = 0; x < 2; ++x) {
                // Gradient
                weights_matrix(i,0) = diff_weights[x] * partial_weight;
                weights_matrix(i,1) = x_weights[x] * partial_weight_dy;
                weights_matrix(i,2) = x_weights[x] * partial_weight_dz;
                // Value
                weights_matrix(i,3) = x_weights[x] * partial_weight;

                ++i;
              }
            }
          }

          return true;
        }

        //! Set the current position to <b>image space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool image (const VectorType& pos) {
          return voxel (Transform::voxelsize.inverse() * pos.template cast<default_type>());
        }

        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        FORCE_INLINE bool scanner (const VectorType& pos) {
          return voxel (Transform::scanner2voxel * pos.template cast<default_type>());
        }


        void value_and_gradient (value_type& value, Eigen::Matrix<coef_type, 1, 3>& gradient) {
          if (Base<ImageType>::out_of_bounds){
            value = out_of_bounds_vec(0);
            gradient.fill(out_of_bounds_vec(0));
            return;
          }

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          Eigen::Matrix<value_type, 1, 8> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_vec[i] = ImageType::value ();
                ++i;
              }
            }
          }

          Eigen::Matrix<value_type, 1, 4> grad_and_value (coeff_vec * weights_matrix);

          gradient = grad_and_value.head(3);
          value = grad_and_value[3];
        }

        void value_and_gradient_wrt_scanner (value_type& value, Eigen::Matrix<coef_type, 1, 3>& gradient) {
          if (Base<ImageType>::out_of_bounds){
            value = out_of_bounds_vec(0);
            gradient.fill(out_of_bounds_vec(0));
            return;
          }
          value_and_gradient(value, gradient);
          gradient = (gradient.template cast<default_type>() * wrt_scanner_transform).eval();
        }

        // Collectively interpolates gradients and values along axis 3
        void value_and_gradient_row (Eigen::Matrix<value_type, Eigen::Dynamic, 1>& value, Eigen::Matrix<value_type, Eigen::Dynamic, 3>& gradient)  {
          if (Base<ImageType>::out_of_bounds) {
            value = out_of_bounds_vec;
            gradient = out_of_bounds_matrix;
            return;
          }

          assert (ImageType::ndim() == 4);

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 8> coeff_matrix (ImageType::size(3), 8);

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_matrix.col (i++) = ImageType::row (3);
              }
            }
          }

          Eigen::Matrix<value_type, Eigen::Dynamic, 4> grad_and_value (coeff_matrix * weights_matrix);
          gradient = grad_and_value.block(0, 0, ImageType::size(3), 3);
          value = grad_and_value.col(3);
        }

        void value_and_gradient_row_wrt_scanner (Eigen::Matrix<value_type, Eigen::Dynamic, 1>& value, Eigen::Matrix<value_type, Eigen::Dynamic, 3>& gradient)  {
          if (Base<ImageType>::out_of_bounds) {
            value = out_of_bounds_vec;
            gradient = out_of_bounds_matrix;
            return;
          }
          value_and_gradient_row(value, gradient);
          gradient = (gradient.template cast<default_type>() * wrt_scanner_transform).eval();
        }

      protected:
        Eigen::Matrix<default_type, 3, 3> wrt_scanner_transform;
        Eigen::Matrix<value_type, 8, 4> weights_matrix;
        Eigen::Matrix<coef_type, Eigen::Dynamic, 1> out_of_bounds_vec;
        Eigen::Matrix<coef_type, Eigen::Dynamic, 3> out_of_bounds_matrix;

    };

    // Template alias for default Linear interpolator
    // This allows an interface that's consistent with other interpolators that all have one template argument
    template <typename ImageType>
    using Linear = LinearInterp<ImageType, LinearInterpProcessingType::Value>;

    template <class ImageType, typename... Args>
      inline Linear<ImageType> make_linear (const ImageType& parent, Args&&... args) {
        return Linear<ImageType> (parent, std::forward<Args> (args)...);
      }

    //! @}

  }
}

#endif
