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

#include "image/header.h"
#include "math/complex.h"

namespace MR {
  namespace Image {

    class Object;

    //! \addtogroup Image
    // @{

    //! This class provides access to the voxel intensities of an image.
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities. It implements all the
     * features of the DataSet abstract class. 
     * \todo implement full complex data support. */
    class Voxel {
      public:
        //! construct a Voxel object to access the data in the Image::Header \p parent
        /*! All coordinates will be initialised to zero. */
        Voxel (Header& parent) : S (new SharedInfo (parent)), offset (S->start) { 
          x = new ssize_t [ndim()]; 
          memset (x, 0, sizeof(ssize_t) * ndim()); 
        }
        
        //! construct a Voxel object to access the same data as \a V
        /*! Useful for multi-threading applications. All coordinates will be initialised to
         * the same value as \a V. */
        Voxel (const Voxel& V) : S (V.S), offset (V.offset) {
          x = new ssize_t [ndim()];
          memcpy (x, V.x, sizeof(ssize_t) * ndim());
        }

        ~Voxel () { delete [] x; }

        const Header& header () const { return (S->H); }
        DataType datatype () const { return (S->H.datatype()); }
        const Math::Matrix<float>& transform () const { return (S->H.transform()); }

        //! test whether the current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool operator! () const { 
          for (size_t n = 0; n < ndim(); n++)
            if (x[n] < 0 || x[n] >= ssize_t(dim(n))) return (true);
          return (false);
        }

        size_t  ndim () const { return (S->H.ndim()); }
        ssize_t dim (size_t axis) const { return (S->H.dim(axis)); }
        float   vox (size_t axis) const { return (S->H.vox(axis)); }
        const std::string& name () const { return (S->H.name()); }

        template <class T> const Voxel& operator= (const T& V) {
          ssize_t shift = 0;
          for (size_t n = 0; n < ndim(); n++) {
            x[n] = V[n];
            shift += S->stride[n] * ssize_t(x[n]);
          }
          offset = S->start + shift;
          return (*this);
        }

        //! reset all coordinates to zero. 
        void reset () { offset = S->start; memset (x, 0, ndim()*sizeof(ssize_t)); }


        class Coordinate {
          public:
            operator ssize_t () const          { return (V.get (axis)); }
            ssize_t operator++ (int notused)   { return (V.inc (axis)); }
            ssize_t operator-- (int notused)   { return (V.dec (axis)); }
            ssize_t operator+= (ssize_t increment) { return (V.move (axis, increment)); }
            ssize_t operator-= (ssize_t increment) { return (V.move (axis, -increment)); }
            ssize_t operator= (ssize_t position)   { return (V.set (axis, position)); }
            ssize_t operator= (const Coordinate& C) { return (V.set (axis, C.V.get(axis))); }
          private:
            Coordinate (Voxel& parent, size_t corresponding_axis) : V (parent), axis (corresponding_axis) { }
            Voxel& V;
            size_t axis;
            friend class Voxel;
        };

        //! return the coordinate along the specified axis.
        const ssize_t   operator[] (const size_t axis) const     { return (x[axis]); }
        //! returns a reference to the coordinate along the specified axis.
        Coordinate operator[] (const size_t axis) { return (Coordinate (*this, axis)); }

