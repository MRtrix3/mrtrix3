/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_buffer_scratch_h__
#define __image_buffer_scratch_h__

#include "bitset.h"
#include "debug.h"
#include "image/info.h"
#include "image/stride.h"
#include "image/voxel.h"
#include "image/adapter/info.h"

namespace MR
{
  namespace Image
  {


    template <typename ValueType>
      class BufferScratch : public ConstInfo
    {
      public:
        template <class Template>
          explicit BufferScratch (const Template& info) :
            ConstInfo (info),
            data_ (Image::voxel_count (*this)) {
              datatype_ = DataType::from<value_type>();
              zero();
            }

        template <class Template>
          explicit BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            data_ (Image::voxel_count (*this)) {
              datatype_ = DataType::from<value_type>();
              zero();
              name_ = label;
            }

        typedef ValueType value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        void zero () {
          memset (address(0), 0, data_.size()*sizeof(value_type));
        }

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) {
          data_[index] = val;
        }

        const value_type* address (size_t index) const {
          return &data_[index];
        }

        value_type* address (size_t index) {
          return &data_[index];
        }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.address(0));
          return stream;
        }

      protected:
        std::vector<value_type> data_;
    };





    template <>
    class BufferScratch<bool> : public ConstInfo
    {
      public:
        template <class Template>
        BufferScratch (const Template& info) :
            ConstInfo (info),
            data_ ((Image::voxel_count (*this)+7)/8, 0)
        {
          Info::datatype_ = DataType::Bit;
        }

        template <class Template>
        BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            data_ ((Image::voxel_count (*this)+7)/8, 0)
        {
          Info::datatype_ = DataType::Bit;
          name_ = label;
        }

        typedef bool value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        void zero () {
          std::fill (data_.begin(), data_.end(), 0);
        }

        bool get_value (size_t index) const {
          return get<bool> (address(), index);
        }

        void set_value (size_t index, bool val) {
          put<bool> (val, address(), index);
        }

        void* address () { return &data_[0]; }
        const void* address () const { return &data_[0]; }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in boolean format (" << V.data_.size() << " bytes), stored at address " + str ((void*) V.address());
          return stream;
        }

      protected:
        std::vector<uint8_t> data_;
    };



  }
}

#endif




