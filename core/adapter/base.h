/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __adapter_base_h__
#define __adapter_base_h__

#include <array>

#include "image_helpers.h"
#include "transform.h"
#include "types.h"
#include "math/math.h"

namespace MR
{
  class Header;

  namespace Adapter
  {

    template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
      inline AdapterType<ImageType> make (const ImageType& parent, Args&&... args) {
        return AdapterType<ImageType> (parent, std::forward<Args> (args)...);
      }

    template <class AdapterType, class ImageType>
    class Base : public ImageBase<AdapterType, typename ImageType::value_type>
    { MEMALIGN (Base<AdapterType,ImageType>)
      public:

        using value_type = typename ImageType::value_type;

        Base (const ImageType& parent) : parent_ (parent) { }


        template <class U>
          const Base& operator= (const U& V) { return parent_ = V; }

        FORCE_INLINE ImageType& parent () { return parent_; }
        FORCE_INLINE bool valid () const { return parent_.valid(); }
        FORCE_INLINE bool operator! () const { return !valid(); }
        FORCE_INLINE const ImageType& parent () const { return parent_; }
        FORCE_INLINE const std::string& name () const { return parent_.name(); }
        FORCE_INLINE size_t ndim () const { return parent_.ndim(); }
        FORCE_INLINE ssize_t size (size_t axis) const { return parent_.size (axis); }
        FORCE_INLINE default_type spacing (size_t axis) const { return parent_.spacing (axis); }
        FORCE_INLINE ssize_t stride (size_t axis) const { return parent_.stride (axis); }
        FORCE_INLINE const transform_type& transform () const { return parent_.transform(); }
        FORCE_INLINE const KeyValues& keyval () const { return parent_.keyval(); }

        FORCE_INLINE ssize_t get_index (size_t axis) const { return parent_.index (axis); }
        FORCE_INLINE void move_index (size_t axis, ssize_t increment) { parent_.index (axis) += increment; }

        FORCE_INLINE value_type get_value () const { return parent_.value(); }
        FORCE_INLINE void set_value (value_type val) { parent_.value() = val; }

        FORCE_INLINE void reset () { parent_.reset(); }

        friend std::ostream& operator<< (std::ostream& stream, const Base& V) {
          stream << "image adapter \"" << V.name() << "\", datatype " << MR::DataType::from<value_type>().specifier() << ", position [ ";
          for (size_t n = 0; n < V.ndim(); ++n)
            stream << V[n] << " ";
          stream << "], value = " << V.value();
          return stream;
        }

      protected:
        ImageType parent_;
    };



    template <class AdapterType, class ImageType>
    class Base1D : public Base<AdapterType, ImageType>
    { MEMALIGN (Base1D<AdapterType,ImageType>)
      public:

        using value_type = typename ImageType::value_type;
        using base_type = Base<AdapterType, ImageType>;

        Base1D (const ImageType& parent, const size_t axis = 0) :
            base_type (parent),
            axis (axis) { }

        void set_axis (const size_t a) { assert (a < 3); axis = a; }
        size_t get_axis() const { return axis; }

      protected:
        size_t axis;
    };



    template <class AdapterType, class ImageType>
    class BaseFiniteDiff1D : public Base1D<AdapterType, ImageType>
    { MEMALIGN (BaseFiniteDiff1D<AdapterType,ImageType>)
      public:

        using value_type = typename ImageType::value_type;
        using base_type = Base1D<AdapterType, ImageType>;

        BaseFiniteDiff1D (const ImageType& parent, const size_t axis = 0, const bool wrt_spacing = false) :
            base_type (parent, axis),
            axis_weights (3, value_type(1))
        {
          if (wrt_spacing) {
            for (size_t i = 0; i != 3; ++i)
              axis_weights[i] /= parent.spacing(i);
          }
        }

      protected:
        vector<default_type> axis_weights;
    };



    template <class AdapterType, class ImageType>
    class BaseFiniteDiff3D : public BaseFiniteDiff1D<AdapterType, ImageType>
    { MEMALIGN(BaseFiniteDiff3D<AdapterType,ImageType>)

      public:
        using base_type = BaseFiniteDiff1D<AdapterType, ImageType>;
        using value_type = typename ImageType::value_type;
        using return_type = Eigen::Matrix<value_type,3,1>;

        BaseFiniteDiff3D (const ImageType& parent,
                          bool wrt_scanner = false) :
            base_type (parent, 0, wrt_scanner), // If w.r.t. scanner, scale values by axis spacings
            wrt_scanner (wrt_scanner),
            transform (parent) { }

        // TODO Remove Vector2Axis; the adapter should be constructing a new template
        //   with an extra dimension, and index() of that axis should go to set_axis of the
        //   underlying finite difference calculator
/*        ssize_t get_index (size_t axis) const
        {
          if (axis < 3)
            return parent_.index (axis);
          if (axis == 3)
            return parent_.get_axis();
          return parent_.index (axis-1);
        }
        FORCE_INLINE void move_index (size_t axis, ssize_t increment) { parent_.index (axis) += increment; }
*/
        return_type value ()
        {
          return_type result;
          for (size_t i = 0; i < 3; ++i) {
            base_type::set_axis(i);
            result[i] = base_type::value();
          }
          if (wrt_scanner)
            result = transform.image2scanner.linear().template cast<typename ImageType::value_type>() * result;

          return result;
        }

      protected:
        const bool wrt_scanner;
        Transform transform;
    };



  }
}

#endif





