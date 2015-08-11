/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#ifndef __interp_cubic_h__
#define __interp_cubic_h__

#include "transform.h"
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
     * auto input = Image<float>::create (Argument[0]);
     *
     * // create an Interp::Cubic object using input as the parent data set:
     * Interp::Cubic<decltype(input) > interp (input);
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
    class SplineInterpBase : public ImageType, public Transform
    {
      public:
        typedef typename ImageType::value_type value_type;

        using ImageType::size;
        using ImageType::index;
        using ImageType::ndim;
        using Transform::set_to_nearest;
        using Transform::voxelsize;
        using Transform::scanner2voxel;
        using Transform::operator!;
        using Transform::out_of_bounds;
        using Transform::bounds;

        //! construct a Nearest object to obtain interpolated values using the
        // parent DataSet class
        SplineInterpBase (const ImageType& parent, value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>()) :
          ImageType (parent),
          Transform (parent),
          out_of_bounds_value (value_when_out_of_bounds),
          H { SplineType(PType), SplineType(PType), SplineType(PType) }
        { }

        const value_type out_of_bounds_value;

      protected:
        SplineType H[3];
        Eigen::Vector3d P;

        ssize_t check (ssize_t x, ssize_t dim) const {
          if (x < 0) return 0;
          if (x > dim) return dim;
          return x;
        }
    };


    template <class ImageType, class SplineType, Math::SplineProcessingType PType>
    class SplineInterp : public SplineInterpBase <ImageType, SplineType, PType>
    {
      public:
        typedef typename ImageType::value_type value_type;

      private:
        SplineInterp ();
    };


    // Specialization of SplineInterp when we're only after interpolated values

    template <class ImageType, class SplineType>
    class SplineInterp<ImageType, SplineType, Math::SplineProcessingType::Value>:
    public SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Value>
    {
      public:
        typedef typename ImageType::value_type value_type;
        using SplineBase = SplineInterpBase<ImageType, SplineType, Math::SplineProcessingType::Value>;

        using SplineBase::size;
        using SplineBase::index;
        using SplineBase::set_to_nearest;
        using SplineBase::voxelsize;
        using SplineBase::scanner2voxel;
        using SplineBase::out_of_bounds;
        using SplineBase::out_of_bounds_value;
        using SplineBase::P;
        using SplineBase::H;
        using SplineBase::check;

        SplineInterp (const ImageType& parent, value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>()) :
          SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Value> (parent, value_when_out_of_bounds)
        { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3d f = set_to_nearest (pos);
          if (out_of_bounds)
            return true;
          P = pos;
          for(size_t i =0; i <3; ++i)
            H[i].set (f[i]);

          // Precompute weights
          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            for (ssize_t y = 0; y < 4; ++y) {
              value_type partial_weight = H[1].weights[y] * H[2].weights[z];
              for (ssize_t x = 0; x < 4; ++x) {
                weights_vec[i] = H[0].weights[x] * partial_weight;
                i += 1;
              }
            }
          }

          return false;
        }

        //! Set the current position to <b>image space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * coordinate relative to the axes of the dataset, in units of
         * millimeters. The origin is taken to be the centre of the voxel at [
         * 0 0 0 ]. */
        template <class VectorType>
        bool image (const VectorType& pos) {
          return voxel (voxelsize.inverse() * pos.template cast<double>());
        }
        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * scanner space coordinate, in units of millimeters. */
        template <class VectorType>
        bool scanner (const VectorType& pos) {
          return voxel (scanner2voxel * pos.template cast<double>());
        }

        value_type value () {
          if (out_of_bounds)
            return out_of_bounds_value;

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, 64, 1> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            index(2) = check (c[2] + z, size (2)-1);
            for (ssize_t y = 0; y < 4; ++y) {
              index(1) = check (c[1] + y, size (1)-1);
              for (ssize_t x = 0; x < 4; ++x) {
                index(0) = check (c[0] + x, size (0)-1);
                coeff_vec[i] = ImageType::value ();
                i += 1;
              }
            }
          }

          return coeff_vec.dot (weights_vec);
        }

        // Collectively interpolates values along axis >= 3
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis) {
          if (out_of_bounds) {
            Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_row (ImageType::size(axis));
            out_of_bounds_row.setOnes();
            out_of_bounds_row *= out_of_bounds_value;
            return out_of_bounds_row;
          }

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 64> coeff_matrix ( size(3), 64 );

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            index(2) = check (c[2] + z, size (2)-1);
            for (ssize_t y = 0; y < 4; ++y) {
              index(1) = check (c[1] + y, size (1)-1);
              for (ssize_t x = 0; x < 4; ++x) {
                index(0) = check (c[0] + x, size (0)-1);
                coeff_matrix.col (i) = ImageType::row (axis);
                i += 1;
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
    {
      public:
        typedef typename ImageType::value_type value_type;
        using SplineBase = SplineInterpBase<ImageType, SplineType, Math::SplineProcessingType::Derivative>;

        using SplineBase::size;
        using SplineBase::index;
        using SplineBase::ndim;
        using SplineBase::set_to_nearest;
        using SplineBase::voxelsize;
        using SplineBase::scanner2voxel;
        using SplineBase::out_of_bounds;
        using SplineBase::out_of_bounds_value;
        using SplineBase::P;
        using SplineBase::H;
        using SplineBase::check;

        SplineInterp (const ImageType& parent, value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>()) :
          SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::Derivative> (parent, value_when_out_of_bounds),
          out_of_bounds_vec (value_when_out_of_bounds, value_when_out_of_bounds, value_when_out_of_bounds)
        { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3d f = set_to_nearest (pos);
          if (out_of_bounds)
            return true;
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
                i += 1;
              }
            }
          }


          return false;
        }

        //! Set the current position to <b>image space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * coordinate relative to the axes of the dataset, in units of
         * millimeters. The origin is taken to be the centre of the voxel at [
         * 0 0 0 ]. */
        template <class VectorType>
        bool image (const VectorType& pos) {
          return voxel (voxelsize.inverse() * pos.template cast<double>());
        }
        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * scanner space coordinate, in units of millimeters. */
        template <class VectorType>
        bool scanner (const VectorType& pos) {
          return voxel (scanner2voxel * pos.template cast<double>());
        }

        //! Returns the image gradient at the current position
        Eigen::Matrix<value_type, 1, 3> gradient () {
          if (out_of_bounds)
            return out_of_bounds_vec;



          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, 1, 64> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            index(2) = check (c[2] + z, size (2)-1);
            for (ssize_t y = 0; y < 4; ++y) {
              index(1) = check (c[1] + y, size (1)-1);
              for (ssize_t x = 0; x < 4; ++x) {
                index(0) = check (c[0] + x, size (0)-1);
                coeff_vec[i] = ImageType::value ();
                i += 1;
              }
            }
          }

          return coeff_vec * weights_matrix;
        }


