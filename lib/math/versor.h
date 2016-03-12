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