        class Value {
          public:
            operator float () const { return (V.S->get (V.offset)); }
            float operator= (const Value& C) { return (operator= (float(C))); }
            float operator= (float value) { V.S->set (V.offset, value); return (value); }
            float operator+= (float value) { value += V.S->get (V.offset); V.S->set (V.offset, value); return (value); }
            float operator-= (float value) { value = V.S->get (V.offset) - value; V.S->set (V.offset, value); return (value); }
            float operator*= (float value) { value *= V.S->get (V.offset); V.S->set (V.offset, value); return (value); }
            float operator/= (float value) { value = V.S->get (V.offset) / value; V.S->set (V.offset, value); return (value); }
          private:
            Value (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        class ComplexValue {
          public:
            operator cfloat () const { return (V.S->getZ(V.offset)); }
            cfloat operator= (const ComplexValue& C) { return (operator= (cfloat(C))); }
            cfloat operator= (cfloat value) { V.S->setZ (V.offset, value); return (value); }
            cfloat operator+= (cfloat value) { value += V.S->getZ (V.offset); V.S->setZ (V.offset, value); return (value); }
            cfloat operator-= (cfloat value) { value = V.S->getZ (V.offset) - value; V.S->setZ (V.offset, value); return (value); }
            cfloat operator*= (cfloat value) { value *= V.S->getZ (V.offset); V.S->setZ (V.offset, value); return (value); }
            cfloat operator/= (cfloat value) { value = V.S->getZ (V.offset) / value; V.S->setZ (V.offset, value); return (value); }
          private:
            ComplexValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        //! %get whether the image data are complex
        /*! \return true if the image data are complex */
        bool  is_complex () const      { return (S->H.datatype().is_complex()); }

        //! returns the value of the voxel at the current position
        float value () const { return (S->get (offset)); }
        //! returns a reference to the value of the voxel at the current position
        Value value ()       { return (Value (*this)); }

        //! returns the complex value stored at the current position
        /*! \note No check is performed to ensure the image is actually complex. Calling this function on real-valued data will produce undefined results */
        cfloat        Z () const { return (S->getZ (offset)); }
        //! returns the complex value stored at the current position
        /*! \note No check is performed to ensure the image is actually complex. Calling this function on real-valued data will produce undefined results */
        ComplexValue  Z ()       { return (ComplexValue (*this)); }

        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V);

      protected:
        class SharedInfo {
          public:
            SharedInfo (Header& header);
            
            Header&   H; //!< reference to the corresponding Image::Header
            std::vector<ssize_t>  stride; //!< the offsets between adjacent voxels along each respective axis
            size_t    start; //!< the offset to the first (logical) voxel in the dataset

            float get (off64_t at) const {
              ssize_t nseg (at / H.handler->voxels_per_segment());
              return (H.scale_from_storage (get_func (H.handler->segment(nseg), at - nseg*H.handler->voxels_per_segment()))); 
            }
            void  set (off64_t at, float val) {
              ssize_t nseg (at / H.handler->voxels_per_segment());
              put_func (H.scale_to_storage (val), H.handler->segment(nseg), at - nseg*H.handler->voxels_per_segment()); 
            }

            cfloat getZ (off64_t at) const;
            void   setZ (off64_t at, cfloat val);

          private:

            void init ();

            float  (*get_func) (const void* data, size_t i);
            void   (*put_func) (float val, void* data, size_t i);

            cfloat  (*getZ_func) (const void* data, size_t i);
            void    (*putZ_func) (cfloat val, void* data, size_t i);
        };

        RefPtr<SharedInfo> S;
        size_t    offset; //!< the offset in memory to the current voxel
        ssize_t*  x; //!< the current image coordinates

        ssize_t set (size_t axis, ssize_t position)   { offset += S->stride[axis] * ssize_t(position - x[axis]); x[axis] = position; return (x[axis]); }
        ssize_t get (size_t axis) const               { return (x[axis]); }
        ssize_t inc (size_t axis)                     { offset += S->stride[axis]; x[axis]++; return (x[axis]); }
        ssize_t dec (size_t axis)                     { offset -= S->stride[axis]; x[axis]--; return (x[axis]); }
        ssize_t move (size_t axis, ssize_t increment) { offset += S->stride[axis] * ssize_t(increment); x[axis] += increment; return (x[axis]); } 

        friend class Coordinate;
        friend class Value;
        friend class ComplexValue;
    };

    //! @}











    inline std::ostream& operator<< (std::ostream& stream, const Voxel& V)
    {
      stream << "position for image \"" << V.name() << "\" = [ ";
      for (size_t n = 0; n < V.ndim(); n++) stream << V.x[n] << " ";
      stream << "]\n  current offset = " << V.offset;
      return (stream);
    }

  }
}

#endif


