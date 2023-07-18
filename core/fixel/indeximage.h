/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


#ifndef __fixel_index_h__
#define __fixel_index_h__

#include "image.h"
#include "image_helpers.h"
#include "fixel/fixel.h"
#include "fixel/helpers.h"


namespace MR
{
  namespace Fixel
  {



    class IndexImage : public Image<Fixel::index_type>
    {

      public:
        using BaseType = Image<Fixel::index_type>;
        using value_type = Fixel::index_type;
        using BaseType::valid;
        using BaseType::operator!;
        using BaseType::keyval;
        using BaseType::name;
        using BaseType::transform;
        size_t ndim() const { return 3; }
        ssize_t size (size_t axis) const { assert (axis < 3); return BaseType::size (axis); }
        default_type spacing (size_t axis) const { assert (axis < 3); return BaseType::spacing (axis); }
        // TODO Construct custom stride information that omits the fourth axis
        using BaseType::reset;


        value_type count() { BaseType::index(3) = 0; return BaseType::get_value(); }
        value_type offset() { BaseType::index(3) = 1; return BaseType::get_value(); }


        class Range
        {
          public:
            Range (IndexImage& index_image)
            {
              index_image.index(3) = 0;
              count = index_image.get_value();
              index_image.index(3) = 1;
              offset = index_image.get_value();
            }
            class iterator : public std::iterator<std::forward_iterator_tag, value_type, std::ptrdiff_t, const value_type*, value_type>
            {
              public:
                explicit iterator (const value_type value) : index (value) { }
                iterator& operator++() { ++index; return *this; }
                iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }
                bool operator== (const iterator& other) const { return index == other.index; }
                bool operator!= (const iterator& other) const { return !(*this == other); }
                value_type operator*() { return index; }
              private:
                value_type index;
            };
            iterator begin() const { return iterator (offset); }
            iterator end() const { return iterator (offset + count); }
          private:
            value_type count, offset;
        };
        Range value() { return Range(*this); }



        IndexImage (const std::string& path) :
            BaseType (BaseType::open (path)),
            fixel_count (get_number_of_fixels()) { }
        IndexImage (Header header) :
            BaseType (header.get_image<value_type>()),
            fixel_count (get_number_of_fixels()) { }

        Fixel::index_type nfixels() const { return fixel_count; }

      private:
        value_type fixel_count;

        // Helper function Fixel::get_number_of_fixels() cannot be used here:
        //   this class reports as being 3D, and so will fail Fixel::check_index_image()
        value_type get_number_of_fixels()
        {
          const auto nfixels_it = keyval().find(MR::Fixel::n_fixels_key);
          if (nfixels_it != keyval().end())
            return to<value_type>(nfixels_it->second);
          value_type result = 0;
          for (auto l = MR::Loop (*this) (*this); l; ++l)
            result = std::max (result, offset() + count());
          return result;
        }

    };



  }
}

#endif
