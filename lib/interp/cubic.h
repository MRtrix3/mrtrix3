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

#ifndef __interp_cubic_h__
#define __interp_cubic_h__

#include "types.h"
#include "interp/base.h"
#include "math/cubic_spline.h"
#include "math/least_squares.h"

namespace MR
{
  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! This class provides access to the voxel intensities of an image using cubic spline interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes.
     * The (integer) position along the remaining axes should be set using the
     * template DataSet class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner().
     * For example:
     * \code
     * auto input = Image<float>::create (argument[0]);
     *
     * // create an Interp::Cubic object using input as the parent data set:
     * Interp::Cubic<decltype(input)> interp (input);
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

    // To avoid unnecessary computation, we want to partially specialize our template based
    // on processing type (value/gradient or both), however each specialization has common core logic
    // which we store in SplineInterpBase

    template <class ImageType, class SplineType, Math::SplineProcessingType PType>
    class SplineInterpBase : public Base<ImageType>
    { MEMALIGN(SplineInterpBase<ImageType,SplineType,PType>)
      public:
        using typename Base<ImageType>::value_type;

        SplineInterpBase (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
            Base<ImageType> (parent, value_when_out_of_bounds),
            H { SplineType(PType), SplineType(PType), SplineType(PType) } { }

      protected:
        SplineType H[3];
        Eigen::Vector3 P;

        ssize_t clamp (ssize_t x, ssize_t dim) const {
          if (x < 0) return 0;
          if (x >= dim) return (dim-1);
          return x;
        }
    };


    template <class ImageType, class SplineType, Math::SplineProcessingType PType>
    class SplineInterp : public SplineInterpBase <ImageType, SplineType, PType>
    { MEMALIGN(SplineInterp<ImageType,SplineType,PType>)
      private:
        SplineInterp ();
    };


    // Specialization of SplineInterp when we're only after interpolated values

    template <class ImageType, class SplineType>
    class SplineInterp<ImageType, SplineType, Math::SplineProcessingType::Value>:
        public SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Value>
    { MEMALIGN(SplineInterp<ImageType,SplineType,Math::SplineProcessingType::Value>)
      public:
        using SplineBase = SplineInterpBase<ImageType, SplineType, Math::SplineProcessingType::Value>;

        typedef typename SplineBase::value_type value_type;
        using SplineBase::P;
        using SplineBase::H;
        using SplineBase::clamp;

        SplineInterp (const ImageType& parent, value_type value_when_out_of_bounds = SplineBase::default_out_of_bounds_value()) :
            SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Value> (parent, value_when_out_of_bounds)
        { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos);
          if (Base<ImageType>::out_of_bounds)
            return false;
          P = pos;
          for(size_t i = 0; i < 3; ++i)
            H[i].set (f[i]);

          // Precompute weights
          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            for (ssize_t y = 0; y < 4; ++y) {
              value_type partial_weight = H[1].weights[y] * H[2].weights[z];
              for (ssize_t x = 0; x < 4; ++x)
                weights_vec[i++] = H[0].weights[x] * partial_weight;
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

        //! Read an interpolated value from the current position
        /*! See file interp/base.h for details. */
        value_type value () {
          if (Base<ImageType>::out_of_bounds)
            return Base<ImageType>::out_of_bounds_value;

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, 64, 1> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_vec[i++] = ImageType::value ();
              }
            }
          }

