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

#ifndef __image_voxel_h__
#define __image_voxel_h__

#include "get_set.h"
#include "image/header.h"
#include "math/complex.h"

#define MAX_FILES_PER_IMAGE 256U
#define MAX_NDIM 16

namespace MR {
  namespace Image {

    class Object;

    //! \cond skip

    template <typename value_type, typename S> static value_type __get   (const void* data, size_t i) { return (MR::get<S> (data, i)); }
    template <typename value_type, typename S> static value_type __getLE (const void* data, size_t i) { return (MR::getLE<S> (data, i)); }
    template <typename value_type, typename S> static value_type __getBE (const void* data, size_t i) { return (MR::getBE<S> (data, i)); }

    template <typename value_type, typename S> static void __put   (value_type val, void* data, size_t i) { return (MR::put<S> (val, data, i)); }
    template <typename value_type, typename S> static void __putLE (value_type val, void* data, size_t i) { return (MR::putLE<S> (val, data, i)); }
    template <typename value_type, typename S> static void __putBE (value_type val, void* data, size_t i) { return (MR::putBE<S> (val, data, i)); }

    // \endcond


    //! \addtogroup Image
    // @{

    //! This class provides access to the voxel intensities of an image.
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities. It implements all the
     * features of the DataSet abstract class. 
     * \todo Provide specialisations of get/set methods to handle conversions
     * between floating-point and integer types */
    template <typename T> class Voxel {
      public:
        //! construct a Voxel object to access the data in the Image::Header \p parent
        /*! All coordinates will be initialised to zero.
         * \note only one Image::Voxel object per Image::Header can be
         * constructed using this consructor. If more Image::Voxel objects are
         * required to access the same image (e.g. for multithreading), use the
         * copy constructor to create direct copies of the first Image::Voxel.
         * For example:
         * \code
         * Image::Header header = argument[0].get_image();
         * Image::Voxel<float> vox1 (header);
         * Image::Voxel<float> vox2 (vox1);
         * \endcode
       */
        Voxel (const Header& parent) : H (parent), handler (*H.handler), x (ndim(), 0) {
          assert (H.handler);
          H.handler->prepare();
          offset = handler.start();

          switch (H.datatype()()) {
            case DataType::Bit:        get_func = &__get<value_type,bool>;       put_func = &__put<value_type,bool>;       return;
            case DataType::Int8:       get_func = &__get<value_type,int8_t>;     put_func = &__put<value_type,int8_t>;     return;
            case DataType::UInt8:      get_func = &__get<value_type,uint8_t>;    put_func = &__put<value_type,uint8_t>;    return;
            case DataType::Int16LE:    get_func = &__getLE<value_type,int16_t>;  put_func = &__putLE<value_type,int16_t>;  return;
            case DataType::UInt16LE:   get_func = &__getLE<value_type,uint16_t>; put_func = &__putLE<value_type,uint16_t>; return;
            case DataType::Int16BE:    get_func = &__getBE<value_type,int16_t>;  put_func = &__putBE<value_type,int16_t>;  return;
            case DataType::UInt16BE:   get_func = &__getBE<value_type,uint16_t>; put_func = &__putBE<value_type,uint16_t>; return;
            case DataType::Int32LE:    get_func = &__getLE<value_type,int32_t>;  put_func = &__putLE<value_type,int32_t>;  return;
            case DataType::UInt32LE:   get_func = &__getLE<value_type,uint32_t>; put_func = &__putLE<value_type,uint32_t>; return;
            case DataType::Int32BE:    get_func = &__getBE<value_type,int32_t>;  put_func = &__putBE<value_type,int32_t>;  return;
            case DataType::UInt32BE:   get_func = &__getBE<value_type,uint32_t>; put_func = &__putBE<value_type,uint32_t>; return;
            case DataType::Float32LE:  get_func = &__getLE<value_type,float>;    put_func = &__putLE<value_type,float>;    return;
            case DataType::Float32BE:  get_func = &__getBE<value_type,float>;    put_func = &__putBE<value_type,float>;    return;
            case DataType::Float64LE:  get_func = &__getLE<value_type,double>;   put_func = &__putLE<value_type,double>;   return;
            case DataType::Float64BE:  get_func = &__getBE<value_type,double>;   put_func = &__putBE<value_type,double>;   return;
            default: throw Exception ("invalid data type in image header");
          }
        }
        
        const Header& header () const { return (H); }
        DataType datatype () const { return (H.datatype()); }
        const Math::Matrix<float>& transform () const { return (H.transform()); }

        typedef T value_type;

        //! test whether the current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool operator! () const { 
          for (size_t n = 0; n < ndim(); n++)
            if (x[n] < 0 || x[n] >= ssize_t(dim(n))) return (true);
          return (false);
        }

