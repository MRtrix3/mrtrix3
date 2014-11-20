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
          BufferScratch (const Template& info) :
            ConstInfo (info),
            data_ (new value_type [Image::voxel_count (*this)]) {
              Info::datatype_ = DataType::from<value_type>();
              zero();
            }

        template <class Template>
          BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            data_ (new value_type [Image::voxel_count (*this)]) {
              zero();
              datatype_ = DataType::from<value_type>();
              name_ = label;
            }

        typedef ValueType value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        void zero () {
          memset (data_, 0, Image::voxel_count (*this) * sizeof(ValueType));
        }

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) {
          data_[index] = val;
        }

        value_type* address (size_t index) const {
          return &data_[index];
        }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.data_);
          return stream;
        }

      protected:
        Ptr<value_type,true> data_;

        template <class InfoType>
          BufferScratch& operator= (const InfoType&) { assert (0); return *this; }

        BufferScratch (const BufferScratch& that) : ConstInfo (that) { assert (0); }
    };





    template <>
    class BufferScratch<bool> : public ConstInfo
    {
      public:
        template <class Template>
        BufferScratch (const Template& info) :
            ConstInfo (info),
            bytes_ ((Image::voxel_count (*this)+7)/8),
            data_ (new uint8_t [bytes_])
        {
          Info::datatype_ = DataType::Bit;
          zero();
        }

        template <class Template>
        BufferScratch (const Template& info, const std::string& label) :
            ConstInfo (info),
            bytes_ ((Image::voxel_count (*this)+7)/8),
            data_ (new uint8_t [bytes_])
        {
          Info::datatype_ = DataType::Bit;
          zero();
          name_ = label;
        }

        typedef bool value_type;
        typedef Image::Voxel<BufferScratch> voxel_type;

        voxel_type voxel() { return voxel_type (*this); }

        void zero () {
          memset (data_, 0x00, bytes_);
        }

        bool get_value (size_t index) const {
          return get<bool> (data_, index);
        }

        void set_value (size_t index, bool val) {
          put<bool> (val, data_, index);
        }

        friend std::ostream& operator<< (std::ostream& stream, const BufferScratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in boolean format (" << V.bytes_ << " bytes), stored at address " + str ((void*) V.data_);
          return stream;
        }

      protected:
        size_t bytes_;
        Ptr<uint8_t,true> data_;

        template <class InfoType>
          BufferScratch& operator= (const InfoType&) { assert (0); return *this; }

        BufferScratch (const BufferScratch& that) : ConstInfo (that), bytes_ (that.bytes_) { assert (0); }

    };



  }
}

#endif