          return coeff_vec.dot (weights_vec);
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

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 64> coeff_matrix ( ImageType::size(3), 64 );

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_matrix.col (i++) = ImageType::row (axis);
              }
            }
          }

          return coeff_matrix * weights_vec;
        }

      protected:
        Eigen::Matrix<value_type, 64, 1> weights_vec;
    };


    // Specialization of SplineInterp when we're only after interpolated gradients

    template <class ImageType, class SplineType>
    class SplineInterp<ImageType, SplineType, Math::SplineProcessingType::Derivative>:
        public SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Derivative>
    { MEMALIGN(SplineInterp<ImageType,SplineType,Math::SplineProcessingType::Derivative>)
      public:
        using SplineBase = SplineInterpBase<ImageType, SplineType, Math::SplineProcessingType::Derivative>;

        typedef typename SplineBase::value_type value_type;
        using SplineBase::P;
        using SplineBase::H;
        using SplineBase::clamp;

        SplineInterp (const ImageType& parent, value_type value_when_out_of_bounds = SplineBase::default_out_of_bounds_value()) :
            SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Derivative> (parent, value_when_out_of_bounds),
            out_of_bounds_vec (value_when_out_of_bounds, value_when_out_of_bounds, value_when_out_of_bounds),
            wrt_scanner_transform (Transform::scanner2image.linear() * Transform::voxelsize.inverse())
            {
              if (ImageType::ndim() == 4) {
                out_of_bounds_matrix.resize(ImageType::size(3), 3);
              } else {
                out_of_bounds_matrix.resize(1, 3);
              }
              out_of_bounds_matrix.fill(value_when_out_of_bounds);
            }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos);
          if (Base<ImageType>::out_of_bounds)
            return false;
          P = pos;
          for(size_t i =0; i <3; ++i)
            H[i].set (f[i]);

          // Precompute weights
          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            for (ssize_t y = 0; y < 4; ++y) {
              value_type partial_weight = H[1].weights[y] * H[2].weights[z];
              value_type partial_weight_dy = H[1].deriv_weights[y] * H[2].weights[z];
              value_type partial_weight_dz = H[1].weights[y] * H[2].deriv_weights[z];

              for (ssize_t x = 0; x < 4; ++x) {
                weights_matrix(i,0) = H[0].deriv_weights[x] * partial_weight;
                weights_matrix(i,1) = H[0].weights[x] * partial_weight_dy;
                weights_matrix(i,2) = H[0].weights[x] * partial_weight_dz;
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
        Eigen::Matrix<value_type, 1, 3> gradient () {
          if (Base<ImageType>::out_of_bounds)
            return out_of_bounds_vec;

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, 1, 64> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
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
        Eigen::Matrix<value_type, Eigen::Dynamic, 3> gradient_row() {
          if (Base<ImageType>::out_of_bounds) {
            return out_of_bounds_matrix;
          }

          assert (ImageType::ndim() == 4);

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 64> coeff_matrix (ImageType::size(3), 64);

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
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
        const Eigen::Matrix<value_type, 1, 3> out_of_bounds_vec;
        Eigen::Matrix<value_type, Eigen::Dynamic, 3> out_of_bounds_matrix;
        Eigen::Matrix<value_type, 64, 3> weights_matrix;
        const Eigen::Matrix<default_type, 3, 3> wrt_scanner_transform;

      private:
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row() { }
        value_type value () { }
    };


    // Specialization of SplineInterp when we're after both interpolated gradients and values
    template <class ImageType, class SplineType>
    class SplineInterp<ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative>:
        public SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative>
    { MEMALIGN(SplineInterp<ImageType,SplineType,Math::SplineProcessingType::ValueAndDerivative>)
      public:
        using SplineBase = SplineInterpBase<ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative>;

        typedef typename SplineBase::value_type value_type;
        using SplineBase::P;
        using SplineBase::H;
        using SplineBase::clamp;

        SplineInterp (const ImageType& parent, value_type value_when_out_of_bounds = SplineBase::default_out_of_bounds_value()) :
            SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative> (parent, value_when_out_of_bounds),
            wrt_scanner_transform (Transform::scanner2image.linear() * Transform::voxelsize.inverse())
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

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos);
          if (Base<ImageType>::out_of_bounds)
            return false;
          P = pos;
          for (size_t i = 0; i < 3; ++i)
            H[i].set (f[i]);

          // Precompute weights
          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            for (ssize_t y = 0; y < 4; ++y) {
              value_type partial_weight = H[1].weights[y] * H[2].weights[z];
              value_type partial_weight_dy = H[1].deriv_weights[y] * H[2].weights[z];
              value_type partial_weight_dz = H[1].weights[y] * H[2].deriv_weights[z];

              for (ssize_t x = 0; x < 4; ++x) {
                // Gradient
                weights_matrix(i,0) = H[0].deriv_weights[x] * partial_weight;
                weights_matrix(i,1) = H[0].weights[x] * partial_weight_dy;
                weights_matrix(i,2) = H[0].weights[x] * partial_weight_dz;
                // Value
                weights_matrix(i,3) = H[0].weights[x] * partial_weight;
                ++i;
              }
            }
          }

          return true;
        }

        void value_and_gradient (value_type& value, Eigen::Matrix<value_type, 1, 3>& gradient) {
          if (Base<ImageType>::out_of_bounds){
            value = out_of_bounds_vec(0);
            gradient = out_of_bounds_matrix.row(0);
            return;
          }

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, 1, 64> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_vec[i++] = ImageType::value ();
              }
            }
          }
          Eigen::Matrix<value_type, 1, 4> grad_and_value (coeff_vec * weights_matrix);

          gradient = grad_and_value.head(3);
          value = grad_and_value[3];
        }

        void value_and_gradient_wrt_scanner (value_type& value, Eigen::Matrix<value_type, 1, 3>& gradient) {
          value_and_gradient(value, gradient);
          if (Base<ImageType>::out_of_bounds)
            return;
          gradient = (gradient.template cast<default_type>() * wrt_scanner_transform).eval();
        }

        // Simultaneously get both the image value and gradient in 4D
        void value_and_gradient_row (Eigen::Matrix<value_type, Eigen::Dynamic, 1>& value, Eigen::Matrix<value_type, Eigen::Dynamic, 3>& gradient) {
          if (Base<ImageType>::out_of_bounds){
            value = out_of_bounds_vec;
            gradient = out_of_bounds_matrix;
            return;
          }

          assert (ImageType::ndim() == 4);

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 64> coeff_matrix (ImageType::size(3), 64);

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                coeff_matrix.col (i++) = ImageType::row (3);
              }
            }
          }
          Eigen::Matrix<value_type, Eigen::Dynamic, 4> grad_and_value (coeff_matrix * weights_matrix);
          gradient = grad_and_value.block(0,0,ImageType::size(3),3);
          value = grad_and_value.col(3);
        }

        void value_and_gradient_row_wrt_scanner (Eigen::Matrix<value_type, Eigen::Dynamic, 1>& value, Eigen::Matrix<value_type, Eigen::Dynamic, 3>& gradient) {
          value_and_gradient_row(value, gradient);
          if (Base<ImageType>::out_of_bounds){
            return;
          }
          gradient = (gradient.template cast<default_type>() * wrt_scanner_transform).eval();
        }

      protected:
        Eigen::Matrix<value_type, 64, 4> weights_matrix;
        const Eigen::Matrix<default_type, 3, 3> wrt_scanner_transform;
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_vec;
        Eigen::Matrix<value_type, Eigen::Dynamic, 3> out_of_bounds_matrix;
    };


    // Template alias for default Cubic interpolator
    // This allows an interface that's consistent with other interpolators that all have one template argument
    template <typename ImageType>
    using Cubic = SplineInterp<ImageType, Math::HermiteSpline<typename ImageType::value_type>, Math::SplineProcessingType::Value>;

    template <typename ImageType>
    using CubicUniform = SplineInterp<ImageType, Math::UniformBSpline<typename ImageType::value_type>, Math::SplineProcessingType::Value>;


    template <class ImageType, typename... Args>
      inline Cubic<ImageType> make_cubic (const ImageType& parent, Args&&... args) {
        return Cubic<ImageType> (parent, std::forward<Args> (args)...);
      }



    //! @}

  }
}

#endif


