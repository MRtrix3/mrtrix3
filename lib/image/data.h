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

#ifndef __image_data_h__
#define __image_data_h__

#include "debug.h"
#include "get_set.h"
#include "image/header.h"
#include "image/data_common.h"
#include "dataset/stride.h"
#include "dataset/value.h"
#include "dataset/position.h"

namespace MR
{
  namespace Image
  {
   
    //! \cond skip
    
    namespace
    {
      template <typename value_type, typename S> value_type __get (const void* data, size_t i)
      {
        return value_type (MR::get<S> (data, i));
      }
      template <typename value_type, typename S> value_type __getLE (const void* data, size_t i)
      {
        return value_type (MR::getLE<S> (data, i));
      }
      template <typename value_type, typename S> value_type __getBE (const void* data, size_t i)
      {
        return value_type (MR::getBE<S> (data, i));
      }

      template <typename value_type, typename S> void __put (value_type val, void* data, size_t i)
      {
        return MR::put<S> (S (val), data, i);
      }
      template <typename value_type, typename S> void __putLE (value_type val, void* data, size_t i)
      {
        return MR::putLE<S> (S (val), data, i);
      }
      template <typename value_type, typename S> void __putBE (value_type val, void* data, size_t i)
      {
        return MR::putBE<S> (S (val), data, i);
      }

      // specialisation for conversion to bool
      template <> bool __getLE<bool,float> (const void* data, size_t i)
      {
        return Math::round (MR::getLE<float> (data, i));
      }
      template <> bool __getBE<bool,float> (const void* data, size_t i)
      {
        return Math::round (MR::getBE<float> (data, i));
      }
      template <> bool __getLE<bool,double> (const void* data, size_t i)
      {
        return Math::round (MR::getLE<double> (data, i));
      }
      template <> bool __getBE<bool,double> (const void* data, size_t i)
      {
        return Math::round (MR::getBE<double> (data, i));
      }
    }

    // \endcond





    //! The standard DataArray interface 
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities. 
     * \todo Provide specialisations of get/set methods to handle conversions
     * between floating-point and integer types */
    template <typename T> class Data : public DataCommon<T>
    {
      using DataCommon<T>::H;

      public:
        //! construct a Data object to access the data in the Image::Header \p parent
        Data (const Header& parent) :
          DataCommon<T> (parent), 
          handler (*H.get_handler()) {
          assert (H.get_handler());
          handler.open();
          set_get_put_functions ();
        }

        typedef T value_type;
        typedef Image::Voxel<Data> voxel_type;

        value_type get (size_t offset) const {
          ssize_t nseg (offset / handler.segment_size());
          return H.scale_from_storage (get_func (handler.segment (nseg), offset - nseg*handler.segment_size()));
        }

        void set (size_t offset, value_type val) {
          ssize_t nseg (offset / handler.segment_size());
          put_func (H.scale_to_storage (val), handler.segment (nseg), offset - nseg*handler.segment_size());
        }

        friend std::ostream& operator<< (std::ostream& stream, const Data& V) {
          stream << "data for image \"" << V.name() << "\": " + str (DataSet::voxel_count (V)) 
            + " voxels in " + V.datatype().specifier() + " format, stored in " + str (V.handler.nsegments()) 
            + " segments of size " + str (V.handler.segment_size()) 
            + ", at addresses [ ";
          for (size_t n = 0; n < V.handler.nsegments(); ++n) 
            stream << str (size_t (V.handler.segment(n))) << " ";
          stream << "]";
          return stream;
        }

      private:
        Handler::Base& handler;

        value_type (*get_func) (const void* data, size_t i);
        void (*put_func) (value_type val, void* data, size_t i);

        void set_get_put_functions () {
          switch (H.datatype() ()) {
            case DataType::Bit:
              get_func = &__get<value_type,bool>;
              put_func = &__put<value_type,bool>;
              return;
            case DataType::Int8:
              get_func = &__get<value_type,int8_t>;
              put_func = &__put<value_type,int8_t>;
              return;
            case DataType::UInt8:
              get_func = &__get<value_type,uint8_t>;
              put_func = &__put<value_type,uint8_t>;
              return;
            case DataType::Int16LE:
              get_func = &__getLE<value_type,int16_t>;
              put_func = &__putLE<value_type,int16_t>;
              return;
            case DataType::UInt16LE:
              get_func = &__getLE<value_type,uint16_t>;
              put_func = &__putLE<value_type,uint16_t>;
              return;
            case DataType::Int16BE:
              get_func = &__getBE<value_type,int16_t>;
              put_func = &__putBE<value_type,int16_t>;
              return;
            case DataType::UInt16BE:
              get_func = &__getBE<value_type,uint16_t>;
              put_func = &__putBE<value_type,uint16_t>;
              return;
            case DataType::Int32LE:
              get_func = &__getLE<value_type,int32_t>;
              put_func = &__putLE<value_type,int32_t>;
              return;
            case DataType::UInt32LE:
              get_func = &__getLE<value_type,uint32_t>;
              put_func = &__putLE<value_type,uint32_t>;
              return;
            case DataType::Int32BE:
              get_func = &__getBE<value_type,int32_t>;
              put_func = &__putBE<value_type,int32_t>;
              return;
            case DataType::UInt32BE:
              get_func = &__getBE<value_type,uint32_t>;
              put_func = &__putBE<value_type,uint32_t>;
              return;
            case DataType::Float32LE:
              get_func = &__getLE<value_type,float>;
              put_func = &__putLE<value_type,float>;
              return;
            case DataType::Float32BE:
              get_func = &__getBE<value_type,float>;
              put_func = &__putBE<value_type,float>;
              return;
            case DataType::Float64LE:
              get_func = &__getLE<value_type,double>;
              put_func = &__putLE<value_type,double>;
              return;
            case DataType::Float64BE:
              get_func = &__getBE<value_type,double>;
              put_func = &__putBE<value_type,double>;
              return;
            default:
              throw Exception ("invalid data type in image header");
          }
        }

    };




  }
}

#endif