//        //! Returns the image gradient at the current position, defined with respect to the scanner coordinate frame of reference.
//        Eigen::Matrix<value_type, 1, 3> gradient_wrt_scanner () {
//          return Transform::voxel2scanner.linear() * gradient ();
//        }

        // Collectively interpolates gradients along axis 3 // TODO: might need to input axis argument for interpolating 5D images
        Eigen::Matrix<value_type, Eigen::Dynamic, 3> gradient_row () {
          if (out_of_bounds)
            return Eigen::Matrix<value_type, Eigen::Dynamic, 3>(); //TODO

          assert (ndim() == 4);

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, Eigen::Dynamic, 64> coeff_matrix (size(3), 64);

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            index(2) = check (c[2] + z, size (2)-1);
            for (ssize_t y = 0; y < 4; ++y) {
              index(1) = check (c[1] + y, size (1)-1);
              for (ssize_t x = 0; x < 4; ++x) {
                index(0) = check (c[0] + x, size (0)-1);
                coeff_matrix.col (i) = ImageType::row (3);
                i += 1;
              }
            }
          }

          return coeff_matrix * weights_matrix;
        }


        //! Collectively interpolates gradients along axis 3, defined with respect to the scanner coordinate frame of reference.
        Eigen::Matrix<default_type, Eigen::Dynamic, 3> gradient_row_wrt_scanner () {
          Eigen::Matrix<default_type, Eigen::Dynamic, 3> gradients = gradient_row().template cast<default_type>();
          return Transform::image2scanner.linear() * voxelsize.inverse() * gradients;
        }

      protected:
        const Eigen::Matrix<value_type, 1, 3> out_of_bounds_vec;
        Eigen::Matrix<value_type, 64, 3> weights_matrix;
    };


    // Specialization of SplineInterp when we're after both interpolated gradients and values
    template <class ImageType, class SplineType>
    class SplineInterp<ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative>:
    public SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative>
    {
      public:
        typedef typename ImageType::value_type value_type;
        using SplineBase = SplineInterpBase<ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative>;

        using SplineBase::size;
        using SplineBase::index;
        using SplineBase::ndim;
        using SplineBase::set_to_nearest;
        using SplineBase::voxelsize;
        using SplineBase::scanner2voxel;
        using SplineBase::out_of_bounds;
        using SplineBase::out_of_bounds_value;
        using SplineBase::P;
        using SplineBase::H;
        using SplineBase::check;

        SplineInterp (const ImageType& parent, value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>()) :
          SplineInterpBase <ImageType, SplineType, Math::SplineProcessingType::ValueAndDerivative> (parent, value_when_out_of_bounds)
        { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3d f = set_to_nearest (pos);
          if (out_of_bounds)
            return true;
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
                // Gradient
                weights_matrix(i,0) = H[0].deriv_weights[x] * partial_weight;
                weights_matrix(i,1) = H[0].weights[x] * partial_weight_dy;
                weights_matrix(i,2) = H[0].weights[x] * partial_weight_dz;
                // Value
                weights_matrix(i,3) = H[0].weights[x] * partial_weight;

                i += 1;
              }
            }
          }

          return false;
        }


        void value_and_gradient (value_type& value, Eigen::Matrix<value_type, 1, 3>& gradient) {
          if (out_of_bounds)
            return;

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          Eigen::Matrix<value_type, 1, 64> coeff_vec;

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            index(2) = check (c[2] + z, size (2)-1);
            for (ssize_t y = 0; y < 4; ++y) {
              index(1) = check (c[1] + y, size (1)-1);
              for (ssize_t x = 0; x < 4; ++x) {
                index(0) = check (c[0] + x, size (0)-1);
                coeff_vec[i] = ImageType::value ();
                i += 1;
              }
            }
          }

          Eigen::Matrix<value_type, 1, 4> grad_and_value (coeff_vec * weights_matrix);

          gradient = grad_and_value.segment(1,3);
          value = grad_and_value[3];
        }

      protected:
        Eigen::Matrix<value_type, 64, 4> weights_matrix;
    };


    // Template alias for default Cubic interpolator
    // This allows an interface that's consistent with other interpolators that all have one template argument
    template <typename ImageType>
    using Cubic = SplineInterp<ImageType, Math::HermiteSpline<typename ImageType::value_type>, Math::SplineProcessingType::Value>;


    template <class ImageType, typename... Args>
      inline Cubic<ImageType> make_cubic (const ImageType& parent, Args&&... args) {
        return Cubic<ImageType> (parent, std::forward<Args> (args)...);
      }



    //! @}

  }
}

#endif


