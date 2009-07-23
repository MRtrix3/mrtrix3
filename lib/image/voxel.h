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

#include "image/object.h"
#include "math/complex.h"

namespace MR {
  namespace Image {

    class Object;

    //! \addtogroup Image
    // @{

    //! This class provides access to the voxel intensities of an image.
    class Voxel {
      public:
        //! construct a Voxel object to point to the data contained in the MR::Image::Object \p parent
        /*! All coordinates will be initialised to zero.
         \note the parent Image::Object must be mapped manually before attempting to access any data. */
        explicit Voxel (Object& parent) : 
          image (parent), 
          offset (image.start),
          stride (image.stride) { 
          memset (x, 0, image.ndim()*sizeof(int)); 
        }

        Object&     image; //!< The MR::Image::Object containing the image data.
        const Math::Matrix<float>& transform () const { return (image.transform()); }

        //! test whether current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool     operator! () const { 
          for (size_t n = 0; n < image.ndim(); n++)
            if (x[n] < 0 || x[n] >= ssize_t(image.dim(n))) return (true);
          return (false);
        }

        size_t  ndim () const { return (image.ndim()); }
        int     dim (size_t axis) const { return (image.dim(axis)); }
        float   vox (size_t axis) const { return (image.vox(axis)); }
        const std::string& name () const { return (image.name()); }

        template <class T> const Voxel& operator= (const T& V) {
          ssize_t shift = 0;
          for (size_t n = 0; n < image.ndim(); n++) {
            x[n] = V[n];
            shift += image.stride[n] * ssize_t(x[n]);
          }
          offset = image.start + shift;
          return (*this);
        }

        //! used to loop over all image coordinates.
        /*! This operator increments the current position to the next voxel, incrementing to the next axis as required.
         * It is used to process all voxels in turn. For example:
         * \code
         * MR::Image::Voxel position (image_object);
         * do {
         *   process (position.value());
         * } while (position++);
         * \endcode
         * \return true once the last voxel has been reached (i.e. the next increment would bring the current position out of bounds), false otherwise. */
        bool        operator++ (int notused);

        //! reset all coordinates to zero. 
        void        reset () { offset = image.start; memset (x, 0, image.ndim()*sizeof(int)); }


        class Coordinate {
          public:
            operator int () const          { return (V.get (axis)); }
            int operator++ (int notused)   { return (V.inc (axis)); }
            int operator-- (int notused)   { return (V.dec (axis)); }
            int operator+= (int increment) { return (V.move (axis, increment)); }
            int operator-= (int increment) { return (V.move (axis, -increment)); }
            int operator= (int position)   { return (V.set (axis, position)); }
            int operator= (const Coordinate& C) { return (V.set (axis, C.V.get(axis))); }
          private:
            Coordinate (Voxel& parent, uint corresponding_axis) : V (parent), axis (corresponding_axis) { }
            Voxel& V;
            size_t axis;
            friend class Voxel;
        };

        //! return the coordinate along the specified axis.
        const ssize_t   operator[] (const size_t axis) const     { return (x[axis]); }
        //! returns a reference to the coordinate along the specified axis.
        Coordinate operator[] (const uint axis) { return (Coordinate (*this, axis)); }

