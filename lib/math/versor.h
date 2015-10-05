/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 05/10/15.

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

#ifndef __math_versor_h__
#define __math_versor_h__

#include "debug.h"
#include "math/math.h"

namespace MR {
  namespace Math {



    template <typename ValueType>
    class Versor : public Eigen::Quaternion<ValueType>
    {

        typedef ValueType value_type;

      public:
        Versor () : Eigen::Quaternion<ValueType> (NAN, NAN, NAN, NAN) { }
        Versor (const value_type& w, const value_type& x, const value_type& y, const value_type& z) :
            Eigen::Quaternion<ValueType> (w, x, y, z) { Eigen::Quaternion<ValueType>::normalize(); }
        Versor (const value_type* data) :
            Eigen::Quaternion<ValueType> (data) { Eigen::Quaternion<ValueType>::normalize(); }
        template <class Derived>
        Versor (const Eigen::QuaternionBase<Derived>& other) :
            Eigen::Quaternion<ValueType> (other) { Eigen::Quaternion<ValueType>::normalize(); }
        Versor (const Eigen::AngleAxis<ValueType>& aa):
            Eigen::Quaternion<ValueType> (aa) { Eigen::Quaternion<ValueType>::normalize(); }
        template <typename Derived>
        Versor (const Eigen::MatrixBase<Derived>& other) :
            Eigen::Quaternion<ValueType> (other) { Eigen::Quaternion<ValueType>::normalize(); }
        template<typename OtherScalar, int OtherOptions>
        Versor (const Eigen::Quaternion<OtherScalar, OtherOptions>& other) :
            Eigen::Quaternion<ValueType> (other) { Eigen::Quaternion<ValueType>::normalize(); }

        Versor (const value_type angle, const Eigen::Matrix<value_type, 3, 1>& axis) {
            Eigen::Quaternion<ValueType>::w() = std::cos (angle/2.0);
            const value_type norm = std::sin (angle/2.0) / axis.norm();
            Eigen::Quaternion<ValueType>::x() = axis[0] * norm;
            Eigen::Quaternion<ValueType>::y() = axis[1] * norm;
            Eigen::Quaternion<ValueType>::z() = axis[2] * norm;
          }

        bool valid() const { return std::isfinite (w()); }
        bool operator! () const { return !valid(); }

        // Don't give reference access to individual elements; need to maintain unit norm
        value_type  w() const { return Eigen::Quaternion<ValueType>::w(); }
        value_type& w() = delete;
        value_type  x() const { return Eigen::Quaternion<ValueType>::x(); }
        value_type& x() = delete;
        value_type  y() const { return Eigen::Quaternion<ValueType>::y(); }
        value_type& y() = delete;
        value_type  z() const { return Eigen::Quaternion<ValueType>::z(); }
        value_type& z() = delete;

        // Shouldn't be any need to perform explicit normalization
        void normalize() = delete;
        Versor normalized() = delete;


        static Versor unit() { return Versor (value_type(1.0), value_type(0.0), value_type(0.0), value_type(0.0)); }

    };

    typedef Versor<float>  Versorf;
    typedef Versor<double> Versord;

    template <typename value_type>
    inline std::ostream& operator<< (std::ostream& stream, const Versor<value_type>& v)
    {
      stream << "[ " << v.w() << " " << v.x() << "i " << v.y() << "j " << v.z() << "k ]";
      return stream;
    }



  }
}

#endif