        size_t  ndim () const { return (H.ndim()); }
        ssize_t dim (size_t axis) const { return (H.dim(axis)); }
        float   vox (size_t axis) const { return (H.vox(axis)); }
        const std::string& name () const { return (H.name()); }

        template <class U> const Voxel& operator= (const U& V) {
          ssize_t shift = 0;
          for (size_t n = 0; n < ndim(); n++) {
            x[n] = V.pos(n);
            shift += handler.stride(n) * x[n];
          }
          offset = handler.start() + shift;
          return (*this);
        }

        //! reset all coordinates to zero. 
        void reset () { offset = handler.start(); for (size_t i = 0; i < ndim(); i++) x[i] = 0; }

        class Coordinate {
          public:
            operator ssize_t () const { return (V.x[a]); }

            ssize_t operator= (ssize_t pos)  { V.offset += V.handler.stride(a)*(pos-V.x[a]); V.x[a] = pos; return (pos); }
            ssize_t operator+= (ssize_t inc) { V.offset += V.handler.stride(a)*inc; V.x[a] += inc; return (V.x[a]); }
            ssize_t operator-= (ssize_t inc) { V.offset -= V.handler.stride(a)*inc; V.x[a] -= inc; return (V.x[a]); }
            ssize_t operator++ ()            { V.offset += V.handler.stride(a); return (++V.x[a]); }
            ssize_t operator-- ()            { V.offset -= V.handler.stride(a); return (--V.x[a]); }
            ssize_t operator++ (int unused)  { V.offset += V.handler.stride(a); return (V.x[a]++); }
            ssize_t operator-- (int unused)  { V.offset -= V.handler.stride(a); return (V.x[a]--); }

          private:
            Coordinate (Voxel& parent, size_t axis) : V (parent), a (axis) { }
            Voxel& V;
            size_t a;

            friend class Voxel;
        };

        ssize_t          operator[] (size_t axis) const { return (x[axis]); }
        Coordinate       operator[] (size_t axis)       { return (Coordinate (*this, axis)); }


        class Value {
          public:
            operator value_type () const { return (V.get()); }
            float operator= (value_type val) { V.set (val); return (val); }
            float operator+= (value_type val) { val += V.get(); V.set (val); return (val); }
            float operator-= (value_type val) { val = V.get() - val; V.set (val); return (val); }
            float operator*= (value_type val) { val *= V.get(); V.set (val); return (val); }
            float operator/= (value_type val) { val = V.get() / val; V.set (val); return (val); }
          private:
            Value (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        value_type value () const { return (get()); }
        Value      value () { return (Value (*this)); }


        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
          stream << "position for image \"" << V.name() << "\" = [ ";
          for (size_t n = 0; n < V.ndim(); n++) stream << V[n] << " ";
          stream << "]\n  current offset = " << V.offset;
          return (stream);
        }

      protected:
        const Header&   H; //!< reference to the corresponding Image::Header
        const Handler::Base& handler;
        size_t   offset; //!< the offset in memory to the current voxel
        std::vector<ssize_t> x;

        value_type get () const { 
          ssize_t nseg (offset / handler.segment_size());
          return (H.scale_from_storage (get_func (handler.segment(nseg), offset - nseg*handler.segment_size()))); 
        }
        void set (value_type val) {
          ssize_t nseg (offset / handler.segment_size());
          put_func (H.scale_to_storage (val), handler.segment(nseg), offset - nseg*handler.segment_size()); 
        }

        value_type (*get_func) (const void* data, size_t i);
        void       (*put_func) (value_type val, void* data, size_t i);
    };


    //! @}
    
  }
}

#endif