        class RealValue {
          public:
            operator float () const { return (V.get_real()); }
            float operator= (const RealValue& C) { return (operator= (float(C))); }
            float operator= (float value) { V.set_real (value); return (value); }
            float operator+= (float value) { value += V.get_real(); V.set_real (value); return (value); }
            float operator-= (float value) { value = V.get_real() - value; V.set_real (value); return (value); }
            float operator*= (float value) { value *= V.get_real(); V.set_real (value); return (value); }
            float operator/= (float value) { value = V.get_real() / value; V.set_real (value); return (value); }
          private:
            RealValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        class ImagValue {
          public:
            operator float () const { return (V.get_imag()); }
            float operator= (const ImagValue& C) { return (operator= (float(C))); }
            float operator= (float value) { V.set_imag (value); return (value); }
            float operator+= (float value) { value += V.get_imag(); V.set_imag (value); return (value); }
            float operator-= (float value) { value = V.get_imag() - value; V.set_imag (value); return (value); }
            float operator*= (float value) { value *= V.get_imag(); V.set_imag (value); return (value); }
            float operator/= (float value) { value = V.get_imag() / value; V.set_imag (value); return (value); }
          private:
            ImagValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        class ComplexValue {
          public:
            operator cfloat () const { return (V.get_complex()); }
            cfloat operator= (const ComplexValue& C) { return (operator= (cfloat(C))); }
            cfloat operator= (cfloat value) { V.set_complex (value); return (value); }
            cfloat operator+= (cfloat value) { value += V.get_complex(); V.set_complex (value); return (value); }
            cfloat operator-= (cfloat value) { value = V.get_complex() - value; V.set_complex (value); return (value); }
            cfloat operator*= (cfloat value) { value *= V.get_complex(); V.set_complex (value); return (value); }
            cfloat operator/= (cfloat value) { value = V.get_complex() / value; V.set_complex (value); return (value); }
          private:
            ComplexValue (Voxel& parent) : V (parent) { }
            Voxel& V;
            friend class Voxel;
        };

        //! %get the voxel size along the axis specified.
        /*! \return the voxel size of the parent image along the axis specified */
        float size (uint index) const { return (image.vox (index)); }

        //! %get whether the image data is complex
        /*! \return true if the image data are complex */
        bool  is_complex () const      { return (image.is_complex()); }

        //! returns the value of the voxel at the current position
        float            value () const { return (get_real()); }
        //! returns a reference to the value of the voxel at the current position
        RealValue        value ()       { return (RealValue (*this)); }

        //! returns the real value of the voxel at the current position
        float            real () const { return (get_real()); }
        //! returns a reference to the real value of the voxel at the current position
        RealValue        real ()       { return (RealValue (*this)); }

        //! returns the imaginary value of the voxel at the current position
        float            imag () const { return (get_imag()); }
        //! returns a reference to the imaginary value of the voxel at the current position
        ImagValue        imag ()       { return (ImagValue (*this)); }

        //! returns the complex value stored at the current position
        /*! \note No check is performed to ensure the image is actually complex. Calling this function on real-valued data will produce undefined results */
        cfloat           Z () const    { return (get_complex()); }
        //! returns the complex value stored at the current position
        /*! \note No check is performed to ensure the image is actually complex. Calling this function on real-valued data will produce undefined results */
        ComplexValue     Z ()          { return (ComplexValue (*this)); }

        friend std::ostream& operator<< (std::ostream& stream, const Voxel& V);

      protected:
        ssize_t         x[MRTRIX_MAX_NDIMS]; //!< the current image coordinates
        size_t          offset; //!< the offset in memory to the current voxel
        const ssize_t*  stride; //!< the offset in memory between adjacent image voxels along each axis

        int set (uint axis, int position)    { offset += stride[axis] * ssize_t(position - x[axis]); x[axis] = position; return (x[axis]); }
        int get (uint axis) const            { return (x[axis]); }
        int inc (uint axis)                  { offset += stride[axis]; x[axis]++; return (x[axis]); }
        int dec (uint axis)                  { offset -= stride[axis]; x[axis]--; return (x[axis]); }
        int move (uint axis, int increment)  { offset += stride[axis] * ssize_t(increment); x[axis] += increment; return (x[axis]); } 

        float   get_real () const { assert (image.is_mapped()); return (image.real (offset)); }
        float   get_imag () const { assert (image.is_mapped()); return (image.imag (offset)); }
        cfloat  get_complex () const { assert (image.is_mapped()); return (cfloat (get_real(), get_imag())); }

        float   set_real (const float value)   { assert (image.is_mapped()); image.real (offset, value); return (value); }
        float   set_imag (const float value)   { assert (image.is_mapped()); image.imag (offset, value); return (value); }
        cfloat  set_complex (const cfloat value) { 
          assert (image.is_mapped());
          image.real (offset, value.real());
          image.imag (offset, value.imag());
          return (value);
        }

        friend class Entry;
        friend class Coordinate;
        friend class RealValue;
        friend class ImagValue;
        friend class ComplexValue;
    };

    //! @}








    inline bool Voxel::operator++ (int notused)
    {
      size_t axis = 0;
      do {
        inc (axis);
        if (x[axis] < ssize_t(image.dim(axis))) return (true);
        set (axis, 0);
        axis++;
      } while (axis < image.ndim());
      return (false);
    }




    inline std::ostream& operator<< (std::ostream& stream, const Voxel& V)
    {
      stream << "position for image \"" << V.image.name() << "\" = [ ";
      for (size_t n = 0; n < V.image.ndim(); n++) stream << V.x[n] << " ";
      stream << "]\n  current offset = " << V.offset;
      return (stream);
    }


  }
}

#endif


