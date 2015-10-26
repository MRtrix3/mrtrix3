/*
   Copyright 2012 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 24/02/2012

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

#ifndef __registration_transform_base_h__
#define __registration_transform_base_h__

#include "types.h"
#include <unsupported/Eigen/MatrixFunctions>
#include <Eigen/SVD>
#include <Eigen/Geometry> // Eigen::Translation

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      /** \addtogroup Transforms
      @{ */

      /*! A base linear transformation class
       *
       * This class supports the ability to define the centre of rotation.
       * This should be set prior to commencing registration based on the centre of the target image.
       * The translation also should be initialised as moving image centre minus the target image centre.
       *
       */
      class Base  {
        public:

          typedef default_type ParameterType;
          #ifdef NONSYMREGISTRATION
            Base (size_t number_of_parameters) :
            number_of_parameters(number_of_parameters),
            optimiser_weights (number_of_parameters) {
              matrix.setIdentity();
              translation.setZero();
              centre.setZero();
              offset.setZero();
            }
          #else
            Base (size_t number_of_parameters) :
              number_of_parameters(number_of_parameters),
              optimiser_weights (number_of_parameters) {
                trafo.matrix().setIdentity();
                trafo_half.matrix().setIdentity();
                trafo_half_inverse.matrix().setIdentity();
                centre.setZero();
            }
          #endif

          template <class OutPointType, class InPointType>
          inline void transform (OutPointType& out, const InPointType& in) const {
            out = trafo * in;
          }

          template <class OutPointType, class InPointType>
          inline void transform_half (OutPointType& out, const InPointType& in) const {
            out = trafo_half * in;
          }

          template <class OutPointType, class InPointType>
          inline void transform_half_inverse (OutPointType& out, const InPointType& in) const {
            out = trafo_half_inverse * in;
          }

          void set_transform (transform_type& transform) {
            trafo.matrix() = transform.matrix();
            compute_offset();
            compute_halfspace_transformations();
          }

          transform_type get_transform () const {
            transform_type transform;
            transform.matrix() = trafo.matrix();
            return transform;
          }

          transform_type get_transform_half () const {
            return trafo_half;
          }

          transform_type get_transform_half_inverse () const {
            return trafo_half_inverse;
          }

          void set_matrix (const Eigen::Matrix<default_type, 3, 3>& mat) {
            trafo.linear() = mat;
            compute_offset();
            compute_halfspace_transformations();
          }

          const Eigen::Matrix<default_type, 3, 3> get_matrix () const {
            return trafo.linear();
          }

          void set_translation (const Eigen::Vector3& trans) {
            trafo.translation() = trans;
            compute_offset();
            compute_halfspace_transformations();
          }

          const Eigen::Vector3 get_translation() const {
            return trafo.translation();
          }

          void set_centre (const Eigen::Vector3& centre_in) {
            centre = centre_in;
            compute_offset();
            compute_halfspace_transformations();
          }

          const Eigen::Vector3 get_centre() const {
            return centre;
          }

          size_t size() const {
            return number_of_parameters;
          }

          void set_optimiser_weights (Eigen::Vector3& weights) {
            assert(size() == (size_t)optimiser_weights.size());
              optimiser_weights = weights;
          }

          Eigen::VectorXd get_optimiser_weights () const {
            return optimiser_weights;
          }

          // Eigen::Vector3 get_offset () const {
          //   return offset;
          // }

          void set_offset (const Eigen::Vector3& offset_in) {
            trafo.translation() = offset_in;
            compute_halfspace_transformations();
          }

        #ifndef NONSYMREGISTRATION
          void debug(){
            INFO("debug():");
            Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", "\n", "", "", "", "");
            INFO("trafo:\n"+str(trafo.matrix().format(fmt)));
            INFO("trafo.inverse():\n"+str(trafo.inverse().matrix().format(fmt)));
            INFO("trafo_half:\n"+str(trafo_half.matrix().format(fmt)));
            INFO("trafo_half_inverse:\n"+str(trafo_half_inverse.matrix().format(fmt)));
            INFO("centre: "+str(centre.transpose(),12));
            Eigen::Vector3 in, out, half, half_inverse;
            in << 1.0, 2.0, 3.0;
            transform (out, in);
            transform_half (half, in);
            transform_half_inverse (half_inverse, in);
            VAR(out.transpose());
            VAR(half.transpose());
            VAR(half_inverse.transpose());
            // assert ((out-in).isApprox(half-half_inverse-in));
          }
        #endif


        protected:

            void compute_offset () {
              Eigen::Vector3 offset = trafo.translation() + centre - trafo.linear() * centre;
              // for( size_t i = 0; i < 3; i++ ) {
              //   offset[i] = translation[i] + centre[i];
              //   for( size_t j = 0; j < 3; j++ )
              //     offset[i] -= matrix(i, j) * centre[j];
              // }
              trafo.translation() = offset;
            }


          void compute_halfspace_transformations(){
            #ifndef NONSYMREGISTRATION
              Eigen::Matrix<default_type, 4, 4> tmp;
              tmp.setIdentity();
              tmp.template block<3,4>(0,0) = trafo.matrix();
              assert((tmp.template block<3,3>(0,0).isApprox(trafo.linear())));
              tmp = tmp.sqrt();
              trafo_half.matrix() = tmp.template block<3,4>(0,0);
              trafo_half_inverse.matrix() = trafo_half.inverse().matrix();
              assert(trafo.matrix().isApprox((trafo_half*trafo_half).matrix()));
              assert(trafo.inverse().matrix().isApprox((trafo_half_inverse*trafo_half_inverse).matrix()));
              // debug();
            #endif
            }

          size_t number_of_parameters;
          // TODO matrix, translation and offset are only here for the rigid class. to be removed
            Eigen::Matrix<default_type, 3, 3> matrix;
            Eigen::Vector3 translation;
            Eigen::Vector3 offset;
          transform_type trafo;
          transform_type trafo_half;
          transform_type trafo_half_inverse;
          Eigen::Vector3 centre;
          Eigen::VectorXd optimiser_weights;

      };
      //! @}
    }
  }
}

#endif
